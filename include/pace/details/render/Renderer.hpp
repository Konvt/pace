#ifndef PACE_RENDERER
#define PACE_RENDERER

#include "../concurrent/ExceptionBox.hpp"
#include "../concurrent/Util.hpp"
#include "../console/TermContext.hpp"
#include "../utils/ScopeGuard.hpp"
#include "../wrappers/UniqueFunction.hpp"
#include <atomic>
#include <thread>
#ifndef __cpp_lib_atomic_wait
# include <condition_variable>
#endif

namespace pace {
  namespace details {
    namespace render {
      template<Channel Tag>
      class Renderer final {
        static constexpr auto _default_working_interval =
          std::chrono::duration_cast<types::Tempus>( std::chrono::milliseconds( 40 ) );

#if PACE__CXX17
        static std::atomic<types::Tempus> _working_interval;
#else
        static std::atomic<types::Tempus>& _working_interval() noexcept
        {
          static std::atomic<types::Tempus> instance { _default_working_interval };
          return instance;
        }
#endif

        std::atomic<std::uint64_t> quota_      = { 0 };
        concurrent::ExceptionBox box_          = {};
        wrappers::UniqueFunction<void()> task_ = {};
        std::thread runner_                    = {};

#ifndef __cpp_lib_atomic_wait
        mutable std::condition_variable cond_var_ = {};
#endif
        mutable concurrent::SharedMutex res_mtx_ = {};
        mutable std::mutex sched_mtx_            = {};

        /***********************************************************
          Dead
            └─ launch() → Dormant
                            ├─ activate<Async>() → Warmup → Loop
                            │                                └ trigger<Async>() → Warmup
                            │
                            ├─ activate<Signal>() → Primed → Pulse
                            │                                  └ trigger<Signal>() → Primed
                            │
                            └─ activate<Sync>() → Idle
                                                   └ trigger<Sync>() → Shot → Idle

          any state
            └─ abort() → Asleep → Dormant

          any state
            └─ drop() → Dead
        ***********************************************************/
        enum class Phase : std::uint8_t { Dead, Asleep, Dormant, Warmup, Loop, Primed, Pulse, Shot, Idle };
        std::atomic<Phase> state_ = { Phase::Dead };

        void launch() &
        {
          console::TermContext<Tag>::itself().virtual_term();
          PACE__ASSERT( runner_.get_id() == std::thread::id() );
          auto guard = utils::make_scope_fail(
            [this]() noexcept { state_.store( Phase::Dead, std::memory_order_relaxed ); } );

          state_.store( Phase::Dormant, std::memory_order_relaxed );
          runner_ = std::thread( [this]() {
            try {
              for ( auto state = state_.load( std::memory_order_acquire ); state != Phase::Dead;
                    state      = state_.load( std::memory_order_acquire ) ) {
                switch ( state ) {
                case Phase::Asleep:
                  concurrent::atomic_commit_all( state_,
                                                 Phase::Asleep,
                                                 Phase::Dormant,
                                                 std::memory_order_release );
                  PACE__FALLTHROUGH;
                case Phase::Dormant: {
#ifdef __cpp_lib_atomic_wait
                  state_.wait( Phase::Dormant, std::memory_order_relaxed );
#else
                  std::unique_lock<std::mutex> lock { sched_mtx_ };
                  cond_var_.wait( lock, [this]() noexcept {
                    return state_.load( std::memory_order_relaxed ) != Phase::Dormant;
                  } );
#endif
                } break;

                case Phase::Warmup: {
                  task_();
                  concurrent::atomic_commit_all( state_,
                                                 Phase::Warmup,
                                                 Phase::Loop,
                                                 std::memory_order_release );
                }
                  PACE__FALLTHROUGH;
                case Phase::Loop: {
                  task_();
                  std::this_thread::sleep_for( working_interval() );
                } break;

                case Phase::Primed: {
                  task_();
                  quota_.fetch_sub( 1, std::memory_order_relaxed );
                  concurrent::atomic_commit_all( state_,
                                                 Phase::Primed,
                                                 Phase::Pulse,
                                                 std::memory_order_release );
                }
                  PACE__FALLTHROUGH;
                case Phase::Pulse: {
#ifdef __cpp_lib_atomic_wait
                  quota_.wait( 0, std::memory_order_relaxed );
#else
                  concurrent::spin_with(
                    [this]() noexcept { return quota_.load( std::memory_order_relaxed ) > 0; },
                    [this]() noexcept {
                      std::unique_lock<std::mutex> lock { sched_mtx_ };
                      cond_var_.wait( lock, [this]() noexcept {
                        return quota_.load( std::memory_order_relaxed ) > 0
                            || state_.load( std::memory_order_relaxed ) != Phase::Pulse;
                      } );
                    },
                    1024 );
#endif
                  do
                    task_();
                  while ( quota_.fetch_sub( 1, std::memory_order_relaxed ) > 1
                          && state_.load( std::memory_order_relaxed ) == Phase::Pulse );
                } break;

                case Phase::Shot: {
                  {
                    concurrent::SharedLock<concurrent::SharedMutex> lock1 { res_mtx_ };
                    std::lock_guard<std::mutex> lock2 { sched_mtx_ };
                    task_();
                  }
                  concurrent::atomic_commit_all( state_,
                                                 Phase::Shot,
                                                 Phase::Idle,
                                                 std::memory_order_release );
                }
                  PACE__FALLTHROUGH;
                case Phase::Idle: {
#ifdef __cpp_lib_atomic_wait
                  state_.wait( Phase::Idle, std::memory_order_relaxed );
#else
                  std::unique_lock<std::mutex> lock { sched_mtx_ };
                  cond_var_.wait( lock, [this]() noexcept {
                    return state_.load( std::memory_order_relaxed ) != Phase::Idle;
                  } );
#endif
                } break;

                default: utils::unreachable();
                }
              }
            } catch ( ... ) {
              auto dump = box_.try_store( std::current_exception() );
              concurrent::atomic_commit_all( state_,
                                             dump ? Phase::Dormant : Phase::Dead,
                                             std::memory_order_release );
              if ( !dump )
                throw;
            }
            PACE__ASSERT( state_ == Phase::Dead );
          } );
        }

        void shutdown() noexcept
        {
          concurrent::atomic_commit_all( state_, Phase::Dead, std::memory_order_relaxed );
#ifndef __cpp_lib_atomic_wait
          {
            std::lock_guard<std::mutex> lock { sched_mtx_ };
            cond_var_.notify_all();
          }
#endif
          if ( runner_.joinable() )
            runner_.join();
          runner_ = std::thread();
          task_   = nullptr;
        }

        Renderer() = default;

      public:
        // Get the current working interval for all threads.
        PACE__NODISCARD static PACE__FORCEINLINE types::Tempus working_interval() noexcept
        {
#if PACE__CXX17
          return _working_interval.load( std::memory_order_relaxed );
#else
          return _working_interval().load( std::memory_order_relaxed );
#endif
        }
        // Adjust the thread working interval between this loop and the next loop.
        static PACE__FORCEINLINE void working_interval( types::Tempus new_rate ) noexcept
        {
#if PACE__CXX17
          _working_interval.store( new_rate, std::memory_order_relaxed );
#else
          _working_interval().store( new_rate, std::memory_order_relaxed );
#endif
        }

        static Renderer& itself() noexcept
        {
          static Renderer instance;
          return instance;
        }

        Renderer( const Renderer& )            = delete;
        Renderer& operator=( const Renderer& ) = delete;
        ~Renderer() noexcept { shutdown(); }

        // `activate` guarantees to perform the render task at least once.
        template<Policy Mode>
        void activate() &
        {
          if ( state_.load( std::memory_order_relaxed ) == Phase::Dead ) {
            std::lock_guard<concurrent::SharedMutex> lock { res_mtx_ };
            if ( state_.load( std::memory_order_relaxed ) == Phase::Dead ) {
              if ( runner_.get_id() == std::thread::id() )
                launch();
              else {
                shutdown();
                launch();
              }
            }
          }

          PACE__ASSERT( state_ != Phase::Dead );
          PACE__ASSERT( task_ != nullptr );
          // The operations below are all thread safe without locking.
          box_.rethrow();
          auto desired = []() noexcept {
            if PACE__CXX17_CNSTXPR ( Mode == Policy::Async )
              return Phase::Warmup;
            else if PACE__CXX17_CNSTXPR ( Mode == Policy::Signal )
              return Phase::Primed;
            else if PACE__CXX17_CNSTXPR ( Mode == Policy::Sync )
              return Phase::Idle;
          };
          auto expected = Phase::Dormant;
          if ( state_.compare_exchange_strong( expected, desired(), std::memory_order_relaxed ) ) {
            quota_.store( 0, std::memory_order_relaxed );
#ifdef __cpp_lib_atomic_wait
            state_.notify_one();
#endif
            if PACE__CXX17_CNSTXPR ( Mode != Policy::Sync ) {
#ifdef __cpp_lib_atomic_wait
              state_.wait( desired(), std::memory_order_relaxed );
#else
              cond_var_.notify_one();
              concurrent::spin_wait(
                [&]() noexcept { return state_.load( std::memory_order_relaxed ) != desired(); } );
#endif
            } else {
              concurrent::SharedLock<concurrent::SharedMutex> lock1 { res_mtx_ };
              std::lock_guard<std::mutex> lock2 { sched_mtx_ };
              task_();
#ifndef __cpp_lib_atomic_wait
              cond_var_.notify_one();
#endif
            }
          }
        }

        // Commit a rendering request and ensure it's executed.
        template<Policy Mode>
        PACE__FORCEINLINE void commit() &
        {
          if PACE__CXX17_CNSTXPR ( Mode == Policy::Signal ) {
            if ( state_.load( std::memory_order_relaxed ) != Phase::Dormant ) {
              quota_.fetch_add( 1, std::memory_order_relaxed );
#ifdef __cpp_lib_atomic_wait
              quota_.notify_one();
#else
              cond_var_.notify_one();
#endif
            }
          } else if PACE__CXX17_CNSTXPR ( Mode == Policy::Sync ) {
            concurrent::SharedLock<concurrent::SharedMutex> lock1 { res_mtx_ };
            // To ensure that only one thread is rendering the bar to the OStream.
            std::lock_guard<std::mutex> lock2 { sched_mtx_ };
            task_();
          }
        }

        template<Policy Mode>
        void trigger() & noexcept
        {
#ifdef __cpp_lib_atomic_wait
          auto state_transfer = [this]( Phase expected, Phase desired ) noexcept {
            if ( concurrent::atomic_commit_one( state_, expected, desired, std::memory_order_relaxed ) )
              state_.wait( desired, std::memory_order_relaxed );
          };
          if PACE__CXX17_CNSTXPR ( Mode == Policy::Async )
            state_transfer( Phase::Loop, Phase::Warmup );
          else if PACE__CXX17_CNSTXPR ( Mode == Policy::Signal ) {
            quota_.fetch_add( 1, std::memory_order_relaxed );
            auto expected = Phase::Pulse;
            if ( state_.compare_exchange_strong( expected, Phase::Primed, std::memory_order_relaxed ) ) {
              quota_.notify_one();
              state_.wait( Phase::Primed, std::memory_order_relaxed );
            }
          } else if PACE__CXX17_CNSTXPR ( Mode == Policy::Sync )
            state_transfer( Phase::Idle, Phase::Shot );
#else
          auto state_transfer = [this]( Phase expected, Phase desired ) noexcept {
            if ( state_.compare_exchange_strong( expected, desired, std::memory_order_relaxed ) ) {
              cond_var_.notify_one();
              concurrent::spin_wait(
                [&]() noexcept { return state_.load( std::memory_order_relaxed ) != desired; } );
            }
          };
          if PACE__CXX17_CNSTXPR ( Mode == Policy::Async ) {
            auto expected = Phase::Loop;
            if ( state_.compare_exchange_strong( expected, Phase::Warmup, std::memory_order_relaxed ) )
              concurrent::spin_wait(
                [this]() noexcept { return state_.load( std::memory_order_relaxed ) != Phase::Warmup; } );
          } else if PACE__CXX17_CNSTXPR ( Mode == Policy::Signal ) {
            quota_.fetch_add( 1, std::memory_order_relaxed );
            state_transfer( Phase::Pulse, Phase::Primed );
          } else if PACE__CXX17_CNSTXPR ( Mode == Policy::Sync )
            state_transfer( Phase::Idle, Phase::Shot );
#endif
        }

        void abort() noexcept
        {
          auto try_update = [this]( Phase expected ) noexcept {
            return concurrent::atomic_commit_one( state_,
                                                  expected,
                                                  Phase::Asleep,
                                                  std::memory_order_relaxed );
          };
          if ( try_update( Phase::Warmup ) || try_update( Phase::Loop ) || try_update( Phase::Primed )
               || try_update( Phase::Pulse ) || try_update( Phase::Shot ) || try_update( Phase::Idle ) ) {
#ifdef __cpp_lib_atomic_wait
            state_.wait( Phase::Asleep, std::memory_order_relaxed );
#else
            cond_var_.notify_all();
            concurrent::spin_wait(
              [this]() noexcept { return state_.load( std::memory_order_relaxed ) != Phase::Asleep; } );
#endif
          }
        }

        template<typename F>
        void dismiss_then( F&& noexpt_fn ) noexcept
        {
          static_assert( noexcept( (void)noexpt_fn() ), "unsafe functor types" );

          abort();
          std::lock_guard<concurrent::SharedMutex> lock { res_mtx_ };
          if ( task_ != nullptr )
            task_ = nullptr;
          (void)noexpt_fn();
        }
        void dismiss() noexcept
        {
          dismiss_then( []() noexcept {} );
        }

        void drop() noexcept
        {
          std::lock_guard<concurrent::SharedMutex> lock { res_mtx_ };
          shutdown();
        }

        PACE__NODISCARD bool try_appoint( wrappers::UniqueFunction<void()>&& task ) &
        {
          std::lock_guard<concurrent::SharedMutex> lock { res_mtx_ };
          if ( task_ != nullptr )
            return false;
          task_ = std::move( task );
          return true;
        }

        PACE__NODISCARD PACE__FORCEINLINE bool interrupted() const noexcept { return !box_.empty(); }
        PACE__NODISCARD PACE__FORCEINLINE bool empty() const noexcept
        {
          concurrent::SharedLock<concurrent::SharedMutex> lock { res_mtx_ };
          return task_ == nullptr;
        }
      };
#if PACE__CXX17
      template<Channel Tag>
      PACE__CXX17_INLINE std::atomic<types::Tempus> Renderer<Tag>::_working_interval {
        Renderer<Tag>::_default_working_interval
      };
#endif
    } // namespace render
  } // namespace details
} // namespace pace

#endif
