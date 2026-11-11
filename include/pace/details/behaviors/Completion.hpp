// This file serves the Automation component rather than providing functional supports for the progress bar.
#ifndef PACE_COMPLETION
#define PACE_COMPLETION

#include "Iterable.hpp"

namespace pace {
  namespace details {
    namespace behaviors {
      template<typename Derived>
      struct Completion : public Iterable<Derived> {
        PACE__FORCEINLINE void tick() &
        {
          auto& derived = static_cast<Derived&>( *this );
          derived->tick();
          if ( derived->completive() )
            derived->finish();
        }
        PACE__FORCEINLINE void tick( std::uint64_t step ) &
        {
          auto& derived = static_cast<Derived&>( *this );
          derived->tick( step );
          if ( derived->completive() )
            derived->finish();
        }
        PACE__FORCEINLINE void tick_to( std::uint8_t percentage ) &
        {
          auto& derived = static_cast<Derived&>( *this );
          derived->tick_to( percentage );
          if ( derived->completive() )
            derived->finish();
        }
      };
      template<typename Derived>
      struct CompletionWith : public Iterable<Derived> {
      private:
        // Make sure only one thread can call `finish`.
        std::atomic_flag gate_ = ATOMIC_FLAG_INIT;

      protected:
        CompletionWith() = default;
        CompletionWith( CompletionWith&& ) noexcept {}

      public:
        PACE__FORCEINLINE void tick() &
        {
          auto& derived = static_cast<Derived&>( *this );
          derived->tick();
          if ( derived->completive() && !gate_.test_and_set( std::memory_order_relaxed ) ) {
            // We have emphasized that the cumulative number of `tick` steps must not exceed the quota,
            // and that `tick` is irreversible. Therefore, even if an exception is thrown here,
            // we will not roll back `gate_`. In this case, calling `tick` again is already
            // an illegal operation, and the user must shut down rendering themselves.
            derived.on_completion( *derived );
            derived->finish();
          }
        }
        PACE__FORCEINLINE void tick( std::uint64_t step ) &
        {
          auto& derived = static_cast<Derived&>( *this );
          derived->tick( step );
          if ( derived->completive() && !gate_.test_and_set( std::memory_order_relaxed ) ) {
            derived.on_completion( *derived );
            derived->finish();
          }
        }
        PACE__FORCEINLINE void tick_to( std::uint8_t percentage ) &
        {
          auto& derived = static_cast<Derived&>( *this );
          derived->tick_to( percentage );
          if ( derived->completive() && !gate_.test_and_set( std::memory_order_relaxed ) ) {
            derived.on_completion( *derived );
            derived->finish();
          }
        }

        void reset() & noexcept { gate_.clear( std::memory_order_relaxed ); }
      };
    } // namespace behaviors
  } // namespace details
} // namespace pace

#endif
