#ifndef PACE_MOVABLE_REF
#define PACE_MOVABLE_REF

#include "../traits/Backport.hpp"
#include <utility>

namespace pace {
  namespace details {
    namespace wrappers {
      template<typename T>
      class MovableRef {
        T* ref_;

      public:
        constexpr MovableRef() noexcept : ref_ { nullptr } {}
        PACE__CXX14_CNSTXPR MovableRef( const MovableRef& )              = default;
        PACE__CXX14_CNSTXPR MovableRef& operator=( const MovableRef& ) & = default;
        PACE__CXX14_CNSTXPR MovableRef( MovableRef&& rhs ) noexcept : ref_ { rhs.ref_ }
        { rhs.ref_ = nullptr; }
        PACE__CXX14_CNSTXPR MovableRef& operator=( MovableRef&& rhs ) & noexcept
        {
          ref_     = rhs.ref_;
          rhs.ref_ = nullptr;
          return *this;
        }

        template<typename U,
                 typename = typename std::enable_if<
                   traits::AllOf<traits::Not<std::is_same<typename std::decay<U>::type, MovableRef>>,
                                 std::is_convertible<U&&, T&>>::value>::type>
        PACE__CXX17_CNSTXPR MovableRef( U&& x ) noexcept
        {
          T& t = std::forward<U>( x );
          ref_ = std::addressof( t );
        }
        PACE__CXX17_CNSTXPR MovableRef& operator=( T& ref ) & noexcept
        {
          ref_ = std::addressof( ref );
          return *this;
        }

        PACE__FORCEINLINE PACE__CXX14_CNSTXPR T& get() & noexcept
        {
          PACE__TRUST( ref_ != nullptr );
          return *ref_;
        }
        PACE__FORCEINLINE PACE__CXX14_CNSTXPR T& get() const& noexcept
        {
          PACE__TRUST( ref_ != nullptr );
          return *ref_;
        }
        PACE__FORCEINLINE PACE__CXX14_CNSTXPR T&& get() && noexcept
        {
          PACE__TRUST( ref_ != nullptr );
          return std::move( *ref_ );
        }

        PACE__FORCEINLINE PACE__CXX14_CNSTXPR T& operator*() & noexcept { return get(); }
        PACE__FORCEINLINE PACE__CXX14_CNSTXPR T& operator*() const& noexcept { return get(); }
        PACE__FORCEINLINE PACE__CXX14_CNSTXPR T&& operator*() && noexcept { return std::move( get() ); }

        PACE__FORCEINLINE PACE__CXX14_CNSTXPR T* operator->() const noexcept { return ref_; }

        // make it be invocable
        PACE__FORCEINLINE PACE__CXX14_CNSTXPR T& operator()() & noexcept { return get(); }
        PACE__FORCEINLINE PACE__CXX14_CNSTXPR T& operator()() const& noexcept { return get(); }
        PACE__FORCEINLINE PACE__CXX14_CNSTXPR T&& operator()() && noexcept { return std::move( get() ); }

        PACE__CXX14_CNSTXPR operator T&() & noexcept { return get(); }
        PACE__CXX14_CNSTXPR operator T&() const& noexcept { return get(); }
        PACE__CXX14_CNSTXPR operator T&&() && noexcept { return std::move( get() ); }

        explicit constexpr operator bool() const noexcept { return ref_ != nullptr; }

        PACE__CXX20_CNSTXPR void swap( MovableRef& other ) noexcept { std::swap( ref_, other.ref_ ); }
        friend PACE__CXX20_CNSTXPR void swap( MovableRef& a, MovableRef& b ) noexcept { a.swap( b ); }

        template<typename U>
        PACE__NODISCARD friend constexpr
          typename std::enable_if<std::is_same<typename std::decay<U>::type, T>::value, bool>::type
          operator==( const MovableRef& a, const MovableRef<const U>& b ) noexcept
        { return a.ref_ == b.ref_; }
        template<typename U>
        PACE__NODISCARD friend constexpr
          typename std::enable_if<std::is_same<typename std::decay<U>::type, T>::value, bool>::type
          operator!=( const MovableRef& a, const MovableRef<const U>& b ) noexcept
        { return !( a == b ); }
        template<typename U>
        PACE__NODISCARD friend constexpr
          typename std::enable_if<std::is_same<typename std::decay<U>::type, T>::value, bool>::type
          operator==( const MovableRef<const U>& a, const MovableRef& b ) noexcept
        { return a.ref_ == b.ref_; }
        template<typename U>
        PACE__NODISCARD friend constexpr
          typename std::enable_if<std::is_same<typename std::decay<U>::type, T>::value, bool>::type
          operator!=( const MovableRef<const U>& a, const MovableRef& b ) noexcept
        { return !( b == a ); }
        PACE__NODISCARD friend constexpr bool operator==( const MovableRef& a, const T& b ) noexcept
        { return a.ref_ == std::addressof( b ); }
        template<typename U>
        PACE__NODISCARD friend constexpr bool operator!=( const MovableRef& a, const T& b ) noexcept
        { return !( a == b ); }
        PACE__NODISCARD friend constexpr bool operator==( const T& a, const MovableRef& b ) noexcept
        { return b == a; }
        PACE__NODISCARD friend constexpr bool operator!=( const T& a, const MovableRef& b ) noexcept
        { return !( b == a ); }
      };

#ifdef __cpp_deduction_guides
      template<typename T>
      MovableRef( T& ) -> MovableRef<T>;
#endif

      template<typename T>
      PACE__NODISCARD PACE__FORCEINLINE PACE__CXX17_CNSTXPR MovableRef<T> mref( T& x ) noexcept
      { return MovableRef<T>( x ); }
      template<typename T>
      PACE__NODISCARD PACE__FORCEINLINE PACE__CXX14_CNSTXPR MovableRef<T> mref( MovableRef<T> x ) noexcept
      {
        if ( x )
          return MovableRef<T>( *x );
        return MovableRef<T>();
      }
      template<typename T>
      void mref( const T&& ) = delete;
    } // namespace wrappers
  } // namespace details
} // namespace pace

#endif
