module;

#include "pace/pace.hpp"

export module pace;

export namespace pace {
  using pace::Channel;
  using pace::Color;
  using pace::Policy;
  using pace::Region;

  using pace::Indicator;
  using pace::iterate;

  using pace::BlockBar;
  using pace::FlowBar;
  using pace::ProgressBar;
  using pace::SpinBar;
  using pace::SweepBar;

  using pace::make_multi;
  using pace::MakeMulti_t;
  using pace::MultiBar;

  using pace::DynamicBar;
  using pace::make_dynamic;

  namespace exception {
    using pace::exception::Error;
    using pace::exception::InvalidArgument;
    using pace::exception::InvalidState;
    using pace::exception::SystemError;
  }

  namespace slice {
    using pace::slice::BoundedSpan;
    using pace::slice::IteratorSpan;
    using pace::slice::NumericSpan;
    using pace::slice::TrackedSpan;
  }

  namespace config {
    using pace::config::auto_style_off;
    using pace::config::hide_completed;
    using pace::config::intty;
    using pace::config::refresh_interval;
    using pace::config::terminal_width;

    using pace::config::provide_for;
    using pace::config::ProvideFor;
    using pace::config::ProvideFor_v;

    using pace::config::Block;
    using pace::config::Flow;
    using pace::config::Line;
    using pace::config::Spin;
    using pace::config::Sweep;
  } // namespace config

  namespace prefab {
    using pace::prefab::BasicBar;
    using pace::prefab::BasicConfig;
  }

  namespace facade {
    using pace::facade::BlockPlot;
    using pace::facade::CharPlot;
    using pace::facade::Counter;
    using pace::facade::Elapsed;
    using pace::facade::ETA;
    using pace::facade::FlowPlot;
    using pace::facade::Percentage;
    using pace::facade::Speed;
    using pace::facade::SpinPlot;
    using pace::facade::SweepPlot;
  } // namespace facade

  namespace option {
    // Animation
    using pace::option::Shift;

    // Bar
    using pace::option::BarWidth;
    using pace::option::EndColor;
    using pace::option::Ending;
    using pace::option::StartColor;
    using pace::option::Starting;

    // Capacity
    using pace::option::Quota;

    // Filler
    using pace::option::Filler;
    using pace::option::FillerColor;

    // Frame
    using pace::option::Lead;
    using pace::option::LeadColor;

    // Remain
    using pace::option::Remain;
    using pace::option::RemainColor;

    // RenderRule
    using pace::option::Bolded;
    using pace::option::Colored;

    // Reversible
    using pace::option::Reversed;

    // Segment
    using pace::option::Divider;
    using pace::option::InfoColor;
    using pace::option::LeftBorder;
    using pace::option::RightBorder;

    // Text
    using pace::option::Postfix;
    using pace::option::PostfixColor;
    using pace::option::Prefix;
    using pace::option::PrefixColor;

    // Speed
    using pace::option::Magnitude;
    using pace::option::SpeedUnit;

    // BasicConfig
    using pace::option::Except;
    using pace::option::Only;
    using pace::option::Projection;
    using pace::option::operator!;
    using pace::option::operator|;
  } // namespace option

  namespace details::types {
    using pace::details::types::Tempus;
  }
} // namespace pace
