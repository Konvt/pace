#ifndef PACE_COLORIZE
#define PACE_COLORIZE

#include "../core/Core.hpp"
#include "TrueColor.hpp"

namespace pace {
  namespace details {
    namespace console {
      struct Forecolor {
        const TrueColor& color;

        PACE__FORCEINLINE friend io::CharPipeline& operator<<( io::CharPipeline& pipeline,
                                                               const Forecolor& self )
        {
#ifdef PACE_NOSTYLE
          (void)self;
#else
          if ( self.color.encoding() != render::Paint::None ) {
            pipeline << '\x1B' << '[';
            self.color.emit_foreground( pipeline );
            pipeline << 'm';
          }
#endif
          return pipeline;
        }
      };

      struct Backcolor {
        const TrueColor& color;

        PACE__FORCEINLINE friend io::CharPipeline& operator<<( io::CharPipeline& pipeline,
                                                               const Backcolor& self )
        {
#ifdef PACE_NOSTYLE
          (void)self;
#else
          if ( self.color.encoding() != render::Paint::None ) {
            pipeline << '\x1B' << '[';
            self.color.emit_background( pipeline );
            pipeline << 'm';
          }
#endif
          return pipeline;
        }
      };

      struct Dualcolor {
        const TrueColor& foreground;
        const TrueColor& background;

        PACE__FORCEINLINE friend io::CharPipeline& operator<<( io::CharPipeline& pipeline,
                                                               const Dualcolor& self )
        {
#ifdef PACE_NOSTYLE
          (void)self;
#else
          if ( self.background.encoding() == render::Paint::None )
            return pipeline << Forecolor { self.foreground };
          if ( self.foreground.encoding() == render::Paint::None )
            return pipeline << Backcolor { self.background };

          pipeline << '\x1B' << '[';
          self.foreground.emit_foreground( pipeline );
          pipeline << ';';
          self.background.emit_background( pipeline );
          pipeline << 'm';
#endif
          return pipeline;
        }
      };
    } // namespace console
  } // namespace details
} // namespace pace

#endif
