#ifndef PACE_SWEEP_BAR
#define PACE_SWEEP_BAR

#include "facade/Counter.hpp"
#include "facade/ETA.hpp"
#include "facade/Elapsed.hpp"
#include "facade/Percentage.hpp"
#include "facade/Speed.hpp"
#include "facade/SweepPlot.hpp"
#include "prefab/BasicBar.hpp"
#include "prefab/BasicConfig.hpp"
#include "slice/TrackedSpan.hpp"

namespace pace {
  namespace config {
    using Sweep = prefab::BasicConfig<facade::Percentage,
                                      facade::SweepPlot,
                                      facade::Counter,
                                      facade::Speed,
                                      facade::Elapsed,
                                      facade::ETA>;
  }

  /**
   * A progress bar with a sweeping indicator, where the lead moves back and forth within the bar area.
   *
   * It's structure is shown below:
   * {LeftBorder}{Prefix}{Percent}{Starting}{Filler}{Lead}{Filler}{Ending}{Counter}{Speed}{Elapsed}{ETA}{Postfix}{RightBorder}
   */
  template<Channel Outlet = Channel::Stderr, Policy Mode = Policy::Async, Region Area = Region::Fixed>
  using SweepBar = prefab::BasicBar<config::Sweep, Outlet, Mode, Area>;

  PACE__PROVIDE_FOR( config::Sweep, option::Colored, true );
  PACE__PROVIDE_FOR( config::Sweep, option::FontBold, true );
  PACE__PROVIDE_FOR( config::Sweep, option::Shift, -3 );
  PACE__PROVIDE_FOR( config::Sweep, option::BarWidth, 30 );
  PACE__PROVIDE_FOR( config::Sweep, option::Magnitude, 1000 );
  PACE__PROVIDE_FOR( config::Sweep, option::Starting, u8"[" );
  PACE__PROVIDE_FOR( config::Sweep, option::Ending, u8"]" );
  PACE__PROVIDE_FOR( config::Sweep, option::Filler, u8"-" );
  PACE__PROVIDE_FOR( config::Sweep, option::Divider, u8" | " );
  PACE__PROVIDE_FOR( config::Sweep, option::InfoColor, Color::Cyan );
  template<>
  struct pace::config::ProvideFor<config::Sweep, option::Projection> {
    static option::Projection provide()
    { return config::Sweep::bake( option::Only<facade::SweepPlot, facade::Elapsed>() ); }
  };
  template<>
  struct pace::config::ProvideFor<config::Sweep, option::Lead> {
    static option::Lead provide() { return { "<=>" }; }
  };
  template<>
  struct pace::config::ProvideFor<config::Sweep, option::SpeedUnit> {
    static option::SpeedUnit provide() { return option::SpeedUnit( { u8"Hz", u8"kHz", u8"MHz", u8"GHz" } ); }
  };
} // namespace pace

#endif
