#ifndef PACE_COLORIZE
#define PACE_COLORIZE

#include "TrueColor.hpp"
#include "pace/details/core/Core.hpp"

namespace pace {
  namespace details {
    namespace console {
      struct Forecolor {
        std::reference_wrapper<const TrueColor> color_;

        PACE__CXX20_CNSTXPR Forecolor( const TrueColor& color ) noexcept : color_ { color } {}

        friend PACE__FORCEINLINE io::CharPipeline& operator<<( io::CharPipeline& pipeline,
                                                               const Forecolor& foreground )
        {
#ifdef PACE_NOSTYLE
          (void)foreground;
#else
          if ( foreground.color_.get().encoding() != render::Paint::None ) {
            pipeline << '\x1B' << '[';
            switch ( foreground.color_.get().encoding() ) {
            case render::Paint::Xterm24bit: pipeline << '3' << '8' << ';' << '2'; PACE__FALLTHROUGH;
            case render::Paint::Csi8:       foreground.color_.get().emit( pipeline ); break;
            default:                        utils::unreachable();
            }
            pipeline << 'm';
          }
#endif
          return pipeline;
        }
      };

      struct Backcolor {
        std::reference_wrapper<const TrueColor> color_;

        PACE__CXX20_CNSTXPR Backcolor( const TrueColor& color ) noexcept : color_ { color } {}

        friend PACE__FORCEINLINE io::CharPipeline& operator<<( io::CharPipeline& pipeline,
                                                               const Backcolor& background )
        {
#ifdef PACE_NOSTYLE
          (void)background;
#else
          if ( background.color_.get().encoding() != render::Paint::None ) {
            pipeline << '\x1B' << '[';
            switch ( background.color_.get().encoding() ) {
            case render::Paint::Xterm24bit: pipeline << '4' << '8' << ';' << '2'; PACE__FALLTHROUGH;
            case render::Paint::Csi8:       background.color_.get().emit( pipeline ); break;
            default:                        utils::unreachable();
            }
            pipeline << 'm';
          }
#endif
          return pipeline;
        }
      };

      struct Dualcolor {
        std::reference_wrapper<const TrueColor> foreground_;
        std::reference_wrapper<const TrueColor> background_;

        PACE__CXX20_CNSTXPR Dualcolor( const TrueColor& foreground, const TrueColor& background ) noexcept
          : foreground_ { foreground }, background_ { background }
        {}

        friend PACE__FORCEINLINE io::CharPipeline& operator<<( io::CharPipeline& pipeline,
                                                               const Dualcolor& dual )
        {
#ifdef PACE_NOSTYLE
          (void)dual;
#else
          if ( dual.background_.get().encoding() == render::Paint::None )
            return pipeline << Forecolor( dual.foreground_ );
          if ( dual.foreground_.get().encoding() == render::Paint::None )
            return pipeline << Backcolor( dual.background_ );

          pipeline << '\x1B' << '[';
          switch ( dual.foreground_.get().encoding() ) {
          case render::Paint::Xterm24bit: pipeline << '3' << '8' << ';' << '2'; PACE__FALLTHROUGH;
          case render::Paint::Csi8:       dual.foreground_.get().emit( pipeline ); break;
          default:                        utils::unreachable();
          }
          pipeline << ';';
          switch ( dual.background_.get().encoding() ) {
          case render::Paint::Xterm24bit: pipeline << '4' << '8' << ';' << '2'; PACE__FALLTHROUGH;
          case render::Paint::Csi8:       dual.background_.get().emit( pipeline ); break;
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
