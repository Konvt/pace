#ifndef PACE_INCREMENTAL
#define PACE_INCREMENTAL

#include "../core/Core.hpp"
#include <atomic>
#include <cstdint>

namespace pace {
  namespace details {
    namespace behaviors {
      template<typename Base, typename Derived>
      class Incremental : public Base {
      protected:
        std::atomic<std::uint64_t> task_cnt_ { 0 };
        std::uint64_t task_end_;

        Incremental() = default;
        Incremental( Incremental&& rhs ) noexcept : Base( std::move( rhs ) ) {}
        Incremental& operator=( Incremental&& rhs ) & noexcept
        {
          this->Base::operator=( std::move( rhs ) );
          return *this;
        }
        ~Incremental() = default;

      public:
        using Base::Base;

        // Get the progress of the task.
        PACE__NODISCARD std::uint64_t progress() const noexcept
        { return task_cnt_.load( std::memory_order_relaxed ); }
        // Check if the test progress bar has been completed.
        PACE__NODISCARD PACE__FORCEINLINE bool completive() const noexcept
        { return static_cast<const Derived*>( this )->test_completion(); }

        PACE__FORCEINLINE void tick() & final
        {
          this->do_tick( [this]() noexcept { task_cnt_.fetch_add( 1, std::memory_order_relaxed ); } );
        }
        PACE__FORCEINLINE void tick( std::uint64_t next_step ) &
        {
          this->do_tick( [&]() noexcept {
            const auto task_cnt = task_cnt_.load( std::memory_order_relaxed );
            task_cnt_.fetch_add( task_cnt + next_step > task_end_ ? task_end_ - task_cnt : next_step,
                                 std::memory_order_relaxed );
          } );
        }
        /**
         * Set the iteration step of the progress bar to a specified percentage.
         * Ignore the call if the iteration count exceeds the given percentage.
         * If `percentage` is bigger than 100, it will be set to 100.
         *
         * @param percentage Value range: [0, 100].
         */
        PACE__FORCEINLINE void tick_to( std::uint8_t percentage ) &
        {
          this->do_tick( [&]() noexcept {
            const auto updater = [this]( std::uint64_t target ) noexcept {
              auto current = task_cnt_.load( std::memory_order_relaxed );
              while ( !task_cnt_.compare_exchange_weak( current, target, std::memory_order_relaxed )
                      && target <= current ) {}
            };
            if ( percentage < 100 ) {
              const auto target = static_cast<std::uint64_t>( task_end_ * percentage * 0.01 );
              PACE__ASSERT( target < task_end_ );
              updater( target );
            } else
              updater( task_end_ );
          } );
        }
      };
    } // namespace behaviors
  } // namespace details
} // namespace pace

#endif
