#ifndef PACE_UTILS_BACKPORT
#define PACE_UTILS_BACKPORT

#include "../core/Core.hpp"
#include "../core/Types.hpp"
#include <exception>
#include <functional>
#include <memory>
#include <utility>
#if !defined( __cpp_lib_invoke ) || ( defined( _MSC_VER ) && !defined( __cpp_lib_is_invocable ) )
# include "../traits/Backport.hpp"
# include "../traits/Concept.hpp"
#elif !PACE__CXX14
# include "../traits/Backport.hpp"
#endif
#ifdef __cpp_lib_ranges
# include <ranges>
#endif
#if !defined( __cpp_lib_forward_like )                                              \
  || ( defined( __clang__ ) && !defined( _LIBCPP_VERSION ) && __clang_major__ >= 19 \
       && __clang_major__ <= 20 )
# include "../traits/Util.hpp"
#endif

namespace pace {
  namespace details {
    namespace utils {
      // Before C++17, not all std entities had feature macros.
#if PACE__CXX14
      using std::make_unique;
#else
      template<typename T, typename... Args>
      PACE__NODISCARD PACE__FORCEINLINE PACE__CXX23_CNSTXPR
        typename std::enable_if<!std::is_array<T>::value, std::unique_ptr<T>>::type make_unique(
          Args&&... args )
      { return std::unique_ptr<T>( ::new T( std::forward<Args>( args )... ) ); }
      template<typename T, typename... Args>
      PACE__NODISCARD PACE__FORCEINLINE PACE__CXX23_CNSTXPR
        typename std::enable_if<std::is_array<T>::value, std::unique_ptr<T>>::type
        make_unique( size_t n )
      { return std::unique_ptr<T>( ::new typename std::remove_extent<T>::type[n] ); }
#endif

#if PACE__CXX14
      using std::exchange;
#else
      template<typename T, typename U = T>
      PACE__CXX14_CNSTXPR T exchange( T& obj, U&& new_val ) noexcept(
        traits::AllOf<std::is_nothrow_move_constructible<T>, std::is_nothrow_assignable<T&, U&&>>::value )
      {
        T old_value = std::move( obj );
        obj         = std::forward<U>( new_val );
        return old_value;
      }
#endif

#ifdef __cpp_lib_uncaught_exceptions
      using std::uncaught_exceptions;
#else
      PACE__FORCEINLINE int uncaught_exceptions() noexcept
      { return static_cast<int>( std::uncaught_exception() ); }
#endif

#if PACE__CXX17
      using std::destroy_at;
#else
      // Available only for objects that constructed by placement new.
      template<typename T>
      PACE__FORCEINLINE PACE__CXX20_CNSTXPR void destroy_at( T* location )
        noexcept( std::is_nothrow_destructible<T>::value )
      {
        static_assert( !std::is_array<T>::value, "the program is ill-formed" );
        if PACE__CXX17_CNSTXPR ( !std::is_trivially_destructible<T>::value )
          location->~T();
      }
#endif
      template<typename T>
      PACE__FORCEINLINE PACE__CXX20_CNSTXPR void destroy_at( T& object )
        noexcept( std::is_nothrow_destructible<T>::value )
      { destroy_at( std::addressof( object ) ); }

#ifdef __cpp_lib_launder
      // Available only for buffers that use placement new.
      template<typename To, typename From>
      PACE__NODISCARD PACE__FORCEINLINE PACE__CXX17_CNSTXPR To* launder_as( From* src ) noexcept
      { return std::launder( reinterpret_cast<To*>( src ) ); }
#else
      // Available only for buffers that use placement new.
      template<typename To, typename From>
      PACE__NODISCARD PACE__FORCEINLINE To* launder_as( From* src ) noexcept
      {
# if defined( __GNUC__ )
#  if __GNUC__ >= 7
        return __builtin_launder( reinterpret_cast<To*>( src ) );
#  else
        // same as the impl of folly::launder (v2017.09.04)
        __asm__( "" : "+r"( src ) );
#  endif
# elif defined( __clang__ )
#  if __clang_major__ >= 8
        return __builtin_launder( reinterpret_cast<To*>( src ) );
#  endif
# endif
        // By default, we can only trust that reinterpret_cast can give us what we want.
        return reinterpret_cast<To*>( src );
      }
# define PACE__METHOD( Qualifier ) \
   template<typename To>           \
   To* launder_as( Qualifier void* src ) = delete
      PACE__METHOD();
      PACE__METHOD( const );
      PACE__METHOD( volatile );
      PACE__METHOD( const volatile );
# undef PACE__METHOD
# ifdef __clang__
#  pragma clang diagnostic push
#  pragma clang diagnostic ignored "-Wambiguous-ellipsis"
# endif
      // see https://github.com/Konvt/pace/issues/8#issuecomment-3723488239
# if !defined( _MSC_VER ) || defined( _MSVC_PERMISSIVE )
#  define PACE__METHOD( Variadic, Noexcept )            \
    template<typename To, typename R, typename... Args> \
    To* launder_as( R ( * )( Args... Variadic ) Noexcept ) = delete
      PACE__METHOD(, );
      PACE__METHOD( ..., );
#  ifdef __cpp_noexcept_function_type
      PACE__METHOD(, noexcept );
      PACE__METHOD( ..., noexcept );
#  endif
#  undef PACE__METHOD
# endif
# ifdef __clang__
#  pragma clang diagnostic pop
# endif
#endif

      // The `std::invoke` in MSVC will result in a hard compilation error when used in SFINAE.
      // Therefore, we need to wait for support for std::is_invocable.
#if defined( __cpp_lib_invoke )                                         \
  && ( !( defined( _MSVC_STL_VERSION ) || defined( _MSVC_STL_UPDATE ) ) \
       || defined( __cpp_lib_is_invocable ) )
      using std::invoke;
#else
      template<typename C, typename MemFn, typename Object, typename... Args>
      PACE__FORCEINLINE constexpr auto invoke( MemFn C::* method, Object&& object, Args&&... args )
        noexcept( noexcept( ( std::forward<Object>( object ).*method )( std::forward<Args>( args )... ) ) ) ->
        typename std::enable_if<
          traits::AllOf<std::is_member_function_pointer<MemFn C::*>,
                        traits::AnyOf<std::is_base_of<C, typename std::decay<Object>::type>,
                                      std::is_same<C, typename std::decay<Object>::type>>>::value,
          decltype( ( std::forward<Object>( object ).*method )( std::forward<Args>( args )... ) )>::type
      { return ( std::forward<Object>( object ).*method )( std::forward<Args>( args )... ); }
      template<typename C, typename MemFn, typename Object, typename... Args>
      PACE__FORCEINLINE constexpr auto invoke( MemFn C::* method, Object&& object, Args&&... args )
        noexcept( noexcept( ( object.get().*method )( std::forward<Args>( args )... ) ) ) ->
        typename std::enable_if<traits::AllOf<std::is_member_function_pointer<MemFn C::*>,
                                              traits::is_instance_of<typename std::decay<Object>::type,
                                                                     std::reference_wrapper>>::value,
                                decltype( ( object.get().*method )( std::forward<Args>( args )... ) )>::type
      { return ( object.get().*method )( std::forward<Args>( args )... ); }
      template<typename C, typename MemFn, typename Object, typename... Args>
      PACE__FORCEINLINE constexpr auto invoke( MemFn C::* method, Object&& object, Args&&... args )
        noexcept( noexcept( ( ( *std::forward<Object>( object ) )
                              .*method )( std::forward<Args>( args )... ) ) ) ->
        typename std::enable_if<
          traits::AllOf<std::is_member_function_pointer<MemFn C::*>,
                        traits::Not<traits::AnyOf<std::is_base_of<C, typename std::decay<Object>::type>,
                                                  std::is_same<C, typename std::decay<Object>::type>,
                                                  traits::is_instance_of<typename std::decay<Object>::type,
                                                                         std::reference_wrapper>>>>::value,
          decltype( ( ( *std::forward<Object>( object ) ).*method )( std::forward<Args>( args )... ) )>::type
      { return ( ( *std::forward<Object>( object ) ).*method )( std::forward<Args>( args )... ); }
      template<typename C, typename MemObj, typename Object>
      PACE__FORCEINLINE constexpr auto invoke( MemObj C::* member, Object&& object ) noexcept ->
        typename std::enable_if<
          traits::AllOf<std::is_member_object_pointer<MemObj C::*>,
                        traits::AnyOf<std::is_base_of<C, typename std::decay<Object>::type>,
                                      std::is_same<C, typename std::decay<Object>::type>>>::value,
          decltype( std::forward<Object>( object ).*member )>::type
      { return std::forward<Object>( object ).*member; }
      template<typename C, typename MemObj, typename Object>
      PACE__FORCEINLINE constexpr auto invoke( MemObj C::* member, Object&& object ) noexcept ->
        typename std::enable_if<traits::AllOf<std::is_member_object_pointer<MemObj C::*>,
                                              traits::is_instance_of<typename std::decay<Object>::type,
                                                                     std::reference_wrapper>>::value,
                                decltype( object.get().*member )>::type
      { return object.get().*member; }
      template<typename C, typename MemObj, typename Object>
      PACE__FORCEINLINE constexpr auto invoke( MemObj C::* member, Object&& object )
        noexcept( noexcept( ( *std::forward<Object>( object ) ).*member ) ) -> typename std::enable_if<
          traits::AllOf<std::is_member_object_pointer<MemObj C::*>,
                        traits::Not<traits::AnyOf<std::is_base_of<C, typename std::decay<Object>::type>,
                                                  std::is_same<C, typename std::decay<Object>::type>,
                                                  traits::is_instance_of<typename std::decay<Object>::type,
                                                                         std::reference_wrapper>>>>::value,
          decltype( ( *std::forward<Object>( object ) ).*member )>::type
      { return ( *std::forward<Object>( object ) ).*member; }
      template<typename Fn, typename... Args>
      PACE__FORCEINLINE constexpr auto invoke( Fn&& fn, Args&&... args )
        noexcept( noexcept( std::forward<Fn>( fn )( std::forward<Args>( args )... ) ) ) ->
        typename std::enable_if<
          traits::Not<
            traits::AnyOf<std::is_member_function_pointer<typename std::remove_reference<Fn>::type>,
                          std::is_member_object_pointer<typename std::remove_reference<Fn>::type>>>::value,
          decltype( std::forward<Fn>( fn )( std::forward<Args>( args )... ) )>::type
      { return std::forward<Fn>( fn )( std::forward<Args>( args )... ); }
#endif

#ifdef __cpp_lib_to_address
      using std::to_address;
#else
      template<typename Ptr>
      PACE__FORCEINLINE constexpr auto to_address( const Ptr& p ) noexcept
        -> decltype( std::pointer_traits<Ptr>::to_address( p ) )
      { return std::pointer_traits<Ptr>::to_address( p ); }
      // template argument None is used to lower the priority of this overloaded
      template<typename Ptr, typename... None>
      PACE__FORCEINLINE constexpr auto to_address( const Ptr& p, None... ) noexcept
        -> decltype( to_address( p.operator->() ) )
      { return to_address( p.operator->() ); }
      template<typename T>
      PACE__FORCEINLINE constexpr T* to_address( T* p ) noexcept
      {
        static_assert( !std::is_function<T>::value, "unexpected type" );
        return p;
      }
#endif

#ifdef __cpp_lib_ranges
      using std::ranges::begin;
      using std::ranges::distance;
      using std::ranges::end;
#else
      template<typename Itr>
      PACE__NODISCARD PACE__FORCEINLINE PACE__CXX17_CNSTXPR auto distance( Itr first, Itr second )
        noexcept( noexcept( std::distance( std::move( first ), std::move( second ) ) ) )
          -> decltype( std::distance( std::move( first ), std::move( second ) ) )
      { return std::distance( std::move( first ), std::move( second ) ); }
      template<typename R>
      PACE__NODISCARD PACE__FORCEINLINE PACE__CXX14_CNSTXPR auto begin( R&& r )
        noexcept( noexcept( std::begin( std::forward<R>( r ) ) ) )
          -> decltype( std::begin( std::forward<R>( r ) ) )
      { return std::begin( std::forward<R>( r ) ); }
      template<typename R>
      PACE__NODISCARD PACE__FORCEINLINE PACE__CXX14_CNSTXPR auto end( R&& r )
        noexcept( noexcept( std::end( std::forward<R>( r ) ) ) )
          -> decltype( std::end( std::forward<R>( r ) ) )
      { return std::end( std::forward<R>( r ) ); }
#endif

#ifdef __cpp_lib_ranges
      using std::ranges::empty;
      using std::ranges::size;
#elif defined( __cpp_lib_nonmember_container_access )
      using std::empty;
      using std::size;
#else
      template<typename Range>
      PACE__NODISCARD PACE__FORCEINLINE constexpr auto size( Range&& rn )
        noexcept( noexcept( std::forward<Range>( rn ).size() ) )
          -> decltype( std::forward<Range>( rn ).size() )
      { return std::forward<Range>( rn ).size(); }
      template<typename Range, std::size_t N>
      PACE__NODISCARD PACE__FORCEINLINE constexpr std::size_t size( const Range ( & )[N] ) noexcept
      { return N; }

      template<typename T, std::size_t N>
      PACE__NODISCARD PACE__FORCEINLINE constexpr bool empty( const T ( & )[N] ) noexcept
      { return false; }
      template<typename C>
      PACE__NODISCARD PACE__FORCEINLINE constexpr auto empty( const C& c ) noexcept( noexcept( c.empty() ) )
        -> decltype( c.empty() )
      { return c.empty(); }
#endif

#if PACE__CXX20
      // The overload beblow cannot constitute an overload of the one in std,
      // which means the introduction is safe.
      using std::construct_at;
#else
      template<typename T, typename... Args>
      PACE__FORCEINLINE PACE__CXX20_CNSTXPR
        typename std::enable_if<std::is_array<T>::value, T*>::type construct_at( T* location )
          noexcept( noexcept( ::new ( std::declval<void*>() ) T[1]() ) )
      {
        // static_assert( !std::is_unbounded_array_v<T> );
        return ::new ( location ) T[1]();
      }
      template<typename T, typename... Args>
      PACE__FORCEINLINE PACE__CXX20_CNSTXPR typename std::enable_if<!std::is_array<T>::value, T*>::type
        construct_at( T* location, Args&&... args )
          noexcept( noexcept( ::new ( std::declval<void*>() ) T( std::forward<Args>( args )... ) ) )
      { return ::new ( location ) T( std::forward<Args>( args )... ); }
#endif
      template<typename T, typename U, typename... Args>
      PACE__FORCEINLINE PACE__CXX20_CNSTXPR T* construct_at( U* location, Args&&... args )
        noexcept( noexcept( construct_at( std::declval<T*>(), std::forward<Args>( args )... ) ) )
      {
        static_assert( !std::is_void<T>::value, "invalid type" );
        return construct_at( reinterpret_cast<T*>( location ), std::forward<Args>( args )... );
      }

#ifdef __cpp_lib_to_underlying
      using std::to_underlying;
#else
      template<typename E>
      PACE__NODISCARD PACE__FORCEINLINE constexpr
        typename std::underlying_type<E>::type to_underlying( E enum_val ) noexcept
      { return static_cast<typename std::underlying_type<E>::type>( enum_val ); }
#endif

      // see https://github.com/llvm/llvm-project/issues/101614
#if defined( __cpp_lib_forward_like ) \
  && ( !defined( __clang__ ) || defined( _LIBCPP_VERSION ) || __clang_major__ < 19 || __clang_major__ > 20 )
      using std::forward_like;
#else
      template<typename As, typename T>
      constexpr auto forward_like( T&& param ) noexcept ->
        typename std::conditional<std::is_lvalue_reference<As>::value,
                                  traits::CopyConst_t<typename std::remove_reference<As>::type,
                                                      typename std::remove_reference<T>::type>&,
                                  traits::CopyConst_t<typename std::remove_reference<As>::type,
                                                      typename std::remove_reference<T>::type>&&>::type
      {
        return static_cast<
          typename std::conditional<std::is_lvalue_reference<As>::value,
                                    traits::CopyConst_t<typename std::remove_reference<As>::type,
                                                        typename std::remove_reference<T>::type>&,
                                    traits::CopyConst_t<typename std::remove_reference<As>::type,
                                                        typename std::remove_reference<T>::type>&&>::type>(
          param );
      }
#endif

#ifdef __cpp_lib_unreachable
      using std::unreachable;
#else
      [[noreturn]] void unreachable()
      {
# if defined( __GNUC__ ) || defined( __clang__ )
        __builtin_unreachable();
# else
        PACE__TRUST( 0 );
# endif
      }
#endif

#ifdef __cpp_lib_start_lifetime_as
      using std::start_lifetime_as_array;
#else
# define PACE__METHOD( Qualifier )                                                                   \
   template<typename T>                                                                              \
   PACE__FORCEINLINE constexpr T* start_lifetime_as_array( Qualifier void* p, std::size_t ) noexcept \
   { return static_cast<T*>( p ); }
      PACE__METHOD()
      PACE__METHOD( const )
      PACE__METHOD( volatile )
      PACE__METHOD( const volatile )
# undef PACE__METHOD
#endif
    } // namespace utils

    namespace traits {
#ifdef __cpp_lib_is_invocable
      using std::is_invocable;
      using std::is_invocable_r;
      using std::is_nothrow_invocable_r;
#else
      template<typename Fn, typename... Args>
      struct _impl_is_invocable {
      private:
        template<typename F>
        static constexpr auto check( int ) -> typename std::enable_if<
          std::is_void<decltype( utils::invoke( std::declval<F>(), std::declval<Args>()... ),
                                 void() )>::value,
          std::true_type>::type;
        template<typename>
        static constexpr auto check( ... ) -> std::false_type;

      public:
        using result_type = decltype( check<Fn>( 0 ) );
      };
      template<typename Fn, typename... Args>
      using is_invocable = typename _impl_is_invocable<Fn, Args...>::result_type;

      template<typename Fn, typename... Args>
      struct _impl_is_nothrow_invocable {
      private:
        template<typename F>
        static auto check( std::true_type )
          -> BoolConstant<noexcept( utils::invoke( std::declval<F>(), std::declval<Args>()... ) )>;
        template<typename>
        static auto check( std::false_type ) -> std::false_type;

      public:
        using result_type = decltype( check<Fn>( std::declval<is_invocable<Fn, Args...>>() ) );
      };
      template<typename Fn, typename... Args>
      using is_nothrow_invocable = typename _impl_is_nothrow_invocable<Fn, Args...>::result_type;

      template<typename Ret, typename Fn, typename... Args>
      struct _impl_is_invocable_r {
      private:
        template<typename F>
        static constexpr auto check( std::true_type )
          -> std::is_convertible<decltype( utils::invoke( std::declval<F>(), std::declval<Args>()... ) ),
                                 Ret>;
        template<typename>
        static constexpr auto check( std::false_type ) -> std::false_type;

      public:
        using result_type = decltype( check<Fn>( std::declval<is_invocable<Fn, Args...>>() ) );
      };
      template<typename Fn, typename... Args>
      struct _impl_is_invocable_r<void, Fn, Args...> : _impl_is_invocable<Fn, Args...> {};
      template<typename Ret, typename Fn, typename... Args>
      using is_invocable_r = typename _impl_is_invocable_r<Ret, Fn, Args...>::result_type;

      template<typename Ret, typename Fn, typename... Args>
      struct _impl_is_nothrow_invocable_r {
      private:
        template<typename F>
        static constexpr auto check( std::true_type )
          -> is_nothrow_convertible<decltype( utils::invoke( std::declval<F>(), std::declval<Args>()... ) ),
                                    Ret>;
        template<typename>
        static constexpr auto check( std::false_type ) -> std::false_type;

      public:
        using result_type = decltype( check<Fn>( std::declval<is_nothrow_invocable<Ret, Fn, Args...>>() ) );
      };
      template<typename Fn, typename... Args>
      struct _impl_is_nothrow_invocable_r<void, Fn, Args...> : _impl_is_nothrow_invocable<Fn, Args...> {};
      template<typename Ret, typename Fn, typename... Args>
      using is_nothrow_invocable_r = typename _impl_is_nothrow_invocable_r<Ret, Fn, Args...>::result_type;
#endif
    } // namespace traits

    namespace utils {
#ifdef __cpp_lib_invoke_r
      using std::invoke_r;
#else
      template<typename R, typename F, typename... Args>
      PACE__FORCEINLINE PACE__CXX14_CNSTXPR
        typename std::enable_if<std::is_void<R>::value>::type invoke_r( F&& fn, Args&&... args )
          noexcept( noexcept( invoke( std::forward<F>( fn ), std::forward<Args>( args )... ) ) )
      { (void)invoke( std::forward<F>( fn ), std::forward<Args>( args )... ); }
      template<typename R, typename F, typename... Args>
      PACE__FORCEINLINE constexpr typename std::enable_if<!std::is_void<R>::value, R>::type invoke_r(
        F&& fn,
        Args&&... args ) noexcept( traits::is_nothrow_invocable_r<R, F, Args...>::value )
      { return invoke( std::forward<F>( fn ), std::forward<Args>( args )... ); }
#endif
    }
  } // namespace details
} // namespace pace
#endif
