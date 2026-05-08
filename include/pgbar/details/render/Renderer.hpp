#ifndef PGBAR_RENDERER
#define PGBAR_RENDERER

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

namespace pgbar {
  namespace _details {
    namespace render {
      template<Channel Tag>
      class Renderer final {
        static std::atomic<_details::types::Tempus> _working_interval;

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

        void launch() & noexcept( false )
        {
          console::TermContext<Tag>::itself().virtual_term();
          PGBAR__ASSERT( runner_.get_id() == std::thread::id() );
          auto guard = utils::make_scope_fail(
            [this]() noexcept { state_.store( Phase::Dead, std::memory_order_release ); } );

          runner_ = std::thread( [this]() {
            try {
              for ( auto state = state_.load( std::memory_order_acquire ); state != Phase::Dead;
                    state      = state_.load( std::memory_order_acquire ) ) {
                switch ( state ) {
                case Phase::Asleep:
                  concurrent::atomic_commit_all( state_, Phase::Asleep, Phase::Dormant );
                  PGBAR__FALLTHROUGH;
                case Phase::Dormant: {
#ifdef __cpp_lib_atomic_wait
                  state_.wait( Phase::Dormant, std::memory_order_acquire );
#else
                  std::unique_lock<std::mutex> lock { sched_mtx_ };
                  cond_var_.wait( lock, [this]() noexcept {
                    return state_.load( std::memory_order_acquire ) != Phase::Dormant;
                  } );
#endif
                } break;

                case Phase::Warmup: {
                  task_();
                  concurrent::atomic_commit_all( state_, Phase::Warmup, Phase::Loop );
                }
                  PGBAR__FALLTHROUGH;
                case Phase::Loop: {
                  task_();
                  std::this_thread::sleep_for( working_interval() );
                } break;

                case Phase::Primed: {
                  task_();
                  quota_.fetch_sub( 1, std::memory_order_release );
                  concurrent::atomic_commit_all( state_, Phase::Primed, Phase::Pulse );
                }
                  PGBAR__FALLTHROUGH;
                case Phase::Pulse: {
#ifdef __cpp_lib_atomic_wait
                  quota_.wait( 0, std::memory_order_acquire );
#else
                  concurrent::spin_with(
                    [this]() noexcept { return quota_.load( std::memory_order_acquire ) > 0; },
                    [this]() noexcept {
                      std::unique_lock<std::mutex> lock { sched_mtx_ };
                      cond_var_.wait( lock, [this]() noexcept {
                        return quota_.load( std::memory_order_acquire ) > 0
                            || state_.load( std::memory_order_acquire ) != Phase::Pulse;
                      } );
                    },
                    1024 );
#endif
                  do
                    task_();
                  while ( quota_.fetch_sub( 1, std::memory_order_acq_rel ) > 1
                          && state_.load( std::memory_order_acquire ) == Phase::Pulse );
                } break;

                case Phase::Shot: {
                  {
                    concurrent::SharedLock<concurrent::SharedMutex> lock1 { res_mtx_ };
                    std::lock_guard<std::mutex> lock2 { sched_mtx_ };
                    task_();
                  }
                  concurrent::atomic_commit_all( state_, Phase::Shot, Phase::Idle );
                }
                  PGBAR__FALLTHROUGH;
                case Phase::Idle: {
#ifdef __cpp_lib_atomic_wait
                  state_.wait( Phase::Idle, std::memory_order_acquire );
#else
                  std::unique_lock<std::mutex> lock { sched_mtx_ };
                  cond_var_.wait( lock, [this]() noexcept {
                    return state_.load( std::memory_order_acquire ) != Phase::Idle;
                  } );
#endif
                } break;

                default: return;
                }
              }
            } catch ( ... ) {
              auto dump = box_.try_store( std::current_exception() );
              concurrent::atomic_commit_all( state_, dump ? Phase::Dormant : Phase::Dead );
              if ( !dump )
                throw;
            }
          } );
          auto expected = Phase::Dead;
          state_.compare_exchange_strong( expected, Phase::Dormant, std::memory_order_release );
        }

        void shutdown() noexcept
        {
          concurrent::atomic_commit_all( state_, Phase::Dead );
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
        PGBAR__NODISCARD static PGBAR__FORCEINLINE _details::types::Tempus working_interval() noexcept
        {
          return _working_interval.load( std::memory_order_acquire );
        }
        // Adjust the thread working interval between this loop and the next loop.
        static PGBAR__FORCEINLINE void working_interval( _details::types::Tempus new_rate ) noexcept
        {
          _working_interval.store( new_rate, std::memory_order_release );
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
        void activate() & noexcept( false )
        {
          if ( state_.load( std::memory_order_acquire ) == Phase::Dead ) {
            std::lock_guard<concurrent::SharedMutex> lock { res_mtx_ };
            if ( runner_.get_id() == std::thread::id() )
              launch();
            else {
              shutdown();
              launch();
            }
          }

          PGBAR__ASSERT( state_ != Phase::Dead );
          PGBAR__ASSERT( task_ != nullptr );
          // The operations below are all thread safe without locking.
          box_.rethrow();
          auto desired = []() noexcept {
            if PGBAR__CXX17_CNSTXPR ( Mode == Policy::Async )
              return Phase::Warmup;
            else if PGBAR__CXX17_CNSTXPR ( Mode == Policy::Signal )
              return Phase::Primed;
            else if PGBAR__CXX17_CNSTXPR ( Mode == Policy::Sync )
              return Phase::Idle;
          };
          auto expected = Phase::Dormant;
          if ( state_.compare_exchange_strong( expected, desired(), std::memory_order_release ) ) {
            quota_.store( 0, std::memory_order_release );
#ifdef __cpp_lib_atomic_wait
            state_.notify_one();
#endif
            if PGBAR__CXX17_CNSTXPR ( Mode != Policy::Sync ) {
#ifdef __cpp_lib_atomic_wait
              state_.wait( desired(), std::memory_order_acquire );
#else
              {
                std::lock_guard<std::mutex> lock { sched_mtx_ };
                cond_var_.notify_one();
              }
              concurrent::spin_wait(
                [&]() noexcept { return state_.load( std::memory_order_acquire ) != desired(); } );
#endif
            } else {
              std::lock_guard<concurrent::SharedMutex> lock1 { res_mtx_ };
              std::lock_guard<std::mutex> lock2 { sched_mtx_ };
#ifndef __cpp_lib_atomic_wait
              cond_var_.notify_one();
#endif
              task_();
            }
          }
        }

        // Commit a rendering request and ensure it's executed.
        template<Policy Mode>
        PGBAR__FORCEINLINE void commit() &
        {
          if PGBAR__CXX17_CNSTXPR ( Mode == Policy::Signal ) {
            if ( state_.load( std::memory_order_acquire ) != Phase::Dormant ) {
              quota_.fetch_add( 1, std::memory_order_release );
#ifdef __cpp_lib_atomic_wait
              quota_.notify_one();
#else
              std::lock_guard<std::mutex> lock { sched_mtx_ };
              cond_var_.notify_one();
#endif
            }
          } else if PGBAR__CXX17_CNSTXPR ( Mode == Policy::Sync ) {
            PGBAR__ASSERT( state_ == Phase::Idle );
            std::lock_guard<concurrent::SharedMutex> lock1 { res_mtx_ };
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
            if ( concurrent::atomic_commit_one( state_, expected, desired ) )
              state_.wait( desired, std::memory_order_acquire );
          };
          if PGBAR__CXX17_CNSTXPR ( Mode == Policy::Async )
            state_transfer( Phase::Loop, Phase::Warmup );
          else if PGBAR__CXX17_CNSTXPR ( Mode == Policy::Signal ) {
            quota_.fetch_add( 1, std::memory_order_release );
            auto expected = Phase::Pulse;
            if ( state_.compare_exchange_strong( expected, Phase::Primed, std::memory_order_release ) ) {
              quota_.notify_one();
              state_.wait( Phase::Primed, std::memory_order_acquire );
            }
          } else if PGBAR__CXX17_CNSTXPR ( Mode == Policy::Sync )
            state_transfer( Phase::Idle, Phase::Shot );
#else
          auto state_transfer = [this]( Phase expected, Phase desired ) noexcept {
            if ( state_.compare_exchange_strong( expected, desired, std::memory_order_release ) ) {
              {
                std::lock_guard<std::mutex> lock { sched_mtx_ };
                cond_var_.notify_one();
              }
              concurrent::spin_wait(
                [&]() noexcept { return state_.load( std::memory_order_acquire ) != desired; } );
            }
          };
          if PGBAR__CXX17_CNSTXPR ( Mode == Policy::Async ) {
            auto expected = Phase::Loop;
            if ( state_.compare_exchange_strong( expected, Phase::Warmup, std::memory_order_release ) )
              concurrent::spin_wait(
                [this]() noexcept { return state_.load( std::memory_order_acquire ) != Phase::Warmup; } );
          } else if PGBAR__CXX17_CNSTXPR ( Mode == Policy::Signal ) {
            quota_.fetch_add( 1, std::memory_order_release );
            state_transfer( Phase::Pulse, Phase::Primed );
          } else if PGBAR__CXX17_CNSTXPR ( Mode == Policy::Sync )
            state_transfer( Phase::Idle, Phase::Shot );
#endif
        }

        void abort() noexcept
        {
          auto try_update = [this]( Phase expected ) noexcept {
            return concurrent::atomic_commit_one( state_, expected, Phase::Asleep );
          };
          if ( try_update( Phase::Warmup ) || try_update( Phase::Loop ) || try_update( Phase::Primed )
               || try_update( Phase::Pulse ) || try_update( Phase::Shot ) || try_update( Phase::Idle ) ) {
#ifdef __cpp_lib_atomic_wait
            state_.wait( Phase::Asleep, std::memory_order_acquire );
#else
            {
              std::lock_guard<std::mutex> lock { sched_mtx_ };
              cond_var_.notify_all();
            }
            concurrent::spin_wait(
              [this]() noexcept { return state_.load( std::memory_order_acquire ) != Phase::Asleep; } );
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

        PGBAR__NODISCARD bool try_appoint( wrappers::UniqueFunction<void()>&& task ) & noexcept( false )
        {
          std::lock_guard<concurrent::SharedMutex> lock { res_mtx_ };
          if ( task_ != nullptr )
            return false;
          task_ = std::move( task );
          return true;
        }

        PGBAR__NODISCARD PGBAR__FORCEINLINE bool interrupted() const noexcept { return !box_.empty(); }
        PGBAR__NODISCARD PGBAR__FORCEINLINE bool empty() const noexcept
        {
          concurrent::SharedLock<concurrent::SharedMutex> lock { res_mtx_ };
          return task_ == nullptr;
        }
      };
      template<Channel Tag>
      std::atomic<_details::types::Tempus> Renderer<Tag>::_working_interval {
        std::chrono::duration_cast<_details::types::Tempus>( std::chrono::milliseconds( 40 ) )
      };
    } // namespace render
  } // namespace _details
} // namespace pgbar

#endif
