#ifndef PGBAR_BLOCK_BAR
#define PGBAR_BLOCK_BAR

#include "facade/BlockPlot.hpp"
#include "facade/Counter.hpp"
#include "facade/ETA.hpp"
#include "facade/Elapsed.hpp"
#include "facade/Percentage.hpp"
#include "facade/Speed.hpp"
#include "prefab/BasicBar.hpp"
#include "prefab/BasicConfig.hpp"
#include "slice/TrackedSpan.hpp"

namespace pgbar {
  namespace config {
    using Block = prefab::BasicConfig<facade::Percentage,
                                      facade::BlockPlot,
                                      facade::Counter,
                                      facade::Speed,
                                      facade::Elapsed,
                                      facade::ETA>;
  }

  /**
   * A progress bar with a smoother bar, requires an Unicode-supported terminal.
   *
   * It's structure is shown below:
   * {LeftBorder}{Prefix}{Percent}{Starting}{Filler}{Lead}{Remain}{Ending}{Counter}{Speed}{Elapsed}{ETA}{Postfix}{RightBorder}
   */
  template<Channel Outlet = Channel::Stderr, Policy Mode = Policy::Async, Region Area = Region::Fixed>
  using BlockBar = prefab::BasicBar<config::Block, Outlet, Mode, Area>;

  PGBAR__PROVIDE_FOR( config::Block, option::Colored, true );
  PGBAR__PROVIDE_FOR( config::Block, option::Bolded, true );
  PGBAR__PROVIDE_FOR( config::Block, option::Reversed, false );
  PGBAR__PROVIDE_FOR( config::Block, option::BarWidth, 30 );
  PGBAR__PROVIDE_FOR( config::Block, option::Magnitude, 1000 );
  PGBAR__PROVIDE_FOR( config::Block, option::Filler, u8"\u2588" );
  PGBAR__PROVIDE_FOR( config::Block, option::Remain, u8" " );
  PGBAR__PROVIDE_FOR( config::Block, option::Divider, u8" | " );
  PGBAR__PROVIDE_FOR( config::Block, option::InfoColor, Color::Cyan );
  template<>
  struct pgbar::config::ProvideFor<config::Block, option::Projection> {
    static option::Projection provide() { return config::Block::bake( option::Except<>() ); }
  };
  template<>
  struct pgbar::config::ProvideFor<config::Block, option::Lead> {
    static option::Lead provide()
    {
      // In some editing environments,
      // directly writing character literals can lead to very strange encoding conversion errors.
      // Therefore, here we use Unicode code points to directly specify the required characters.
      return option::Lead(
        { u8" ", u8"\u258F", u8"\u258E", u8"\u258D", u8"\u258C", u8"\u258B", u8"\u258A", u8"\u2589" } );
    }
  };
  template<>
  struct pgbar::config::ProvideFor<config::Block, option::SpeedUnit> {
    static option::SpeedUnit provide() { return option::SpeedUnit( { u8"Hz", u8"kHz", u8"MHz", u8"GHz" } ); }
  };

} // namespace pgbar
#endif
