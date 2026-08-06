#ifndef PACE_UNIQUE_FUNCTION
#define PACE_UNIQUE_FUNCTION

#include <type_traits>
#ifdef __cpp_lib_moveonly_function
# include <functional>
#else
# include "../core/Types.hpp"
# include "../traits/Backport.hpp"
# include "../traits/Util.hpp"
# include "../utils/Backport.hpp"
#endif

namespace pace {
  namespace details {
    namespace wrappers {
#ifdef __cpp_lib_moveonly_function
      template<typename... Signature>
      using UniqueFunction = std::moveonly_function<Signature...>;
#else
      // `CrefInfo` can be any types that contains the `cref` info of the functor.
      // e.g. For the function type `void () const&`, the `CrefInfo` can be: `const int&`.
      template<typename CrefInfo, typename R, bool Noexcept, typename... Params>
      class FnStorage {
        template<typename T>
        using Param_t = typename std::conditional<std::is_scalar<T>::value, T, T&&>::type;
        template<typename T>
        using Callee_t = typename std::
          conditional<std::is_const<typename std::remove_reference<CrefInfo>::type>::value, const T, T>::type;

      protected:
        template<typename Fn>
        using Fn_t =
          decltype( utils::forward_like<CrefInfo>( std::declval<typename std::decay<Fn>::type>() ) );

        union alignas( std::max_align_t ) AnyFn {
          types::Byte sso[sizeof( void* ) * 2];
          void* dptr;

          constexpr AnyFn() noexcept : dptr { nullptr } {}
        };
        struct Life {
          // The handling of function types by msvc is very strange:
          // it often triggers internal compiler errors for no apparent reason,
          // so here we have to manually write the function pointer type.
          void ( *const destroy )( AnyFn& ) noexcept;
          void ( *const move )( AnyFn& dst, AnyFn& src ) noexcept;

# if PACE__CXX20
          friend constexpr bool operator==( const Life&, const Life& ) = default;
          friend constexpr bool operator!=( const Life&, const Life& ) = default;
# else
          friend constexpr bool operator==( const Life& a, const Life& b ) noexcept
          { return a.destroy == b.destroy && a.move == b.move; }
          friend constexpr bool operator!=( const Life& a, const Life& b ) noexcept { return !( a == b ); }
# endif
        };
        struct VTable final {
          R ( *invoke )( const AnyFn&, Param_t<Params>... )
# ifdef __cpp_noexcept_function_type
            noexcept( Noexcept )
# endif
              ;
          const Life* life;

# if PACE__CXX20
          friend constexpr bool operator==( const VTable&, const VTable& ) = default;
          friend constexpr bool operator!=( const VTable&, const VTable& ) = default;
# else
          friend constexpr bool operator==( const VTable& a, const VTable& b ) noexcept
          { return a.invoke == b.invoke && a.life == b.life; }
          friend constexpr bool operator!=( const VTable& a, const VTable& b ) noexcept
          { return !( a == b ); }
# endif
        };

        template<typename T>
        using is_inlinable = traits::AllOf<
          std::is_nothrow_move_constructible<T>,
          traits::BoolConstant<( sizeof( AnyFn::sso ) >= sizeof( T ) && alignof( AnyFn ) >= alignof( T ) )>>;

        AnyFn callee_;
        VTable vtable_;

        static PACE__CXX14_CNSTXPR R invoke_null( const AnyFn&, Param_t<Params>... ) noexcept( Noexcept )
        {
          PACE__ASSERT( false );
          utils::unreachable();
          // The standard says this should trigger an undefined behavior.
        }
        static PACE__CXX14_CNSTXPR void movenull( AnyFn&, AnyFn& ) noexcept {}

        template<typename T>
        static PACE__CXX14_CNSTXPR R invoke_inline( const AnyFn& fn, Param_t<Params>... params )
          noexcept( Noexcept )
        {
          const auto ptr = utils::launder_as<Callee_t<T>>( ( &const_cast<AnyFn&>( fn ).sso ) );
          return utils::invoke_r<R>( utils::forward_like<CrefInfo>( *ptr ),
                                     std::forward<Params>( params )... );
        }
        template<typename T>
        static PACE__CXX20_CNSTXPR void destroyinline( AnyFn& fn ) noexcept
        { utils::destroy_at( utils::launder_as<T>( &fn.sso ) ); }
        template<typename T>
        static PACE__CXX20_CNSTXPR void moveinline( AnyFn& dst, AnyFn& src ) noexcept
        {
          utils::construct_at<T>( &dst.sso, std::move( *utils::launder_as<T>( &src.sso ) ) );
          if PACE__CXX17_CNSTXPR ( !std::is_trivially_destructible<T>::value )
            destroyinline<T>( src );
        }

        template<typename T>
        static PACE__CXX14_CNSTXPR R invoke_dynamic( const AnyFn& fn, Param_t<Params>... params )
          noexcept( Noexcept )
        {
          const auto dptr = utils::launder_as<Callee_t<T>>( fn.dptr );
          return utils::invoke_r<R>( utils::forward_like<CrefInfo>( *dptr ),
                                     std::forward<Params>( params )... );
        }
        template<typename T>
        static PACE__CXX20_CNSTXPR void destroydynamic( AnyFn& fn ) noexcept
        {
          const auto dptr = utils::launder_as<T>( fn.dptr );
          utils::destroy_at( dptr );
# ifdef __cpp_aligned_new
          ::operator delete( fn.dptr_, std::align_val_t( alignof( T ) ) );
# else
          ::operator delete( fn.dptr );
# endif
        }
        template<typename T>
        static PACE__CXX20_CNSTXPR void movedynamic( AnyFn& dst, AnyFn& src ) noexcept
        {
          dst.dptr = src.dptr;
          src.dptr = nullptr;
        }

        template<typename T>
        static PACE__CXX23_CNSTXPR VTable table_inline() noexcept
        {
          static Life life { std::is_trivially_destructible<T>::value ? nullptr : destroyinline<T>,
                             moveinline<T> };
          return { invoke_inline<T>, &life };
        }
        template<typename T>
        static PACE__CXX23_CNSTXPR VTable table_dynamic() noexcept
        {
          static Life life { destroydynamic<T>, movedynamic<T> };
          return { invoke_dynamic<T>, &life };
        }
        static PACE__CXX23_CNSTXPR VTable table_null() noexcept
        {
          static Life life { nullptr, movenull };
          return { invoke_null, &life };
        }

        template<typename F, typename... Args>
        static PACE__CXX23_CNSTXPR typename std::enable_if<is_inlinable<F>::value>::type
          store( VTable& vtable, AnyFn& any, Args&&... args )
            noexcept( std::is_nothrow_constructible<F, Args...>::value )
        {
          const auto location = utils::construct_at<F>( &any.sso, std::forward<Args>( args )... );
          PACE__TRUST( static_cast<void*>( location ) == static_cast<void*>( &any.sso ) );
          (void)location;
          vtable = table_inline<F>();
        }
        template<typename F, typename... Args>
        static PACE__CXX23_CNSTXPR typename std::enable_if<!is_inlinable<F>::value>::type
          store( VTable& vtable, AnyFn& any, Args&&... args )
        {
          auto dptr = std::unique_ptr<void, void ( * )( void* )>(
# ifdef __cpp_aligned_new
            ::operator new( sizeof( F ), std::align_val_t( alignof( F ) ) ),
            +[]( void* ptr ) { ::operator delete( ptr, std::align_val_t( alignof( F ) ) ); }
# else
            ::operator new( sizeof( F ) ),
            +[]( void* ptr ) { ::operator delete( ptr ); }
# endif
          );

          const auto location = utils::construct_at<F>( dptr.get(), std::forward<Args>( args )... );
          PACE__ASSERT( static_cast<void*>( location ) == dptr.get() );
          (void)location;

          any.dptr = dptr.release();
          vtable   = table_dynamic<F>();
        }

        PACE__CXX23_CNSTXPR FnStorage() noexcept : vtable_ { table_null() } {}

        PACE__CXX23_CNSTXPR void reset() noexcept
        {
          if ( vtable_.life->destroy != nullptr )
            vtable_.life->destroy( callee_ );
          vtable_ = table_null();
        }
        template<typename F>
        PACE__CXX23_CNSTXPR void reset( F&& fn )
          noexcept( traits::AllOf<is_inlinable<typename std::decay<F>::type>,
                                  std::is_nothrow_constructible<typename std::decay<F>::type, F>>::value )
        {
          VTable vtable;
          AnyFn tmp;
          store<typename std::decay<F>::type>( vtable, tmp, std::forward<F>( fn ) );
# ifdef _MSC_VER
#  pragma warning( push )
#  pragma warning( disable : 4297 )
# else
#  pragma GCC diagnostic push
#  ifdef __clang__
#   pragma GCC diagnostic ignored "-Wunknown-warning-option"
#  endif
#  pragma GCC diagnostic ignored "-Wterminate"
# endif
          try {
            reset();
            vtable.life->move( callee_, tmp );
          } catch ( ... ) {
            if ( vtable_.life->destroy != nullptr )
              vtable.life->destroy( tmp );
            throw;
          }
# ifdef _MSC_VER
#  pragma warning( pop )
# else
#  pragma GCC diagnostic pop
# endif
          std::swap( vtable_, vtable );
        }

      public:
        using result_type = R;

        PACE__CXX23_CNSTXPR FnStorage( FnStorage&& rhs ) noexcept : vtable_ { rhs.vtable_ }
        {
          vtable_.life->move( callee_, rhs.callee_ );
          rhs.vtable_ = table_null();
        }
        PACE__CXX23_CNSTXPR FnStorage& operator=( FnStorage&& rhs ) & noexcept
        {
          PACE__TRUST( this != &rhs );
          reset();
          std::swap( vtable_, rhs.vtable_ );
          vtable_.life->move( callee_, rhs.callee_ );
          return *this;
        }
        PACE__CXX23_CNSTXPR ~FnStorage() noexcept { reset(); }

        PACE__CXX20_CNSTXPR void swap( FnStorage& other ) noexcept
        {
          AnyFn tmp;
          vtable_.life->move( tmp, callee_ );
          other.vtable_.life->move( callee_, other.callee_ );
          vtable_.life->move( other.callee_, tmp );
          std::swap( vtable_, other.vtable_ );
        }
        friend PACE__CXX23_CNSTXPR void swap( FnStorage& a, FnStorage& b ) noexcept { return a.swap( b ); }
        friend constexpr bool operator==( const FnStorage& a, std::nullptr_t ) noexcept
        { return !static_cast<bool>( a ); }
        friend constexpr bool operator!=( const FnStorage& a, std::nullptr_t ) noexcept
        { return static_cast<bool>( a ); }
        explicit constexpr operator bool() const noexcept { return vtable_ != table_null(); }
      };

      // A simplified implementation of std::moveonly_function
      template<typename...>
      class UniqueFunction;
      template<typename R, typename... Params>
      class UniqueFunction<R( Params... )> : public FnStorage<int&, R, false, Params...> {
        // Function types without ref qualifier will be treated as non-const lvalue reference types.
        using Base = FnStorage<int&, R, false, Params...>;

      public:
        UniqueFunction( const UniqueFunction& )            = delete;
        UniqueFunction& operator=( const UniqueFunction& ) = delete;

        constexpr UniqueFunction()                                          = default;
        constexpr UniqueFunction( UniqueFunction&& )                        = default;
        PACE__CXX14_CNSTXPR UniqueFunction& operator=( UniqueFunction&& ) & = default;
        PACE__CXX20_CNSTXPR ~UniqueFunction()                               = default;

        constexpr UniqueFunction( std::nullptr_t ) noexcept : UniqueFunction() {}
        template<typename F,
                 typename = typename std::enable_if<traits::AllOf<
                   // It's so strange that even if we have a no-template overload here,
                   // using an earlier standard (c++14) in msvc still causes compile error
                   // because the compiler attempts to instantiate the template version with std::nullptr_t.
                   // Therefore we need to add a SFINAE below to prevent the instantiation.
                   traits::Not<std::is_same<typename std::decay<F>::type, std::nullptr_t>>,
                   // And it's not available for std::is_null_pointer in c++11 libc++.
                   std::is_constructible<typename std::decay<F>::type, F>,
                   traits::is_invocable_r<R, typename Base::template Fn_t<F>, Params...>>::value>::type>
        PACE__CXX23_CNSTXPR UniqueFunction( F&& fn )
          noexcept( traits::AllOf<typename Base::template is_inlinable<typename std::decay<F>::type>,
                                  std::is_nothrow_constructible<typename std::decay<F>::type, F>>::value )
        {
          Base::template store<typename std::decay<F>::type>( this->vtable_,
                                                              this->callee_,
                                                              std::forward<F>( fn ) );
        }
        // In C++11, `std::in_place_type` does not exist, and we will not use it either.
        // Therefore, we do not provide an overloaded constructor for this type here.

        template<typename F>
        PACE__CXX23_CNSTXPR typename std::enable_if<
          traits::AllOf<traits::Not<std::is_same<typename std::decay<F>::type, std::nullptr_t>>,
                        std::is_constructible<typename std::decay<F>::type, F>,
                        traits::is_invocable_r<R, typename Base::template Fn_t<F>, Params...>>::value,
          UniqueFunction&>::type
          operator=( F&& fn ) & noexcept(
            traits::AllOf<typename Base::template is_inlinable<typename std::decay<F>::type>,
                          std::is_nothrow_constructible<typename std::decay<F>::type, F>>::value )
        {
          this->reset( std::forward<F>( fn ) );
          return *this;
        }
        PACE__CXX23_CNSTXPR UniqueFunction& operator=( std::nullptr_t ) noexcept
        {
          this->reset();
          return *this;
        }

        PACE__FORCEINLINE PACE__CXX14_CNSTXPR R operator()( Params... params )
        { return this->vtable_.invoke( this->callee_, std::forward<Params>( params )... ); }
      };
#endif
    } // namespace wrappers
  } // namespace details
} // namespace pace

#endif
