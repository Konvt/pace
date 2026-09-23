#ifndef PACE_SHARED_MUTEX
#define PACE_SHARED_MUTEX

#include <cstddef>

#if defined( __cpp_lib_shared_mutex ) || defined( __cpp_lib_shared_timed_mutex )
# include <shared_mutex>
#else
# include "../core/Platform.hpp"
# if _WIN32_WINNT >= 0x0601
#  define PACE__WIN_SRWLOCK
# elif defined( _POSIX_READER_WRITER_LOCKS ) && _POSIX_READER_WRITER_LOCKS >= 0
#  include <cerrno>
#  include <pthread.h>
#  include <system_error>
#  define PACE__PTHREAD_RWLOCK
# else
#  include <atomic>
#  include <condition_variable>
#  include <mutex>
# endif
#endif

namespace pace {
  namespace details {
    namespace concurrent {
#ifdef __cpp_lib_shared_mutex
      using SharedMutex = std::shared_mutex;
#elif defined( __cpp_lib_shared_timed_mutex )
      using SharedMutex = std::shared_timed_mutex;
#else
      class SharedMutex {
# ifdef PACE__WIN_SRWLOCK
        SRWLOCK native_;
# elif defined( PACE__PTHREAD_RWLOCK )
        pthread_rwlock_t native_;
# else
        std::mutex gate_;
        std::condition_variable cv_;
        std::atomic<std::size_t> readers_ { 0 };
# endif

      public:
# ifdef PACE__WIN_SRWLOCK
        using native_handle_type = SRWLOCK*;
# elif defined( PACE__PTHREAD_RWLOCK )
        using native_handle_type = pthread_rwlock_t*;
# endif

# if !defined( PACE__PTHREAD_RWLOCK ) && !defined( PACE__WIN_SRWLOCK )
        SharedMutex() = default;
# else
        SharedMutex()
        {
#  ifdef PACE__WIN_SRWLOCK
          InitializeSRWLock( &native_ );
#  elif defined( PACE__PTHREAD_RWLOCK )
          const auto result = pthread_rwlock_init( &native_, nullptr );
          if ( result != 0 )
            throw std::system_error( result, std::system_category(), "pthread_rwlock_init" );
#  endif
        }
# endif

        ~SharedMutex() noexcept
        {
# ifdef PACE__PTHREAD_RWLOCK
          // The standard mutex destructor is noexcept.
          // Destroying a locked mutex is already outside the valid lifetime requirements,
          // so the error cannot usefully be propagated here.
          (void)pthread_rwlock_destroy( &native_ );
# endif
        }

        SharedMutex( const SharedMutex& )            = delete;
        SharedMutex& operator=( const SharedMutex& ) = delete;

        void lock()
        {
# ifdef PACE__WIN_SRWLOCK
          AcquireSRWLockExclusive( &native_ );
# elif defined( PACE__PTHREAD_RWLOCK )
          const auto result = pthread_rwlock_wrlock( &native_ );
          if ( result != 0 )
            throw std::system_error( result, std::system_category(), "pthread_rwlock_wrlock" );
# else
          std::unique_lock<std::mutex> lock { gate_ };
          cv_.wait( lock, [this]() noexcept { return readers_.load( std::memory_order_relaxed ) == 0; } );
          lock.release();
# endif
        }

        bool try_lock()
        {
# ifdef PACE__WIN_SRWLOCK
          return TryAcquireSRWLockExclusive( &native_ ) != FALSE;
# elif defined( PACE__PTHREAD_RWLOCK )
          const auto result = pthread_rwlock_trywrlock( &native_ );
          if ( result == 0 )
            return true;
          if ( result == EBUSY )
            return false;
          throw std::system_error( result, std::system_category(), "pthread_rwlock_trywrlock" );
# else
          if ( readers_.load( std::memory_order_relaxed ) == 0 && gate_.try_lock() ) {
            if ( readers_.load( std::memory_order_relaxed ) == 0 )
              return true;
            gate_.unlock();
          }
          return false;
# endif
        }

        void unlock() noexcept
        {
# ifdef PACE__WIN_SRWLOCK
          ReleaseSRWLockExclusive( &native_ );
# elif defined( PACE__PTHREAD_RWLOCK )
          (void)pthread_rwlock_unlock( &native_ );
# else
          gate_.unlock();
# endif
        }

        void lock_shared()
        {
# ifdef PACE__WIN_SRWLOCK
          AcquireSRWLockShared( &native_ );
# elif defined( PACE__PTHREAD_RWLOCK )
          const auto result = pthread_rwlock_rdlock( &native_ );
          if ( result != 0 )
            throw std::system_error( result, std::system_category(), "pthread_rwlock_rdlock" );
# else
          std::lock_guard<std::mutex> lock { gate_ };
          readers_.fetch_add( 1, std::memory_order_relaxed );
          PACE__ASSERT( readers_ > 0 ); // overflow checking
# endif
        }

        bool try_lock_shared()
        {
# ifdef PACE__WIN_SRWLOCK
          return TryAcquireSRWLockShared( &native_ ) != FALSE;
# elif defined( PACE__PTHREAD_RWLOCK )
          const auto result = pthread_rwlock_tryrdlock( &native_ );
          if ( result == 0 )
            return true;
          if ( result == EBUSY )
            return false;
          throw std::system_error( result, std::system_category(), "pthread_rwlock_tryrdlock" );
# else
          if ( gate_.try_lock() ) {
            readers_.fetch_add( 1, std::memory_order_relaxed );
            PACE__ASSERT( readers_ > 0 );
            gate_.unlock();
            return true;
          }
          return false;
# endif
        }

        void unlock_shared() noexcept
        {
# ifdef PACE__WIN_SRWLOCK
          ReleaseSRWLockShared( &native_ );
# elif defined( PACE__PTHREAD_RWLOCK )
          (void)pthread_rwlock_unlock( &native_ );
# else
          PACE__ASSERT( readers_ > 0 ); // underflow checking
          if ( readers_.fetch_sub( 1, std::memory_order_relaxed ) == 1 ) {
            std::lock_guard<std::mutex> lock { gate_ };
            cv_.notify_all();
          }
# endif
        }

# if defined( PACE__WIN_SRWLOCK ) || defined( PACE__PTHREAD_RWLOCK )
        native_handle_type native_handle() noexcept { return &native_; }
# endif
      };
#endif
    } // namespace concurrent
  } // namespace details
} // namespace pace

#undef PACE__WIN_SRWLOCK
#undef PACE__PTHREAD_RWLOCK

#endif
