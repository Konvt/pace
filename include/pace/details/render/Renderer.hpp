#ifndef PACE_RENDERER
#define PACE_RENDERER

#include "../concurrent/ExceptionBox.hpp"
#include "../concurrent/Util.hpp"
#include "../utils/ScopeGuard.hpp"
#include "../utils/Singleton.hpp"
#include "../wrappers/UniqueFunction.hpp"
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <thread>

namespace pace {
  namespace details {
    namespace render {
      // A global renderer;
      // It ensures that only one thread is executing the rendering at any given time.
      template<Channel Sink>
      class Renderer final : public utils::Singleton<Renderer<Sink>> {
        friend class utils::Singleton<Renderer>;

        // Since nanoseconds are too small
        // and certain configurations make it impossible for rendering duration to be less than 1 μs,
        // we require the rendering interval to be specified in microseconds.
        static constexpr auto _default_working_interval =
          std::chrono::duration_cast<std::chrono::microseconds>( std::chrono::milliseconds( 40 ) );

#ifdef __cpp_inline_variables
        static std::atomic<std::chrono::microseconds> _working_interval;
#else
        static std::atomic<std::chrono::microseconds>& _working_interval() noexcept
        {
          static std::atomic<std::chrono::microseconds> instance { _default_working_interval };
          return instance;
        }
#endif

        std::atomic<std::uint64_t> quota_ { 0 };
        concurrent::ExceptionBox box_;
        wrappers::UniqueFunction<void()> task_;
        std::thread runner_;

        mutable std::condition_variable cond_var_;
        mutable std::mutex sched_mtx_;
        mutable std::mutex res_mtx_;

        /***********************************************************
          Dead
            └─ launch() → Dormant
                            ├─ activate<Async>() → Cycle → Loop
                            │
                            └─ activate<Signal>() → Shot → Pulse

          any state
            └─ abort() → Asleep → Dormant

          any state
            └─ drop() → Dead
        ***********************************************************/
        enum class Phase : std::uint8_t { Dead, Asleep, Dormant, Cycle, Loop, Shot, Pulse };
        std::atomic<Phase> state_ { Phase::Dead };

        void launch() &
        {
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

                case Phase::Cycle: {
                  auto expected = Phase::Cycle;
                  if ( !state_.compare_exchange_strong( expected, Phase::Loop, std::memory_order_release ) )
                    PACE__UNLIKELY break;
                  concurrent::atomic_notify_all( state_ );
                }
                  PACE__FALLTHROUGH;
                case Phase::Loop: {
                  {
                    std::lock_guard<std::mutex> lock { res_mtx_ };
                    task_();
                  }
                  std::unique_lock<std::mutex> lock { sched_mtx_ };
                  cond_var_.wait_for( lock, working_interval(), [this] {
                    return state_.load( std::memory_order_relaxed ) != Phase::Loop;
                  } );
                } break;

                case Phase::Shot: {
                  auto expected = Phase::Shot;
                  if ( !state_.compare_exchange_strong( expected, Phase::Pulse, std::memory_order_release ) )
                    PACE__UNLIKELY break;
                  concurrent::atomic_notify_all( state_ );
                  {
                    std::lock_guard<std::mutex> lock { res_mtx_ };
                    task_();
                  }
                  quota_.fetch_sub( 1, std::memory_order_relaxed );
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
                            || state_.load( std::memory_order_acquire ) != Phase::Pulse;
                      } );
                    },
                    1024 );
#endif
                  while ( state_.load( std::memory_order_acquire ) == Phase::Pulse
                          && quota_.fetch_sub( 1, std::memory_order_relaxed ) > 1 )
                    task_();
                } break;

                default: utils::unreachable();
                }
              }
            } catch ( ... ) {
              const auto dump = box_.try_store( std::current_exception() );
              concurrent::atomic_commit_all( state_,
                                             dump ? Phase::Dormant : Phase::Dead,
                                             std::memory_order_release );
              if ( !dump )
                throw;
            }
            PACE__ASSERT( state_ == Phase::Dead );
          } );
        }

        Renderer() = default;

      public:
        // Get the current working interval for all threads.
        PACE__NODISCARD static PACE__FORCEINLINE std::chrono::microseconds working_interval() noexcept
        {
#ifdef __cpp_inline_variables
          return _working_interval.load( std::memory_order_relaxed );
#else
          return _working_interval().load( std::memory_order_relaxed );
#endif
        }
        // Adjust the thread working interval between this loop and the next loop.
        static PACE__FORCEINLINE void working_interval( std::chrono::microseconds new_rate ) noexcept
        {
#ifdef __cpp_inline_variables
          _working_interval.store( new_rate, std::memory_order_relaxed );
#else
          _working_interval().store( new_rate, std::memory_order_relaxed );
#endif
        }

        ~Renderer() noexcept { reset(); }

        // `activate` guarantees to perform the render task at least once.
        template<Policy Mode>
        void activate() &
        { // This method is NOT designed for concurrent calls.
          box_.rethrow();
          PACE__ASSERT( task_ != nullptr );
          PACE__ASSERT( state_ == Phase::Dormant || state_ == Phase::Dead );
          if PACE__CXX17_CNSTXPR ( Mode == Policy::Sync )
            execute();
          else {
            if ( state_.load( std::memory_order_acquire ) == Phase::Dead ) {
              if ( runner_.get_id() == std::thread::id() )
                launch();
              else {
                reset();
                launch();
              }
            }
            PACE__ASSERT( state_ != Phase::Dead );
            PACE__CXX17_CNSTXPR const auto desired = []() PACE__CXX17_CNSTXPR noexcept {
              if PACE__CXX17_CNSTXPR ( Mode == Policy::Async )
                return Phase::Cycle;
              else
                return Phase::Shot;
            }();
            auto expected = Phase::Dormant;
            if ( state_.compare_exchange_strong( expected, desired, std::memory_order_release ) ) {
              if PACE__CXX17_CNSTXPR ( Mode == Policy::Signal )
                quota_.store( 1, std::memory_order_relaxed );
              concurrent::atomic_notify_one( state_ );
#ifndef __cpp_lib_atomic_wait
              {
                std::lock_guard<std::mutex> lock { sched_mtx_ };
                cond_var_.notify_one();
              }
#endif
              concurrent::atomic_wait( state_, desired, std::memory_order_acquire );
            }
          }
        }

        // Commit a rendering request.
        template<Policy Mode>
        PACE__FORCEINLINE void commit() &
        { // This method IS designed for concurrent calls.
          if PACE__CXX17_CNSTXPR ( Mode == Policy::Signal ) {
            quota_.fetch_add( 1, std::memory_order_relaxed );
            concurrent::atomic_notify_one( quota_ );
#ifndef __cpp_lib_atomic_wait
            std::lock_guard<std::mutex> lock { sched_mtx_ };
            cond_var_.notify_one();
#endif
          } else if PACE__CXX17_CNSTXPR ( Mode == Policy::Sync ) {
            // If the lock cannot be obtained,
            // it indicates that multiple threads are simultaneously calling this method.
            // At this point, at least one thread is capable of performing the rendering operation,
            // so there is no need to acquire the lock.
            if ( res_mtx_.try_lock() ) {
              std::lock_guard<std::mutex> lock { res_mtx_, std::adopt_lock };
              // To ensure that only one thread is rendering the bar to the OStream.
              task_();
            }
          }
        }

        // Execute a rendering.
        void execute() &
        { // This method IS designed for concurrent calls.
          std::lock_guard<std::mutex> lock { res_mtx_ };
          PACE__ASSERT( task_ != nullptr );
          task_();
        }

        // Trigger a rendering execution.
        template<Policy Mode>
        void trigger() & noexcept( Mode != Policy::Sync )
        {
          box_.rethrow();
          if PACE__CXX17_CNSTXPR ( Mode == Policy::Async ) {
            auto expected = Phase::Loop;
            if ( state_.compare_exchange_strong( expected, Phase::Cycle, std::memory_order_release ) ) {
              {
                std::lock_guard<std::mutex> lock { sched_mtx_ };
                cond_var_.notify_one();
              }
              concurrent::atomic_wait( state_, Phase::Cycle, std::memory_order_acquire );
            }
          } else if PACE__CXX17_CNSTXPR ( Mode == Policy::Signal ) {
            auto expected = Phase::Pulse;
            if ( state_.compare_exchange_strong( expected, Phase::Shot, std::memory_order_release ) ) {
              quota_.fetch_add( 1, std::memory_order_relaxed );
#ifdef __cpp_lib_atomic_wait
              concurrent::atomic_notify_one( quota_ );
#else
              {
                std::lock_guard<std::mutex> lock { sched_mtx_ };
                cond_var_.notify_one();
              }
#endif
              concurrent::atomic_wait( state_, Phase::Shot, std::memory_order_acquire );
            }
          } else if PACE__CXX17_CNSTXPR ( Mode == Policy::Sync )
            execute();
        }

        void abort() noexcept
        {
          const auto try_update = [this]( Phase expected ) noexcept {
            return concurrent::atomic_commit_one( state_,
                                                  expected,
                                                  Phase::Asleep,
                                                  std::memory_order_acq_rel );
          };
          bool waiting = false;
#ifdef __cpp_lib_atomic_wait
          if ( try_update( Phase::Cycle ) || try_update( Phase::Loop ) ) {
            waiting = true;
            std::lock_guard<std::mutex> lock { sched_mtx_ };
            cond_var_.notify_all();
          } else if ( try_update( Phase::Shot ) || try_update( Phase::Pulse ) ) {
            waiting = true;
            quota_.store( 1, std::memory_order_relaxed );
            concurrent::atomic_notify_all( quota_ );
          }
#else
          if ( try_update( Phase::Cycle ) || try_update( Phase::Shot ) || try_update( Phase::Loop )
               || try_update( Phase::Pulse ) ) {
            waiting = true;
            std::lock_guard<std::mutex> lock { sched_mtx_ };
            cond_var_.notify_all();
          }
#endif
          if ( waiting )
            concurrent::atomic_wait( state_, Phase::Asleep, std::memory_order_acquire );
        }

        template<typename F>
        void dismiss_then( F&& fn )
        { // This method is NOT designed for concurrent calls.
          abort();
          std::lock_guard<std::mutex> lock { res_mtx_ };
          task_ = nullptr;
          (void)fn();
        }
        void dismiss() noexcept
        { // This method is NOT designed for concurrent calls.
          dismiss_then( []() noexcept {} );
        }

        void reset() noexcept
        { // This method is NOT designed for concurrent calls.
          concurrent::atomic_commit_all( state_, Phase::Dead );
          quota_.store( 1, std::memory_order_relaxed );
          concurrent::atomic_notify_all( quota_ );
          {
            std::lock_guard<std::mutex> lock { sched_mtx_ };
            cond_var_.notify_all();
          }
          if ( runner_.joinable() )
            runner_.join();
          // Single-threaded assumption, no need for locking.
          runner_ = std::thread();
          task_   = nullptr;
        }

        PACE__NODISCARD bool try_appoint( wrappers::UniqueFunction<void()>&& task ) & noexcept
        { // This method IS designed for concurrent calls.
          std::lock_guard<std::mutex> lock { res_mtx_ };
          if ( task_ != nullptr )
            return false;
          task_ = std::move( task );
          return true;
        }

        PACE__NODISCARD PACE__FORCEINLINE bool interrupted() const noexcept { return !box_.empty(); }
        PACE__NODISCARD PACE__FORCEINLINE bool empty() const noexcept
        {
          std::lock_guard<std::mutex> lock { res_mtx_ };
          return task_ == nullptr;
        }
      };
#ifdef __cpp_inline_variables
      template<Channel Sink>
      PACE__CXX17_INLINE std::atomic<std::chrono::microseconds> Renderer<Sink>::_working_interval {
        Renderer<Sink>::_default_working_interval
      };
#endif
    } // namespace render
  } // namespace details
} // namespace pace

#endif
