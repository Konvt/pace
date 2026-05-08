#ifndef PGBAR_FLOW_BAR
#define PGBAR_FLOW_BAR

#include "details/prefabs/BasicBar.hpp"
#include "details/prefabs/BasicConfig.hpp"
#include "facade/Counter.hpp"
#include "facade/ETA.hpp"
#include "facade/Elapsed.hpp"
#include "facade/FlowPlot.hpp"
#include "facade/Percentage.hpp"
#include "facade/Speed.hpp"
#include "slice/TrackedSpan.hpp"

namespace pgbar {
  namespace config {
    using Flow = _details::prefabs::BasicConfig<facade::Percentage,
                                                facade::FlowPlot,
                                                facade::Counter,
                                                facade::Speed,
                                                facade::Elapsed,
                                                facade::ETA>;
  }

  /**
   * A progress bar with a flowing indicator, where the lead moves in a single direction within the bar area.
   *
   * It's structure is shown below:
   * {LeftBorder}{Prefix}{Percent}{Starting}{Filler}{Lead}{Filler}{Ending}{Counter}{Speed}{Elapsed}{ETA}{Postfix}{RightBorder}
   */
  template<Channel Outlet = Channel::Stderr, Policy Mode = Policy::Async, Region Area = Region::Fixed>
  using FlowBar = _details::prefabs::BasicBar<config::Flow, Outlet, Mode, Area>;

  PGBAR__PROVIDE_FOR( config::Flow, option::Colored, true );
  PGBAR__PROVIDE_FOR( config::Flow, option::Bolded, true );
  PGBAR__PROVIDE_FOR( config::Flow, option::Reversed, false );
  PGBAR__PROVIDE_FOR( config::Flow, option::Shift, -3 );
  PGBAR__PROVIDE_FOR( config::Flow, option::BarWidth, 30 );
  PGBAR__PROVIDE_FOR( config::Flow, option::Magnitude, 1000 );
  PGBAR__PROVIDE_FOR( config::Flow, option::Starting, u8"[" );
  PGBAR__PROVIDE_FOR( config::Flow, option::Ending, u8"]" );
  PGBAR__PROVIDE_FOR( config::Flow, option::Filler, u8" " );
  PGBAR__PROVIDE_FOR( config::Flow, option::Divider, u8" | " );
  PGBAR__PROVIDE_FOR( config::Flow, option::InfoColor, Color::Cyan );
  template<>
  struct pgbar::config::ProvideFor<config::Flow, option::Projection> {
    static option::Projection provide()
    {
      return config::Flow::bake( option::Only<facade::FlowPlot, facade::Elapsed>() );
    }
  };
  template<>
  struct pgbar::config::ProvideFor<config::Flow, option::Lead> {
    static option::Lead provide() { return { "====" }; }
  };
  template<>
  struct pgbar::config::ProvideFor<config::Flow, option::SpeedUnit> {
    static option::SpeedUnit provide() { return option::SpeedUnit( { u8"Hz", u8"kHz", u8"MHz", u8"GHz" } ); }
  };
} // namespace pgbar

#endif
