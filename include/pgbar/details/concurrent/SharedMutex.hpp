#ifndef PGBAR_SHARED_MUTEX
#define PGBAR_SHARED_MUTEX

#include <cstddef>
#ifdef __cpp_lib_shared_mutex
# include <shared_mutex>
#else
# include "../core/Core.hpp"
# include <atomic>
# include <mutex>
# include <thread>
#endif

namespace pgbar {
  namespace details {
    namespace concurrent {
#ifdef __cpp_lib_shared_mutex
      using SharedMutex = std::shared_mutex;
#else
      // A simple `Shared Mutex` implementation for any C++ version.
      class SharedMutex final {
      protected:
        std::atomic<std::uint64_t> num_readers_;
        std::mutex writer_mtx_;
        /**
         * Although the `lock()` and `unlock()` functions of `std::mutex`
         * in the standard library are not marked as `noexcept`,
         * in practice, it is very rare for exceptions to be thrown,
         * and usually indicate a bug in the program itself.
         * Therefore, it is assumed here that `std::mutex` is `noexcept`.

         * For more details, see: https://stackoverflow.com/questions/17551256
         */

      public:
        SharedMutex( const SharedMutex& )            = delete;
        SharedMutex& operator=( const SharedMutex& ) = delete;

        SharedMutex() noexcept : num_readers_ { 0 } {}
        ~SharedMutex() = default;

        void lock() & noexcept
        {
          while ( true ) {
            while ( num_readers_.load( std::memory_order_acquire ) != 0 )
              std::this_thread::yield();

            writer_mtx_.lock();
            if ( num_readers_.load( std::memory_order_acquire ) == 0 )
              break;
            else // unlock it and wait for readers to finish
              writer_mtx_.unlock();
          }
        }
        bool try_lock() & noexcept
        {
          if ( num_readers_.load( std::memory_order_acquire ) == 0 && writer_mtx_.try_lock() ) {
            if ( num_readers_.load( std::memory_order_acquire ) == 0 )
              return true;
            else
              writer_mtx_.unlock();
          }
          return false;
        }
        void unlock() & noexcept { writer_mtx_.unlock(); }

        void lock_shared() & noexcept
        {
          writer_mtx_.lock();

          num_readers_.fetch_add( 1, std::memory_order_release );
          PGBAR__ASSERT( num_readers_ > 0 ); // overflow checking

          writer_mtx_.unlock();
        }
        bool try_lock_shared() & noexcept
        {
          if ( writer_mtx_.try_lock() ) {
            num_readers_.fetch_add( 1, std::memory_order_release );
            PGBAR__ASSERT( num_readers_ > 0 );
            writer_mtx_.unlock();
            return true;
          }
          return false;
        }
        void unlock_shared() & noexcept
        {
          PGBAR__ASSERT( num_readers_ > 0 ); // underflow checking
          num_readers_.fetch_sub( 1, std::memory_order_release );
        }
      };
#endif
    }
  } // namespace details
} // namespace pgbar

#endif
