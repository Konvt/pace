// This file serves the Automation component rather than providing functional supports for the progress bar.
#ifndef PACE_ITERABLE
#define PACE_ITERABLE

#include "../../exception/Error.hpp"
#include "../../views/IteratorSpan.hpp"
#include "../../views/NumericSpan.hpp"
#include "../../views/SizedSpan.hpp"
#include "../../views/TrackedSpan.hpp"

namespace pace {
  namespace details {
    namespace behaviors {
      template<typename Derived>
      struct Iterable {
      private:
        // Throws the exception::StateError if current object is active.
        PACE__FORCEINLINE void throw_if_active()
        {
          if ( static_cast<Derived&>( *this )->active() )
            PACE__UNLIKELY throw exception::StateError(
              charcodes::make_literal( "pace: try to iterate using an active object" ) );
        }

      public:
        /**
         * Visualize unidirectional traversal of a numeric interval defined by parameters.
         *
         * @return Return a range `[startpoint, endpoint)` that moves unidirectionally.
         */
        template<typename N>
        PACE__NODISCARD auto iterate( N startpoint, N endpoint, N step ) &
#ifdef __cpp_concepts
          -> views::TrackedSpan<views::NumericSpan<N>, Derived*>
          requires std::is_arithmetic_v<N>
#else
          -> typename std::enable_if<std::is_arithmetic<N>::value,
                                     views::TrackedSpan<views::NumericSpan<N>, Derived*>>::type
#endif
        { // default parameter will cause ambiguous overloads
          // so we have to write them all
          throw_if_active();
          return {
            { startpoint, endpoint, step },
            static_cast<Derived*>( this )
          };
        }
        template<typename N, typename Proc>
        auto iterate( N startpoint, N endpoint, N step, Proc&& op )
#ifdef __cpp_concepts
          requires std::is_arithmetic_v<N>
#else
          -> typename std::enable_if<std::is_arithmetic<N>::value>::type
#endif
        {
          for ( N e : iterate( startpoint, endpoint, step ) )
            (void)op( e );
        }

        template<typename N>
        PACE__NODISCARD auto iterate( N endpoint, N step ) &
#ifdef __cpp_concepts
          -> views::TrackedSpan<views::NumericSpan<N>, Derived*>
          requires std::is_floating_point_v<N>
#else
          -> typename std::enable_if<std::is_floating_point<N>::value,
                                     views::TrackedSpan<views::NumericSpan<N>, Derived*>>::type
#endif
        {
          throw_if_active();
          return {
            { {}, endpoint, step },
            static_cast<Derived*>( this )
          };
        }
        template<typename N, typename Proc>
        auto iterate( N endpoint, N step, Proc&& op )
#ifdef __cpp_concepts
          requires std::is_floating_point_v<N>
#else
          -> typename std::enable_if<std::is_floating_point<N>::value>::type
#endif
        {
          for ( N e : iterate( endpoint, step ) )
            (void)op( e );
        }

        // Only available for integer types.
        template<typename N>
        PACE__NODISCARD auto iterate( N startpoint, N endpoint ) &
#ifdef __cpp_concepts
          -> views::TrackedSpan<views::NumericSpan<N>, Derived*>
          requires std::is_integral_v<N>
#else
          -> typename std::enable_if<std::is_integral<N>::value,
                                     views::TrackedSpan<views::NumericSpan<N>, Derived*>>::type
#endif
        {
          throw_if_active();
          return {
            { startpoint, endpoint },
            static_cast<Derived*>( this )
          };
        }
        template<typename N, typename Proc>
        auto iterate( N startpoint, N endpoint, Proc&& op )
#ifdef __cpp_concepts
          requires std::is_integral_v<N>
#else
          -> typename std::enable_if<std::is_integral<N>::value>::type
#endif
        {
          for ( N e : iterate( startpoint, endpoint ) )
            (void)op( e );
        }

        template<typename N>
        PACE__NODISCARD auto iterate( N endpoint ) &
#ifdef __cpp_concepts
          -> views::TrackedSpan<views::NumericSpan<N>, Derived*>
          requires std::is_integral_v<N>
#else
          -> typename std::enable_if<std::is_integral<N>::value,
                                     views::TrackedSpan<views::NumericSpan<N>, Derived*>>::type
#endif
        {
          throw_if_active();
          return { { endpoint }, static_cast<Derived*>( this ) };
        }
        template<typename N, typename Proc>
        auto iterate( N endpoint, Proc&& op )
#ifdef __cpp_concepts
          requires std::is_integral_v<N>
#else
          -> typename std::enable_if<std::is_integral<N>::value>::type
#endif
        {
          for ( N e : iterate( endpoint ) )
            (void)op( e );
        }

        // Visualize unidirectional traversal of a iterator interval defined by parameters.
        template<typename Itr, typename Snt>
        PACE__NODISCARD auto iterate( Itr startpoint, Snt endpoint ) & noexcept(
          traits::AllOf<std::is_nothrow_move_constructible<Itr>,
                        std::is_nothrow_move_constructible<Snt>>::value )
#ifdef __cpp_concepts
          -> views::TrackedSpan<views::IteratorSpan<Itr, Snt>, Derived*>
          requires traits::is_sized_cursor<Itr, Snt>::value
#else
          -> typename std::enable_if<traits::is_sized_cursor<Itr, Snt>::value,
                                     views::TrackedSpan<views::IteratorSpan<Itr, Snt>, Derived*>>::type
#endif
        {
          throw_if_active();
          return {
            { std::move( startpoint ), std::move( endpoint ) },
            static_cast<Derived*>( this )
          };
        }
        template<typename Itr, typename Snt, typename Proc>
        auto iterate( Itr startpoint, Snt endpoint, Proc&& op )
#ifdef __cpp_concepts
          requires traits::is_sized_cursor<Itr, Snt>::value
#else
          -> typename std::enable_if<traits::is_sized_cursor<Itr, Snt>::value>::type
#endif
        {
          for ( auto&& e : iterate( std::move( startpoint ), std::move( endpoint ) ) )
            (void)utils::invoke( op, std::forward<decltype( e )>( e ) );
        }

        // Visualize unidirectional traversal of a abstract range interval defined by `container`'s
        // views.
        template<class R>
        PACE__NODISCARD auto iterate( R& container ) &
#ifdef __cpp_concepts
          -> views::TrackedSpan<views::SizedSpan<std::remove_reference_t<R>>, Derived*>
          requires( traits::is_sized_range<std::remove_reference_t<R>>::value
                    && !std::ranges::view<std::remove_reference_t<R>> )
#else
          -> typename std::enable_if<
            traits::is_sized_range<typename std::remove_reference<R>::type>::value,
            views::TrackedSpan<views::SizedSpan<typename std::remove_reference<R>::type>, Derived*>>::type
#endif
        {
          throw_if_active();
          return { { container }, static_cast<Derived*>( this ) };
        }
#ifdef __cpp_concepts
        template<class R>
          requires( traits::is_sized_range<R>::value && std::ranges::view<R> )
        PACE__NODISCARD auto iterate( R view ) & -> views::TrackedSpan<R, Derived*>
        {
          throw_if_active();
          return { std::move( view ), static_cast<Derived*>( this ) };
        }
#endif
        template<class R, typename Proc>
        auto iterate( R&& range, Proc&& op )
#ifdef __cpp_concepts
          requires traits::is_sized_range<std::remove_reference_t<R>>::value
#else
          -> typename std::enable_if<
            traits::is_sized_range<typename std::remove_reference<R>::type>::value>::type
#endif
        {
          for ( auto&& e : iterate( std::forward<R>( range ) ) )
            (void)utils::invoke( op, std::forward<decltype( e )>( e ) );
        }
      };

    } // namespace behaviors
  } // namespace details
} // namespace pace

#endif
