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

      PACE__CXX17_INLINE const auto stylereset  = Escode( "0" );
      PACE__CXX17_INLINE const auto fgcolorrest = Escode( "39" );
      PACE__CXX17_INLINE const auto bgcolorrest = Escode( "49" );

      PACE__CXX17_INLINE const auto fontbold      = Escode( "1" );
      PACE__CXX17_INLINE const auto fontfaint     = Escode( "2" );
      PACE__CXX17_INLINE const auto fontitalic    = Escode( "3" );
      PACE__CXX17_INLINE const auto fontunderline = Escode( "4" );
      PACE__CXX17_INLINE const auto fontinverse   = Escode( "7" );
      PACE__CXX17_INLINE const auto fonthidden    = Escode( "8" );
      PACE__CXX17_INLINE const auto fontcrossed   = Escode( "9" );

      PACE__CXX17_INLINE constexpr auto& savecursor      = u8"\x1B[s";
      PACE__CXX17_INLINE constexpr auto& resetcursor     = u8"\x1B[u";
      PACE__CXX17_INLINE constexpr auto& linewipe        = u8"\x1B[K";
      PACE__CXX17_INLINE constexpr auto& prevline        = u8"\x1B[A";
      PACE__CXX17_INLINE constexpr types::Char nextline  = '\n';
      PACE__CXX17_INLINE constexpr types::Char linestart = '\r';
    } // namespace console
  } // namespace details
} // namespace pace

#endif
