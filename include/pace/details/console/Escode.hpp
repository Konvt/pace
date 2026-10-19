#ifndef PACE_ESCODE
#define PACE_ESCODE

#include "../io/CharPipeline.hpp"
#include "../utils/Format.hpp"

namespace pace {
  namespace details {
    namespace console {
#define PACE__METHOD( FunctionName, Param )                                      \
  PACE__FORCEINLINE io::CharPipeline& FunctionName( io::CharPipeline& pipeline ) \
  { return pipeline << Param; }

      PACE__METHOD( resetstyle, "\x1B[0m" );
      PACE__METHOD( resetfgcolor, "\x1B[39m" );
      PACE__METHOD( resetbgcolor, "\x1B[49m" );
      PACE__METHOD( resetcolor, "\x1B[39;49m" );

      PACE__METHOD( fontbold, "\x1B[1m" );
      PACE__METHOD( fontfaint, "\x1B[2m" );
      PACE__METHOD( fontitalic, "\x1B[3m" );
      PACE__METHOD( fontunderline, "\x1B[4m" );
      PACE__METHOD( fontinverse, "\x1B[7m" );
      PACE__METHOD( fonthidden, "\x1B[8m" );
      PACE__METHOD( fontcrossed, "\x1B[9m" );

      PACE__METHOD( savecursor, "\x1B[s" );
      PACE__METHOD( restorecursor, "\x1B[u" );
      PACE__METHOD( linewipe, "\x1B[K" );
      PACE__METHOD( prevline, "\x1B[F" );

#ifdef __cpp_inline_variables
      // Since manipulators are not called as functions,
      // we can optimize these two special (single-character-returning) functions
      // so they are more likely to benefit from vectorization when inserted into CharPipeline.
      PACE__CXX17_INLINE constexpr auto nextline  = '\n';
      PACE__CXX17_INLINE constexpr auto linestart = '\r';
#else
      PACE__METHOD( nextline, '\n' );
      PACE__METHOD( linestart, '\r' );
#endif

      PACE__METHOD( cursorup, "\x1B[A" );
      PACE__METHOD( cursordown, "\x1B[B" );
      PACE__METHOD( resetscroll, "\x1B[r" );

#undef PACE__METHOD

      inline io::CharPipeline& prevline_n( io::CharPipeline& pipeline, std::uint16_t n )
      {
        pipeline.append( "\x1B[" );
        utils::format_to( std::back_inserter( pipeline ), n );
        return pipeline.push_back( 'F' );
      }

      inline io::CharPipeline& cursorup_n( io::CharPipeline& pipeline, std::uint16_t n )
      {
        pipeline.append( "\x1B[" );
        utils::format_to( std::back_inserter( pipeline ), n );
        return pipeline.push_back( 'A' );
      }

      inline io::CharPipeline& cursordown_n( io::CharPipeline& pipeline, std::uint16_t n )
      {
        pipeline.append( "\x1B[" );
        utils::format_to( std::back_inserter( pipeline ), n );
        return pipeline.push_back( 'B' );
      }

      inline io::CharPipeline& setcursor( io::CharPipeline& pipeline, std::uint16_t x, std::uint16_t y )
      {
        pipeline.append( "\x1B[" );
        utils::format_to( std::back_inserter( pipeline ), x );
        pipeline.push_back( ';' );
        utils::format_to( std::back_inserter( pipeline ), y );
        return pipeline.push_back( 'H' );
      }

      inline io::CharPipeline& scrollregion( io::CharPipeline& pipeline,
                                             std::uint16_t top,
                                             std::uint16_t bottom )
      {
        pipeline.append( "\x1B[" );
        utils::format_to( std::back_inserter( pipeline ), top );
        pipeline.push_back( ';' );
        utils::format_to( std::back_inserter( pipeline ), bottom );
        return pipeline.push_back( 'r' );
      }
    } // namespace console

    namespace io {
      struct Prevlines {
        std::tuple<std::uint16_t> value;

        PACE__FORCEINLINE friend PACE__CXX23_CNSTXPR io::CharPipeline& operator<<( io::CharPipeline& pipeline,
                                                                                   const Prevlines& self )
        { return console::prevline_n( pipeline, std::get<0>( self.value ) ); }
      };
      PACE__NODISCARD PACE__FORCEINLINE Prevlines prevline_n( std::uint16_t n ) noexcept
      { return { { n } }; }

      struct CursorUp {
        std::tuple<std::uint16_t> value;

        PACE__FORCEINLINE friend PACE__CXX23_CNSTXPR io::CharPipeline& operator<<( io::CharPipeline& pipeline,
                                                                                   const CursorUp& self )
        { return console::cursorup_n( pipeline, std::get<0>( self.value ) ); }
      };
      PACE__NODISCARD PACE__FORCEINLINE CursorUp cursorup( std::uint16_t n ) noexcept
      { return { { n } }; }

      struct CursorDown {
        std::tuple<std::uint16_t> value;

        PACE__FORCEINLINE friend PACE__CXX23_CNSTXPR io::CharPipeline& operator<<( io::CharPipeline& pipeline,
                                                                                   const CursorDown& self )
        { return console::cursordown_n( pipeline, std::get<0>( self.value ) ); }
      };
      PACE__NODISCARD PACE__FORCEINLINE CursorDown cursordown( std::uint16_t n ) noexcept
      { return { { n } }; }

      struct SetCursor {
        std::tuple<std::uint16_t, std::uint16_t> value;

        PACE__FORCEINLINE friend PACE__CXX23_CNSTXPR io::CharPipeline& operator<<( io::CharPipeline& pipeline,
                                                                                   const SetCursor& self )
        { return console::setcursor( pipeline, std::get<0>( self.value ), std::get<1>( self.value ) ); }
      };
      PACE__NODISCARD PACE__FORCEINLINE SetCursor setcursor( std::uint16_t x, std::uint16_t y ) noexcept
      {
        return {
          { x, y }
        };
      }

      struct Scroll {
        std::tuple<std::uint16_t, std::uint16_t> value;

        PACE__FORCEINLINE friend PACE__CXX23_CNSTXPR io::CharPipeline& operator<<( io::CharPipeline& pipeline,
                                                                                   const Scroll& self )
        { return console::scrollregion( pipeline, std::get<0>( self.value ), std::get<1>( self.value ) ); }
      };
      PACE__NODISCARD PACE__FORCEINLINE Scroll scroll( std::uint16_t top, std::uint16_t bottom ) noexcept
      {
        return {
          { top, bottom }
        };
      }
    } // namespace io
  } // namespace details
} // namespace pace

#endif
