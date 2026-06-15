# Changelog
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.2.2] - 2026-06-15
### Added
- None

### Changed
- Replaced SFINAE constraints on some methods with `static_assert` to provide more user-friendly compiler diagnostics
- Optimized CV notification by removing lock on atomic shared state
- Renamed `ProvideFor_v` to `provide_for_v`
- Adjusted the implementation of `UniqueFunction` to conform to the standard

### Fixed
- Fixed incorrect type constraints
- Corrected the outdated docs

### Removed
- None

- - -

## [1.2.1] - 2026-05-20
### Added
- None

### Changed
- Changed the definition methods of some global variables to meet ODR
- Updated the unicode mapping with 17.0 standard code chart
- Adjusted the memory order of atomic variables

### Fixed
- Fixed several compilation errors under different standards
- Fixed the display issue of ETA, Elapsed and Speed under boundary conditions

### Removed
- None

- - -

## [1.2.0] - 2026-05-15
### Added
- Allowed custom format strings for the timer
- Allowed the total number of tasks in the Counter to be hidden
- Allowed any number of speed units to be passed, no longer limiting it to a maximum of 4

### Changed
- Updated the docs

### Fixed
- Added the boundary condition handling code that Speed had omitted

### Removed
- None

- - -

## [1.1.0] - 2026-05-12
### Added
- Added the support of foreground and background colors

### Changed
- Renamed all components related to color
- Updated documents

### Fixed
- Fixed the missing option registry of Postfix

### Removed
- None

- - -

## [1.0.0] - 2026-05-11
### Added
- Added several internal components
- Added a new example file: demo/custom.cpp

### Changed
- Renamed the library from pgbar to pace
- Rewrote prefabs/BasicConfig.hpp and prefabs/BasicBar.hpp; they are now generated entirely at compile time
- Refactored the rendering system; the rendering engine is now generated entirely at compile time
- Reimplemented the rendering logic of `MultiBar` and `DynamicBar` using a state machine
- Changed the semantics of `auto_style_off`
- Split the components of assets/TUI.hpp and assets/Driver.hpp
- Renamed _details to details
- Moved module interface files into the standalone module/ directory

### Fixed
- Fixed an issue where `MultiBar` and `DynamicBar` could not correctly remove completed progress bars in certain cases
- Fixed a division-by-zero error in floating-point scenarios

### Removed
- Removed option/Option.hpp
- Removed several internal types and redundant assertion messages

- - -

## [1.0.0-alpha.4] - 2026-02-05
### Added
- Added rvalue overloads for some methods to allow config types and bar types to call these methods and return rvalue references when in rvalue form
- Added a Copy-on-Write string type so that exception types can contain certain context information

### Changed
- Renamed the internal component `Stringbuf` to `CharPipeline`
- Modified some constraints and return types of internal templates
- Corrected the implementation of some interal backport components
- Modified the implementation of internal traits to ensure that traits returning a bool value strictly derive from either `std::true_type` or `std::false_type`
- Corrected the implementation of NumericSpan

### Fixed
- Fixed a deadlock bug when using pre-c++20 standard
- Fixed the compilation errors that should not exist when compiling without the `/permissive-` mode in MSVC
- Fixed issue #9: no matching function for call to 'max'

### Removed
- Removed some unnecessary type aliases

- - -

## [1.0.0-alpha.3] - 2026-01-03
### Added
- None

### Changed
- Renamed an internal method
- Now the return value of `terminal_witdth()` is 0 instead of 100 when the platform neither Windows nor unix-like
- Modified the synchronization method of the atomic state machine in the renderer

### Fixed
- Fixed a potential data race issue in the `Renderer`
- Fixed the compilation errors on MSVC pointed out in Issue #8
- Corrected some iterator categories

### Removed
- None

- - -

## [1.0.0-alpha.2] - 2025-12-24
### Added
- Marked `tick()` as a pure virtual function in `Indicator`
- Introduced an enum `pgbar::Color` to replace the namespace `pgbar::color`
- Added a new rendering strategy: `Policy::Signal`
- Allowed the config types to use `operator|` for chained calls

### Changed
- The renderer has been re-implemented
- Modified the way to obtain the name of the build artifact in the Release action
- Renamed some functions and types
- Renamed the `set()` method of the config type to `with()`
- Supplemented the docs

### Fixed
- Corrected some incorrect feature testing macros

### Removed
- Removed the duplicate vtable implementation in `DynamicBar`
- Removed the namespace `pgbar::color`
- Removed the symmetric parameter design of `operator|`, and now `operator|` must be called in a left-associative form

- - -

## [1.0.0-alpha.1] - 2025-11-26
### Added
- Introduced CI/CD automated release process
- Added a `CHANGELOG.md`
- Added a `Version.hpp`, which includes multiple macros related to version numbers: `*_MAJOR`, `*_MINOR`, `*_PATCH`, and `*_VERSION`, etc.
- Added a CMake script that automatically parses the version number from `Version.hpp`
- Added a CMake script for handling directory adjustments during packaging

### Changed
- Renamed `main.cpp` to `iterate.cpp`
- Improved the internal template meta-tools
- Modified `.gitignore` to track `.github/`
- Squashed some small, semantically ambiguous or meaningless commits
- Altered the CMake packaging logic, no longer relying on CPack to package the project

### Fixed
- None

### Removed
- None

- - -
