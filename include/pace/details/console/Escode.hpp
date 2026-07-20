#ifndef PACE_ESCODE
#define PACE_ESCODE

#include "../io/CharPipeline.hpp"

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
