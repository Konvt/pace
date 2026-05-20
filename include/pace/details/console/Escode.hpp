#ifndef PACE_ESCODE
#define PACE_ESCODE

#include "../io/CharPipeline.hpp"
#include <algorithm>
#include <array>

namespace pace {
  namespace details {
    namespace console {
      class Escode {
#ifndef PACE_NOSTYLE
        std::array<types::Char, 2> sequences_ {};
        std::uint8_t length_;
#endif

      public:
        template<types::Size N>
        PACE__CXX14_CNSTXPR Escode( const types::Char ( &str )[N] ) noexcept
        {
#ifdef PACE_NOSTYLE
          (void)str;
#else
          static_assert( N <= std::tuple_size<decltype( sequences_ )>::value + 1,
                         "the literal string is too long" );
          std::copy( str, str + N - 1, sequences_.begin() );
          length_ = N - 1;
#endif
        }

        Escode( const Escode& )              = default;
        Escode& operator=( const Escode& ) & = default;

        friend io::CharPipeline& operator<<( io::CharPipeline& pipeline, const Escode& esc )
        {
#ifdef PACE_NOSTYLE
          (void)esc;
#else
          pipeline.append( '\x1B' )
            .append( '[' )
            .append( esc.sequences_.data(), esc.sequences_.data() + esc.length_ )
            .append( 'm' );
#endif
          return pipeline;
        }
      };

      // We wrote a Monoid `Brush` to abstract a series of coloring actions into a fixed behavior chain.
      // To enable these "actions" to be materialized as values that can be placed in the chain,
      // the following vars must exist in the form of variables.
      // However, pace is a library that is compatible with C++11.
      // If the "actions" here are treated as variables, then in some cases it might violate the ODR.
      // Therefore we have to let them be a return value of a constexpr factory function.

      PACE__NODISCARD PACE__FORCEINLINE PACE__CXX14_CNSTXPR Escode resetstyle() noexcept
      { return { "0" }; }
      PACE__NODISCARD PACE__FORCEINLINE PACE__CXX14_CNSTXPR Escode resetfgcolor() noexcept
      { return { "39" }; }
      PACE__NODISCARD PACE__FORCEINLINE PACE__CXX14_CNSTXPR Escode resetbgcolor() noexcept
      { return { "49" }; }

      PACE__NODISCARD PACE__FORCEINLINE PACE__CXX14_CNSTXPR Escode fontbold() noexcept
      { return { "1" }; }
      PACE__NODISCARD PACE__FORCEINLINE PACE__CXX14_CNSTXPR Escode fontfaint() noexcept
      { return { "2" }; }
      PACE__NODISCARD PACE__FORCEINLINE PACE__CXX14_CNSTXPR Escode fontitalic() noexcept
      { return { "3" }; }
      PACE__NODISCARD PACE__FORCEINLINE PACE__CXX14_CNSTXPR Escode fontunderline() noexcept
      { return { "4" }; }
      PACE__NODISCARD PACE__FORCEINLINE PACE__CXX14_CNSTXPR Escode fontinverse() noexcept
      { return { "7" }; }
      PACE__NODISCARD PACE__FORCEINLINE PACE__CXX14_CNSTXPR Escode fonthidden() noexcept
      { return { "8" }; }
      PACE__NODISCARD PACE__FORCEINLINE PACE__CXX14_CNSTXPR Escode fontcrossed() noexcept
      { return { "9" }; }

      // The following operations do not need to participate in the effect composition of the rendering,
      // so they can be written in the form of functions.

#define PACE__METHOD( FunctionName, Param )                    \
  io::CharPipeline& FunctionName( io::CharPipeline& pipeline ) \
  { return pipeline << Param; }

      PACE__METHOD( savecursor, "\x1B[s" );
      PACE__METHOD( resetcursor, "\x1B[u" );
      PACE__METHOD( linewipe, "\x1B[K" );
      PACE__METHOD( prevline, "\x1B[A" );
      PACE__METHOD( nextline, '\n' );
      PACE__METHOD( linestart, '\r' );

#undef PACE__METHOD
    } // namespace console
  } // namespace details
} // namespace pace

#endif
