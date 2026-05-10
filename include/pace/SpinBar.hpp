#ifndef PACE_SPIN_BAR
#define PACE_SPIN_BAR

#include "details/render/Builder.hpp"
#include "facade/Counter.hpp"
#include "facade/ETA.hpp"
#include "facade/Elapsed.hpp"
#include "facade/Percentage.hpp"
#include "facade/Speed.hpp"
#include "facade/SpinPlot.hpp"
#include "prefab/BasicBar.hpp"
#include "prefab/BasicConfig.hpp"
#include "slice/TrackedSpan.hpp"

namespace pace {
  namespace config {
    using Spin = prefab::BasicConfig<facade::SpinPlot,
                                     facade::Percentage,
                                     facade::Counter,
                                     facade::Speed,
                                     facade::Elapsed,
                                     facade::ETA>;
  }

  /**
   * A progress bar without bar indicator, replaced by a fixed animation component.
   *
   * It's structure is shown below:
   * {LeftBorder}{Prefix}{Lead}{Percent}{Counter}{Speed}{Elapsed}{ETA}{Postfix}{RightBorder}
   */
  template<Channel Outlet = Channel::Stderr, Policy Mode = Policy::Async, Region Area = Region::Fixed>
  using SpinBar = prefab::BasicBar<config::Spin, Outlet, Mode, Area>;

  PACE__PROVIDE_FOR( config::Spin, option::Colored, true );
  PACE__PROVIDE_FOR( config::Spin, option::FontBold, true );
  PACE__PROVIDE_FOR( config::Spin, option::Shift, -3 );
  PACE__PROVIDE_FOR( config::Spin, option::Magnitude, 1000 );
  PACE__PROVIDE_FOR( config::Spin, option::Divider, u8" | " );
  PACE__PROVIDE_FOR( config::Spin, option::InfoColor, Color::Cyan );
  template<>
  struct pace::config::ProvideFor<config::Spin, option::Projection> {
    static option::Projection provide()
    { return config::Spin::bake( option::Only<facade::SpinPlot, facade::Elapsed>() ); }
  };
  template<>
  struct pace::config::ProvideFor<config::Spin, option::Lead> {
    static option::Lead provide() { return option::Lead( { u8"/", u8"-", u8"\\", u8"|" } ); }
  };
  template<>
  struct pace::config::ProvideFor<config::Spin, option::SpeedUnit> {
    static option::SpeedUnit provide() { return option::SpeedUnit( { u8"Hz", u8"kHz", u8"MHz", u8"GHz" } ); }
  };

  namespace details {
    namespace render {
      template<>
      struct Builder<config::Spin> final : public Assembler<config::Spin> {
      private:
        using Config = config::Spin;
        using Base   = Assembler<config::Spin>;

      public:
        using Base::Base;

        io::CharPipeline& build( io::CharPipeline& pipeline, Parameter params ) const
        {
          concurrent::SharedLock<concurrent::SharedMutex> lock { this->rw_mtx_ };
          this->font_effect( pipeline, params.style_off_ );

          if ( !this->prefix_.empty() || !this->postfix_.empty() || this->projection_.any() ) {
            pipeline << this->with_dye( this->info_col_, params.style_off_ ) << this->l_border_;
          }
          // For SpinBar, we manually remove the divider between the Lead and the Percent.
          this->traits::BaseOf_t<typename Config::Layout, aspects::Prefix>::build( pipeline, params );
          this->traits::BaseOf_t<typename Config::Layout, facade::SpinPlot>::build( pipeline, params );

          this->template render_each<facade::Percentage,
                                     facade::Counter,
                                     facade::Speed,
                                     facade::Elapsed,
                                     facade::ETA>( pipeline, params );

          this->traits::BaseOf_t<typename Config::Layout, aspects::Postfix>::build( pipeline, params );
          if ( !this->prefix_.empty() || !this->postfix_.empty() || this->projection_.any() ) {
            pipeline << this->clear_then_dye( this->info_col_, params.style_off_ ) << this->r_border_;
          }

          return this->reset_style( pipeline, params.style_off_ );
        }
      };
    } // namespace render
  } // namespace details
} // namespace pace

#endif
