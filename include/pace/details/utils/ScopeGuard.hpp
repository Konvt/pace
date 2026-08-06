#ifndef PACE_SCOPE_GUARD
#define PACE_SCOPE_GUARD

#include "../traits/Backport.hpp"
#include "Backport.hpp"
#include <limits>

namespace pace {
  namespace details {
    namespace utils {
      template<typename EF, typename = void>
      class ScpStorage { // scope, not that "scp"
        EF callback_;

      protected:
        template<typename Fn>
        constexpr ScpStorage( std::true_type, Fn&& fn ) noexcept : callback_ { std::forward<Fn>( fn ) }
        {}
        template<typename Fn>
        constexpr ScpStorage( std::false_type, Fn&& fn )
          noexcept( std::is_nothrow_constructible<EF, Fn&>::value )
          : callback_ { fn }
        {}

        PACE__FORCEINLINE PACE__CXX14_CNSTXPR EF& callback() noexcept { return callback_; }
      };
      template<typename EF>
      class ScpStorage<EF,
                       typename std::enable_if<
                         traits::AllOf<std::is_empty<EF>, traits::Not<traits::is_final<EF>>>::value>::type>
        : private EF {
      protected:
        template<typename Fn>
        constexpr ScpStorage( std::true_type, Fn&& fn ) noexcept : EF( std::forward<Fn>( fn ) )
        {}
        template<typename Fn>
        constexpr ScpStorage( std::false_type, Fn&& fn )
          noexcept( std::is_nothrow_constructible<EF, Fn&>::value )
          : EF( fn )
        {}

        PACE__FORCEINLINE PACE__CXX14_CNSTXPR EF& callback() noexcept { return static_cast<EF&>( *this ); }
      };

      template<typename EF>
      class
#if PACE__CXX17
        PACE__NODISCARD
#endif
        ScopeFail : private ScpStorage<EF> {
        static_assert( traits::is_invocable<typename std::remove_reference<EF>::type&>::value,
                       "invalid callback type" );
        using Base = ScpStorage<EF>;

        int exceptions_on_entry_;

      public:
        ScopeFail( const ScopeFail& )            = delete;
        ScopeFail& operator=( ScopeFail&& )      = delete;
        ScopeFail& operator=( const ScopeFail& ) = delete;

        template<typename Fn,
                 typename = typename std::enable_if<
                   traits::AllOf<traits::Not<std::is_same<typename std::decay<Fn>::type, ScopeFail>>,
                                 std::is_constructible<EF, Fn>>::value>::type>
        explicit ScopeFail( Fn&& fn ) noexcept( traits::AnyOf<std::is_nothrow_constructible<EF, Fn>,
                                                              std::is_nothrow_constructible<EF, Fn&>>::value )
        try
          : Base( traits::AllOf<traits::Not<std::is_lvalue_reference<Fn>>,
                                std::is_nothrow_constructible<EF, Fn>>(),
                  std::forward<Fn>( fn ) )
          , exceptions_on_entry_ { uncaught_exceptions() } {
        } catch ( ... ) {
          (void)fn();
          // it would be a bit contradictory to rethrow an exception here...
          // at least the gnu experimental/scope doesn't do that.
        }
        template<
          typename Fn = EF,
          typename    = typename std::enable_if<traits::AnyOf<std::is_nothrow_move_constructible<Fn>,
                                                              std::is_copy_constructible<Fn>>::value>::type>
        ScopeFail( ScopeFail&& rhs ) noexcept( traits::AnyOf<std::is_nothrow_move_constructible<EF>,
                                                             std::is_nothrow_copy_constructible<EF>>::value )
          : Base( std::is_nothrow_move_constructible<EF>(), std::forward<EF>( rhs.callback() ) )
          , exceptions_on_entry_ { rhs.exceptions_on_entry_ }
        { rhs.release(); }
        ~ScopeFail() noexcept
        {
          if ( exceptions_on_entry_ < uncaught_exceptions() ) {
            (void)( this->callback()() );
            this->release();
          }
        }

        PACE__FORCEINLINE PACE__CXX14_CNSTXPR void release() noexcept
        { exceptions_on_entry_ = ( std::numeric_limits<int>::max )(); }
      };

#ifdef __cpp_deduction_guides
      template<typename F>
      ScopeFail( F fn ) -> ScopeFail<F>;
#endif

      // The move construction implementation can directly use `auto var = std::move( /* guard */ )`,
      // so the corresponding factory function overload is not provided.
      template<typename F>
      PACE__NODISCARD ScopeFail<F> make_scope_fail( F&& fn )
        noexcept( std::is_nothrow_constructible<ScopeFail<F>, F&&>::value )
      { return ScopeFail<F>( std::forward<F>( fn ) ); }
    } // namespace utils
  } // namespace details
} // namespace pace

#endif
