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
  template<Channel Sink = Channel::Stderr, Policy Mode = Policy::Async, Region Zone = Region::Fixed>
  using SpinBar = prefab::BasicBar<config::Spin, Sink, Mode, Zone>;

  PACE__PROVIDE_FOR( config::Spin, option::Colored, true );
  PACE__PROVIDE_FOR( config::Spin, option::FontBold, true );
  PACE__PROVIDE_FOR( config::Spin, option::ShowQuota, true );
  PACE__PROVIDE_FOR( config::Spin, option::Shift, -3 );
  PACE__PROVIDE_FOR( config::Spin, option::Magnitude, 1000 );
  PACE__PROVIDE_FOR( config::Spin, option::Divider, " | " );
  PACE__PROVIDE_FOR( config::Spin, option::InfoForecolor, Color::Cyan );
  template<>
  struct config::ProvideFor<config::Spin, option::ElapsedFormat> {
    static option::ElapsedFormat provide()
    {
      static const auto cached_option = option::ElapsedFormat( "+%H:%M:%S" );
      return cached_option;
    }
  };
  template<>
  struct config::ProvideFor<config::Spin, option::ETAFormat> {
    static option::ETAFormat provide()
    {
      static const auto cached_option = option::ETAFormat( "-%H:%M:%S" );
      return cached_option;
    }
  };
  template<>
  struct config::ProvideFor<config::Spin, option::Projection> {
    static option::Projection provide()
    { return config::Spin::bake( option::Only<facade::SpinPlot, facade::Elapsed>() ); }
  };
  template<>
  struct config::ProvideFor<config::Spin, option::Lead> {
    static option::Lead provide() { return option::Lead( { "/", "-", "\\", "|" } ); }
  };
  template<>
  struct config::ProvideFor<config::Spin, option::SpeedUnit> {
    static option::SpeedUnit provide() { return option::SpeedUnit( { "Hz", "kHz", "MHz", "GHz" } ); }
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
          this->font_effect( pipeline, params.style_off );

          const auto brush = details::io::when( !params.style_off && this->colorful() );
          if ( !this->prefix_.empty() || !this->postfix_.empty() || this->projection_.any() )
            pipeline << brush( console::Dualcolor { this->info_forecolor_, this->info_backcolor_ } )
                     << this->l_border_;
          // For SpinBar, we manually remove the divider between the Lead and the Percent.
          this->traits::BaseOf_t<typename Config::layout_type, aspects::Prefix>::build( pipeline, params );
          if ( this->projection_.test( 0 ) ) {
            // zero is facade::SpinPlot
            this->traits::BaseOf_t<typename Config::layout_type, facade::SpinPlot>::build( pipeline, params );
            pipeline << ' ';
          }

          this->template render_each<facade::Percentage,
                                     facade::Counter,
                                     facade::Speed,
                                     facade::Elapsed,
                                     facade::ETA>( pipeline, params );

          this->traits::BaseOf_t<typename Config::layout_type, aspects::Postfix>::build( pipeline, params );
          if ( !this->prefix_.empty() || !this->postfix_.empty() || this->projection_.any() )
            pipeline << brush( console::resetcolor,
                               console::Dualcolor { this->info_forecolor_, this->info_backcolor_ } )
                     << this->r_border_;

          return pipeline << io::when( !params.style_off && this->rich(), console::resetstyle );
        }

        // SpinBar does not contain a progress bar.
        std::uint64_t fixed_width() const noexcept final
        { return Base::fixed_width() + this->projection_.test( 0 ); }
      };
    } // namespace render
  } // namespace details
} // namespace pace

#endif
