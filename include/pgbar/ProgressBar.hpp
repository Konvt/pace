#ifndef PGBAR_PROGRESS_BAR
#define PGBAR_PROGRESS_BAR

#include "details/prefabs/BasicBar.hpp"
#include "details/prefabs/BasicConfig.hpp"
#include "facade/CharPlot.hpp"
#include "facade/Counter.hpp"
#include "facade/ETA.hpp"
#include "facade/Elapsed.hpp"
#include "facade/Percentage.hpp"
#include "facade/Speed.hpp"
#include "slice/TrackedSpan.hpp"

namespace pgbar {
  namespace config {
    using Line = _details::prefabs::BasicConfig<facade::Percentage,
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
  template<Channel Outlet = Channel::Stderr, Policy Mode = Policy::Async, Region Area = Region::Fixed>
  using ProgressBar = _details::prefabs::BasicBar<config::Line, Outlet, Mode, Area>;

  PGBAR__PROVIDE_FOR( config::Line, option::Colored, true );
  PGBAR__PROVIDE_FOR( config::Line, option::Bolded, true );
  PGBAR__PROVIDE_FOR( config::Line, option::Reversed, false );
  PGBAR__PROVIDE_FOR( config::Line, option::Shift, -2 );
  PGBAR__PROVIDE_FOR( config::Line, option::BarWidth, 30 );
  PGBAR__PROVIDE_FOR( config::Line, option::Magnitude, 1000 );
  PGBAR__PROVIDE_FOR( config::Line, option::Starting, u8"[" );
  PGBAR__PROVIDE_FOR( config::Line, option::Ending, u8"]" );
  PGBAR__PROVIDE_FOR( config::Line, option::Remain, u8" " );
  PGBAR__PROVIDE_FOR( config::Line, option::Filler, u8"=" );
  PGBAR__PROVIDE_FOR( config::Line, option::Divider, u8" | " );
  PGBAR__PROVIDE_FOR( config::Line, option::InfoColor, Color::Cyan );
  template<>
  struct pgbar::config::ProvideFor<config::Line, option::Projection> {
    static option::Projection provide() { return config::Line::bake( option::Except<>() ); }
  };
  template<>
  struct pgbar::config::ProvideFor<config::Line, option::Lead> {
    static option::Lead provide() { return { ">" }; }
  };
  template<>
  struct pgbar::config::ProvideFor<config::Line, option::SpeedUnit> {
    static option::SpeedUnit provide() { return option::SpeedUnit( { u8"Hz", u8"kHz", u8"MHz", u8"GHz" } ); }
  };
} // namespace pgbar

#endif
