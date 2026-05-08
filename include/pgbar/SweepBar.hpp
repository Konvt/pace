#ifndef PGBAR_SWEEP_BAR
#define PGBAR_SWEEP_BAR

#include "facade/Counter.hpp"
#include "facade/ETA.hpp"
#include "facade/Elapsed.hpp"
#include "facade/Percentage.hpp"
#include "facade/Speed.hpp"
#include "facade/SweepPlot.hpp"
#include "prefab/BasicBar.hpp"
#include "prefab/BasicConfig.hpp"
#include "slice/TrackedSpan.hpp"

namespace pgbar {
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

  PGBAR__PROVIDE_FOR( config::Sweep, option::Colored, true );
  PGBAR__PROVIDE_FOR( config::Sweep, option::Bolded, true );
  PGBAR__PROVIDE_FOR( config::Sweep, option::Shift, -3 );
  PGBAR__PROVIDE_FOR( config::Sweep, option::BarWidth, 30 );
  PGBAR__PROVIDE_FOR( config::Sweep, option::Magnitude, 1000 );
  PGBAR__PROVIDE_FOR( config::Sweep, option::Starting, u8"[" );
  PGBAR__PROVIDE_FOR( config::Sweep, option::Ending, u8"]" );
  PGBAR__PROVIDE_FOR( config::Sweep, option::Filler, u8"-" );
  PGBAR__PROVIDE_FOR( config::Sweep, option::Divider, u8" | " );
  PGBAR__PROVIDE_FOR( config::Sweep, option::InfoColor, Color::Cyan );
  template<>
  struct pgbar::config::ProvideFor<config::Sweep, option::Projection> {
    static option::Projection provide()
    {
      return config::Sweep::bake( option::Only<facade::SweepPlot, facade::Elapsed>() );
    }
  };
  template<>
  struct pgbar::config::ProvideFor<config::Sweep, option::Lead> {
    static option::Lead provide() { return { "<=>" }; }
  };
  template<>
  struct pgbar::config::ProvideFor<config::Sweep, option::SpeedUnit> {
    static option::SpeedUnit provide() { return option::SpeedUnit( { u8"Hz", u8"kHz", u8"MHz", u8"GHz" } ); }
  };
} // namespace pgbar

#endif
