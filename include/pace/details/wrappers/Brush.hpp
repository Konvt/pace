#ifndef PACE_BRUSH
#define PACE_BRUSH

#include "../io/CharPipeline.hpp"

namespace pace {
  namespace details {
    namespace wrappers {
      // A monoid type for implementing CPS transformation.
      template<typename Effect, typename NextAction = void>
      struct Brush {
        static_assert( !traits::is_instance_of<Effect, Brush>::value, "invalid recursive effect" );
#ifdef PACE_NOSTYLE
        constexpr Brush( const Effect*, NextAction&& ) noexcept {}

        friend PACE__FORCEINLINE io::CharPipeline& operator<<( io::CharPipeline& pipeline, const Brush& )
        {
          return pipeline;
        }

#else
        const Effect* effect_;
        NextAction next_;

        constexpr Brush( const Effect* effect, NextAction&& next )
          noexcept( std::is_nothrow_move_constructible<NextAction>::value )
          : effect_ { effect }, next_ { std::move( next ) }
        {}

        friend PACE__FORCEINLINE io::CharPipeline& operator<<( io::CharPipeline& pipeline,
                                                               const Brush& brush )
        {
          if ( brush.effect_ != nullptr )
            pipeline << *brush.effect_;
          return pipeline << brush.next_;
        }
#endif
      };

      template<typename Effect>
      struct Brush<Effect, void> {
        static_assert( !traits::is_instance_of<Effect, Brush>::value, "invalid recursive effect" );
#ifdef PACE_NOSTYLE
        constexpr Brush( const Effect* ) noexcept {}

        friend PACE__FORCEINLINE io::CharPipeline& operator<<( io::CharPipeline& pipeline, const Brush& )
        {
          return pipeline;
        }
#else
        const Effect* effect_ = nullptr;

        constexpr Brush( const Effect* effect ) noexcept : effect_ { effect } {}

        friend PACE__FORCEINLINE io::CharPipeline& operator<<( io::CharPipeline& pipeline,
                                                               const Brush& brush ) noexcept
        {
          if ( brush.effect_ != nullptr )
            pipeline << *brush.effect_;
          return pipeline;
        }
#endif
      };

      // Provide for those that are of the reference type, such as raw arrays.
      template<typename Effect, typename NextAction = void>
      using Brush_t = Brush<typename std::remove_reference<Effect>::type, NextAction>;
    } // namespace wrappers
  } // namespace details
} // namespace pace

#endif
