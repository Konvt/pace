[![Ask DeepWiki](https://deepwiki.com/badge.svg)](https://deepwiki.com/Konvt/pace)

## 特点
- **Header-only**: 所有功能全部定义在 `include/pace/`。
- **低运行时开销**: 纳秒级别的调用开销[^1]。
- **C++11 及更高版本兼容**: 支持从 C++11 到 C++23 的所有已发布标准。
- **Unicode 支持**: 按 UTF-8 编码解析每个字符串。
- **RGB 支持**: 可通过十六进制 RGB 值定制进度条颜色。
- **模块化设计**: 可以在编译期生成新的进度条。
- **线程安全设计**: 可以在多线程环境下安全使用。
- **类 `tqdm` 接口**: 基于模板元编程的链式调用方法。

[^1]: 在 AMD Ryzen 7 5800H 上以 `-Og` 优化等级和 `Policy::Async` 执行策略测得，在 tight loop 测试中 `tick` 的平均调用开销 `≤5ns`。

## 样式
### ProgressBar
```
{LeftBorder}{Prefix}{Percent}{Starting}{Filler}{Lead}{Remain}{Ending}{Counter}{Speed}{Elapsed}{ETA}{Postfix}{RightBorder}
 30.87% | [=========>                    ] |  662933732/2147483647 |  11.92 MHz | +00:00:55 | -00:02:03
```
![progressbar](../images/progressbar.gif)
### BlockBar
```
{LeftBorder}{Prefix}{Percent}{Starting}{Filler}{Lead}{Remain}{Ending}{Counter}{Speed}{Elapsed}{ETA}{Postfix}{RightBorder}
 35.22% | ██████████▋                    |  47275560/134217727 |  16.80 MHz | +00:00:02 | -00:00:05
```
![BlockBar](../images/blockbar.gif)
### SpinBar
```
{LeftBorder}{Prefix}{Lead}{Percent}{Counter}{Speed}{Elapsed}{ETA}{Postfix}{RightBorder}
\ |  48.64% |  65288807/134217727 |  17.84 MHz | +00:00:03 | -00:00:03
```
![spinbar](../images/spinbar.gif)
### SweepBar
```
{LeftBorder}{Prefix}{Percent}{Starting}{Filler}{Lead}{Filler}{Ending}{Counter}{Speed}{Elapsed}{ETA}{Postfix}{RightBorder}
 39.82% | [-------<=>--------------------] |  53458698/134217727 |  17.89 MHz | +00:00:02 | -00:00:04
```
![sweepbar](../images/sweepbar.gif)
### FlowBar
```
{LeftBorder}{Prefix}{Percent}{Starting}{Filler}{Lead}{Filler}{Ending}{Counter}{Speed}{Elapsed}{ETA}{Postfix}{RightBorder}
 73.93% | [                     ====     ] |   99224978/134217727 |  81.02 MHz | +00:00:01 | -00:00:00
```
![flowbar](../images/flowbar.gif)
### 自定义进度条
![custom](../images/custom.gif)
### MultiBar
![multibar](../images/multibar.gif)
### DynamicBar
![dynamicbar](../images/dynamicbar.gif)

## 用例
```cpp
#include "pace/ProgressBar.hpp"

int main()
{
  pace::ProgressBar<> bar { pace::option::Remain( "-" ),
                            pace::option::Filler( "=" ),
                            pace::option::Except<>(),
                            pace::option::RemainForecolor( "#A52A2A" ),
                            pace::option::FillerForecolor( 0x0099FF ),
                            pace::option::InfoForecolor( pace::Color::Yellow ),
                            pace::option::Quota( 100 ) };

  for ( auto _ = 0; _ < 100; ++_ )
    bar.tick();
}
```

更多用例详见 [GUIDE.zh.md](GUIDE.zh.md) 及 [demo/](../demo/)。

## FAQ
### 如何构建？
#### 仅头文件
你可以将 `include/pace` 复制到项目的包含路径中，随后在源文件内直接包含对应头文件。
#### C++20 Module
如果你在使用已经支持 `module` 功能的编译器，那么可以使用 `module/` 中的 `pace.cppm` 文件，并在项目中调用它。

在正式使用时，请务必一并在代码文件中 `import std`，否则就目前来看会出现奇怪的编译错误。
#### 发行包
从 [Releases](https://github.com/Konvt/pace/releases) 处获取发行包（zip 文件）。

解压压缩包，并在项目中 `CMakeLists.txt` 的对应位置添加以下代码：

```cmake
# 如果解压后的文件被放在 CMake 可以直接搜索到的路径中时，不需要下面这行
list(APPEND CMAKE_PREFIX_PATH "path/to/package/pace/lib/cmake")
find_package(pace CONFIG REQUIRED)
# ...
add_executable(TargetName ${SOURCES})
target_link_libraries(TargetName PRIVATE pace)
```
#### 子模块（submodule）
使用 `git` 将 `pace` 作为子模块引入你的项目目录中：

```bash
git submodule add https://github.com/Konvt/pace path/to/pace
git submodule update --init --recursive
```

并将以下代码添加到项目中 `CMakeLists.txt` 的对应位置即可。

```cmake
add_subdirectory(path/to/pace)
# ...
add_executable(TargetName ${SOURCES})
target_link_libraries(TargetName PRIVATE pace)
```

> 注意：`pace` 的 `main` 分支为 stable 版本。如果需要获取最新更新，可切换为 `nightly` 分支：

```bash
cd path/to/pace
git checkout nightly
```

可以使用以下命令编译 `demo/` 下的示例文件。

```bash
cmake -S . -DPACE_BUILD_DEMO=ON -B build
cmake --build build --target demo
# 或者使用 demo_{filename} 编译 demo/ 下的指定文件
```

或者在 `demo/` 文件夹中直接使用 `make` 指令编译。

```bash
make all
# 或者使用 {filename} 编译 demo/ 下的指定文件
```
#### 安装
执行以下命令将 `pace` 安装到系统默认目录。

```bash
cmake -S . -DPACE_INSTALL=ON -B build
# 或者安装到指定目录：
# cmake -S . -DPACE_INSTALL=ON -DCMAKE_INSTALL_PREFIX=/usr -B build
cmake --install build
# 等价指令可以是：
# cmake --build build --target install
```

卸载时需要依赖安装时生成的缓存文件；如果缓存文件已经被移除了，那么可以再执行一次安装命令生成。

执行以下命令将 `pace` 从系统中移除。

```bash
cmake --build build --target uninstall
```

### 进度条的更新工作会拖慢程序本身吗？
不，正如[特点](#特点)中指出的，进度条的更新*基本上可以看作*零开销。

实际上进度的更新是均摊开销，任务数量越多，或者任务执行时间越长，均摊到每次更新上的开销就越小。

但具体表现更多取决于处理器的单核性能。
### 可以运行在 Windows/Linux 上吗？
没问题，我写这个库的初衷就是想在两个系统上同时使用一个可视化迭代进度的库。

不过需要注意：如果是在 Windows 平台下，那么 `pace` 会依赖于 `Windows.h` 头文件；而且会定义 `NOMINMAX` 禁用 `min` 与 `max` 宏。
### 支持 Unicode 吗？
正如一开头指出的，没有问题。

虽然目前也仅支持 UTF-8 编码的字符串，使用任何非 UTF-8 编码字符串都会导致异常。

如果你使用的是 C++20 标准，那么 `pace` 的函数也支持 `u8string`。

![unicode](../images/unicode.gif)

## 许可
项目遵从 [MIT](../LICENSE) 许可。
