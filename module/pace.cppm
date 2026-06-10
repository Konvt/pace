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
  using pace::MultiBar;
  using pace::MultiBar_t;

  using pace::DynamicBar;
  using pace::make_dynamic;

  namespace exception {
    using pace::exception::Error;
    using pace::exception::InvalidArgument;
    using pace::exception::InvalidState;
    using pace::exception::SystemError;
  }

  namespace slice {
    using pace::slice::IteratorSpan;
    using pace::slice::NumericSpan;
    using pace::slice::SizedSpan;
    using pace::slice::TrackedSpan;
  }

  namespace config {
    using pace::config::auto_style_off;
    using pace::config::hide_completed;
    using pace::config::intty;
    using pace::config::refresh_interval;
    using pace::config::terminal_width;

    using pace::config::provide_for;
    using pace::config::provide_for_v;
    using pace::config::ProvideFor;

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
    using pace::option::EndBackcolor;
    using pace::option::EndForecolor;
    using pace::option::Ending;
    using pace::option::StartBackcolor;
    using pace::option::StartForecolor;
    using pace::option::Starting;

    // Capacity
    using pace::option::Quota;

    // Filler
    using pace::option::Filler;
    using pace::option::FillerBackcolor;
    using pace::option::FillerForecolor;

    // Frame
    using pace::option::Lead;
    using pace::option::LeadBackcolor;
    using pace::option::LeadForecolor;

    // Remain
    using pace::option::Remain;
    using pace::option::RemainBackcolor;
    using pace::option::RemainForecolor;

    // RenderRule
    using pace::option::Colored;
    using pace::option::FontBold;
    using pace::option::FontCrossed;
    using pace::option::FontFaint;
    using pace::option::FontHidden;
    using pace::option::FontInverse;
    using pace::option::FontItalic;
    using pace::option::FontUnderline;

    // Reversible
    using pace::option::Reversed;

    // Segment
    using pace::option::Divider;
    using pace::option::InfoBackcolor;
    using pace::option::InfoForecolor;
    using pace::option::LeftBorder;
    using pace::option::RightBorder;

    // Text
    using pace::option::Postfix;
    using pace::option::PostfixBackcolor;
    using pace::option::PostfixForecolor;
    using pace::option::Prefix;
    using pace::option::PrefixBackcolor;
    using pace::option::PrefixForecolor;

    // Elapsed
    using pace::option::ElapsedFormat;

    // ETA
    using pace::option::ETAFormat;

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
