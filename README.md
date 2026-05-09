[中文文档见此](docs/README.zh.md)。

## Features
- **Header-only design**: All functions are defined in `include/pace/`.
- **Low-overhead updates**: Nanosecond level cost per call.[^1]
- **C++11 & later compatible**: Supports all standard revisions from C++11 through C++23.
- **Unicode support**: Parse each string in UTF-8 encoding.
- **RGB color support**: Customizable progress bar colors.
- **Modular design**: Generate new progress bars during compile-time.
- **Thread-safe design**: Can be safely used in multi-threaded environments.
- **`tqdm`-like interface**: Chainable methods powered by template metaprogramming.

[^1]: On AMD Ryzen 7 5800H with `-Og` optimization and `Policy::Async` execution strategy, `tick` call overhead measured `≤5ns` in tight loop tests.

## Styles
### ProgressBar
```
{LeftBorder}{Prefix}{Percent}{Starting}{Filler}{Lead}{Remain}{Ending}{Counter}{Speed}{Elapsed}{Countdown}{Postfix}{RightBorder}
 30.87% | [=========>                    ] |  662933732/2147483647 |  11.92 MHz | +00:00:55 | -00:02:03
```
![progressbar](images/progressbar.gif)
### BlockBar
```
{LeftBorder}{Prefix}{Percent}{Starting}{Filler}{Lead}{Remain}{Ending}{Counter}{Speed}{Elapsed}{Countdown}{Postfix}{RightBorder}
 35.22% | ██████████▋                    |  47275560/134217727 |  16.80 MHz | +00:00:02 | -00:00:05
```
![BlockBar](images/blockbar.gif)
### SpinBar
```
{LeftBorder}{Prefix}{Lead}{Percent}{Counter}{Speed}{Elapsed}{Countdown}{Postfix}{RightBorder}
\ |  48.64% |  65288807/134217727 |  17.84 MHz | +00:00:03 | -00:00:03
```
![spinbar](images/spinbar.gif)
### SweepBar
```
{LeftBorder}{Prefix}{Percent}{Starting}{Filler}{Lead}{Filler}{Ending}{Counter}{Speed}{Elapsed}{Countdown}{Postfix}{RightBorder}
 39.82% | [-------<=>--------------------] |  53458698/134217727 |  17.89 MHz | +00:00:02 | -00:00:04
```
![sweepbar](images/sweepbar.gif)
### FlowBar
```
{LeftBorder}{Prefix}{Percent}{Starting}{Filler}{Lead}{Filler}{Ending}{Counter}{Speed}{Elapsed}{Countdown}{Postfix}{RightBorder}
 73.93% | [                     ====     ] |   99224978/134217727 |  81.02 MHz | +00:00:01 | -00:00:00
```
![flowbar](images/flowbar.gif)
### Custom
![custom](images/custom.gif)
### MultiBar
![multibar](images/multibar.gif)
### DynamicBar
![dynamicbar](images/dynamicbar.gif)

### Usage
```cpp
#include "pace/ProgressBar.hpp"

int main()
{
  pace::ProgressBar<> bar { pace::option::Remain( "-" ),
                             pace::option::Filler( "=" ),
                             pace::option::Styles( pace::config::Line::Entire ),
                             pace::option::RemainColor( "#A52A2A" ),
                             pace::option::FillerColor( 0x0099FF ),
                             pace::option::InfoColor( pace::color::Yellow ),
                             pace::option::Tasks( 100 ) };

  for ( auto _ = 0; _ < 100; ++_ )
    bar.tick();
}
```

For more examples, see [GUIDE.md](docs/GUIDE.md) and [demo/](demo/).

## FAQ
### How to build?
#### Header-only
You can copy `include/pace` to the inclusion path of the project, and then directly include the corresponding header file within the source file.
#### C++20 Module
If you are using a compiler that already supports the `module` feature, you can use the `pace.cppm` file in the `module/` and import it within your project.

When using it officially, be sure to `import std` in the code file at the same time; otherwise, strange compilation errors will occur as of now.
#### Release package
Download the release package (zip file) from [Releases](https://github.com/Konvt/pace/releases).

Unzip the compressed package and add the following code at the corresponding position in the `CMakeLists.txt` file of the project:

```cmake
# If the decompressed files are placed in a path that CMake can directly search, the following line is not needed.
list(APPEND CMAKE_PREFIX_PATH "path/to/package/pace/lib/cmake")
find_package(pace CONFIG REQUIRED)
# ...
add_executable(TargetName ${SOURCES})
target_link_libraries(TargetName PRIVATE pace)
```
#### Submodule
Use `git` to introduce `pace` as a sub-module into your project directory:

```bash
git submodule add https://github.com/Konvt/pace path/to/pace
git submodule update --init --recursive
```

Then, add the following lines to your `CMakeLists.txt`:

```cmake
add_subdirectory(path/to/pace)
# ...
add_executable(TargetName ${SOURCES})
target_link_libraries(TargetName PRIVATE pace)
```

> Note: The `main` branch of `pace` is the stable version. If you need the latest updates, you can switch to the `nightly` branch:

```bash
cd path/to/pace
git checkout nightly
```

The sample files under `demo/` can be compiled using the following commands.

```bash
cmake -S . -DPACE_BUILD_DEMO=ON -B build
cmake --build build --target demo
# Or use demo_{filename} to compile the specified file under demo/
```

Or compile directly using the `make` command in the `demo/` folder.

```bash
make all
# Or use {filename} to compile the specified file under demo/
```
#### Installation
Execute the following commands to install `pace` to the default directory of the system.

```bash
cmake -S . -DPACE_INSTALL=ON -B build
# Or install it to the specified directory:
# cmake -S . -DPACE_INSTALL=ON -DCMAKE_INSTALL_PREFIX=/usr -B build
cmake --install build
# Equivalent instructions can be:
# cmake --build build --target install
```

When uninstalling, it needs to rely on the cache files generated during installation. If the cache files has been removed, the installation command generation can be executed again.

Execute the following commands to remove `pace` from the system.

```bash
cmake --build build --target uninstall
```

### Does updating the progress bar slow down the program?
No, as mentioned in the [Features](#features) section, updating the progress bar can *essentially be regarded as* zero overhead.

In fact, the progress update is based on the average cost. The more tasks there are or the longer the execution time of each task is, the smaller the cost per update will be.

However, the specific performance largely depends on the single-core performance of the processor.
### Is it compatible with Windows/Linux?
Absolutely. I designed this library to work seamlessly on both systems, providing a unified visualization of iteration progress.

Btw, it should be noted that if it is on the Windows platform, then `pace` will depend on the `Windows.h` header file; Moreover, `NOMINMAX` will be defined to disable the `min` and `max` macros.
### Does it support Unicode?
As pointed out at the beginning, there is no problem.

Although only UTF-8 encoded strings are currently supported, using any non-UTF-8 encoded string will result in an exception.

If you are using the C++20, `pace`'s functions also support `u8string`.

![unicode](images/unicode.gif)

## License
This project is licensed under the [MIT](LICENSE) license.
