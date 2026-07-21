#ifndef PACE_COLORIZE
#define PACE_COLORIZE

#include "../core/Core.hpp"
#include "TrueColor.hpp"

namespace pace {
  namespace details {
    namespace console {
      struct Forecolor {
        const TrueColor& value;

        PACE__FORCEINLINE friend io::CharPipeline& operator<<( io::CharPipeline& pipeline,
                                                               const Forecolor& self )
        {
#ifdef PACE_NOSTYLE
          (void)self;
#else
          if ( self.value.encoding() != render::Paint::None ) {
            pipeline << '\x1B' << '[';
            switch ( self.value.encoding() ) {
            case render::Paint::Xterm24bit: pipeline << '3' << '8' << ';' << '2'; PACE__FALLTHROUGH;
            case render::Paint::Csi8:       self.value.emit( pipeline ); break;
            default:                        utils::unreachable();
            }
            pipeline << 'm';
          }
#endif
          return pipeline;
        }
      };

      struct Backcolor {
        const TrueColor& value;

        PACE__FORCEINLINE friend io::CharPipeline& operator<<( io::CharPipeline& pipeline,
                                                               const Backcolor& self )
        {
#ifdef PACE_NOSTYLE
          (void)self;
#else
          if ( self.value.encoding() != render::Paint::None ) {
            pipeline << '\x1B' << '[';
            switch ( self.value.encoding() ) {
            case render::Paint::Xterm24bit: pipeline << '4' << '8' << ';' << '2'; PACE__FALLTHROUGH;
            case render::Paint::Csi8:       self.value.emit( pipeline ); break;
            default:                        utils::unreachable();
            }
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
          switch ( self.foreground.encoding() ) {
          case render::Paint::Xterm24bit: pipeline << '3' << '8' << ';' << '2'; PACE__FALLTHROUGH;
          case render::Paint::Csi8:       self.foreground.emit( pipeline ); break;
          default:                        utils::unreachable();
          }
          pipeline << ';';
          switch ( self.background.encoding() ) {
          case render::Paint::Xterm24bit: pipeline << '4' << '8' << ';' << '2'; PACE__FALLTHROUGH;
          case render::Paint::Csi8:       self.background.emit( pipeline ); break;
          default:                        utils::unreachable();
          }
          pipeline << 'm';
#endif
          return pipeline;
        }
      };
    } // namespace console
  } // namespace details
} // namespace pace

#endif
