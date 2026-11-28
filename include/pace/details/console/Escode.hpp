#ifndef PACE_ESCODE
#define PACE_ESCODE

#include "../io/CharPipeline.hpp"
#include "../utils/Format.hpp"

namespace pace {
  namespace details {
    namespace console {
#define PACE__METHOD( FunctionName, Param )                    \
  io::CharPipeline& FunctionName( io::CharPipeline& pipeline ) \
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

      // SCP; Save the current cursor position.
      PACE__METHOD( savecursor, "\x1B[s" );
      // RCP; Restore the previously saved cursor position.
      PACE__METHOD( restorecursor, "\x1B[u" );

      // EL 0; Erase from the cursor to the end of the line.
      PACE__METHOD( eraseafter, "\x1B[K" );
      // EL 1; Erase the entire line.
      PACE__METHOD( eraseline, "\x1B[2K" );

      // CNL; Move the cursor to the beginning of the next line.
      PACE__METHOD( nextline, "\x1B[E" );
      // CPL; Move the cursor to the beginning of the previous line.
      PACE__METHOD( prevline, "\x1B[F" );

      // LF; Move the cursor down by one line.
      PACE__METHOD( linefeed, '\n' );
      // CR; Move the cursor to the beginning of the current line.
      PACE__METHOD( linestart, '\r' );

      // CUP 1;1; Move the cursor to the home position.
      PACE__METHOD( firstline, "\x1B[H" );

      // IL 1; Insert one line at the current cursor position.
      PACE__METHOD( insertline, "\x1B[L" );
      // DL 1; Delete one line at the current cursor position.
      PACE__METHOD( deleteline, "\x1B[M" );

      // CUU 1; Move the cursor up by one line.
      PACE__METHOD( cursorup, "\x1B[A" );
      // CUD 1; Move the cursor down by one line.
      PACE__METHOD( cursordown, "\x1B[B" );
      // DECSTBM; Reset the scrolling region to the full screen.
      PACE__METHOD( resetscroll, "\x1B[r" );

      // IND; Move the cursor to the next line and scroll down if necessary.
      PACE__METHOD( cursornext, "\033D" );

#undef PACE__METHOD

      // CNL n; Move the cursor to the beginning of the previous line n times.
      inline io::CharPipeline& prevline_n( io::CharPipeline& pipeline, std::uint16_t n )
      {
        pipeline.append( "\x1B[" );
        utils::format_to( std::back_inserter( pipeline ), n );
        return pipeline.push_back( 'F' );
      }

      // LF n; Move the cursor down by n lines.
      inline PACE__CXX20_CNSTXPR io::CharPipeline& linefeed_n( io::CharPipeline& pipeline, std::uint16_t n )
      { return pipeline.append( '\n', n ); }

      // CUU n; Move the cursor up by n lines.
      inline io::CharPipeline& cursorup_n( io::CharPipeline& pipeline, std::uint16_t n )
      {
        pipeline.append( "\x1B[" );
        utils::format_to( std::back_inserter( pipeline ), n );
        return pipeline.push_back( 'A' );
      }
      // CUD n; Move the cursor down by n lines.
      inline io::CharPipeline& cursordown_n( io::CharPipeline& pipeline, std::uint16_t n )
      {
        pipeline.append( "\x1B[" );
        utils::format_to( std::back_inserter( pipeline ), n );
        return pipeline.push_back( 'B' );
      }

      // CUP x;1; Move the cursor to the beginning of line x.
      inline io::CharPipeline& setline( io::CharPipeline& pipeline, std::uint16_t x )
      {
        pipeline.append( "\x1B[" );
        utils::format_to( std::back_inserter( pipeline ), x );
        return pipeline.append( ";1H" );
      }

      // DECSTBM top;bottom; Set the scrolling region from line top to line bottom.
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

      // IND n; Move the cursor down by n lines, scrolling the screen if necessary.
      inline io::CharPipeline& cursornext_n( io::CharPipeline& pipeline, std::uint16_t n )
      { return pipeline.append( "\033D", n ); }

      // SD n; Scroll the scrolling region down by n lines.
      inline io::CharPipeline& scrolldown_n( io::CharPipeline& pipeline, std::uint16_t n )
      {
        pipeline.append( "\x1B[" );
        utils::format_to( std::back_inserter( pipeline ), n );
        return pipeline.push_back( 'T' );
      }

      // SU n; Scroll the scrolling region up by n lines.
      inline io::CharPipeline& scrollup_n( io::CharPipeline& pipeline, std::uint16_t n )
      {
        pipeline.append( "\x1B[" );
        utils::format_to( std::back_inserter( pipeline ), n );
        return pipeline.push_back( 'S' );
      }
    } // namespace console

    namespace io {
      struct Prevline {
        std::tuple<std::uint16_t> value;

        PACE__FORCEINLINE friend io::CharPipeline& operator<<( io::CharPipeline& pipeline,
                                                               const Prevline& self )
        { return console::prevline_n( pipeline, std::get<0>( self.value ) ); }
      };
      // CNL n; Create a manipulator that moves the cursor to the beginning of the previous line n times.
      PACE__NODISCARD PACE__FORCEINLINE PACE__CXX14_CNSTXPR Prevline prevline_n( std::uint16_t n ) noexcept
      { return { { n } }; }

      struct Linefeed {
        std::tuple<std::uint16_t> value;

        PACE__FORCEINLINE friend PACE__CXX20_CNSTXPR io::CharPipeline& operator<<( io::CharPipeline& pipeline,
                                                                                   const Linefeed& self )
        { return console::linefeed_n( pipeline, std::get<0>( self.value ) ); }
      };
      // LF n; Create a manipulator that moves the cursor down by n lines.
      PACE__NODISCARD PACE__FORCEINLINE PACE__CXX14_CNSTXPR Linefeed linefeed_n( std::uint16_t n ) noexcept
      { return { { n } }; }

      struct CursorUp {
        std::tuple<std::uint16_t> value;

        PACE__FORCEINLINE friend io::CharPipeline& operator<<( io::CharPipeline& pipeline,
                                                               const CursorUp& self )
        { return console::cursorup_n( pipeline, std::get<0>( self.value ) ); }
      };
      // CUU n; Create a manipulator that moves the cursor up by n lines.
      PACE__NODISCARD PACE__FORCEINLINE PACE__CXX14_CNSTXPR CursorUp cursorup_n( std::uint16_t n ) noexcept
      { return { { n } }; }

      struct CursorDown {
        std::tuple<std::uint16_t> value;

        PACE__FORCEINLINE friend io::CharPipeline& operator<<( io::CharPipeline& pipeline,
                                                               const CursorDown& self )
        { return console::cursordown_n( pipeline, std::get<0>( self.value ) ); }
      };
      // CUD n; Create a manipulator that moves the cursor down by n lines.
      PACE__NODISCARD PACE__FORCEINLINE PACE__CXX14_CNSTXPR CursorDown
        cursordown_n( std::uint16_t n ) noexcept
      { return { { n } }; }

      struct SetLine {
        std::tuple<std::uint16_t> value;

        PACE__FORCEINLINE friend io::CharPipeline& operator<<( io::CharPipeline& pipeline,
                                                               const SetLine& self )
        { return console::setline( pipeline, std::get<0>( self.value ) ); }
      };
      // CUP x;1; Create a manipulator that moves the cursor to the beginning of line x.
      PACE__NODISCARD PACE__FORCEINLINE PACE__CXX14_CNSTXPR SetLine setline( std::uint16_t x ) noexcept
      { return { { x } }; }

      struct SetScroll {
        std::tuple<std::uint16_t, std::uint16_t> value;

        PACE__FORCEINLINE friend io::CharPipeline& operator<<( io::CharPipeline& pipeline,
                                                               const SetScroll& self )
        { return console::setscroll( pipeline, std::get<0>( self.value ), std::get<1>( self.value ) ); }
      };
      // DECSTBM top;bottom; Create a manipulator that sets the scrolling region.
      PACE__NODISCARD PACE__FORCEINLINE PACE__CXX14_CNSTXPR SetScroll
        setscroll( std::uint16_t top, std::uint16_t bottom ) noexcept
      {
        return {
          { top, bottom }
        };
      }

      struct CursorNext {
        std::tuple<std::uint16_t> value;

        PACE__FORCEINLINE friend io::CharPipeline& operator<<( io::CharPipeline& pipeline,
                                                               const CursorNext& self )
        { return console::cursornext_n( pipeline, std::get<0>( self.value ) ); }
      };
      // IND n; Create a manipulator that moves the cursor down by n lines, scrolling if necessary.
      PACE__NODISCARD PACE__FORCEINLINE PACE__CXX14_CNSTXPR CursorNext
        cursornext_n( std::uint16_t n ) noexcept
      { return { { n } }; }

      struct ScrollDown {
        std::tuple<std::uint16_t> value;

        PACE__FORCEINLINE friend io::CharPipeline& operator<<( io::CharPipeline& pipeline,
                                                               const ScrollDown& self )
        { return console::scrolldown_n( pipeline, std::get<0>( self.value ) ); }
      };
      // SD n; Create a manipulator that scrolls the scrolling region down by n lines.
      PACE__NODISCARD PACE__FORCEINLINE PACE__CXX14_CNSTXPR ScrollDown
        scrolldown_n( std::uint16_t n ) noexcept
      { return { { n } }; }

      struct ScrollUp {
        std::tuple<std::uint16_t> value;

        PACE__FORCEINLINE friend io::CharPipeline& operator<<( io::CharPipeline& pipeline,
                                                               const ScrollUp& self )
        { return console::scrollup_n( pipeline, std::get<0>( self.value ) ); }
      };
      // SU n; Create a manipulator that scrolls the scrolling region up by n lines.
      PACE__NODISCARD PACE__FORCEINLINE PACE__CXX14_CNSTXPR ScrollUp scrollup_n( std::uint16_t n ) noexcept
      { return { { n } }; }
    } // namespace io
  } // namespace details
} // namespace pace

#endif
