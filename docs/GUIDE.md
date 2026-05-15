**Contents**
- [Shared Spec](#shared-spec)
  - [Quick Start](#quick-start)
  - [Configuration](#configuration)
    - [Data modification](#data-modification)
    - [Component switch](#component-switch)
  - [Component Subfeatures](#component-subfeatures)
    - [Speed](#speed)
    - [Elapsed and ETA](#elapsed-and-eta)
  - [Iterating Over Ranges](#iterating-over-ranges)
  - [The Template Argument](#the-template-argument)
    - [Output stream](#output-stream)
    - [Rendering policy](#rendering-policy)
    - [Rendering region](#rendering-region)
  - [Callback support](#callback-support)
  - [Hide The Bar](#hide-the-bar)
- [The Differences Of Components](#the-differences-of-components)
  - [ProgressBar](#progressbar)
    - [Element composition](#element-composition)
    - [Variable bar width](#variable-bar-width)
  - [BlockBar](#blockbar)
    - [Element composition](#element-composition-1)
    - [Variable bar width](#variable-bar-width-1)
  - [SpinBar](#spinbar)
    - [Element composition](#element-composition-2)
  - [SweepBar](#sweepbar)
    - [Element composition](#element-composition-3)
    - [Variable bar width](#variable-bar-width-2)
  - [FlowBar](#flowbar)
    - [Element composition](#element-composition-4)
    - [Variable bar width](#variable-bar-width-3)
- [Independent Components](#independent-components)
  - [MultiBar](#multibar)
    - [Quick start](#quick-start-1)
    - [Factory functions](#factory-functions)
    - [Rendering behavior](#rendering-behavior)
    - [Tuple protocol](#tuple-protocol)
  - [DynamicBar](#dynamicbar)
    - [Quick start](#quick-start-2)
    - [Helper functions](#helper-functions)
    - [Rendering behavior](#rendering-behavior-1)
  - [NumericSpan](#numericspan)
    - [Member methods](#member-methods)
    - [Iterator type](#iterator-type)
  - [IteratorSpan](#iteratorspan)
    - [Member methods](#member-methods-1)
    - [Iterator type](#iterator-type-1)
  - [SizedSpan](#sizedspan)
    - [Member functions](#member-functions)
    - [Iterator type](#iterator-type-2)
  - [TrackedSpan](#trackedspan)
    - [Member functions](#member-functions-1)
    - [Iterator type](#iterator-type-3)
  - [iterate](#iterate)
- [Composition Model](#composition-model)
  - [Modular Components](#modular-components)
  - [Composing New Configuration Types](#composing-new-configuration-types)
  - [Custom Components](#custom-components)
    - [Custom facade](#custom-facade)
    - [Custom rendering](#custom-rendering)
- [Design Notes](#design-notes)
  - [Assertion Checks](#assertion-checks)
  - [Consistency Between Update Count and Total Task Count](#consistency-between-update-count-and-total-task-count)
  - [Lifecycle of Bar Objects](#lifecycle-of-bar-objects)
  - [Unicode Support](#unicode-support)
  - [Renderer Design](#renderer-design)
  - [Exception Propagation Mechanism](#exception-propagation-mechanism)
  - [Compilation Time Issues](#compilation-time-issues)
  - [Internal Design](#internal-design)
    - [Base data structure design](#base-data-structure-design)
    - [Progress bar type design](#progress-bar-type-design)

# Shared Spec
## Quick Start
Due to design considerations, *most progress bar type interfaces in pace are similar*, so this section uses `pace::ProgressBar` as an example to demonstrate how to use the various progress bar types in pace.

Progress bars are designed around executing a task with a fixed quota, so nearly all progress bar types require a task quota parameter, followed by iterative calls to the `tick()` method.

For `pace::ProgressBar` and `pace::BlockBar`, calling `tick()` without providing an initial quota parameter will throw the `pace::exception::InvalidState` exception.

All progress bar types in pace are template types, but all template parameters provide default values, so an empty parameter object can be instantiated directly.

```cxx
{
  pace::ProgressBar<> bar;
  // Since C++20, this can also be written as:
  // pace::ProgressBar bar;
  try {
    bar.tick();
  } catch ( const pace::exception::InvalidState& e ) {
    std::cerr << e.what() << std::endl;
  }
}
{
  pace::ProgressBar<> bar;
  // pace adopts a design that separates data from behavior,
  // so injecting task parameters requires calling config()
  // to access the internal configuration data object
  bar.config().quota( 200 );

  bar.tick( 20 );    // Advance by 20 steps
  bar.tick_to( 50 ); // Set progress to 50%

  for ( int i = 0; i < 100; ++i )
    bar.tick(); // Each call advances by only 1 step
}
{
  // Besides calling config(), task parameters can also be injected
  // by passing a wrapper type with the expected parameters during construction
  pace::ProgressBar<> bar { pace::option::Quota( 150 ) }; // In general, wrapper types share the same name as their corresponding methods
  // Note: passing duplicated parameters will result in a compilation error
  bar.tick_to( 20 );  // Set progress to 20%
  bar.tick_to( 130 ); // Any value beyond 100% will be discarded, and the progress bar will be locked at 100%
}
```

To inspect the runtime state of a progress bar or forcibly terminate it, use the `active()` and `reset()` methods.

```cxx
pace::ProgressBar<> bar { pace::option::Quota( 500 ) };

for ( int i = 0; i < 400; ++i ) {
  if ( i > 0 ) // Note that the progress bar only becomes active after tick() has been called once
    assert( bar.active() );
  bar.tick();
}

assert( bar.progress() == 400 ); // This method retrieves the current iteration count of the progress bar
bar.reset();
assert( bar.active() == false );
```

All progress bar types satisfy the move-only and swappable requirements, so objects can be move-constructed from another object or swapped with another object to exchange their configuration data.

```cxx
{
  pace::ProgressBar<> bar1 { /* Pass some complex configuration data */ };
  pace::ProgressBar<> bar2 { std::move( bar1 ) };
}
{
  pace::ProgressBar<> bar1 { /* Pass some complex configuration data */ };
  pace::ProgressBar<> bar2;
  bar2.swap( bar1 );
  // or
  using std::swap;
  swap( bar1, bar2 );
}
```

However, pace does not consider swapping or moving objects while a progress bar is **actively running** to be a valid operation, because doing so violates ownership semantics and may lead to unpredictable behavior.

```cxx
pace::ProgressBar<> bar1 { pace::option::Quota( 500 ) };

bar1.tick();
assert( bar1.active() );

// pace::ProgressBar<> bar2 { std::move( bar1 ) }; No!
// When the PACE_DEBUG macro is defined,
// this operation will trigger an assertion failure
```

Progress bar objects in pace strictly follow C++ object lifetime semantics. Therefore, when a progress bar object is destroyed, all rendering operations are forcibly stopped and all resources are properly released.

## Configuration
### Data modification
As mentioned in the previous section, nearly all configuration operations on progress bar objects must be performed through the `config()` method.

This method returns a reference to the internal configuration object, whose type can be found under `pace::config`.

Using `pace::ProgressBar` as an example, its corresponding configuration type is `pace::config::Line`.

`pace::config::Line` is a pure data type that stores all data members used to describe all the elements of the `pace::ProgressBar`. It satisfies the properties of being copyable, movable, and swappable.

```cxx
pace::config::Line cfg1;

auto cfg2 = cfg1;              // copy
auto cfg3 = std::move( cfg1 ); // move
cfg3.swap( cfg2 );             // swap
// or
using std::swap;
swap( cfg2, cfg3 );
```

The configuration type can be modified, moved, or swapped while the associated progress bar type is running. This also means that concurrent modifications to the configuration type are thread-safe.

```cxx
pace::ProgressBar<> bar { pace::option::Quota( 150 ) };

bar.tick_to( 20 );
bar.tick_to( 130 );

bar.config().swap( pace::config::Line() ); // ok
```

Note that this kind of dynamic configuration change does **not affect the number of tasks that have already been injected**. After the configuration object changes, the runtime progress bar will still continue iterating using the task count from before the modification.

```cxx
pace::ProgressBar<> pbar;

pbar.config().quota( 100 );
for ( auto i = 0; i < 100; ++i ) {
  pbar.tick();
  if ( i == 30 ) // nothing happens
    pbar.config().quota( 50 );
  std::this_thread::sleep_for( std::chrono::milliseconds( 20 ) );
}
```

All configuration types in pace support two forms of data injection: variadic construction based on wrapper types, and chain-style interfaces.

```cxx
pace::config::Line config1 {
  pace::option::Quota( 100 ),
  pace::option::SpeedUnit( { "B/s", "kiB/s", "MiB/s", "GiB/s" } ),
  pace::option::Magnitude( 1024 ),
  pace::option::InfoForecolor( "#39C5BB" )
  // pace::option::InfoForecolor(0x39C5BB) Don't do that!
};
// Note: passing multiple identical wrapper types will cause a compilation error

pace::config::Line config2;
config2.quota( 100 )
  .speed_unit( { "B/s", "kiB/s", "MiB/s", "GiB/s" } )
  .magnitude( 1024 )
  .info_color( "#39C5BB" );

auto config3 = config2; // variadic template parameters can still be used after construction
config3.with( pace::option::Prefix( "Do something" ), pace::option::PrefixForecolor( 0xFFE211 ) );

// The configuration type overloads operator| and operator|=,
// allowing parameters to be passed in a pipeline-like manner
auto config4 = pace::config::Line() | pace::option::Quota( 114514 ) | pace::option::Magnitude( 1024 );
```
### Component switch
It is not difficult to observe that a progress bar type is composed of multiple components, and these components collectively determine the final appearance of a progress bar.

Regardless of the progress bar type, component toggling can be implemented through `pace::option::Only` or `pace::option::Except`.

The meaning of `pace::option::Only` is: retain only the selected components; the meaning of `pace::option::Except` is: disable only the selected components.

In practice, the components of a progress bar type originate from several template facade base classes in `pace::facade`. These template base classes are combined at compile time according to a [compile-time algorithm](#progress-bar-type-design)
, generating the target configuration type, which is then used to derive the target progress bar type.

> For more details, refer to [this section](#composition-model).

In summary, component toggling for a progress bar can be implemented as follows:

```cxx
pace::config::Line pgbar;

// Enable all components, then disable Percentage and Speed individually
pgbar.enable_all().disable<pace::facade::Percentage, pace::facade::Speed>();
// Equivalent to the following
pgbar.with( pace::option::Except<pace::facade::Percentage, pace::facade::Speed>() );

// Disable all components, then enable CharPlot, Elapsed, and ETA individually
pgbar.disable_all().enable<pace::facade::CharPlot, pace::facade::Elapsed, pace::facade::ETA>();
// Likewise
pgbar.with( pace::option::Only<pace::facade::CharPlot, pace::facade::Elapsed, pace::facade::ETA>() );

// Additional components can also be enabled individually,
// but this operation cannot be expressed with Only or Except
pgbar.enable<pace::facade::Speed>();

// Multiple Only selections can be concatenated together; the same applies to Except
auto selection = pace::option::Only<pace::facade::CharPlot>() | pace::option::Only<pace::facade::Elapsed>();

// However, a progress bar type will reject an Only or Except
// containing components that do not belong to it
// pgbar.with( selection | pace::option::Only<pace::facade::BlockPlot>() ); No!
```

## Component Subfeatures
### Speed
All progress bar configuration types that enable `pace::facade::Speed` may additionally configure speed-related data.

A `pace::facade::Speed` consists of two parts: the radix step size `pace::option::Magnitude` and the unit set `pace::option::SpeedUnit`.

pace does not require the number of units in `pace::option::SpeedUnit` to strictly match the cumulative radix limit, which allows highly flexible rate display behavior.

For example, `pace::option::Magnitude( 1024 )` + `pace::option::SpeedUnit( {"B/s", "kiB/s", "MiB/s", "GiB/s"} )` produces a rate display using a radix of 1024.

Meanwhile, `pace::option::Magnitude( 1000 )` + `pace::option::SpeedUnit( {"B/s", "kB/s", "MB/s", "GB/s"} )` produces a rate display using a radix of 1000.

An insufficient number of units does not cause an error. Instead, the display remains fixed at the final unit. If `pace::option::Magnitude` is less than or equal to 1, the output is always `nan.` rather than a valid rate.

The term “rate” refers to the amount of task progress increment per unit time. The displayed unit is automatically adjusted according to the current rate. When the rate reaches or exceeds the threshold corresponding to the current unit, the display switches to the next unit.

A special case should be noted when `pace::option::Magnitude` is set to a very large value, such as a value close to `UINT16_MAX`. Since the fourth power of `UINT16_MAX` is slightly smaller than the currently representable upper limit `UINT64_MAX`, under such conditions the effective number of usable units in `pace::option::SpeedUnit` is limited to 5.

Furthermore, when a large radix value causes multiplication overflow during cumulative scaling, the display becomes fixed at the last reachable unit and outputs `inf`.
### Elapsed and ETA
Progress bars supporting `pace::facade::Elapsed` or `pace::facade::ETA` may define a clock format using a format string. Both features share the same format string parsing logic.

The syntax of the format string is:

```txt
%[ ':' <fill-char> ][ <width> ]<unit>
```

For example, consider the following format string:

```txt
`%: 3H:%2M:%S`
 ^~~~^ ^~^ ^^ --- Time units
  ^~^   ^     --- Fill character and width constraint
      ^   ^   --- Literals

 %: 3H - `3` specifies a width of 3. Decimal values shorter than this width are padded using the fill character; values exceeding the width are replaced with a fixed `###`.
         `:` indicates that the next Unicode character, and only the next Unicode character, is interpreted as the fill character. In this example, the character is ` `.
         `:` is optional and does not support Unicode combining characters.
```

Any part outside formatting directives is interpreted literally and emitted unchanged. The string encoding must be UTF-8.

If the width is omitted, the default width is 2. If the fill character is omitted, the default fill character is `0`.

The display results are fixed to be right-aligned. Other alignment methods are not supported for the time being.

If the decimal representation of a time value exceeds the specified width limit, the value is replaced by a sequence of `#` characters whose length equals the configured width.

Specifying a width of zero, or passing an empty format string, causes the output to become a fixed `?` character.

Only the largest unit does not carry over. All smaller units are reduced modulo 60 as usual:

txt
%H:%M:%S -> 01:01:01
%M:%S    -> 61:01
%:4S     -> 3661

If the provided format string does not conform to the grammar above, or contains duplicate time units, or has no time units at all, an exception of type `pace::exception::InvalidArgument` is thrown.

## Iterating Over Ranges
When processing iteration tasks involving iterable types or numeric ranges, pace can integrate progress bars into these scenarios through the `iterate` method.

The usage of `iterate` is similar to Python's `range` function. It supports iteration over ranges specified by numeric values, while the task count of the progress bar object is configured automatically by `iterate`.

```cxx
pace::ProgressBar<> bar;

// Iteration range: [100, 0), step: -1
for ( auto num : bar.iterate( 100, 0, -1 ) ) {
  std::this_thread::sleep_for( 100ms );
}
// Iteration range: [0.0, -2.0), step: -0.01
for ( auto fnum : bar.iterate( -2.0, -0.01 ) ) {
  std::this_thread::sleep_for( 100ms );
}
// Iteration range: [0, 100), step: 1
bar.iterate( 100, []( int ) { std::this_thread::sleep_for( 100ms ); } );
```

In addition, progress bar objects can interact with types satisfying the `std::ranges::sized_range` concept (without requiring C++20), such as `std::vector` and raw arrays.

```cxx
pace::ProgressBar<> bar;

std::vector<int> arr1 {
  0, 1, 2, 3, 4, 5, 6,
};
int arr2[] { 100, 99, 98, 97, 96, 95, 94, 93, 92, 91 };

for ( auto& ele : bar.iterate( arr1.begin(), arr1.end() ) ) {
  ele += 1; // ele is a reference to the element inside the vector
  std::this_thread::sleep_for( 300ms );
}
// Reverse iteration
bar.iterate( arr2, []( int& ) { std::this_thread::sleep_for( 300ms ); } );
```

If the `std::ranges::sized_range` constraint is satisfied and the C++20 standard is used, `iterate` can correctly handle the reference lifetime of view types satisfying the `std::ranges::view` concept.

## The Template Argument
All progress bar types in pace are template types. They require three template parameters: `pace::Channel`, `pace::Policy`, and `pace::Region`.
### Output stream
`pace::Channel` specifies the output target of the progress bar. Currently, pace only allows choosing between `pace::Channel::Stderr` and `pace::Channel::Stdout`, and defaults to `Stderr`.

```cxx
static_assert( std::is_same<pace::ProgressBar<>,
                            pace::ProgressBar<pace::Channel::Stderr>>::value,
                "" );

pace::ProgressBar<pace::Channel::Stdout> bar; // Bind to stdout
```

At runtime, pace dynamically checks whether the specified output stream is actually attached to a terminal device. If the output stream does not point to a terminal, pace disables character rendering effects and terminal cursor manipulation.

More specifically, when pace detects that the specified output stream is not attached to a terminal (this can be checked using `pace::config::intty( pace::Channel )`), it first disables all terminal cursor manipulation sequences during rendering, and then checks the return value of `pace::config::auto_style_off()`.

If `pace::config::auto_style_off()` returns `true`, pace disables all character rendering effects, and the resulting progress bar becomes plain text output. Otherwise, the progress bar still preserves terminal-oriented rendering sequences.

By default, `pace::config::auto_style_off()` always returns `true`, but its return value can be modified through `pace::config::auto_style_off( bool )`.

In particular, if the macro `PACE_INTTY` is defined before including the pace library, pace will forcibly treat all output streams as terminal-attached. Likewise, if the macro `PACE_NOSTYLE` is defined, pace will forcibly disable all character rendering effects.

It should be noted that, among all progress bar objects bound to the same output stream, at most one is allowed to run at the same time. Otherwise, the exception `pace::exception::InvalidState` will be thrown.

```cxx
{
  pace::ProgressBar<> bar1;
  pace::SweepBar<> bar2;
  pace::SpinBar<pace::Channel::Stdout> bar3;

  bar1.config().quota( 100 );
  bar1.tick();

  try {
    bar2.tick(); // Oops!
  } catch ( const pace::exception::InvalidState& e ) {
    std::cerr << std::endl << e.what() << std::endl;
  }

  bar3.tick(); // Ok!
}

pace::ProgressBar<> bar;
bar.config().quota( 100 );

bar.tick(); // Ok!
```

If multiple progress bars need to be rendered on the same output stream, use `pace::MultiBar` or `pace::DynamicBar`.
### Rendering policy
`pace::Policy` determines the rendering strategy of the progress bar. Currently, pace provides three policy types: `pace::Policy::Async`, `pace::Policy::Signal`, and `pace::Policy::Sync`. Different rendering policies determine which thread is responsible for performing rendering operations.

Under `pace::Policy::Async`, all rendering operations are executed by the renderer's background thread. Rendering is completely detached from the foreground thread, and after each rendering operation the background thread sleeps for a certain interval. This interval can be queried using `pace::config::refresh_interval()` and modified using `pace::config::refresh_interval( pace::details::types::Tempus )`.

Under `pace::Policy::Signal`, every call to `tick()` or `tick_to()` submits a rendering request to the renderer from the calling thread. Each rendering task is still executed by the background thread, but the thread does not sleep after processing a task.

If multiple tasks are submitted to the renderer within a short period of time, the renderer will consume these rendering requests as quickly as possible. However, due to the asynchronous execution model, the actual execution time of each rendering operation may be slightly later than the corresponding `tick()` or `tick_to()` call.

If the progress bar triggers `reset()` or `abort()` before all queued tasks are consumed, the remaining rendering tasks will be discarded, although this does not affect the actual rendering result. In other words, `pace::Policy::Signal` only guarantees that the number of rendering operations does not exceed the number of `tick()` or `tick_to()` calls.

Under pace::Policy::Sync, rendering is executed directly by the thread calling `tick()` or `tick_to()`. That is, every `tick()` call not only updates the progress state, but also immediately renders the latest progress bar to the terminal. In this mode, every rendering operation strictly corresponds to a t`ick()` or `tick_to()` call.

```cxx
static_assert( std::is_same<pace::ProgressBar<>,
                            pace::ProgressBar<pace::Channel::Stderr, pace::Policy::Async>>::value,
                "" );

pace::ProgressBar<pace::Channel::Stderr, pace::Policy::Sync> bar; // Use synchronous rendering
```
### Rendering region
`pace::Region` determines the rendering position of the progress bar in the terminal. Currently, pace only allows choosing between `pace::Region::Fixed` and `pace::Region::Relative`, and defaults to `Fixed`.

`pace::Region::Fixed` saves the current terminal cursor position during the first rendering and always refreshes the progress bar relative to that position. In this mode, any other content written to the same output stream will be overwritten by subsequent progress bar refreshes.

`pace::Region::Relative` rewinds according to the number of lines produced during the previous rendering and overwrites the old progress bar content. In this mode, if additional information is written to the same output stream and followed by a suitable number of newline characters, the extra information can be preserved.

However, if the progress bar string becomes excessively wide, using `pace::Region::Relative` may lead to terminal rendering issues.

> The rendering structure of any progress bar occupies two terminal lines in total (assuming the progress bar length does not exceed the single-line character limit): one line for the progress bar itself and one empty line.

When using `pace::Region::Relative` together with custom output, `pace::Policy::Signal` or `pace::Policy::Sync` is generally required. Otherwise, the asynchronous rendering mechanism may cause terminal scrolling.

```cxx
pace::ProgressBar</* any channel */, pace::Policy::Signal, pace::Region::Relative> bar;
bar.config().quota( 100 );

for ( size_t i = 0; i < 95; ++i )
  bar.tick(); /* do something... */

// Notice: At least two newlines must be inserted after the output information
std::cerr << "Extra log information\n\n" << std::flush;

while ( bar.active() )
  bar.tick();
```

## Callback support
All progress bar types in pace provide an `action` method that can register or clear a callback function of type `void()` or `void( /* Bar Type */& )`.

This callback is invoked when the progress bar object calls its `reset()` method, **before** progress bar rendering is terminated (that is, before the return value of `active()` changes from `true` to `false`).

Likewise, if the progress bar runs and terminates normally, the progress bar type will internally call `reset()` on its own, and the callback will still be invoked before termination.

All progress bar objects that support `action` also provide `operator|` and `operator|=` overloads, allowing callbacks to be passed directly through these operators.

```cxx
pace::ProgressBar<> bar;
bool flag = true;
auto callback = [&]( pace::ProgressBar<>& self ) {
  if ( flag )
    self.config().prefix( "✔ Mission Accomplished" ).prefix_color( pace::Color::Green );
  else
    self.config().prefix( "❌ Mission failed" ).prefix_color( pace::Color::Red );
};

bar.action( callback );
// or
bar |= callback;
// or
bar | callback;
```

The callback type passed in must satisfy `std::is_move_constructible`; moreover, methods that *modify the internal state of the progress bar object itself* (such as `tick` or `reset`, but excluding `config`) should **never** be called inside the callback, otherwise a *deadlock* may occur.

If you want to terminate the object manually while skipping callback execution, call the object's `abort` method instead; additionally, termination caused by object destruction will also bypass the preset callback function.

## Hide The Bar
pace supports automatically hiding completed progress bar strings. When `pace::config::hide_completed()` returns `true`, a completed progress bar will be hidden immediately as soon as its `active()` method changes from `true` to `false`, provided that the specified output stream is attached to a terminal.

The actual behavior can be modified by calling `pace::config::hide_completed( bool )`. The default value is `false`.

# The Differences Of Components
## ProgressBar
In runtime behavior, when `config().quota()` is 0 and the `tick` method is invoked, `pace::ProgressBar` throws a `pace::exception::InvalidState` exception.
### Element composition
`pace::ProgressBar` consists of the following elements:

```text
{LeftBorder}{Prefix}{Percent}{Starting}{Filler}{Lead}{Remain}{Ending}{Counter}{Speed}{Elapsed}{ETA}{Postfix}{RightBorder}
```

The customizable parts are: `LeftBorder`, `Prefix`, `Starting`, `Filler`, `Lead`, `Remain`, `Ending`, `Speed`, `Postfix`, and `RightBorder`. Their functionality matches their names.

These elements can be directly found as corresponding wrapper types in `pace::option`:

```cxx
pace::option::Colored;       // Color effect
pace::option::FontBold;      // Bold font
pace::option::FontFaint;     // Faint font
pace::option::FontItalic;    // Italic effect
pace::option::FontUnderline; // Underlined font
pace::option::FontInverse;   // Inverse font display
pace::option::FontHidden;    // Hidden font
pace::option::FontCrossed;   // Strikethrough

pace::option::LeftBorder;  // Modify the left starting border of the entire progress bar
pace::option::RightBorder; // Modify the right ending border of the entire progress bar

pace::option::Prefix;      // Modify the prefix description
pace::option::Postfix;     // Modify the postfix description

pace::option::Starting;  // Modify the element between Percent and the left side of the bar block
pace::option::Ending;    // Modify the element between the right side of the bar block and Counter
pace::option::Filler;    // Modify the fill character of the completed section
pace::option::Lead;      // Modify each frame of the animated section
pace::option::Remain;    // Modify the fill character of the remaining section
pace::option::Reversed;  // Adjust the growth direction of the progress bar (`false` means left-to-right)
pace::option::Shift;     // Adjust the animation speed of the animated section (Lead)
pace::option::BarWidth;  // Adjust the width of the progress bar

pace::option::SpeedUnit; // Modify the unit used in the Speed section
pace::option::Magnitude; // Adjust the carry ratio used in the Speed section

pace::option::Quota;   // Adjust the task count
pace::option::Divider; // Modify the separator between two elements

pace::option::PrefixForecolor;  // Modify the foreground color of Prefix
pace::option::PrefixBackcolor;  // Modify the background color of Prefix
pace::option::PostfixForecolor; // Modify the foreground color of Postfix
pace::option::PostfixBackcolor; // Modify the background color of Postfix
pace::option::StartForecolor;   // Modify the foreground color of Starting
pace::option::StartBackcolor;   // Modify the background color of Starting
pace::option::EndForecolor;     // Modify the foreground color of Ending
pace::option::EndBackcolor;     // Modify the background color of Ending
pace::option::FillerForecolor;  // Modify the foreground color of Filler
pace::option::FillerBackcolor;  // Modify the background color of Filler
pace::option::RemainForecolor;  // Modify the foreground color of Remain
pace::option::RemainBackcolor;  // Modify the background color of Remain
pace::option::LeadForecolor;    // Modify the foreground color of Lead
pace::option::LeadBackcolor;    // Modify the background color of Lead
pace::option::InfoForecolor;    // Modify the foreground color of Divider, Percent, Counter, Speed, Elapsed, and ETA
pace::option::InfoBackcolor;    // Modify the background color of Divider, Percent, Counter, Speed, Elapsed, and ETA
```

The configuration type also provides methods with the same names but different naming styles.
### Variable bar width
The section between `Starting` and `Ending` is the progress indicator called `CharPlot` (excluding `Starting` and `Ending`). The width of this progress indicator is variable.

Each progress bar has a default initial width of 30 characters. If the progress bar needs to fill an entire terminal line, or if it is too long and needs to be narrowed, the width of the progress indicator can be adjusted using the `bar_width()` method or the `pace::option::BarWidth` wrapper.

If the progress bar should exactly occupy an entire terminal line, the progress bar type provided by pace exposes the width of all parts except the progress indicator through the `config().fixed_width()` method.

> Do not directly call the `fixed_width()` method of the configuration object. Refer to the code comments ([BasicConfig.hpp](../include/pace/prefab/BasicConfig.hpp)) for the specific reason.

```cxx
pace::ProgressBar<> bar;
assert( bar.config().bar_width() == 30 );  // Default value
assert( bar.config().fixed_width() != 0 ); // Actual value depends on member contents
```

The actual terminal line width (measured in characters) can be obtained using `pace::config::terminal_width()`. If the provided output stream does not point to a real terminal device, the return value is 0.

> If the runtime platform is neither `Windows` nor `unix-like`, this function only returns a fixed value 0.

```cxx
pace::ProgressBar<> bar;

assert( pace::config::terminal_width( pace::Channel::Stdout ) > bar.config().fixed_width() );
bar.config().bar_width( pace::config::terminal_width( pace::Channel::Stdout )
                          - bar.config().fixed_width() );
```

It should be noted that if the `pace::facade::Counter` component is enabled, the length of the progress bar is also affected by the current task count. In this case, the task count must be configured before a correct progress bar length can be obtained.

## BlockBar
At runtime, when `config().quota()` is 0 and the `tick` method is invoked, `pace::BlockBar` throws a `pace::exception::InvalidState` exception.
### Element composition
`pace::BlockBar` consists of the following elements:

```text
{LeftBorder}{Prefix}{Percent}{Starting}{Filler}{Lead}{Remain}{Ending}{Counter}{Speed}{Elapsed}{ETA}{Postfix}{RightBorder}
```

Among them, the customizable parts are: `LeftBorder`, `Prefix`, `Starting`, `Lead`, `Filler`, `Remain`, `Ending`, `Speed`, `Postfix`, and `RightBorder`. Their purposes are consistent with their names.

These elements can be found directly in `pace::option` as corresponding wrapper types:

```cxx
pace::option::Colored;       // Color effect
pace::option::FontBold;      // Bold font
pace::option::FontFaint;     // Faint font
pace::option::FontItalic;    // Italic effect
pace::option::FontUnderline; // Underlined font
pace::option::FontInverse;   // Inverse font display
pace::option::FontHidden;    // Hidden font
pace::option::FontCrossed;   // Strikethrough

pace::option::LeftBorder;  // Modify the starting border on the left side of the entire progress bar
pace::option::RightBorder; // Modify the ending border on the right side of the entire progress bar

pace::option::Prefix;      // Modify the prefix description
pace::option::Postfix;     // Modify the postfix description

pace::option::Starting;  // Modify the element on the left side of the progress block and on the right side of Percent
pace::option::Ending;    // Modify the element on the right side of the progress block and on the left side of Counter
pace::option::Lead;      // Modify each frame of the animated section
pace::option::Filler;    // Modify the fill character for the completed portion
pace::option::Remain;    // Modify the fill character for the remaining portion
pace::option::Reversed;  // Adjust the growth direction of the progress bar (false means left-to-right)
pace::option::BarWidth;  // Adjust the width of the progress bar

pace::option::SpeedUnit; // Modify the unit used in the Speed section
pace::option::Magnitude; // Adjust the carry ratio used in the Speed section

pace::option::Quota;   // Adjust the task count
pace::option::Divider; // Modify the separator placed between two elements

pace::option::PrefixForecolor;  // Modify the foreground color of Prefix
pace::option::PrefixBackcolor;  // Modify the background color of Prefix
pace::option::PostfixForecolor; // Modify the foreground color of Postfix
pace::option::PostfixBackcolor; // Modify the background color of Postfix
pace::option::StartForecolor;   // Modify the foreground color of Starting
pace::option::StartBackcolor;   // Modify the background color of Starting
pace::option::EndForecolor;     // Modify the foreground color of Ending
pace::option::EndBackcolor;     // Modify the background color of Ending
pace::option::FillerForecolor;  // Modify the foreground color of Filler
pace::option::FillerBackcolor;  // Modify the background color of Filler
pace::option::RemainForecolor;  // Modify the foreground color of Remain
pace::option::RemainBackcolor;  // Modify the background color of Remain
pace::option::LeadForecolor;    // Modify the foreground color of Lead
pace::option::LeadBackcolor;    // Modify the background color of Lead
pace::option::InfoForecolor;    // Modify the foreground color of Divider, Percent, Counter, Speed, Elapsed, and ETA
pace::option::InfoBackcolor;    // Modify the background color of Divider, Percent, Counter, Speed, Elapsed, and ETA
```

The configuration type also provides methods with the same names but different naming styles.
### Variable bar width
The section between the `Starting` and `Ending` elements is called the `BlockPlot` progress indicator (excluding `Starting` and `Ending` themselves). The width of this indicator is variable.

This behavior of `pace::BlockBar` is consistent with `pace::ProgressBar`; refer to the previous section for details.

## SpinBar
At runtime, `pace::SpinBar` does not always require a configured task count. Instead, it can begin running immediately by directly invoking `tick`.

### Element composition
`SpinBar` consists of the following elements:

```text
{LeftBorder}{Prefix}{Lead}{Percent}{Counter}{Speed}{Elapsed}{ETA}{Postfix}{RightBorder}
```

Among them, the customizable parts are: `LeftBorder`, `Prefix`, `Lead`, `Speed`, `Postfix`, and `RightBorder`. Their purposes are consistent with their names.

These elements can be found directly in `pace::option` as corresponding wrapper types:

```cxx
pace::option::Colored;       // Color effect
pace::option::FontBold;      // Bold font
pace::option::FontFaint;     // Faint font
pace::option::FontItalic;    // Italic effect
pace::option::FontUnderline; // Underlined font
pace::option::FontInverse;   // Inverse font display
pace::option::FontHidden;    // Hidden font
pace::option::FontCrossed;   // Strikethrough

pace::option::LeftBorder;  // Modify the starting border on the left side of the entire progress bar
pace::option::RightBorder; // Modify the ending border on the right side of the entire progress bar

pace::option::Prefix;      // Modify the prefix description
pace::option::Postfix;     // Modify the postfix description

pace::option::Lead;      // Modify each frame of the animated section
pace::option::Shift;     // Adjust the animation speed of the animated section (Lead)

pace::option::SpeedUnit; // Modify the unit used in the Speed section
pace::option::Magnitude; // Adjust the carry ratio used in the Speed section

pace::option::Quota;   // Adjust the task count
pace::option::Divider; // Modify the separator placed between two elements

pace::option::PrefixForecolor;  // Modify the foreground color of Prefix
pace::option::PrefixBackcolor;  // Modify the background color of Prefix
pace::option::PostfixForecolor; // Modify the foreground color of Postfix
pace::option::PostfixBackcolor; // Modify the background color of Postfix
pace::option::LeadForecolor;    // Modify the foreground color of Lead
pace::option::LeadBackcolor;    // Modify the background color of Lead
pace::option::InfoForecolor;    // Modify the foreground color of Divider, Percent, Counter, Speed, Elapsed, and ETA
pace::option::InfoBackcolor;    // Modify the background color of Divider, Percent, Counter, Speed, Elapsed, and ETA
```

The configuration type also provides methods with the same names but different naming styles.

`pace::SpinBar` does not contain a progress bar.

## SweepBar
In terms of runtime behavior, `pace::SweepBar` does not always require configuring the task count. Instead, you can directly call `tick` to start running immediately.
### Element composition
`SweepBar` consists of the following elements:

```text
{LeftBorder}{Prefix}{Percent}{Starting}{Filler}{Lead}{Filler}{Ending}{Counter}{Speed}{Elapsed}{ETA}{Postfix}{RightBorder}
```

Among them, the customizable parts are: `LeftBorder`, `Prefix`, `Starting`, `Filler`, `Lead`, `Ending`, `Speed`, `Postfix`, and `RightBorder`. Their purposes are consistent with their names.

These elements can be found directly as corresponding wrapper types under `pace::option`:

```cxx
pace::option::Colored;       // Color effect
pace::option::FontBold;      // Bold font
pace::option::FontFaint;     // Faint font
pace::option::FontItalic;    // Italic effect
pace::option::FontUnderline; // Underlined font
pace::option::FontInverse;   // Inverse font display
pace::option::FontHidden;    // Hidden font
pace::option::FontCrossed;   // Strikethrough

pace::option::LeftBorder;  // Modify the starting border on the left side of the entire progress bar
pace::option::RightBorder; // Modify the ending border on the right side of the entire progress bar

pace::option::Prefix;      // Modify the prefix description
pace::option::Postfix;     // Modify the postfix description

pace::option::Starting;  // Modify the element on the left side of the progress bar block and on the right side of Percent
pace::option::Ending;    // Modify the element on the right side of the progress bar block and on the left side of Counter
pace::option::Filler;    // Modify the fill character of the progress bar background
pace::option::Lead;      // Modify each frame of the animated section
pace::option::Shift;     // Adjust the animation speed of the animated section (Lead)
pace::option::BarWidth;  // Adjust the width of the progress bar

pace::option::SpeedUnit; // Modify the unit used in the Speed section
pace::option::Magnitude; // Adjust the carry multiplier used in the Speed section

pace::option::Quota;   // Adjust the task count
pace::option::Divider; // Modify the separator placed between two elements

pace::option::PrefixForecolor;  // Modify the foreground color of Prefix
pace::option::PrefixBackcolor;  // Modify the background color of Prefix
pace::option::PostfixForecolor; // Modify the foreground color of Postfix
pace::option::PostfixBackcolor; // Modify the background color of Postfix
pace::option::StartForecolor;   // Modify the foreground color of Starting
pace::option::StartBackcolor;   // Modify the background color of Starting
pace::option::EndForecolor;     // Modify the foreground color of Ending
pace::option::EndBackcolor;     // Modify the background color of Ending
pace::option::FillerForecolor;  // Modify the foreground color of Filler
pace::option::FillerBackcolor;  // Modify the background color of Filler
pace::option::LeadForecolor;    // Modify the foreground color of Lead
pace::option::LeadBackcolor;    // Modify the background color of Lead
pace::option::InfoForecolor;    // Modify the foreground color of Divider, Percent, Counter, Speed, Elapsed, and ETA
pace::option::InfoBackcolor;    // Modify the background color of Divider, Percent, Counter, Speed, Elapsed, and ETA
```

In addition, configuration types provide methods with the same names but in different naming styles.
### Variable bar width
The section between the `Starting` and `Ending` elements is called `SweepPlot`, which represents the sweeping progress bar (excluding `Starting` and `Ending`). The width of this sweeping progress bar is variable.

The behavior of this part in `pace::SweepBar` is consistent with that of `pace::ProgressBar`. Refer to the previous chapter for details.

## FlowBar
In terms of runtime behavior, `pace::SweepBar` does not always require configuring the task count. Instead, you can directly call `tick` to start running immediately.
### Element composition
`FlowBar` consists of the following elements:

```text
{LeftBorder}{Prefix}{Percent}{Starting}{Filler}{Lead}{Filler}{Ending}{Counter}{Speed}{Elapsed}{ETA}{Postfix}{RightBorder}
```

Among them, the customizable parts are: `LeftBorder`, `Prefix`, `Starting`, `Filler`, `Lead`, `Ending`, `Speed`, `Postfix`, and `RightBorder`. Their purposes are consistent with their names.

These elements can be found directly as corresponding wrapper types under `pace::option`:

```cxx
pace::option::Colored;       // Color effect
pace::option::FontBold;      // Bold font
pace::option::FontFaint;     // Faint font
pace::option::FontItalic;    // Italic effect
pace::option::FontUnderline; // Underlined font
pace::option::FontInverse;   // Inverse font display
pace::option::FontHidden;    // Hidden font
pace::option::FontCrossed;   // Strikethrough

pace::option::LeftBorder;  // Modify the starting border on the left side of the entire progress bar
pace::option::RightBorder; // Modify the ending border on the right side of the entire progress bar

pace::option::Prefix;      // Modify the prefix description
pace::option::Postfix;     // Modify the postfix description

pace::option::Starting;  // Modify the element on the left side of the progress bar block and on the right side of Percent
pace::option::Ending;    // Modify the element on the right side of the progress bar block and on the left side of Counter
pace::option::Filler;    // Modify the fill character of the progress bar background
pace::option::Lead;      // Modify each frame of the animated section
pace::option::Shift;     // Adjust the animation speed of the animated section (Lead)
pace::option::BarWidth;  // Adjust the width of the progress bar

pace::option::SpeedUnit; // Modify the unit used in the Speed section
pace::option::Magnitude; // Adjust the carry multiplier used in the Speed section

pace::option::Quota;   // Adjust the task count
pace::option::Divider; // Modify the separator placed between two elements

pace::option::PrefixForecolor;  // Modify the foreground color of Prefix
pace::option::PrefixBackcolor;  // Modify the background color of Prefix
pace::option::PostfixForecolor; // Modify the foreground color of Postfix
pace::option::PostfixBackcolor; // Modify the background color of Postfix
pace::option::StartForecolor;   // Modify the foreground color of Starting
pace::option::StartBackcolor;   // Modify the background color of Starting
pace::option::EndForecolor;     // Modify the foreground color of Ending
pace::option::EndBackcolor;     // Modify the background color of Ending
pace::option::FillerForecolor;  // Modify the foreground color of Filler
pace::option::FillerBackcolor;  // Modify the background color of Filler
pace::option::LeadForecolor;    // Modify the foreground color of Lead
pace::option::LeadBackcolor;    // Modify the background color of Lead
pace::option::InfoForecolor;    // Modify the foreground color of Divider, Percent, Counter, Speed, Elapsed, and ETA
pace::option::InfoBackcolor;    // Modify the background color of Divider, Percent, Counter, Speed, Elapsed, and ETA
```

In addition, configuration types provide methods with the same names but in different naming styles.
### Variable bar width
The section between the `Starting` and `Ending` elements is called `FlowPlot`, which represents the sweeping progress bar (excluding `Starting` and `Ending`). The width of this sweeping progress bar is variable.

The behavior of this part in `pace::FlowBar` is consistent with that of `pace::ProgressBar`. Refer to the previous chapter for details.

# Independent Components
## MultiBar
`pace::MultiBar` is a tuple-like type. It is not a progress bar type itself; instead, it must receive multiple different progress bar objects and combine them to achieve multi-progress-bar rendering.
### Quick start
`pace::MultiBar` requires all contained objects to have identical template parameters, while configuration types may differ.

```cxx
pace::MultiBar<pace::ProgressBar<>, pace::ProgressBar<>, pace::BlockBar<>> mbar1;
// or
pace::MultiBar<pace::ProgressBar<pace::Channel::Stdout>,
                pace::ProgressBar<pace::Channel::Stdout>,
                pace::ProgressBar<pace::Channel::Stdout>>
  mbar2;

// If a MultiBar containing multiple repeated progress bar types is needed, MultiBar_t can be used
pace::MultiBar_t<pace::ProgressBar<pace::Channel::Stdout>, 3> mbar3;
static_assert( std::is_same_v<decltype( mbar2 ), decltype( mbar3 )> );

mbar1.config<0>().quota( 100 );
mbar1.config<1>().quota( 200 );
mbar1.config<2>().quota( 300 );

// Direct access to the corresponding progress bar object
mbar1.at<0>().tick();
// Indirect access
mbar1.tick<1>();
// Access via unqualified get()
using std::get;
get<2>( mbar1 );

// Methods without template parameters refer to the MultiBar object itself
assert( mbar1.active() );

// do tasks...
```

The constructor of `pace::MultiBar` can accept individual progress bar objects or their configuration types. If the C++ standard is above C++17, `pace::MultiBar` also provides a class template deduction guide.

```cxx
// MultiBar requires all types in its template parameter list to have the same output stream attribute and execution policy
pace::ProgressBar<> bar1;
pace::BlockBar<> bar2, bar3;

// Since bar is move-only, std::move must be used here
auto mbar1 = pace::MultiBar<pace::ProgressBar<>, pace::BlockBar<>, pace::BlockBar<>>( std::move( bar1 ),
                                                                                      std::move( bar2 ),
                                                                                      std::move( bar3 ) );
auto mbar2 =
  pace::MultiBar<pace::ProgressBar<>, pace::BlockBar<>, pace::ProgressBar<>>( pace::config::Line(),
                                                                              pace::config::Block(),
                                                                              pace::config::Line() );

#if __cplusplus >= 201703L
// In C++17 and later, the following statement is valid
auto mbar3 = pace::MultiBar( pace::config::Line(), pace::config::Block(), pace::config::Line() );
// This object will have a type corresponding to a MultiBar using pace::Channel::Stderr

static_assert( std::is_same<decltype( mbar3 ), decltype( mbar2 )>::value );
#endif
```

All methods of a progress bar type can be accessed through pace::MultiBar as template functions; in some sense, `pace::MultiBar` behaves more like a container than a progress bar type.

Similar to standalone progress bar types, `pace::MultiBar` is movable and swappable. However, it should not be moved or swapped while running.

Attempting to move or swap `pace::MultiBar` across multiple threads, or moving/swapping it in one thread while accessing it from others, is not thread-safe.
### Factory functions
pace provides multiple overloads of `make_multi` to simplify construction of `pace::MultiBar`.

Their behaviors are as follows:

```cxx
// Create a MultiBar whose size matches the number of arguments
auto bar1 = pace::make_multi<pace::Channel::Stdout>( pace::config::Line(), pace::config::Block() );
auto bar2 = pace::make_multi<>( pace::ProgressBar<>(), pace::BlockBar<>() );

// Create a fixed-size MultiBar where all progress bars share the same type,
// initialized using the provided configuration object
auto bar3 = pace::make_multi<6, pace::Channel::Stdout>( pace::config::Spin() );
auto bar4 = pace::make_multi<6>( pace::SpinBar<pace::Channel::Stdout>() );
// All internal progress bars in bar3 and bar4 share identical configuration data

// Create a fixed-size MultiBar where identical types are used,
// but arguments are applied sequentially to internal progress bars
auto bar5 = pace::make_multi<pace::config::Sweep, 3>( pace::config::Sweep() );
auto bar6 =
  pace::make_multi<pace::SweepBar<pace::Channel::Stdout>, 3>( pace::SweepBar<pace::Channel::Stdout>() );
// Only the first progress bar is initialized with the provided argument;
// the remaining ones are default-initialized
```
### Rendering behavior
The rendering strategy of `pace::MultiBar` is similar to other progress bars, but with differences.

Since `pace::MultiBar` renders multiple progress bars across multiple lines, when using `pace::Region::Relative`, the number of lines occupied depends on the number of contained progress bar types.

The number of active progress bars can be obtained via `active_count()`, and the rendering structure occupies `active_count() + 1` lines.

Example:

```cxx
// Since newline characters are output successively here,
// synchronization is used to avoid inconsistent output behavior
auto bar = pace::make_multi<pace::Channel::Stderr, pace::Policy::Sync, pace::Region::Relative>(
  pace::config::Line( pace::option::Quota( 100 ) ),
  pace::config::Line( pace::option::Quota( 150 ) ),
  pace::config::Line( pace::option::Quota( 200 ) ) );

for ( size_t i = 0; i < 95; ++i ) {
  bar.tick<0>();
  bar.tick<1>();
}

std::cerr << "Extra log information";
// Note: at least `active_count() + 1` newline characters must be inserted after output
for ( size_t i = 0; i < bar.active_count() + 1; ++i )
  std::cerr << '\n';
std::cerr << std::flush;

bar.tick<2>();
while ( bar.active<0>() )
  bar.tick<0>();
while ( bar.active<1>() )
  bar.tick<1>();
while ( bar.active<2>() )
  bar.tick<2>();
```
### Tuple protocol
`pace::MultiBar` provides specializations for `std::tuple_element` and `std::tuple_size`, and also overloads `get`. Therefore, it can be treated as a specialized version of `std::tuple`.

In C++17 and later, structured bindings are supported:

```cxx
static_assert( __cplusplus >= 201703L );

auto mbar = pace::make_multi( pace::ProgressBar<>( pace::option::Quota( 2 ) ),
                              pace::BlockBar<>( pace::option::Quota( 3 ) ) );

// after C++17
auto& [bar1, bar2] = mbar;

bar1.tick();
bar2.tick();

std::this_thread::sleep_for( std::chrono::seconds( 4 ) );
```

## DynamicBar
In contrast to `pace::MultiBar`, `pace::DynamicBar` is a factory-type component. It holds almost no data and is only responsible for establishing lifecycle relationships between different progress bar types.
### Quick start
Unlike other types, `pace::DynamicBar` accepts a progress bar type or a configuration class type and returns a `std::unique_ptr` pointing to the corresponding progress bar type. All method calls on the progress bar must be made by dereferencing this returned pointer. Each `std::unique_ptr` returned by `pace::DynamicBar` can enable terminal rendering of progress bars; however, terminal rendering will only stop when all `std::unique_ptr` instances are either destroyed or stopped.

`pace::DynamicBar` can be destroyed even when multiple `std::unique_ptr` instances have already been created. This only means that it is no longer possible to query whether this `pace::DynamicBar` is running, nor can it be used to shut down all progress bar objects owned by the created `std::unique_ptr` instances.

If a `std::unique_ptr` returned by `pace::DynamicBar` is destroyed, and the corresponding progress bar object is still running, `pace::DynamicBar` will detect the invalid object and remove it from the rendering list.

In summary, `pace::DynamicBar` can accept an arbitrary number of progress bar objects at runtime and coordinate their rendering order in the terminal. The rendering order depends on their start time; later-started progress bars will appear lower in the terminal.

In addition, attempting to move or swap `pace::DynamicBar` across multiple threads, or moving/swapping it in one thread while calling its methods in other threads, is not thread-safe.

Similarly, all progress bar types passed into `pace::DynamicBar` must have identical template parameters.

```cxx
std::vector<std::thread> pool;
{
  pace::DynamicBar<> dbar;

  auto bar1 = dbar.insert<pace::ProgressBar<>>();
  // bar1 and bar2 are both std::unique_ptr</* ProgressBar */> types
  auto bar2 =
    dbar.insert( pace::config::Line( pace::option::Prefix( "No.2" ), pace::option::Quota( 8000 ) ) );

  pool.emplace_back( [&bar1]() {
    bar1->config().prefix( "No.1" ).quota( 1919810 );
    std::this_thread::sleep_for( std::chrono::seconds( 5 ) );
    do {
      bar1->tick();
      std::this_thread::sleep_for( std::chrono::milliseconds( 5 ) );
    } while ( bar1->active() );
  } );
  pool.emplace_back( [&bar2]() {
    std::this_thread::sleep_for( std::chrono::seconds( 3 ) );
    do {
      bar2->tick();
      std::this_thread::sleep_for( std::chrono::microseconds( 900 ) );
    } while ( bar2->active() );
  } );
  pool.emplace_back( [&dbar]() {
    auto bar =
      dbar.insert<pace::config::Line>( pace::option::Prefix( "No.3" ), pace::option::Quota( 1000 ) );
    for ( int i = 0; i < 850; ++i ) {
      bar->tick();
      std::this_thread::sleep_for( std::chrono::milliseconds( 5 ) );
    }
    bar->reset();

    // "No.3" will reappear at the bottom of the terminal
    for ( int i = 0; i < 400; ++i ) {
      bar->tick();
      std::this_thread::sleep_for( std::chrono::milliseconds( 5 ) );
    }
    // let bar be destroyed
  } );

  std::this_thread::sleep_for( std::chrono::seconds( 1 ) );
  assert( dbar.active() );
} // dbar is destroyed here, but this is safe

for ( auto& td : pool )
  td.join();
```
### Helper functions
Since `pace::DynamicBar` is only a factory-type component, pace provides a `make_dynamic` function. This function can return multiple progress bar objects wrapped in `std::unique_ptr` without requiring direct interaction with `pace::DynamicBar`.

Note: The objects returned by this function are std::unique_ptr, and the objects they point to **do not have the ability to determine whether they belong to the same `pace::DynamicBar` or another one**. Therefore, `std::unique_ptr` instances created by different `pace::DynamicBar` objects are type-compatible at the type level.

Mixing `std::unique_ptr` from different sources will often throw a `pace::exception::InvalidState` exception, indicating that a running progress bar instance already exists.

```cxx
// Obtain std::unique_ptr objects equal to the number of arguments
auto bars1 = pace::make_dynamic<pace::Channel::Stdout>( pace::config::Line(), pace::config::Block() );
auto bars2 = pace::make_dynamic<>( pace::ProgressBar<>(), pace::BlockBar<>() );
// To store different progress bar types, bars1 and bars2 are tuple types containing multiple std::unique_ptr objects

// Create a std::vector<std::unique_ptr</* Bar Type */>> where all progress bar types are identical
// and initialize all internal progress bars using the provided configuration objects
auto bar3 = pace::make_dynamic<pace::Channel::Stdout>( pace::config::Spin(), 6 );
auto bar4 = pace::make_dynamic( pace::SpinBar<pace::Channel::Stdout>(), 6 );
// All internal progress bars in bar3 and bar4 are initialized with identical configuration data

// Create a std::vector<std::unique_ptr</* Bar Type */>> where all progress bar types are identical
// The provided parameters are applied sequentially to the internal progress bar objects
auto bar5 = pace::make_dynamic<pace::config::Sweep>( 3, pace::config::Sweep() );
auto bar6 =
  pace::make_dynamic<pace::SweepBar<pace::Channel::Stdout>>( 3, pace::SweepBar<pace::Channel::Stdout>() );
// Only the first progress bar object in bar5 and bar6 is initialized with the provided parameter; the others are default-initialized

// For the last two functions, if the number of arguments does not match the number of expected objects,
// a pace::exception::InvalidArgument exception will be thrown
try {
  auto _ = pace::make_dynamic<pace::config::Sweep>( 2,
                                                    pace::config::Sweep(),
                                                    pace::config::Sweep(),
                                                    pace::config::Sweep() );
} catch ( const pace::exception::InvalidArgument& e ) {
  std::cerr << "Oops! " << e.what() << std::endl;
}
```
### Rendering behavior
The rendering behavior of `pace::DynamicBar` is similar to `pace::MultiBar`: it renders multiple progress bars simultaneously across multiple lines. Therefore, when using pace::Region::Relative, the number of rows occupied by the rendering structure depends on the number of active progress bars in `pace::DynamicBar`.

The number of progress bars managed by `pace::DynamicBar` can be obtained via the `active_count()` method, and the rendering structure will occupy `active_count() + 1` lines.

Example:

```cxx
// Since the newline character is output successively here,
// the scheduling strategy has chosen synchronization to avoid inconsistent output behavior
pace::DynamicBar</* any channel */, pace::Policy::Sync, pace::Region::Relative> dbar;

auto bar1 = dbar.insert( pace::config::Line( pace::option::Quota( 100 ) ) );
auto bar2 = dbar.insert( pace::config::Line( pace::option::Quota( 150 ) ) );
auto bar3 = dbar.insert( pace::config::Line( pace::option::Quota( 200 ) ) );

for ( size_t i = 0; i < 95; ++i ) {
  bar1->tick();
  bar2->tick();
}

std::cerr << "Extra log information";
// Notice: At least `active_count() + 1` nextline must be inserted after the output information
for ( size_t i = 0; i < dbar.active_count() + 1; ++i )
  std::cerr << '\n';
std::cerr << std::flush;

bar3->tick();
while ( bar1->active() )
  bar1->tick();
while ( bar2->active() )
  bar2->tick();
while ( bar3->active() )
  bar3->tick();
```

## NumericSpan
`pace::slice::NumericSpan` is a template type used to represent a numerical range defined by a start point, an end point, and a step size. Mathematically, this range is expressed as `[start, end)`.

During construction or when modifying member values, the following conditions will trigger an exception `pace::exception::InvalidArgument`:

1. The start is greater than the end while the step is positive;
2. The start is less than the end while the step is negative;
3. The step is zero.
### Member methods
`pace::slice::NumericSpan` provides the following methods:

```cxx
iterator begin() const noexcept; // returns an iterator pointing to the start of the numerical range
iterator end() const noexcept;   // returns an iterator pointing to the end of the numerical range

N front() const noexcept;           // returns the value at the start of the range
N back() const noexcept;            // returns the value at the end of the range
N step() const noexcept;            // returns the current step size
/* size_t */ size() const noexcept; // returns the number of steps in the current range

void swap( NumericSpan& ) noexcept; // swaps two numerical ranges
```
### Iterator type
`pace::slice::NumericSpan::iterator` is a forward iterator. It overloads, but is not limited to, operators such as `operator++()`, `operator++( int )`, `operator+=()`, `operator*()`, and equality comparison operators.

The number of valid iterations corresponds to the value returned by `pace::slice::NumericSpan::size()`. In particular, if the step size exceeds the numerical range, advancing the iterator may produce values beyond the end of the range.

## IteratorSpan
`pace::slice::IteratorSpan` is a template type used to represent an abstract range defined by two iterators. It can be regarded as a heavily simplified version of `std::views::ref_view`.

`pace::slice::IteratorSpan` requires that the provided iterator type must be copy-constructible or move-constructible, and must support computing the distance between two iterator instances; otherwise, compilation will fail.

If the provided iterators are a pair of reverse iterators over a non-reversible type, an exception `pace::exception::InvalidArgument` will be thrown.

```cxx
int arr1[50] = {};
std::vector<int> arr2;

try {
 auto reverse_span1 = pace::slice::IteratorSpan<int*>( arr1 + 49, arr1 - 1 );
} catch ( const pace::exception::InvalidArgument& ) {
 // ...
}
auto reverse_span2 =
 pace::slice::IteratorSpan<std::reverse_iterator<std::vector<int>::iterator>>( arr2.rbegin(),
                                                                               arr2.rend() );
```
### Member methods
`pace::slice::IteratorSpan` provides the following methods:

```cxx
iterator begin() const noexcept; // returns an iterator pointing to the start of the abstract range
iterator end() const noexcept;   // returns an iterator pointing to the end of the abstract range

/* reference */ front() const noexcept; // returns a reference to the element at the start iterator
/* reference */ back() const noexcept;  // returns a reference to the element before the end iterator
/* size_t */ step() const noexcept;     // returns the current step size, usually a compile-time constant 1
/* size_t */ size() const noexcept;     // returns the size of the current abstract range

void swap( IteratorSpan& ) noexcept; // swaps two abstract ranges
```
### Iterator type
`pace::slice::IteratorSpan::iterator` is a forward iterator. It overloads, but is not limited to, operators such as `operator++()`, `operator++( int )`, `operator+=()`, `operator*()`, and equality comparison operators.

Since it is a forward iterator and does not provide decrement operators, all reverse operations depend on the implementation of the underlying iterator type.

## SizedSpan
`pace::slice::SizedSpan` is a template type used to represent an iterable range that satisfies the concept `std::ranges::sized_range` and does not satisfy the concept `std::ranges::view`.

In simple terms, `pace::slice::SizedSpan` can be regarded as a simplified version of `std::ranges::ref_view`; it is a view over container types and array types.
### Member functions
`pace::slice::SizedSpan` provides the following methods:

```cxx
/* iterator */ begin() const; // Returns an iterator pointing to the beginning of the abstract range
/* sentinel */ end() const;   // Returns a sentinel pointing to the end of the abstract range

/* reference */ front() const; // Returns a reference to the element pointed to by the begin iterator of the abstract range
/* reference */ back() const;  // Returns a reference to the element before the end iterator of the abstract range
/* size_t */ step() const noexcept; // Returns the current step size, usually a compile-time constant 1
/* size_t */ size() const;          // Returns the size of the current abstract range

void swap(SizedSpan&) noexcept; // Swaps two abstract ranges
```
### Iterator type
The iterator type of `pace::slice::SizedSpan::iterator` is equivalent to the iterator type of its underlying range.

## TrackedSpan
`pace::slice::TrackedSpan` is a template type used to represent an iterable range associated with a progress bar type.

`pace::slice::TrackedSpan` only accepts an object of a view type satisfying the concept `std::ranges::sized_range`, together with a progress bar object. Its purpose is to simplify the interaction between progress bar instances and enhanced for-loops or other contexts requiring iterators.

This is a move-only special type. It should only be constructed and returned by factory functions, such as a progress bar’s `iterate` method, and should not be constructed manually.

Calling the `begin` method of `pace::slice::TrackedSpan` produces a side effect: the object will attempt to set the task count of the referenced progress bar instance based on the size of its internal abstract range.
### Member functions
`pace::slice::TrackedSpan` provides the following methods:

```cxx
/* iterator */ begin() &;          // Assigns a value to the internal progress bar instance and returns the starting iterator
/* sentinel */ end() const;        // Returns the sentinel (end iterator)
bool empty() const noexcept;       // Checks whether the object refers to a valid progress bar instance
explicit operator bool() noexcept; // Checks whether the object is non-empty

void swap(TrackedSpan&) noexcept; // Swaps two proxy ranges
```
### Iterator type
The iterator type of `pace::slice::TrackedSpan::iterator` is a forward iterator. The increment operator of this iterator attempts to call the `tick()` method of the bound progress bar instance, which may trigger side effects in unexpected contexts.

## iterate
`iterate` is an overloaded name for a series of template functions. It is a wrapper interface for the `iterate` method of the pace progress bar type.

This function allows visualizing an iteration process without explicitly constructing a progress bar object, while still using the progress bar’s `iterate` method.

Its usage is the same as the `iterate` method; however, the `iterate` function allows passing any number of additional parameters to customize the style of the progress bar.

```cxx
// Iteration range: [100, 0), step: -1
pace::iterate<pace::ProgressBar<>>( 100, 0, -1, []( int ) { std::this_thread::sleep_for( 100ms ); } );

// Iteration range: [0.0, -2.0), step: -0.01
pace::iterate<pace::config::Line>(
 -2.0,
 -0.01,
 []( int ) { std::this_thread::sleep_for( 100ms ); },
 pace::option::InfoForecolor( "#FFDD88" ),
 pace::option::Prefix( "Iterating..." ) );

// Iteration range: [100, 0), step: 1
pace::iterate<pace::ProgressBar<>>(
 100,
 []( int ) { std::this_thread::sleep_for( 100ms ); },
 pace::config::Line( pace::option::InfoForecolor( "#FF8899" ),
                     pace::option::SpeedUnit( { "files/s", "k files/s", "M files/s", "G files/s" } ) ) );

int arr1[] { 100, 99, 98, 97, 96, 95, 94, 93, 92, 91 };
std::vector<int> arr2 {
 0, 1, 2, 3, 4, 5, 6,
};

pace::iterate<pace::BlockBar<pace::Channel::Stdout>>( arr1,
                                                     arr1 + ( sizeof( arr1 ) / sizeof( int ) ),
                                                     []( int& ele ) {
                                                       ele += 1;
                                                       std::this_thread::sleep_for( 300ms );
                                                     } );
// Iteration over a STL container.
pace::iterate<pace::config::Block, pace::Channel::Stderr, pace::Policy::Sync>( arr2, []( int ) {
 std::this_thread::sleep_for( 300ms );
} );
```

# Composition Model
In fact, all progress bar types and configuration types in the pace library are generated at compile time.
## Modular Components
pace divides the progress bar into three parts: the facade component `pace::facade`, the progress bar object behavior `pace::details::behaviors`, and the supporting functional components `pace::details::aspects`.

These three parts are connected through multiple registration structures: the behavior components of the progress bar object declare dependency relationships between different features via the `pace::details::traits::InheritOrder` structure; the functional components also declare dependencies among themselves using the same structure; the facade components likewise declare dependencies on functional components through this structure, and declare their dependency on behavior components via the `pace::details::aspects::EntailOf` structure.

The template parameters of `pace::prefab::BasicConfig` can accept multiple facade components, and a dependency resolution algorithm is used to collect the functional component dependencies of these facade components, producing the target configuration type.

The configuration type is then injected into `pace::prefab::BasicBar` to obtain the target progress bar type.

To parse the data in `pace::prefab::BasicConfig` and render it into a terminal string, pace defines a compile-time rendering engine `pace::details::render::Assembler` and `pace::details::render::Builder`. They generate the final progress bar string according to the order of facade components declared in the template parameters of `pace::prefab::BasicConfig`.

All of the above operations occur entirely at compile time.

## Composing New Configuration Types
Thanks to its modular design, pace allows the creation at compile time of a new progress bar type that is completely different from the default provided types.

```cxx
using AnotherConfig = pace::prefab::BasicConfig<pace::facade::Elapsed, pace::facade::ETA>;
using AnotherBar    = pace::prefab::BasicBar<AnotherConfig>;

static_assert( sizeof( AnotherConfig ) != sizeof( pace::config::Line )
              && sizeof( AnotherConfig ) != sizeof( pace::config::Block )
              && sizeof( AnotherConfig ) != sizeof( pace::config::Spin )
              && sizeof( AnotherConfig ) != sizeof( pace::config::Sweep )
              && sizeof( AnotherConfig ) != sizeof( pace::config::Flow ) );

AnotherBar another;
another.config().quota( 100 ).divider( " | " ).enable_all();

for ( int i = 0; i < 100; ++i ) {
 another.tick();
 std::this_thread::sleep_for( std::chrono::milliseconds( 500 ) );
}
```

If the `config()` part of the code above is removed, the compiled progress bar will display nothing; this is because no default parameters are provided for this progress bar.

When a progress bar type and a configuration type are default-constructed, the configuration type attempts to access the `pace::config::ProvideFor` structure to obtain a default value. In the absence of special configuration, this structure returns the default-constructed result of a wrapper type.

pace allows specialization of `pace::config::ProvideFor` to provide non-empty default values:

```cxx
using AnotherConfig = pace::prefab::BasicConfig<pace::facade::Elapsed, pace::facade::ETA>;

template<>
struct pace::config::ProvideFor<AnotherConfig, pace::option::Divider> {
  static constexpr pace::option::Divider provide() { return { " | " }; }
};

// The default switch configuration of components is relatively complex
template<>
struct pace::config::ProvideFor<AnotherConfig, pace::option::Projection> {
  static constexpr pace::option::Projection provide()
  {
    // Since Only and Except are compile-time variable components,
    // they cannot be injected into ProvideFor.
    // To make the default value independent of specific types,
    // we must use the static method provided by BasicConfig to strip
    // the parameter list of Only or Except,
    // thereby obtaining a Projection type that requires no template parameters
    // and can be used as a default value.
    return AnotherConfig::bake( pace::option::Except<>() );
  }
};

// In C++14 and later, a lambda-based variable template ProvideFor_v can be used
// In this case, there is no need to specialize the entire ProvideFor type
template<>
auto pace::config::ProvideFor_v<AnotherConfig, pace::option::Prefix> =
  []() -> pace::option::Prefix { return { "sample" }; };

int main()
{
  using AnotherBar = pace::prefab::BasicBar<AnotherConfig>;

  static_assert( sizeof( AnotherConfig ) != sizeof( pace::config::Line )
                 && sizeof( AnotherConfig ) != sizeof( pace::config::Block )
                 && sizeof( AnotherConfig ) != sizeof( pace::config::Spin )
                 && sizeof( AnotherConfig ) != sizeof( pace::config::Sweep )
                 && sizeof( AnotherConfig ) != sizeof( pace::config::Flow ) );

  AnotherBar another;
  // In general, it is not recommended to provide a default value for the task quota
  another.config().quota( 100 );

  for ( int i = 0; i < 100; ++i ) {
    another.tick();
    std::this_thread::sleep_for( std::chrono::milliseconds( 500 ) );
  }
}
```

Although pace supports mixing different progress bar indicators (facade components ending with Plot), due to the nature of the dependency resolution algorithm, these indicators actually share the same set of functional components `pace::details::aspects`. Mixing them will reduce the visual distinction between progress bars and may produce unexpected rendering effects.

In practice, it is not recommended to declare more than one progress bar renderer at the same time.

## Custom Components
The following content depends on internal components of pace. Some internal components are not stable for updates; specific usage should follow the library source code.
### Custom facade
pace allows constructing a new facade externally and injecting it into `pace::prefab::BasicConfig`.

Taking a clock that displays time as an example, this clock supports switching between 24-hour and 12-hour formats, and can also apply different color styles:

```cxx
struct TimeFormat {
  bool data_;

  TimeFormat() = default;
  TimeFormat( bool data ) noexcept : data_ { data } {}
};

struct ClockColor {
  pace::details::console::escodes::RGBColor color_;

  ClockColor() = default;
  ClockColor( pace::details::console::TrueColor color ) noexcept : color_ { color } {}
};

template<typename Base, typename Derived>
class Clock : public Base {
  friend void unpack( Clock& self, TimeFormat&& format ) noexcept { self.format_ = format.data_; }
  friend void unpack( Clock& self, ClockColor&& color ) noexcept { self.color_ = color.color_; }

  pace::details::console::TrueColor color_;
  // true means 12-hour system
  // false means 24-hour system
  bool format_;

protected:
  pace::details::io::CharPipeline& build( pace::details::io::CharPipeline& pipeline,
                                          const pace::details::render::Parameter& params ) const noexcept
  { // requires C++20
    const auto local =
      std::chrono::zoned_time { std::chrono::current_zone(),
                                std::chrono::floor<std::chrono::seconds>( std::chrono::system_clock::now() ) }
        .get_local_time();
    std::chrono::hh_mm_ss time_of_day { local - std::chrono::floor<std::chrono::days>( local ) };
    auto hours = time_of_day.hours().count();

    // embed styling before rendering the clock
    pipeline << this->clear_then_dye( color_, params.style_off_ );

    if ( format_ ) {
      // 12 hour
      auto h12 = hours % 12;
      if ( h12 == 0 )
        h12 = 12;
      pipeline << std::format( "{:02}:{:02}:{:02} {}",
                               h12,
                               time_of_day.minutes().count(),
                               time_of_day.seconds().count(),
                               ( hours >= 12 ) ? 'P' : 'A' );
    } else // 24 hour
      pipeline << std::format( "{:02}:{:02}:{:02}",
                               hours,
                               time_of_day.minutes().count(),
                               time_of_day.seconds().count() );

    return pipeline;
  }

  uint64_t fixed_length() const noexcept { return 8 + ( format_ ? 0 : 2 ); }

  template<typename... Option>
  Clock( pace::details::traits::TypeSet<Option...> tag ) : Base( tag )
  {
    using OptionSet = pace::details::traits::TypeSet<Option...>;
    if constexpr ( !pace::details::traits::TpContain<OptionSet, TimeFormat>::value )
      unpack( *this, pace::config::provide_for<Derived, TimeFormat>() );
    if constexpr ( !pace::details::traits::TpContain<OptionSet, ClockColor>::value )
      unpack( *this, pace::config::provide_for<Derived, ClockColor>() );
  }
};

// If the facade component depends on aspects, the dependency relationship must be declared here
// and if coloring is required, dependency on RenderRule must be declared
template<>
struct pace::details::traits::InheritOrder<Clock> {
  // Clock must be the first element
  using type = pace::details::traits::C3Container<Clock, pace::details::aspects::RenderRule>;
};

template<>
struct pace::details::aspects::EntailOf<Clock> {
  // If the facade does not require task quantity configuration, only these two types need to be declared
  // otherwise dependency on pace::details::behaviors::Determinate must be declared
  // if the facade depends on frame counters, dependency on pace::details::behaviors::Fancy should be declared
  // more details can be found in the implementation of pace::details::aspects
  using type = pace::details::traits::C3Container<pace::details::behaviors::Indeterminate,
                                                  pace::details::behaviors::Plain>;
};

// All components are rendered in the order they are declared
using AnotherConfig = pace::prefab::BasicConfig<pace::facade::Elapsed, pace::facade::ETA, Clock>;

template<>
auto pace::config::ProvideFor_v<AnotherConfig, pace::option::Colored> =
  []() { return pace::option::Colored( true ); };
template<>
auto pace::config::ProvideFor_v<AnotherConfig, pace::option::Divider> =
  []() { return pace::option::Divider( " | " ); };
template<>
auto pace::config::ProvideFor_v<AnotherConfig, ClockColor> =
  []() -> pace::details::console::TrueColor { return { 0xFF8899 }; };
template<>
auto pace::config::ProvideFor_v<AnotherConfig, TimeFormat> = []() -> TimeFormat { return { true }; };
template<>
auto pace::config::ProvideFor_v<AnotherConfig, pace::option::Projection> =
  []() { return AnotherConfig::bake( pace::option::Except<>() ); };

int main()
{
  using AnotherBar = pace::prefab::BasicBar<AnotherConfig>;

  static_assert( sizeof( AnotherConfig ) != sizeof( pace::config::Line )
                 && sizeof( AnotherConfig ) != sizeof( pace::config::Block )
                 && sizeof( AnotherConfig ) != sizeof( pace::config::Spin )
                 && sizeof( AnotherConfig ) != sizeof( pace::config::Sweep )
                 && sizeof( AnotherConfig ) != sizeof( pace::config::Flow ) );

  AnotherBar another;
  another.config().quota( 100 );

  for ( int i = 0; i < 100; ++i ) {
    another.tick();
    std::this_thread::sleep_for( std::chrono::milliseconds( 500 ) );
  }
}
```
### Custom rendering
Similarly, the rendering engine is a template type that can be specialized and overridden, but it cannot be replaced.

The concrete implementation can be found in the implementation of [pace/SpinBar.hpp](../include/pace/SpinBar.hpp).

In addition, if needed in practice, it is completely possible to build a different `BasicConfig`, or provide partial specializations; as long as the interface defined by the original `BasicConfig` is followed.

However, `pace::prefab::BasicBar` does not support overriding. This is because `pace::prefab::BasicBar` follows the principle of minimal dependencies: it does not care about the implementation of the configuration type, nor does it care about what data the configuration type contains, nor does it care which `pace::prefab::behaviors` it inherits from. It only collects dependency information from `pace::details::aspects::EntailOf` and constructs a base class through a dependency resolution algorithm. Overriding it provides no benefit.

Secondly, `pace::MultiBar` and `pace::DynamicBar` explicitly depend on the name `pace::prefab::BasicBar`, so replacing it with a new `BasicBar` would break integration with the existing multi-progress-bar rendering system.

Although components in `pace::prefab::behaviors` are also replaceable, this must be built on top of the internal interfaces required by `pace::MultiBar` and `pace::DynamicBar`, and must also provide a specialization for `pace::details::aspects::EntailOf`.

# Design Notes
## Assertion Checks
pace uses `assert` from `<cassert>` to insert multiple assertion checks in the code. These assertions only take effect when the macro `PACE_DEBUG` is defined and the standard library assertions are enabled.

Most assertions are used to verify the validity of certain internal parameters. Only a small number of assertions are placed in locations such as constructors and assignment operators; these are used to ensure that the current object state matches expectations.

For example, pace does not allow any progress bar object to call `operator=()` or `swap()` when its method `active()` returns `true`. Therefore, assertions in these locations help detect whether such illegal operations occur in the program.

Self-assignment is also checked and rejected via assertions.

## Consistency Between Update Count and Total Task Count
Progress bar types in pace are activated when any `tick` method is called. When the number of completed tasks produced by `tick` reaches the predefined quota, the progress bar automatically stops.

At the same time, pace guarantees that within the same task lifecycle (from the first `tick` call to the last), all calls to `tick` and `reset` are thread-safe.

However, if the total number of `tick` calls exceeds the task quota, this guarantee becomes invalid:

```cxx
pace::ProgressBar<> bar;
bar.config().quota( 1000 );

std::vector<std::thread> pool;
pool.emplace_back( [&]() {
 for ( size_t i = 0; i < 500; ++i )
   bar.tick();
} );
pool.emplace_back( [&]() {
 std::this_thread::sleep_for( std::chrono::milliseconds( 200 ) );
 bar.tick( 700 );
} );
pool.emplace_back( [&]() {
 std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) );
 bar.tick_to( 80 );
} );
```

In the above code, the combined number of `tick` calls across threads exceeds the configured quota of `1000`. Therefore, the excess `(500 + 700 + 1000 * 0.8) - 1000 = 1000` calls are not guaranteed to be thread-safe. These calls may either be discarded or counted into the next runtime cycle of the progress bar.

In addition, `tick_to` only guarantees that the total task progress is **not lower than** the specified percentage after the call. It does not guarantee synchronization of `tick` counts across threads. That is, if multiple threads are concurrently calling `tick`, invoking `tick_to` may result in some `tick` calls being lost.

## Lifecycle of Bar Objects

The lifecycle of each progress bar object strictly follows C++ object lifecycle rules:

- Objects created in a local scope are destroyed when control flow leaves that scope.
- Objects created dynamically exist from `new` until `delete`.
- 
This lifecycle detail is important because a progress bar will immediately terminate during destruction regardless of current iteration progress.

This forced termination differs from calling `reset()`. The `reset()` method allows the progress bar to optionally invoke pre-registered callbacks before stopping. In contrast, destruction-based termination skips this process entirely, immediately shutting down the associated global renderer and cleaning up resources.

> In essence, this is equivalent to calling `abort()`.

A progress bar stopped due to destruction will not append any further output to the terminal, which may cause partial rendering artifacts in the terminal.

## Unicode Support
pace assumes all input strings are encoded in UTF-8 by default. Using any non-UTF-8 encoded string may result in one of the following outcomes:

1. It is considered an incomplete UTF-8 string, and a `pace::exception::InvalidArgument` exception is thrown;
2. It is considered a corrupted UTF-8 string with invalid bytes, and the same exception is thrown;
3. It is considered a non-standard UTF-8 string, resulting in the same exception;
4. It is incorrectly treated as valid UTF-8, with no exception thrown.

pace only processes Unicode encoding related to character types and does not actively modify the terminal encoding environment at runtime.

However, on Windows, when the output stream is bound to the terminal, pace converts internal UTF-8 strings into the corresponding terminal code page encoding via WinAPI before output. This allows correct display without modifying terminal encoding settings.

> pace does not guarantee that corresponding UTF-8 characters have correct font mapping in the terminal encoding environment.

If the program uses C++20 or later, pace also accepts standard library types such as `std::u8string` and `std::u8string_view`.

## Renderer Design
pace adopts a multithreaded cooperative model. The renderer is therefore a background worker thread, implemented as a singleton.

Specifically, each `tick`, `tick_to`, and similar method is treated as a state update, applied to atomic variables inside the progress bar instance. On the `first` tick call of any instance, a task is dispatched to the global singleton renderer, and is cleared after iteration ends.

> When using synchronous rendering mode `pace::Policy::Sync`, rendering is usually not performed by a background thread. Instead, the thread calling `tick` or `tick_to` performs rendering directly.

After task dispatch, the progress bar instance starts the renderer. During this period, threads calling `tick` will wait for the background rendering thread to initialize. Similarly, when stopping, they wait for the renderer thread to suspend.

Progress bar instances may operate on different output streams. Therefore, the global singleton renderer is split into separate instances for `stdout` and `stderr`, which are independent and have no dependency.

Globally, pace enforces that only one progress bar instance per output stream may dispatch tasks to the renderer at any given time.

If multiple progress bar objects are created in the same scope and attempt to dispatch tasks sequentially, the first one succeeds, while subsequent attempts throw a `pace::exception::InvalidState` exception due to the renderer already being occupied.

In multithreaded environments, which thread “dispatches first” depends on the thread scheduling strategy.

```cxx
{
 pace::ProgressBar<> bar1;
 pace::SweepBar<> bar2;
 pace::SpinBar<pace::Channel::Stdout> bar3;

 bar1.config().quota( 100 );
 bar1.tick();

 try {
   bar2.tick(); // Oops!
 } catch ( const pace::exception::InvalidState& e ) {
   std::cerr << std::endl << e.what() << std::endl;
 }

 bar3.tick(); // Ok!
}

pace::ProgressBar<> bar;
bar.config().quota( 100 );

bar.tick(); // Ok!
```

In this code, three different progress bar objects are created:

`bar1` is configured with `bar1.config().quota(100)` and successfully dispatches a task via `bar1.tick()`.

Then `bar2.tick()` throws an exception because the global renderer is already occupied by `bar1`.

`bar3.tick()` succeeds because it uses a different output stream and does not conflict with the occupied renderer.

After the block ends, the previously occupying progress bar objects are destroyed, and newly created `pace::ProgressBar` instances can again successfully dispatch tasks. In other words, the global renderer becomes available again after the previous lifecycle ends.

## Exception Propagation Mechanism
pace involves extensive dynamic memory allocation. Therefore, standard library exceptions may be thrown during most copy, construction, and default initialization processes.

pace internally handles IO operations, and platform-specific exception checks may exist.

On Windows, if pace cannot obtain the standard output handle of the current process, it throws a system-level exception `pace::exception::SystemError`.

## Compilation Time Issues
Because pace heavily uses template metaprogramming to achieve more complex abstractions, using more "static" types (such as `pace::MultiBar`) introduces significant compile-time computation, resulting in slower compilation.

Additionally, all progress bar types in pace are generated entirely at compile time via template metaprogramming rather than being directly encoded in source files. Therefore, long compilation times are an inherent limitation with no simple solution.

## Internal Design
### Base data structure design
From a performance perspective, pace implements many specialized optimized data structures internally, including but not limited to template metaprogramming components, a simplified `std::move_only_function`, number formatting utilities with different implementations depending on the C++ standard, and IO functions that bypass standard library buffering.

These components are internal-only and not guaranteed for external compatibility or usability.
### Progress bar type design
Based on the principles described in [this article](https://zhuanlan.zhihu.com/p/1956112462068815023), pace progress bar types use a Mixin pattern with multiple internal template base classes. These base classes adopt a CRTP design (Curiously Recurring Template Pattern, where a class inherits from a template instantiated with itself), enabling chain-style configuration method calls.

To avoid ambiguous base class construction order caused by multiple inheritance, and to handle limitations where CRTP cannot be directly used in virtual inheritance scenarios, pace uses a compile-time C3 linearization algorithm to generate the final inheritance structure.

C3 linearization is the same algorithm used in languages such as Python to determine consistent method resolution order (MRO).
