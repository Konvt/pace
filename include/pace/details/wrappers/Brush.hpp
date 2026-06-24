#ifndef PACE_BRUSH
#define PACE_BRUSH

#include "../io/CharPipeline.hpp"
#include "../traits/ConceptTraits.hpp"
#include "../utils/Backport.hpp"

namespace pace {
  namespace details {
    namespace wrappers {
      // A monoid type for implementing CPS transformation.
      template<typename Effect, typename NextAction = void>
      class Brush;

      template<typename Effect>
      class Brush<Effect, void> {
        static_assert( !std::is_reference<Effect>::value, "incomplete type" );
        static_assert( !traits::is_instance_of<Effect, Brush>::value, "invalid recursive effect" );
#ifdef PACE_NOSTYLE
      public:
        constexpr Brush() = default;
        constexpr Brush( Effect&& ) noexcept {}

        Brush( Brush&& )              = default;
        Brush& operator=( Brush&& ) & = default;

        template<typename AnotherAction>
        void cast_to( Brush<Effect, AnotherAction>& ) noexcept
        {}

        friend PACE__FORCEINLINE io::CharPipeline& operator<<( io::CharPipeline& pipeline, const Brush& )
        { return pipeline; }
#else
        template<typename, typename>
        friend class Brush;

      protected:
        alignas( Effect ) types::Byte effect_[sizeof( Effect )];
        bool existed_;

      public:
        PACE__CXX20_CNSTXPR Brush() noexcept : existed_ { false } {}
        PACE__CXX20_CNSTXPR Brush( Effect&& effect )
          noexcept( std::is_nothrow_move_constructible<Effect>::value )
          : Brush()
        {
          utils::construct_at<Effect>( &effect_, std::move( effect ) );
          existed_ = true;
        }

        PACE__CXX20_CNSTXPR Brush( Brush&& rhs ) noexcept( std::is_nothrow_move_constructible<Effect>::value )
          : Brush()
        {
          if ( rhs.existed_ ) {
            if PACE__CXX17_CNSTXPR ( std::is_trivially_move_constructible<Effect>::value )
              std::copy( rhs.effect_, rhs.effect_ + sizeof( Effect ), effect_ );
            else
              utils::construct_at<Effect>( &effect_,
                                           std::move( *utils::launder_as<Effect>( &rhs.effect_ ) ) );
            if PACE__CXX17_CNSTXPR ( !std::is_trivially_destructible<Effect>::value )
              utils::destroy_at( utils::launder_as<Effect>( &rhs.effect_ ) );
            std::swap( existed_, rhs.existed_ );
          }
        }
        PACE__CXX20_CNSTXPR Brush& operator=( Brush&& rhs ) & noexcept(
          traits::all_of<std::is_nothrow_move_constructible<Effect>,
                         std::is_nothrow_move_assignable<Effect>>::value )
        {
          rhs.cast_to( *this );
          return *this;
        }

        template<typename AnotherAction>
        PACE__CXX20_CNSTXPR void cast_to( Brush<Effect, AnotherAction>& to )
          noexcept( traits::all_of<std::is_nothrow_move_constructible<Effect>,
                                   std::is_nothrow_move_assignable<Effect>,
                                   std::is_nothrow_destructible<Effect>>::value )
        {
          if ( existed_ ) {
            if PACE__CXX17_CNSTXPR ( std::is_trivially_move_assignable<Effect>::value )
              std::copy( effect_, effect_ + sizeof( Effect ), to.effect_ );
            else if ( to.existed_ )
              *utils::launder_as<Effect>( to.effect_ ) = std::move( *utils::launder_as<Effect>( effect_ ) );
            else
              utils::construct_at<Effect>( &to.effect_, std::move( *utils::launder_as<Effect>( &effect_ ) ) );
            if PACE__CXX17_CNSTXPR ( !std::is_trivially_destructible<Effect>::value )
              utils::destroy_at( utils::launder_as<Effect>( &effect_ ) );
            existed_    = false;
            to.existed_ = true;
          }
        }

        template<typename NewEffect>
        PACE__CXX20_CNSTXPR Brush<Effect, Brush<typename std::decay<NewEffect>::type, void>> append(
          NewEffect&& new_effect ) noexcept( traits::all_of<std::is_nothrow_move_constructible<NewEffect>,
                                                            std::is_nothrow_move_constructible<Effect>,
                                                            std::is_nothrow_destructible<Effect>>::value )
        {
          // We cannot extract the original type from the universal reference,
          // we have to use std::decay here.
          Brush<Effect, Brush<typename std::decay<NewEffect>::type, void>> ret { { std::forward<NewEffect>(
            new_effect ) } };
          cast_to( ret );
          return ret;
        }

        PACE__CXX20_CNSTXPR ~Brush() noexcept
        {
          if ( existed_ )
            utils::destroy_at( utils::launder_as<Effect>( &effect_ ) );
        }

        friend PACE__FORCEINLINE io::CharPipeline& operator<<( io::CharPipeline& pipeline,
                                                               const Brush& brush ) noexcept
        {
          if ( brush.existed_ )
            pipeline << *utils::launder_as<const Effect>( &brush.effect_ );
          return pipeline;
        }
#endif
      };

      template<typename Effect, typename NextAction>
      class Brush : private Brush<Effect, void> {
        static_assert( traits::is_instance_of<NextAction, Brush>::value, "invalid control flow" );

#ifdef PACE_NOSTYLE
      public:
        constexpr Brush( NextAction&& ) noexcept {}
        constexpr Brush( Effect&&, NextAction&& ) noexcept {}

        Brush( Brush&& )              = default;
        Brush& operator=( Brush&& ) & = default;

        template<typename AnotherAction>
        void map( Brush<Effect, AnotherAction>& ) noexcept
        {}

        friend PACE__FORCEINLINE io::CharPipeline& operator<<( io::CharPipeline& pipeline, const Brush& )
        { return pipeline; }
#else
        template<typename, typename>
        friend class Brush;
        using Base = Brush<Effect, void>;

        NextAction next_;

      public:
        PACE__CXX20_CNSTXPR Brush( NextAction&& next )
          noexcept( std::is_nothrow_move_constructible<NextAction>::value )
          : Base(), next_ { std::move( next ) }
        {}
        PACE__CXX20_CNSTXPR Brush( Effect&& effect, NextAction&& next )
          noexcept( traits::all_of<std::is_nothrow_move_constructible<Effect>,
                                   std::is_nothrow_move_constructible<NextAction>>::value )
          : Base( std::move( effect ) ), next_ { std::move( next ) }
        {}

        PACE__CXX20_CNSTXPR Brush( Brush&& rhs ) noexcept( std::is_nothrow_move_constructible<Effect>::value )
          : Base( std::move( rhs ) ), next_ { std::move( rhs.next_ ) }
        {}
        PACE__CXX20_CNSTXPR Brush& operator=( Brush&& rhs ) & noexcept(
          traits::all_of<std::is_nothrow_move_constructible<Effect>,
                         std::is_nothrow_move_assignable<Effect>>::value )
        {
          Base::operator=( std::move( rhs ) );
          next_ = std::move( rhs.next_ );
          return *this;
        }

        PACE__CXX20_CNSTXPR ~Brush() = default;

        template<typename NewEffect>
        PACE__CXX20_CNSTXPR auto append( NewEffect&& new_effect )
          noexcept( noexcept( std::declval<NextAction>().append( std::forward<NewEffect>( new_effect ) ) ) )
            -> Brush<Effect, decltype( std::declval<NextAction>().append( std::declval<NewEffect>() ) )>
        {
          auto next = next_.append( std::forward<NewEffect>( new_effect ) );
          Brush<Effect, decltype( next )> ret { std::move( next ) };
          this->cast_to( ret );
          return ret;
        }

        friend PACE__FORCEINLINE io::CharPipeline& operator<<( io::CharPipeline& pipeline,
                                                               const Brush& brush )
        {
          if ( brush.existed_ )
            pipeline << *utils::launder_as<const Effect>( &brush.effect_ );
          return pipeline << brush.next_;
        }
#endif
      };

    } // namespace wrappers
  } // namespace details
} // namespace pace

#endif
