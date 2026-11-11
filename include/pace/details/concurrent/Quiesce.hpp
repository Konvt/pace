#ifndef PACE_QUIESCE
#define PACE_QUIESCE

#include "../utils/Backport.hpp"
#include "Util.hpp"
#include <atomic>

namespace pace {
  namespace details {
    namespace concurrent {
      class Quiesce {
        std::atomic<std::size_t> counter_ { 0 };

        template<bool NotifyAll>
        class Ticket {
          friend Quiesce;

          Quiesce& gate_;
#if !PACE__CXX17
          bool expired_ = false;
#endif

          Ticket( Quiesce& gate ) noexcept : gate_ { gate }
          { gate_.counter_.fetch_add( 1, std::memory_order_relaxed ); }

        public:
          Ticket( const Ticket& )            = delete;
          Ticket& operator=( const Ticket& ) = delete;

#if !PACE__CXX17
          Ticket( Ticket&& rhs ) noexcept
            : gate_ { rhs.gate_ }, expired_ { utils::exchange( rhs.expired_, true ) }
          {}
#endif

          ~Ticket() noexcept
          {
#if !PACE__CXX17
            if ( !expired_ )
#endif
              if ( gate_.counter_.fetch_sub( 1, std::memory_order_release ) == 1 ) {
                std::atomic_thread_fence( std::memory_order_acquire );
                if PACE__CXX17_CNSTXPR ( NotifyAll )
                  concurrent::atomic_notify_all( gate_.counter_ );
                else
                  concurrent::atomic_notify_one( gate_.counter_ );
              }
          }
        };

      public:
        Quiesce()                            = default;
        Quiesce( const Quiesce& )            = delete;
        Quiesce& operator=( const Quiesce& ) = delete;

        // Acquire a ticket that notifies one waiter upon release.
        PACE__NODISCARD Ticket<false> enter() & noexcept { return { *this }; }
        // Acquire a ticket that notifies all waiters upon release.
        PACE__NODISCARD Ticket<true> enroll() & noexcept { return { *this }; }

        // Wait for a notification when all tickets have been released.
        void quiet() noexcept
        {
          auto count = counter_.load( std::memory_order_acquire );
          if ( count > 0 )
            concurrent::atomic_wait( counter_, count, std::memory_order_acquire );
        }
        // Wait until all tickets have been released.
        void quiesce() noexcept
        {
          for ( auto count = counter_.load( std::memory_order_acquire ); count > 0;
                count      = counter_.load( std::memory_order_acquire ) )
            concurrent::atomic_wait( counter_, count, std::memory_order_acquire );
        }
      };
    } // namespace concurrent
  } // namespace details
} // namespace pace

#endif
