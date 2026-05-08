#ifndef PGBAR_SHARED_LOCK
#define PGBAR_SHARED_LOCK

#include "../core/Core.hpp"
#if PGBAR__CXX14
# include <shared_mutex>
#else
# include <mutex>
#endif

namespace pgbar {
  namespace _details {
    namespace concurrent {
#if PGBAR__CXX14
      template<typename Mtx>
      using SharedLock = std::shared_lock<Mtx>;
#else
      template<typename Mtx>
      class SharedLock final {
        Mtx* mtx_;
        bool is_owner_;

      public:
        using mutex_type = Mtx;

        SharedLock( SharedLock&& rhs ) noexcept : mtx_ { rhs.mtx_ }, is_owner_ { rhs.is_owner_ }
        {
          rhs.mtx_      = nullptr;
          rhs.is_owner_ = false;
        }
        SharedLock& operator=( SharedLock&& rhs ) & noexcept
        {
          PGBAR__TRUST( this != &rhs );
          if ( is_owner_ )
            mtx_->unlock_shared();
          mtx_      = nullptr;
          is_owner_ = false;
          std::swap( mtx_, rhs.mtx_ );
          std::swap( is_owner_, rhs.is_owner_ );
          return *this;
        }

        SharedLock( mutex_type& m ) noexcept( noexcept( mtx_->lock_shared() ) )
          : mtx_ { std::addressof( m ) }, is_owner_ { false }
        {
          mtx_->lock_shared();
          is_owner_ = true;
        }
        SharedLock( mutex_type& m, std::defer_lock_t ) noexcept
          : mtx_ { std::addressof( m ) }, is_owner_ { false }
        {}
        SharedLock( mutex_type& m, std::adopt_lock_t ) noexcept
          : mtx_ { std::addressof( m ) }, is_owner_ { true }
        {}
        ~SharedLock() noexcept
        {
          if ( is_owner_ )
            mtx_->unlock_shared();
        }

        PGBAR__FORCEINLINE void lock() & noexcept
        {
          PGBAR__TRUST( is_owner_ == false );
          mtx_->lock_shared();
          is_owner_ = true;
        }
        PGBAR__FORCEINLINE bool try_lock() & noexcept
        {
          PGBAR__TRUST( is_owner_ == false );
          is_owner_ = mtx_->try_lock_shared();
          return is_owner_;
        }
        PGBAR__FORCEINLINE void unlock() & noexcept
        {
          PGBAR__TRUST( is_owner_ == true );
          mtx_->unlock_shared();
          is_owner_ = false;
        }
      };
#endif
    }
  } // namespace _details
} // namespace pgbar
#endif
