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
  template<Channel Sink = Channel::Stderr, Policy Mode = Policy::Async, Region Zone = Region::Fixed>
  using SweepBar = prefab::BasicBar<config::Sweep, Sink, Mode, Zone>;

  PACE__PROVIDE_FOR( config::Sweep, option::Colored, true );
  PACE__PROVIDE_FOR( config::Sweep, option::FontBold, true );
  PACE__PROVIDE_FOR( config::Sweep, option::ShowQuota, true );
  PACE__PROVIDE_FOR( config::Sweep, option::Shift, -3 );
  PACE__PROVIDE_FOR( config::Sweep, option::BarWidth, 30 );
  PACE__PROVIDE_FOR( config::Sweep, option::Magnitude, 1000 );
  PACE__PROVIDE_FOR( config::Sweep, option::Starting, "[" );
  PACE__PROVIDE_FOR( config::Sweep, option::Ending, "]" );
  PACE__PROVIDE_FOR( config::Sweep, option::Filler, "-" );
  PACE__PROVIDE_FOR( config::Sweep, option::Divider, " | " );
  PACE__PROVIDE_FOR( config::Sweep, option::InfoForecolor, Color::Cyan );
  template<>
  struct config::ProvideFor<config::Sweep, option::ElapsedFormat> {
    static option::ElapsedFormat provide()
    {
      static auto cached_option = option::ElapsedFormat( "+%H:%M:%S" );
      return cached_option;
    }
  };
  template<>
  struct config::ProvideFor<config::Sweep, option::ETAFormat> {
    static option::ETAFormat provide()
    {
      static auto cached_option = option::ETAFormat( "-%H:%M:%S" );
      return cached_option;
    }
  };
  template<>
  struct config::ProvideFor<config::Sweep, option::Projection> {
    static option::Projection provide()
    { return config::Sweep::bake( option::Only<facade::SweepPlot, facade::Elapsed>() ); }
  };
  template<>
  struct config::ProvideFor<config::Sweep, option::Lead> {
    static option::Lead provide() { return { "<=>" }; }
  };
  template<>
  struct config::ProvideFor<config::Sweep, option::SpeedUnit> {
    static option::SpeedUnit provide() { return option::SpeedUnit( { "Hz", "kHz", "MHz", "GHz" } ); }
  };
} // namespace pace

#endif
