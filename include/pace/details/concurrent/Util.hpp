#ifndef PACE_CONCURRENT_UTIL
#define PACE_CONCURRENT_UTIL

#include "../core/Core.hpp"
#include "../core/Types.hpp"
#include <atomic>
#include <thread>

namespace pace {
  namespace details {
    namespace concurrent {
      // spin with specified action
      template<typename F, typename Act>
      PACE__FORCEINLINE void spin_with( F&& pred, Act&& action, types::Size threshold )
        noexcept( noexcept( pred() ) && noexcept( action() ) )
      {
        for ( types::Size cnt = 0; !pred(); ) {
          if ( cnt >= threshold )
            (void)action();
          ++cnt;
        }
      }

      // Wait for pred to be true.
      template<typename F>
      PACE__FORCEINLINE void spin_wait( F&& pred, types::Size threshold ) noexcept( noexcept( pred() ) )
      {
        spin_with( std::forward<F>( pred ), []() noexcept { std::this_thread::yield(); }, threshold );
      }
      template<typename F>
      PACE__FORCEINLINE void spin_wait( F&& pred ) noexcept( noexcept( pred() ) )
      { spin_wait( std::forward<F>( pred ), 128 ); }

      template<typename T>
      PACE__FORCEINLINE void atomic_notify_one( std::atomic<T>& atom ) noexcept
      {
#ifdef __cpp_lib_atomic_wait
        atom.notify_one();
#else
        (void)atom;
#endif
      }
      template<typename T>
      PACE__FORCEINLINE void atomic_notify_all( std::atomic<T>& atom ) noexcept
      {
#ifdef __cpp_lib_atomic_wait
        atom.notify_all();
#else
        (void)atom;
#endif
      }

      template<typename T>
      PACE__FORCEINLINE bool atomic_commit_one( std::atomic<T>& atom,
                                                T expected,
                                                T alter,
                                                std::memory_order order = std::memory_order_seq_cst ) noexcept
      {
        bool cas = atom.compare_exchange_strong( expected, alter, order );
        if ( cas )
          atomic_notify_one( atom );
        return cas;
      }
      template<typename T>
      PACE__FORCEINLINE void atomic_commit_all( std::atomic<T>& atom,
                                                T alter,
                                                std::memory_order order = std::memory_order_seq_cst ) noexcept
      {
        atom.store( alter, order );
        atomic_notify_all( atom );
      }
      template<typename T>
      PACE__FORCEINLINE bool atomic_commit_all( std::atomic<T>& atom,
                                                T expected,
                                                T alter,
                                                std::memory_order order = std::memory_order_seq_cst ) noexcept
      {
        bool cas = atom.compare_exchange_strong( expected, alter, order );
        if ( cas )
          atomic_notify_all( atom );
        return cas;
      }
    } // namespace concurrent
  } // namespace details
} // namespace pace

#endif
