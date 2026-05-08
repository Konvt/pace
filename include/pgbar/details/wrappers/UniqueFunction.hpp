#ifndef PGBAR_UNIQUE_FUNCTION
#define PGBAR_UNIQUE_FUNCTION

#include <type_traits>
#ifdef __cpp_lib_move_only_function
# include <functional>
#else
# include "../traits/Backport.hpp"
# include "../utils/Backport.hpp"
# include <new>
#endif

namespace pgbar {
  namespace details {
    namespace wrappers {
#ifdef __cpp_lib_move_only_function
      template<typename... Signature>
      using UniqueFunction = std::move_only_function<Signature...>;
#else
      template<typename Derived>
      class FnStore {
      protected:
        union alignas( std::max_align_t ) AnyFn {
          types::Byte sso_[sizeof( void* ) * 2];
          types::Byte* dptr_;

          constexpr AnyFn() noexcept : dptr_ { nullptr } {}
        };
        struct VTable final {
          const typename Derived::Delegate invoke;
          // The handling of function types by msvc is very strange:
          // it often triggers internal compiler errors for no apparent reason,
          // so here we have to manually write the function pointer type.
          void ( *const destroy )( AnyFn& ) noexcept;
          void ( *const move )( AnyFn& dst, AnyFn& src ) noexcept;
        };

        template<typename T>
        using Inlinable = traits::AllOf<
          std::is_nothrow_move_constructible<T>,
          traits::BoolConstant<( sizeof( AnyFn::sso_ ) >= sizeof( T ) && alignof( AnyFn ) >= alignof( T ) )>>;

        const VTable* vtable_;
        AnyFn callee_;

        static PGBAR__NOINLINE PGBAR__CXX14_CNSTXPR void destroy_null( AnyFn& ) noexcept {}
        static PGBAR__NOINLINE PGBAR__CXX14_CNSTXPR void move_null( AnyFn&, AnyFn& ) noexcept {}

        template<typename T>
        static PGBAR__NOINLINE PGBAR__CXX20_CNSTXPR void destroy_inline( AnyFn& fn ) noexcept
        {
          utils::destroy_at( utils::launder_as<T>( &fn.sso_ ) );
        }
        template<typename T>
        static PGBAR__CXX20_CNSTXPR PGBAR__NOINLINE void move_inline( AnyFn& dst, AnyFn& src ) noexcept
        {
          utils::construct_at<T>( &dst.sso_, std::move( *utils::launder_as<T>( &src.sso_ ) ) );
          destroy_inline<T>( src );
        }

        template<typename T>
        static PGBAR__NOINLINE PGBAR__CXX20_CNSTXPR void destroy_dynamic( AnyFn& fn ) noexcept
        {
          const auto dptr = utils::launder_as<T>( fn.dptr_ );
          utils::destroy_at( dptr );
# ifdef __cpp_aligned_new
          ::operator delete( fn.dptr_, std::align_val_t( alignof( T ) ) );
# else
          ::operator delete( fn.dptr_ );
# endif
        }
        template<typename T>
        static PGBAR__NOINLINE PGBAR__CXX23_CNSTXPR void move_dynamic( AnyFn& dst, AnyFn& src ) noexcept
        {
          dst.dptr_ = src.dptr_;
          src.dptr_ = &table_null();
        }

        template<typename F>
        static PGBAR__CXX23_CNSTXPR
          typename std::enable_if<Inlinable<typename std::decay<F>::type>::value>::type
          store_fn( const VTable*( &vtable ), AnyFn& any, F&& fn ) noexcept
        {
          using T             = typename std::decay<F>::type;
          const auto location = utils::construct_at<T>( &any.sso_, std::forward<F>( fn ) );
          PGBAR__TRUST( static_cast<void*>( location ) == static_cast<void*>( &any.sso_ ) );
          (void)location;
          vtable = &table_inline<T>();
        }
        template<typename F>
        static PGBAR__CXX23_CNSTXPR
          typename std::enable_if<!Inlinable<typename std::decay<F>::type>::value>::type
          store_fn( const VTable*( &vtable ), AnyFn& any, F&& fn ) noexcept( false )
        {
          using T   = typename std::decay<F>::type;
          auto dptr = std::unique_ptr<void, void ( * )( void* )>(
# ifdef __cpp_aligned_new
            ::operator new( sizeof( T ), std::align_val_t( alignof( T ) ) ),
            +[]( void* ptr ) { ::operator delete( ptr, std::align_val_t( alignof( T ) ) ); }
# else
            ::operator new( sizeof( T ) ),
            +[]( void* ptr ) { ::operator delete( ptr ); }
# endif
          );

          const auto location = utils::construct_at<T>( dptr.get(), std::forward<F>( fn ) );
          PGBAR__ASSERT( static_cast<void*>( location ) == dptr.get() );
          (void)location;

          any.dptr_ = static_cast<types::Byte*>( dptr.release() );
          vtable    = &table_dynamic<T>();
        }

        template<typename T>
        static PGBAR__CXX23_CNSTXPR const VTable& table_inline() noexcept
        {
          static const VTable tbl { Derived::template invoke_inline<T>, destroy_inline<T>, move_inline<T> };
          return tbl;
        }
        template<typename T>
        static PGBAR__CXX23_CNSTXPR const VTable& table_dynamic() noexcept
        {
          static const VTable tbl { Derived::template invoke_dynamic<T>,
                                    destroy_dynamic<T>,
                                    move_dynamic<T> };
          return tbl;
        }
        static PGBAR__CXX23_CNSTXPR const VTable& table_null() noexcept
        {
          static const VTable tbl { Derived::invoke_null, destroy_null, move_null };
          return tbl;
        }

        PGBAR__CXX23_CNSTXPR FnStore() noexcept : vtable_ { &table_null() } {}

        PGBAR__CXX23_CNSTXPR void reset() noexcept
        {
          PGBAR__TRUST( vtable_ != nullptr );
          vtable_->destroy( callee_ );
          vtable_ = &table_null();
        }
        template<typename F>
        PGBAR__CXX23_CNSTXPR void reset( F&& fn ) noexcept( Inlinable<typename std::decay<F>::type>::value )
        {
          const VTable* vtable = nullptr;
          AnyFn tmp;
          store_fn( vtable, tmp, std::forward<F>( fn ) );
          PGBAR__TRUST( vtable != nullptr );
          reset();
          std::swap( vtable_, vtable );
          PGBAR__TRUST( this->vtable_ != nullptr );
          vtable_->move( callee_, tmp );
        }

      public:
        PGBAR__CXX23_CNSTXPR FnStore( FnStore&& rhs ) noexcept : vtable_ { rhs.vtable_ }
        {
          PGBAR__TRUST( rhs.vtable_ != nullptr );
          vtable_->move( callee_, rhs.callee_ );
          rhs.vtable_ = &table_null();
        }
        PGBAR__CXX23_CNSTXPR FnStore& operator=( FnStore&& rhs ) & noexcept
        {
          PGBAR__TRUST( this != &rhs );
          PGBAR__TRUST( vtable_ != nullptr );
          PGBAR__TRUST( rhs.vtable_ != nullptr );
          reset();
          std::swap( vtable_, rhs.vtable_ );
          PGBAR__TRUST( this->vtable_ != nullptr );
          PGBAR__TRUST( rhs.vtable_ != nullptr );
          vtable_->move( callee_, rhs.callee_ );
          return *this;
        }
        PGBAR__CXX23_CNSTXPR ~FnStore() noexcept { reset(); }

        PGBAR__CXX23_CNSTXPR void swap( FnStore& other ) noexcept
        {
          PGBAR__TRUST( vtable_ != nullptr );
          PGBAR__TRUST( other.vtable_ != nullptr );
          AnyFn tmp;
          vtable_->move( tmp, callee_ );
          other.vtable_->move( callee_, other.callee_ );
          vtable_->move( other.callee_, tmp );
          std::swap( vtable_, other.vtable_ );
          PGBAR__TRUST( this->vtable_ != nullptr );
          PGBAR__TRUST( other.vtable_ != nullptr );
        }
        friend PGBAR__CXX23_CNSTXPR void swap( FnStore& a, FnStore& b ) noexcept { return a.swap( b ); }
        friend constexpr bool operator==( const FnStore& a, std::nullptr_t ) noexcept
        {
          return !static_cast<bool>( a );
        }
        friend constexpr bool operator!=( const FnStore& a, std::nullptr_t ) noexcept
        {
          return static_cast<bool>( a );
        }
        explicit constexpr operator bool() const noexcept { return vtable_ != &table_null(); }
      };
      // `CrefInfo` can be any types that contains the `cref` info of the functor.
      // e.g. For the function type `void () const&`, the `CrefInfo` can be: `const int&`.
      template<typename Derived, typename CrefInfo, typename R, bool Noexcept, typename... Args>
      class FnInvoker : public FnStore<Derived> {
        friend class FnStore<Derived>;

        using typename FnStore<Derived>::AnyFn;
        template<typename T>
        using Param_t = typename std::conditional<std::is_scalar<T>::value, T, T&&>::type;
        template<typename T>
        using Delegate_t = typename std::
          conditional<std::is_const<typename std::remove_reference<CrefInfo>::type>::value, const T, T>::type;

      protected:
        using Delegate = R ( * )( const AnyFn&, Param_t<Args>... )
# ifdef __cpp_noexcept_function_type
          noexcept( Noexcept )
# endif
          ;

        template<typename Fn>
        using Fn_t =
          decltype( utils::forward_like<CrefInfo>( std::declval<typename std::decay<Fn>::type>() ) );

        static PGBAR__NOINLINE PGBAR__CXX14_CNSTXPR R invoke_null( const AnyFn&, Param_t<Args>... )
          noexcept( Noexcept )
        {
          utils::unreachable();
          // The standard says this should trigger an undefined behavior.
        }
        template<typename T>
        static PGBAR__NOINLINE PGBAR__CXX14_CNSTXPR R invoke_inline( const AnyFn& fn, Param_t<Args>... args )
          noexcept( Noexcept )
        {
          const auto ptr = utils::launder_as<Delegate_t<T>>( ( &const_cast<AnyFn&>( fn ).sso_ ) );
          return utils::invoke( utils::forward_like<CrefInfo>( *ptr ), std::forward<Args>( args )... );
        }
        template<typename T>
        static PGBAR__NOINLINE PGBAR__CXX14_CNSTXPR R invoke_dynamic( const AnyFn& fn, Param_t<Args>... args )
          noexcept( Noexcept )
        {
          const auto dptr = utils::launder_as<Delegate_t<T>>( fn.dptr_ );
          return utils::invoke( utils::forward_like<CrefInfo>( *dptr ), std::forward<Args>( args )... );
        }

        constexpr FnInvoker()                                      = default;
        constexpr FnInvoker( FnInvoker&& )                         = default;
        PGBAR__CXX14_CNSTXPR FnInvoker& operator=( FnInvoker&& ) & = default;
        PGBAR__CXX20_CNSTXPR ~FnInvoker()                          = default;

      public:
        using result_type = R;
      };

      // A simplified implementation of std::move_only_function
      template<typename...>
      class UniqueFunction;
      template<typename R, typename... Args>
      class UniqueFunction<R( Args... )>
        : public FnInvoker<UniqueFunction<R( Args... )>, int&, R, false, Args...> {
        // Function types without ref qualifier will be treated as non-const lvalue reference types.
        using Base = FnInvoker<UniqueFunction, int&, R, false, Args...>;

      public:
        UniqueFunction( const UniqueFunction& )            = delete;
        UniqueFunction& operator=( const UniqueFunction& ) = delete;

        constexpr UniqueFunction()                                           = default;
        constexpr UniqueFunction( UniqueFunction&& )                         = default;
        PGBAR__CXX14_CNSTXPR UniqueFunction& operator=( UniqueFunction&& ) & = default;
        PGBAR__CXX20_CNSTXPR ~UniqueFunction()                               = default;

        constexpr UniqueFunction( std::nullptr_t ) noexcept : UniqueFunction() {}
        template<typename F,
                 typename = typename std::enable_if<traits::AllOf<
                   // It's so strange that even if we have a no-template overload here,
                   // using an earlier standard (c++14) in msvc still causes compile error
                   // because the compiler attempts to instantiate the template version with std::nullptr_t.
                   // Therefore we need to add a SFINAE below to prevent the instantiation.
                   traits::Not<std::is_same<typename std::decay<F>::type, std::nullptr_t>>,
                   std::is_constructible<typename std::decay<F>::type, F>,
                   traits::is_invocable_to<R, typename Base::template Fn_t<F>, Args...>>::value>::type>
        UniqueFunction( F&& fn ) noexcept( Base::template Inlinable<typename std::decay<F>::type>::value )
        {
          Base::store_fn( this->vtable_, this->callee_, std::forward<F>( fn ) );
          PGBAR__TRUST( this->vtable_ != nullptr );
        }
        // In C++11, `std::in_place_type` does not exist, and we will not use it either.
        // Therefore, we do not provide an overloaded constructor for this type here.
        template<typename F>
        PGBAR__CXX14_CNSTXPR typename std::enable_if<
          traits::AllOf<traits::Not<std::is_same<typename std::decay<F>::type, std::nullptr_t>>,
                        std::is_constructible<typename std::decay<F>::type, F>,
                        traits::is_invocable_to<R, typename Base::template Fn_t<F>, Args...>>::value,
          UniqueFunction&>::type
          operator=( F&& fn ) & noexcept( Base::template Inlinable<typename std::decay<F>::type>::value )
        {
          this->reset( std::forward<F>( fn ) );
          return *this;
        }
        PGBAR__CXX14_CNSTXPR UniqueFunction& operator=( std::nullptr_t ) noexcept
        {
          this->reset();
          return *this;
        }

        PGBAR__FORCEINLINE PGBAR__CXX14_CNSTXPR R operator()( Args... args )
        {
          PGBAR__TRUST( this->vtable_ != nullptr );
          return ( *this->vtable_->invoke )( this->callee_, std::forward<Args>( args )... );
        }
      };
#endif
    } // namespace wrappers
  } // namespace details
} // namespace pgbar

#endif
