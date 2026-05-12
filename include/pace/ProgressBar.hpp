#ifndef PACE_PROGRESS_BAR
#define PACE_PROGRESS_BAR

#include "facade/CharPlot.hpp"
#include "facade/Counter.hpp"
#include "facade/ETA.hpp"
#include "facade/Elapsed.hpp"
#include "facade/Percentage.hpp"
#include "facade/Speed.hpp"
#include "prefab/BasicBar.hpp"
#include "prefab/BasicConfig.hpp"
#include "slice/TrackedSpan.hpp"

namespace pace {
  namespace config {
    using Line = prefab::BasicConfig<facade::Percentage,
                                     facade::CharPlot,
                                     facade::Counter,
                                     facade::Speed,
                                     facade::Elapsed,
                                     facade::ETA>;
  }

  /**
   * The simplest progress bar, which is what you think it is.
   *
   * It's structure is shown below:
   * {LeftBorder}{Prefix}{Percent}{Starting}{Filler}{Lead}{Remain}{Ending}{Counter}{Speed}{Elapsed}{ETA}{Postfix}{RightBorder}
   */
  template<Channel Sink = Channel::Stderr, Policy Mode = Policy::Async, Region Zone = Region::Fixed>
  using ProgressBar = prefab::BasicBar<config::Line, Sink, Mode, Zone>;

  PACE__PROVIDE_FOR( config::Line, option::Colored, true );
  PACE__PROVIDE_FOR( config::Line, option::FontBold, true );
  PACE__PROVIDE_FOR( config::Line, option::Reversed, false );
  PACE__PROVIDE_FOR( config::Line, option::Shift, -2 );
  PACE__PROVIDE_FOR( config::Line, option::BarWidth, 30 );
  PACE__PROVIDE_FOR( config::Line, option::Magnitude, 1000 );
  PACE__PROVIDE_FOR( config::Line, option::Starting, u8"[" );
  PACE__PROVIDE_FOR( config::Line, option::Ending, u8"]" );
  PACE__PROVIDE_FOR( config::Line, option::Remain, u8" " );
  PACE__PROVIDE_FOR( config::Line, option::Filler, u8"=" );
  PACE__PROVIDE_FOR( config::Line, option::Divider, u8" | " );
  PACE__PROVIDE_FOR( config::Line, option::InfoForecolor, Color::Cyan );
  template<>
  struct pace::config::ProvideFor<config::Line, option::Projection> {
    static option::Projection provide() { return config::Line::bake( option::Except<>() ); }
  };
  template<>
  struct pace::config::ProvideFor<config::Line, option::Lead> {
    static option::Lead provide() { return { ">" }; }
  };
  template<>
  struct pace::config::ProvideFor<config::Line, option::SpeedUnit> {
    static option::SpeedUnit provide() { return option::SpeedUnit( { u8"Hz", u8"kHz", u8"MHz", u8"GHz" } ); }
  };
} // namespace pace

#endif
