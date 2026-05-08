#ifndef PGBAR_MOVABLE_REF
#define PGBAR_MOVABLE_REF

#include "../traits/Backport.hpp"
#include <utility>

namespace pgbar {
  namespace details {
    namespace wrappers {
      template<typename T>
      class MovableRef {
        T* ref_;

      public:
        constexpr MovableRef() noexcept : ref_ { nullptr } {}
        PGBAR__CXX14_CNSTXPR MovableRef( const MovableRef& )              = default;
        PGBAR__CXX14_CNSTXPR MovableRef& operator=( const MovableRef& ) & = default;
        PGBAR__CXX14_CNSTXPR MovableRef( MovableRef&& rhs ) noexcept : ref_ { rhs.ref_ }
        {
          rhs.ref_ = nullptr;
        }
        PGBAR__CXX14_CNSTXPR MovableRef& operator=( MovableRef&& rhs ) & noexcept
        {
          ref_     = rhs.ref_;
          rhs.ref_ = nullptr;
          return *this;
        }

        template<typename U,
                 typename = typename std::enable_if<
                   traits::AllOf<traits::Not<std::is_same<typename std::decay<U>::type, MovableRef>>,
                                 std::is_convertible<U&&, T&>>::value>::type>
        PGBAR__CXX17_CNSTXPR MovableRef( U&& x ) noexcept
        {
          T& t = std::forward<U>( x );
          ref_ = std::addressof( t );
        }
        PGBAR__CXX17_CNSTXPR MovableRef& operator=( T& ref ) & noexcept
        {
          ref_ = std::addressof( ref );
          return *this;
        }

        PGBAR__FORCEINLINE PGBAR__CXX14_CNSTXPR T& get() & noexcept
        {
          PGBAR__TRUST( ref_ != nullptr );
          return *ref_;
        }
        PGBAR__FORCEINLINE PGBAR__CXX14_CNSTXPR T& get() const& noexcept
        {
          PGBAR__TRUST( ref_ != nullptr );
          return *ref_;
        }
        PGBAR__FORCEINLINE PGBAR__CXX14_CNSTXPR T&& get() && noexcept
        {
          PGBAR__TRUST( ref_ != nullptr );
          return std::move( *ref_ );
        }

        PGBAR__FORCEINLINE PGBAR__CXX14_CNSTXPR T& operator*() & noexcept { return get(); }
        PGBAR__FORCEINLINE PGBAR__CXX14_CNSTXPR T& operator*() const& noexcept { return get(); }
        PGBAR__FORCEINLINE PGBAR__CXX14_CNSTXPR T&& operator*() && noexcept { return std::move( get() ); }

        PGBAR__FORCEINLINE PGBAR__CXX14_CNSTXPR T* operator->() const noexcept { return ref_; }

        // make it be invocable
        PGBAR__FORCEINLINE PGBAR__CXX14_CNSTXPR T& operator()() & noexcept { return get(); }
        PGBAR__FORCEINLINE PGBAR__CXX14_CNSTXPR T& operator()() const& noexcept { return get(); }
        PGBAR__FORCEINLINE PGBAR__CXX14_CNSTXPR T&& operator()() && noexcept { return std::move( get() ); }

        PGBAR__CXX14_CNSTXPR operator T&() & noexcept { return get(); }
        PGBAR__CXX14_CNSTXPR operator T&() const& noexcept { return get(); }
        PGBAR__CXX14_CNSTXPR operator T&&() && noexcept { return std::move( get() ); }

        explicit constexpr operator bool() const noexcept { return ref_ != nullptr; }

        PGBAR__CXX20_CNSTXPR void swap( MovableRef& other ) noexcept { std::swap( ref_, other.ref_ ); }
        friend PGBAR__CXX20_CNSTXPR void swap( MovableRef& a, MovableRef& b ) noexcept { a.swap( b ); }

        template<typename U>
        PGBAR__NODISCARD friend constexpr
          typename std::enable_if<std::is_same<typename std::decay<U>::type, T>::value, bool>::type
          operator==( const MovableRef& a, const MovableRef<const U>& b ) noexcept
        {
          return a.ref_ == b.ref_;
        }
        template<typename U>
        PGBAR__NODISCARD friend constexpr
          typename std::enable_if<std::is_same<typename std::decay<U>::type, T>::value, bool>::type
          operator!=( const MovableRef& a, const MovableRef<const U>& b ) noexcept
        {
          return !( a == b );
        }
        template<typename U>
        PGBAR__NODISCARD friend constexpr
          typename std::enable_if<std::is_same<typename std::decay<U>::type, T>::value, bool>::type
          operator==( const MovableRef<const U>& a, const MovableRef& b ) noexcept
        {
          return a.ref_ == b.ref_;
        }
        template<typename U>
        PGBAR__NODISCARD friend constexpr
          typename std::enable_if<std::is_same<typename std::decay<U>::type, T>::value, bool>::type
          operator!=( const MovableRef<const U>& a, const MovableRef& b ) noexcept
        {
          return !( b == a );
        }
        PGBAR__NODISCARD friend constexpr bool operator==( const MovableRef& a, const T& b ) noexcept
        {
          return a.ref_ == std::addressof( b );
        }
        template<typename U>
        PGBAR__NODISCARD friend constexpr bool operator!=( const MovableRef& a, const T& b ) noexcept
        {
          return !( a == b );
        }
        PGBAR__NODISCARD friend constexpr bool operator==( const T& a, const MovableRef& b ) noexcept
        {
          return b == a;
        }
        PGBAR__NODISCARD friend constexpr bool operator!=( const T& a, const MovableRef& b ) noexcept
        {
          return !( b == a );
        }
      };

#ifdef __cpp_deduction_guides
      template<typename T>
      MovableRef( T& ) -> MovableRef<T>;
#endif

      template<typename T>
      PGBAR__NODISCARD PGBAR__FORCEINLINE PGBAR__CXX17_CNSTXPR MovableRef<T> mref( T& x ) noexcept
      {
        return MovableRef<T>( x );
      }
      template<typename T>
      PGBAR__NODISCARD PGBAR__FORCEINLINE PGBAR__CXX14_CNSTXPR MovableRef<T> mref( MovableRef<T> x ) noexcept
      {
        if ( x )
          return MovableRef<T>( *x );
        return MovableRef<T>();
      }
      template<typename T>
      void mref( const T&& ) = delete;
    } // namespace wrappers
  } // namespace details
} // namespace pgbar

#endif
