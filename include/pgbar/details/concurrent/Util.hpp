#ifndef PGBAR_CONCURRENT_UTIL
#define PGBAR_CONCURRENT_UTIL

#include "../core/Core.hpp"
#include "../types/Types.hpp"
#include <atomic>
#include <thread>

namespace pgbar {
  namespace _details {
    namespace concurrent {
      // spin with specified action
      template<typename F, typename Act>
      PGBAR__FORCEINLINE void spin_with( F&& pred, Act&& action, types::Size threshold )
        noexcept( noexcept( pred() ) && noexcept( action() ) )
      {
        for ( types::Size cnt = 0; !pred(); ) {
          if ( cnt >= threshold )
            (void)action();
          ++cnt;
        }
      }
      template<typename F, typename Act, typename Rep, typename Period>
      PGBAR__FORCEINLINE bool spin_with_for( F&& pred,
                                             Act&& action,
                                             types::Size threshold,
                                             const std::chrono::duration<Rep, Period>& timeout )
        noexcept( noexcept( pred() ) && noexcept( action() ) )
      {
        const auto start = std::chrono::steady_clock::now();
        for ( types::Size cnt; !pred(); ) {
          if ( std::chrono::steady_clock::now() - start >= timeout )
            return false;
          else if ( cnt >= threshold )
            (void)action();
          ++cnt;
        }
        return true;
      }

      // Wait for pred to be true.
      template<typename F>
      PGBAR__FORCEINLINE void spin_wait( F&& pred, types::Size threshold ) noexcept( noexcept( pred() ) )
      {
        spin_with( std::forward<F>( pred ), []() noexcept { std::this_thread::yield(); }, threshold );
      }
      template<typename F>
      PGBAR__FORCEINLINE void spin_wait( F&& pred ) noexcept( noexcept( pred() ) )
      {
        spin_wait( std::forward<F>( pred ), 128 );
      }

      template<typename F, typename Rep, typename Period>
      PGBAR__FORCEINLINE bool spin_wait_for( F&& pred,
                                             types::Size threshold,
                                             const std::chrono::duration<Rep, Period>& timeout )
        noexcept( noexcept( pred() ) )
      {
        return spin_with_for(
          std::forward<F>( pred ),
          []() noexcept { std::this_thread::yield(); },
          threshold,
          timeout );
      }
      template<typename F, typename Rep, typename Period>
      PGBAR__FORCEINLINE bool spin_wait_for( F&& pred, const std::chrono::duration<Rep, Period>& timeout )
        noexcept( noexcept( pred() ) )
      {
        return spin_wait_for( std::forward<F>( pred ), 128, timeout );
      }

      template<typename T>
      PGBAR__FORCEINLINE void atomic_commit_all(
        std::atomic<T>& atom,
        T alter,
        std::memory_order order = std::memory_order_release ) noexcept
      {
        atom.store( alter, order );
#ifdef __cpp_lib_atomic_wait
        atom.notify_all();
#endif
      }
      template<typename T>
      PGBAR__FORCEINLINE bool atomic_commit_all(
        std::atomic<T>& atom,
        T expected,
        T alter,
        std::memory_order order = std::memory_order_release ) noexcept
      {
        bool cas = atom.compare_exchange_strong( expected, alter, order );
#ifdef __cpp_lib_atomic_wait
        if ( cas )
          atom.notify_all();
#endif
        return cas;
      }
      template<typename T>
      PGBAR__FORCEINLINE bool atomic_commit_one(
        std::atomic<T>& atom,
        T expected,
        T alter,
        std::memory_order order = std::memory_order_release ) noexcept
      {
        bool cas = atom.compare_exchange_strong( expected, alter, order );
#ifdef __cpp_lib_atomic_wait
        if ( cas )
          atom.notify_one();
#endif
        return cas;
      }
    } // namespace concurrent
  } // namespace _details
} // namespace pgbar

#endif
