#ifndef PACE_EXCEPTION_BOX
#define PACE_EXCEPTION_BOX

#include "SharedLock.hpp"
#include "SharedMutex.hpp"
#include <exception>
#include <mutex>

namespace pace {
  namespace details {
    namespace concurrent {
      // A nullable container that holds an exception pointer.
      class ExceptionBox final {
        std::exception_ptr exception_;
        mutable SharedMutex rw_mtx_;

      public:
        ExceptionBox()  = default;
        ~ExceptionBox() = default;

        ExceptionBox( ExceptionBox&& rhs ) noexcept : ExceptionBox()
        {
          using std::swap;
          swap( exception_, rhs.exception_ );
        }
        ExceptionBox& operator=( ExceptionBox&& rhs ) & noexcept
        {
          PACE__TRUST( this != &rhs );
          // Exception pointers should not be discarded due to movement semantics.
          // Thus we only swap them here.
          swap( rhs );
          return *this;
        }

        PACE__NODISCARD PACE__FORCEINLINE bool empty() const noexcept
        {
          SharedLock<SharedMutex> lock { rw_mtx_ };
          return !static_cast<bool>( exception_ );
        }

        // Store the exception if it is empty and return true, otherwise return false.
        PACE__NODISCARD PACE__FORCEINLINE bool try_store( const std::exception_ptr& e ) & noexcept
        {
          std::lock_guard<SharedMutex> lock { rw_mtx_ };
          if ( exception_ )
            return false;
          exception_ = e;
          return true;
        }
        PACE__FORCEINLINE std::exception_ptr load() const noexcept
        {
          SharedLock<SharedMutex> lock { rw_mtx_ };
          return exception_;
        }
        PACE__FORCEINLINE ExceptionBox& clear() noexcept
        {
          std::lock_guard<SharedMutex> lock { rw_mtx_ };
          exception_ = std::exception_ptr();
          return *this;
        }

        // Rethrow the exception pointed if it isn't null.
        void rethrow()
        {
          std::lock_guard<SharedMutex> lock { rw_mtx_ };
          if ( exception_ ) {
            auto exception_ptr = exception_;
            exception_         = std::exception_ptr();
            if ( exception_ptr )
              std::rethrow_exception( std::move( exception_ptr ) );
          }
        }

        void swap( ExceptionBox& other ) noexcept
        {
          PACE__TRUST( this != &other );
          using std::swap; // ADL custom point
          swap( exception_, other.exception_ );
        }
        friend void swap( ExceptionBox& a, ExceptionBox& b ) noexcept { a.swap( b ); }
      };
    } // namespace concurrent
  } // namespace details
} // namespace pace

#endif
