#ifndef PACE_COLORIZE
#define PACE_COLORIZE

#include "../core/Core.hpp"
#include "TrueColor.hpp"

namespace pace {
  namespace details {
    namespace console {
      struct Forecolor {
        const TrueColor& value;

        friend PACE__FORCEINLINE io::CharPipeline& operator<<( io::CharPipeline& pipeline,
                                                               const Forecolor& foreground )
        {
#ifdef PACE_NOSTYLE
          (void)foreground;
#else
          if ( foreground.value.encoding() != render::Paint::None ) {
            pipeline << '\x1B' << '[';
            switch ( foreground.value.encoding() ) {
            case render::Paint::Xterm24bit: pipeline << '3' << '8' << ';' << '2'; PACE__FALLTHROUGH;
            case render::Paint::Csi8:       foreground.value.emit( pipeline ); break;
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

        friend PACE__FORCEINLINE io::CharPipeline& operator<<( io::CharPipeline& pipeline,
                                                               const Backcolor& background )
        {
#ifdef PACE_NOSTYLE
          (void)background;
#else
          if ( background.value.encoding() != render::Paint::None ) {
            pipeline << '\x1B' << '[';
            switch ( background.value.encoding() ) {
            case render::Paint::Xterm24bit: pipeline << '4' << '8' << ';' << '2'; PACE__FALLTHROUGH;
            case render::Paint::Csi8:       background.value.emit( pipeline ); break;
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

        friend PACE__FORCEINLINE io::CharPipeline& operator<<( io::CharPipeline& pipeline,
                                                               const Dualcolor& dual )
        {
#ifdef PACE_NOSTYLE
          (void)dual;
#else
          if ( dual.background.encoding() == render::Paint::None )
            return pipeline << Forecolor { dual.foreground };
          if ( dual.foreground.encoding() == render::Paint::None )
            return pipeline << Backcolor { dual.background };

          pipeline << '\x1B' << '[';
          switch ( dual.foreground.encoding() ) {
          case render::Paint::Xterm24bit: pipeline << '3' << '8' << ';' << '2'; PACE__FALLTHROUGH;
          case render::Paint::Csi8:       dual.foreground.emit( pipeline ); break;
          default:                        utils::unreachable();
          }
          pipeline << ';';
          switch ( dual.background.encoding() ) {
          case render::Paint::Xterm24bit: pipeline << '4' << '8' << ';' << '2'; PACE__FALLTHROUGH;
          case render::Paint::Csi8:       dual.background.emit( pipeline ); break;
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
