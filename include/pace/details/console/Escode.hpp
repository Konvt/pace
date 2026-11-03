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

      PACE__METHOD( nextline, '\n' );
      PACE__METHOD( linestart, '\r' );
      // Set the cursor to the beginning of the first line.
      PACE__METHOD( firstline, "\x1B[H" );

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
      inline PACE__CXX20_CNSTXPR io::CharPipeline& nextline_n( io::CharPipeline& pipeline, std::uint16_t n )
      { return pipeline.append( '\n', n ); }

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

      // Set the cursor to the beginning of line x.
      inline io::CharPipeline& setline( io::CharPipeline& pipeline, std::uint16_t x )
      {
        pipeline.append( "\x1B[" );
        utils::format_to( std::back_inserter( pipeline ), x );
        return pipeline.append( ";1H" );
      }

      inline io::CharPipeline& setscroll( io::CharPipeline& pipeline,
                                          std::uint16_t top,
                                          std::uint16_t bottom )
      { // DECSTBM
        pipeline.append( "\x1B[" );
        utils::format_to( std::back_inserter( pipeline ), top );
        pipeline.push_back( ';' );
        utils::format_to( std::back_inserter( pipeline ), bottom );
        return pipeline.push_back( 'r' );
      }
    } // namespace console

    namespace io {
      struct Prevline {
        std::tuple<std::uint16_t> value;

        PACE__FORCEINLINE friend io::CharPipeline& operator<<( io::CharPipeline& pipeline,
                                                               const Prevline& self )
        { return console::prevline_n( pipeline, std::get<0>( self.value ) ); }
      };
      PACE__NODISCARD PACE__FORCEINLINE PACE__CXX14_CNSTXPR Prevline prevline_n( std::uint16_t n ) noexcept
      { return { { n } }; }

      struct Nextline {
        std::tuple<std::uint16_t> value;

        PACE__FORCEINLINE friend PACE__CXX20_CNSTXPR io::CharPipeline& operator<<( io::CharPipeline& pipeline,
                                                                                   const Nextline& self )
        { return console::nextline_n( pipeline, std::get<0>( self.value ) ); }
      };
      PACE__NODISCARD PACE__FORCEINLINE PACE__CXX14_CNSTXPR Nextline nextline_n( std::uint16_t n ) noexcept
      { return { { n } }; }

      struct CursorUp {
        std::tuple<std::uint16_t> value;

        PACE__FORCEINLINE friend io::CharPipeline& operator<<( io::CharPipeline& pipeline,
                                                               const CursorUp& self )
        { return console::cursorup_n( pipeline, std::get<0>( self.value ) ); }
      };
      PACE__NODISCARD PACE__FORCEINLINE PACE__CXX14_CNSTXPR CursorUp cursorup( std::uint16_t n ) noexcept
      { return { { n } }; }

      struct CursorDown {
        std::tuple<std::uint16_t> value;

        PACE__FORCEINLINE friend io::CharPipeline& operator<<( io::CharPipeline& pipeline,
                                                               const CursorDown& self )
        { return console::cursordown_n( pipeline, std::get<0>( self.value ) ); }
      };
      PACE__NODISCARD PACE__FORCEINLINE PACE__CXX14_CNSTXPR CursorDown cursordown( std::uint16_t n ) noexcept
      { return { { n } }; }

      struct SetLine {
        std::tuple<std::uint16_t> value;

        PACE__FORCEINLINE friend io::CharPipeline& operator<<( io::CharPipeline& pipeline,
                                                               const SetLine& self )
        { return console::setline( pipeline, std::get<0>( self.value ) ); }
      };
      PACE__NODISCARD PACE__FORCEINLINE PACE__CXX14_CNSTXPR SetLine setline( std::uint16_t x ) noexcept
      { return { { x } }; }

      struct SetScroll {
        std::tuple<std::uint16_t, std::uint16_t> value;

        PACE__FORCEINLINE friend io::CharPipeline& operator<<( io::CharPipeline& pipeline,
                                                               const SetScroll& self )
        { return console::setscroll( pipeline, std::get<0>( self.value ), std::get<1>( self.value ) ); }
      };
      PACE__NODISCARD PACE__FORCEINLINE PACE__CXX14_CNSTXPR SetScroll
        setscroll( std::uint16_t top, std::uint16_t bottom ) noexcept
      {
        return {
          { top, bottom }
        };
      }
    } // namespace io
  } // namespace details
} // namespace pace

#endif
