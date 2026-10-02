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
    using pace::config::Provider;
    using pace::config::provider_v;

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

    // Percentage
    using pace::option::PercentDecs;

    // Speed
    using pace::option::Magnitude;
    using pace::option::SpeedUnit;

    // BasicConfig
    using pace::option::Except;
    using pace::option::Only;
    using pace::option::Projection;
    using pace::option::operator!;
  } // namespace option

  /// NOTE: The following entities are intended solely for providing additional specialized implementations
  ///       for the rendering engine `Builder`; if new facades need to be added,
  ///       they should be based on the header and reference the corresponding required components.
  namespace details {
    namespace traits {
      using pace::details::traits::BaseOf_t;
    }

    namespace render {
      using pace::details::render::Assembler;
      using pace::details::render::Builder;
      using pace::details::render::Parameter;
    }

    namespace console {
      using pace::details::console::Backcolor;
      using pace::details::console::Dualcolor;
      using pace::details::console::Forecolor;

      using pace::details::console::resetbgcolor;
      using pace::details::console::resetcolor;
      using pace::details::console::resetfgcolor;
      using pace::details::console::resetstyle;
    } // namespace console

    namespace io {
      using pace::details::io::CharPipeline;

      using pace::details::io::align;
      using pace::details::io::choice;
      using pace::details::io::concat;
      using pace::details::io::format;
      using pace::details::io::join;
      using pace::details::io::nop;
      using pace::details::io::repeat;
      using pace::details::io::unless;
      using pace::details::io::until;
      using pace::details::io::when;
    } // namespace io

    namespace aspects {
      using pace::details::aspects::Animation;
      using pace::details::aspects::Bar;
      using pace::details::aspects::Capacity;
      using pace::details::aspects::Filler;
      using pace::details::aspects::Frame;
      using pace::details::aspects::Postfix;
      using pace::details::aspects::Prefix;
      using pace::details::aspects::Remain;
      using pace::details::aspects::RenderRule;
      using pace::details::aspects::Reversible;
      using pace::details::aspects::Segment;
      // no Schema
    } // namespace aspects
  } // namespace details

  // Explicitly instantiate commonly used types to avoid redundant type dependency calculations later.
  template class prefab::BasicBar<config::Line, Channel::Out, Policy::Async, Region::Fixed>;
  template class prefab::BasicBar<config::Line, Channel::Out, Policy::Sync, Region::Relative>;
  template class prefab::BasicBar<config::Block, Channel::Out, Policy::Async, Region::Fixed>;
  template class prefab::BasicBar<config::Block, Channel::Out, Policy::Sync, Region::Relative>;
  template class prefab::BasicBar<config::Flow, Channel::Out, Policy::Async, Region::Fixed>;
  template class prefab::BasicBar<config::Flow, Channel::Out, Policy::Sync, Region::Relative>;
  template class prefab::BasicBar<config::Sweep, Channel::Out, Policy::Async, Region::Fixed>;
  template class prefab::BasicBar<config::Sweep, Channel::Out, Policy::Sync, Region::Relative>;
  template class prefab::BasicBar<config::Spin, Channel::Out, Policy::Async, Region::Fixed>;
  template class prefab::BasicBar<config::Spin, Channel::Out, Policy::Sync, Region::Relative>;

  template class prefab::BasicBar<config::Line, Channel::Err, Policy::Async, Region::Fixed>;
  template class prefab::BasicBar<config::Line, Channel::Err, Policy::Sync, Region::Relative>;
  template class prefab::BasicBar<config::Block, Channel::Err, Policy::Async, Region::Fixed>;
  template class prefab::BasicBar<config::Block, Channel::Err, Policy::Sync, Region::Relative>;
  template class prefab::BasicBar<config::Flow, Channel::Err, Policy::Async, Region::Fixed>;
  template class prefab::BasicBar<config::Flow, Channel::Err, Policy::Sync, Region::Relative>;
  template class prefab::BasicBar<config::Sweep, Channel::Err, Policy::Async, Region::Fixed>;
  template class prefab::BasicBar<config::Sweep, Channel::Err, Policy::Sync, Region::Relative>;
  template class prefab::BasicBar<config::Spin, Channel::Err, Policy::Async, Region::Fixed>;
  template class prefab::BasicBar<config::Spin, Channel::Err, Policy::Sync, Region::Relative>;
} // namespace pace
