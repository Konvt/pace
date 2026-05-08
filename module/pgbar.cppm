module;

#include "pgbar/pgbar.hpp"

export module pgbar;

export namespace pgbar {
  using pgbar::Channel;
  using pgbar::Color;
  using pgbar::Policy;
  using pgbar::Region;

  using pgbar::Indicator;
  using pgbar::iterate;

  using pgbar::BlockBar;
  using pgbar::FlowBar;
  using pgbar::ProgressBar;
  using pgbar::SpinBar;
  using pgbar::SweepBar;

  using pgbar::make_multi;
  using pgbar::MakeMulti_t;
  using pgbar::MultiBar;

  using pgbar::DynamicBar;
  using pgbar::make_dynamic;

  namespace exception {
    using pgbar::exception::Error;
    using pgbar::exception::InvalidArgument;
    using pgbar::exception::InvalidState;
    using pgbar::exception::SystemError;
  }

  namespace slice {
    using pgbar::slice::BoundedSpan;
    using pgbar::slice::IteratorSpan;
    using pgbar::slice::NumericSpan;
    using pgbar::slice::TrackedSpan;
  }

  namespace config {
    using pgbar::config::auto_style_off;
    using pgbar::config::hide_completed;
    using pgbar::config::intty;
    using pgbar::config::refresh_interval;
    using pgbar::config::terminal_width;

    using pgbar::config::provide_for;
    using pgbar::config::ProvideFor;
    using pgbar::config::ProvideFor_v;

    using pgbar::config::Block;
    using pgbar::config::Flow;
    using pgbar::config::Line;
    using pgbar::config::Spin;
    using pgbar::config::Sweep;
  } // namespace config

  namespace prefab {
    using pgbar::prefab::BasicBar;
    using pgbar::prefab::BasicConfig;
  }

  namespace facade {
    using pgbar::facade::BlockPlot;
    using pgbar::facade::CharPlot;
    using pgbar::facade::Counter;
    using pgbar::facade::Elapsed;
    using pgbar::facade::ETA;
    using pgbar::facade::FlowPlot;
    using pgbar::facade::Percentage;
    using pgbar::facade::Speed;
    using pgbar::facade::SpinPlot;
    using pgbar::facade::SweepPlot;
  } // namespace facade

  namespace option {
    // Animation
    using pgbar::option::Shift;

    // Bar
    using pgbar::option::BarWidth;
    using pgbar::option::EndColor;
    using pgbar::option::Ending;
    using pgbar::option::StartColor;
    using pgbar::option::Starting;

    // Capacity
    using pgbar::option::Quota;

    // Filler
    using pgbar::option::Filler;
    using pgbar::option::FillerColor;

    // Frame
    using pgbar::option::Lead;
    using pgbar::option::LeadColor;

    // Remain
    using pgbar::option::Remain;
    using pgbar::option::RemainColor;

    // RenderRule
    using pgbar::option::Bolded;
    using pgbar::option::Colored;

    // Reversible
    using pgbar::option::Reversed;

    // Segment
    using pgbar::option::Divider;
    using pgbar::option::InfoColor;
    using pgbar::option::LeftBorder;
    using pgbar::option::RightBorder;

    // Text
    using pgbar::option::Postfix;
    using pgbar::option::PostfixColor;
    using pgbar::option::Prefix;
    using pgbar::option::PrefixColor;

    // Speed
    using pgbar::option::Magnitude;
    using pgbar::option::SpeedUnit;

    // BasicConfig
    using pgbar::option::Except;
    using pgbar::option::Only;
    using pgbar::option::Projection;
    using pgbar::option::operator!;
    using pgbar::option::operator|;
  } // namespace option

  namespace details::types {
    using pgbar::details::types::Tempus;
  }
} // namespace pgbar
