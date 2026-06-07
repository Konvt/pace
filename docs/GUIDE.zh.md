**Contents**
- [公共规范](#公共规范)
  - [快速上手](#快速上手)
  - [配置系统](#配置系统)
    - [数据修改](#数据修改)
    - [组件开关](#组件开关)
  - [组件子功能](#组件子功能)
    - [Speed](#speed)
    - [Elapsed 与 ETA](#elapsed-与-eta)
  - [范围迭代支持](#范围迭代支持)
  - [进度条模板参数](#进度条模板参数)
    - [输出流](#输出流)
    - [渲染策略](#渲染策略)
    - [渲染位置](#渲染位置)
  - [回调系统](#回调系统)
  - [隐藏进度条](#隐藏进度条)
- [组件差异](#组件差异)
  - [ProgressBar](#progressbar)
    - [元素构成](#元素构成)
    - [可变的进度条长度](#可变的进度条长度)
  - [BlockBar](#blockbar)
    - [元素构成](#元素构成-1)
    - [可变的进度条长度](#可变的进度条长度-1)
  - [SpinBar](#spinbar)
    - [元素构成](#元素构成-2)
  - [SweepBar](#sweepbar)
    - [元素构成](#元素构成-3)
    - [可变的进度条长度](#可变的进度条长度-2)
  - [FlowBar](#flowbar)
    - [元素构成](#元素构成-4)
    - [可变的进度条长度](#可变的进度条长度-3)
- [独立组件](#独立组件)
  - [MultiBar](#multibar)
    - [快速上手](#快速上手-1)
    - [工厂函数](#工厂函数)
    - [渲染行为](#渲染行为)
    - [元组协议](#元组协议)
  - [DynamicBar](#dynamicbar)
    - [快速上手](#快速上手-2)
    - [辅助函数](#辅助函数)
    - [渲染行为](#渲染行为-1)
  - [NumericSpan](#numericspan)
    - [成员方法](#成员方法)
    - [迭代器类型](#迭代器类型)
  - [IteratorSpan](#iteratorspan)
    - [成员方法](#成员方法-1)
    - [迭代器类型](#迭代器类型-1)
  - [SizedSpan](#sizedspan)
    - [成员方法](#成员方法-2)
    - [迭代器类型](#迭代器类型-2)
  - [TrackedSpan](#trackedspan)
    - [成员方法](#成员方法-3)
    - [迭代器类型](#迭代器类型-3)
  - [iterate](#iterate)
- [组合模型](#组合模型)
  - [模块化组件](#模块化组件)
  - [拼接新的配置类型](#拼接新的配置类型)
  - [自定义组件](#自定义组件)
    - [自定义 facade](#自定义-facade)
    - [自定义渲染](#自定义渲染)
- [设计说明](#设计说明)
  - [断言检查](#断言检查)
  - [更新计数与任务总数一致性](#更新计数与任务总数一致性)
  - [进度条对象的生命周期](#进度条对象的生命周期)
  - [Unicode 支持](#unicode-支持)
  - [渲染器设计](#渲染器设计)
  - [异常传播机制](#异常传播机制)
  - [编译时长问题](#编译时长问题)
  - [内部设计](#内部设计)
    - [基础数据结构设计](#基础数据结构设计)
    - [进度条类型设计](#进度条类型设计)

# 公共规范
## 快速上手
因为设计问题，pace 的*大部分进度条类型的接口都是类似的*，因此我们以 `pace::ProgressBar` 为例介绍如何使用 pace 的各种进度条类型。

进度条是以执行一个固定数量大小的任务为目标设计的，因此几乎所有进度条类型都需要获取一个任务数量参数，然后调用 `tick()` 方法进行迭代。

对于 `pace::ProgressBar` 和 `pace::BlockBar` 来说，不注入初始任务参数就调用 `tick()` 会抛出异常 `pace::exception::InvalidState`。

pace 的所有进度条类型都是模板类型，但模板参数都被赋予了默认值，因此可以直接实例化一个空参数对象。

```cxx
{
  pace::ProgressBar<> bar;
  // C++20 之后可以写成：
  // pace::ProgressBar bar;
  try {
    bar.tick();
  } catch ( const pace::exception::InvalidState& e ) {
    std::cerr << e.what() << std::endl;
  }
}
{
  pace::ProgressBar<> bar;
  //  采用了数据与行为互相分离的设计，因此注入任务参数时必须调用 config() 方法访问内部的配置数据对象
  bar.config().quota( 200 );

  bar.tick( 20 );    // 前进 20 步
  bar.tick_to( 50 ); // 将进度设置为 50%

  for ( int i = 0; i < 100; ++i )
    bar.tick(); // 每次调用仅前进 1 步
}
{
  // 除了调用 config() 方法，注入任务参数也可以通过构造时传递一个带有预期参数的包装类型完成
  pace::ProgressBar<> bar { pace::option::Quota( 150 ) }; // 一般来说包装类型与对应的方法同名
  // 注意：传递重复的参数会导致编译错误
  bar.tick_to( 20 );  // 将进度设置为 20%
  bar.tick_to( 130 ); // 超出 100% 的部分会被丢弃，并将进度条进度锁定到 100%
}
```

如果想要检查进度条运行情况，或是强行终止进度条的运行，那么可以使用 `active()` 和 `reset()` 方法。

```cxx
pace::ProgressBar<> bar { pace::option::Quota( 500 ) };

for ( int i = 0; i < 400; ++i ) {
  if ( i > 0 ) // 要注意，只有调用一次 tick() 后进度条才开始运行
    assert( bar.active() );
  bar.tick();
}

assert( bar.progress() == 400 ); // 该方法可以获取进度条当前的迭代数
bar.reset();
assert( bar.active() == false );
```

所有进度条类型都满足 move only 且 swappable，所以你可以使用另一个对象移动构造，或者与另一个对象交换彼此的配置数据。

```cxx
{
  pace::ProgressBar<> bar1 { /* 传递一些复杂的配置数据 */ };
  pace::ProgressBar<> bar2 { std::move( bar1 ) };
}
{
  pace::ProgressBar<> bar1 { /* 传递一些复杂的配置数据 */ };
  pace::ProgressBar<> bar2;
  bar2.swap( bar1 );
  // or
  using std::swap;
  swap( bar1, bar2 );
}
```

但注意，pace 不认为在进度条**运行过程中**交换或移动对象是一个合法操作，因为这违反了所有权语义，会导致不可预知的错误。

```cxx
pace::ProgressBar<> bar1 { pace::option::Quota( 500 ) };

bar1.tick();
assert( bar1.active() );

// pace::ProgressBar<> bar2 { std::move( bar1 ) }; No!
// 在定义了 PACE_DEBUG 宏的情况下，这种操作会触发断言失败
```

pace 的进度条对象严格遵循 C++ 的生命周期，因此当一个进度条对象被析构时，所有渲染操作都会强制停止且正常释放所有资源。

## 配置系统
### 数据修改
正如前面一节中提到的，进度条对象的几乎所有配置操作都需要经由方法 `config()` 完成。

该方法返回的是对内部配置对象的引用，该配置对象的类型可以在 `pace::config` 中找到。

以 `pace::ProgressBar` 为例，它的配置类型对应的是 `pace::config::Line`。

`pace::config::Line` 是一个纯数据类型，该类型存储了所有用于描述 `pace::ProgressBar` 元素的数据成员；它满足 copyable、movable 和 swappable 三个性质。

```cxx
pace::config::Line cfg1;

auto cfg2 = cfg1;              // copy
auto cfg3 = std::move( cfg1 ); // move
cfg3.swap( cfg2 );             // swap
// or
using std::swap;
swap( cfg2, cfg3 );
```

可以在配置类型所属的进度条类型运行的时候修改、移动或交换配置类型；而且这意味着对配置类型的并发修改是线程安全的。

```cxx
pace::ProgressBar<> bar { pace::option::Quota( 150 ) };

bar.tick_to( 20 );
bar.tick_to( 130 );

bar.config().swap( pace::config::Line() ); // ok
```

注意，这种动态配置数据变更不会影响**已经注入的任务数量**，配置对象变更后，运行时的进度条仍然会保持变更之前的任务数量进行迭代。

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

pace 的所有配置类型都有两种数据注入方式：基于包装器类型的可变参数构造，和基于链式调用的接口。

```cxx
pace::config::Line config1 {
  pace::option::Quota( 100 ),
  pace::option::SpeedUnit( { "B/s", "kiB/s", "MiB/s", "GiB/s" } ),
  pace::option::Magnitude( 1024 ),
  pace::option::InfoForecolor( "#39C5BB" )
  // pace::option::InfoForecolor(0x39C5BB) Don't do that!
};
// 注意：传入多个相同的包装器类型会导致编译错误

pace::config::Line config2;
config2.quota( 100 )
  .speed_unit( { "B/s", "kiB/s", "MiB/s", "GiB/s" } )
  .magnitude( 1024 )
  .info_forecolor( "#39C5BB" );

auto config3 = config2; // 构造后也能使用可变模板参数调整
config3.with( pace::option::Prefix( "Do something" ), pace::option::PrefixForecolor( 0xFFE211 ) );

// 配置类型重载了 operator| 和 operator|=，因此可以像使用管道一样传递参数
auto config4 = pace::config::Line() | pace::option::Quota( 114514 ) | pace::option::Magnitude( 1024 );
```
### 组件开关
不难发现，一个进度条类型是由多个组件构成的，这些组件共同决定了一个进度条的形态。

无论是哪种进度条类型，它的组件开关都可以使用 `pace::option::Only` 或 `pace::option::Except` 完成。

`pace::option::Only` 的含义是：仅保留选中的若干个组件；`pace::option::Except` 的含义是：仅关闭选中的若干个组件。

进度条类型的组件实际上来自于 `pace::facade` 的若干模板基类，这些模板基类会根据[某种编译期算法](#进度条类型设计)在编译期根据需求进行组合，然后生成目标配置类型，之后再根据配置类型得到目标进度条类型。

> 具体信息可以阅读[这一章节](#组合模型)。

总而言之，进度条组件的开关可以这么实现：

```cxx
pace::config::Line pgbar;

// 打开全部组件，然后单独关闭 Percentage 和 Speed
pgbar.enable_all().disable<pace::facade::Percentage, pace::facade::Speed>();
// 上面的操作与下面等价
pgbar.with( pace::option::Except<pace::facade::Percentage, pace::facade::Speed>() );

// 关闭全部组件，然后单独打开进度条 CharPlot、Elapsed 和 ETA
pgbar.disable_all().enable<pace::facade::CharPlot, pace::facade::Elapsed, pace::facade::ETA>();
// 同理
pgbar.with( pace::option::Only<pace::facade::CharPlot, pace::facade::Elapsed, pace::facade::ETA>() );

// 也可以再单独打开某些组件，但是这种操作就不能用 Only 或 Except 完成
pgbar.enable<pace::facade::Speed>();

// 多个 Only 之间可以互相拼接，Except 同理
auto selection = pace::option::Only<pace::facade::CharPlot>() | pace::option::Only<pace::facade::Elapsed>();

// 但是进度条类型不会接受一个包含了不属于它组件的 Only 或 Except
// pgbar.with( selection | pace::option::Only<pace::facade::BlockPlot>() ); No!
```

## 组件子功能
### Speed
所有开启了 `pace::facade::Speed` 的进度条配置类型都可以额外配置速度相关的数据。

一个 `pace::facade::Speed` 由两个部分组成：进位大小 `pace::option::Magnitude` 和单位 `pace::option::SpeedUnit`。

pace 不要求 `pace::option::SpeedUnit` 的单位数量与倍率累计上限严格对应，因此 pace 允许有非常灵活的速率显示方式。

例如 `pace::option::Magnitude( 1024 )` + `pace::option::SpeedUnit( {"B/s", "kiB/s", "MiB/s", "GiB/s"} )` 可以得到每 1024 进 1 的速率显示。

而 `pace::option::Magnitude( 1000 )` + `pace::option::SpeedUnit( {"B/s", "kB/s", "MB/s", "GB/s"} )` 则可以得到每 1000 进 1 的速率显示。

单位数量不足不会导致错误，而是固定在最后一个单位继续显示。若 `pace::option::Magnitude` 不大于 1，会始终输出 `nan.` 而不是速率。

所谓的速率指的是单位时间内，进度条任务数递增的量。速率单位会根据当前速率自动切换。当速率大于等于当前单位对应阈值时，会切换到下一个单位。

特别需要注意的是，如果 `pace::option::Magnitude` 选取的值非常大，例如接近 `UINT16_MAX`，由于 `UINT16_MAX` 的四次方恰小于目前所能表示的数值上限 `UINT64_MAX`，因此这种情况下 `pace::option::SpeedUnit` 的有效单位数量只有 5 个。

且当选取的进位比较大时，倍率累计过程发生了乘法上溢出，则会固定使用最后一个可达单位并输出 `inf`。
### Elapsed 与 ETA
支持 `pace::facade::Elapsed` 或 `pace::facade::Elapsed` 的进度条都可以使用一个格式字符串定义时钟格式；且这两个功能共享同一个格式字符串解析逻辑。

格式字符串的语法结构为：`%[ ':' <fill-char> ][ <width> ]<unit>`。

举例来说，有这样一个格式字符串，它的每个部分解释如下：

```txt
`%: 3H:%2M:%S`
 ^~~~^ ^~^ ^^ --- 时间单位
  ^~^   ^     --- 填充字符和显示宽度限制
      ^   ^   --- 字面量

 %: 3H - `3` 宽度为 3，小于该宽度的十进制数值会用填充字符补齐，大于时则会被固定的 `###` 替代。
         `:` 表示读取且仅读取下一个 Unicode 字符作为填充字符，此处的字符是 ` `。
         `:` 是可选的，而且不支持 Unicode 组合字符。
```

除了控制格式之外的部分会按照字面量解释并原样输出；字符串编码必须采用 UTF-8。

不显式指定宽度时默认取 2，不显式指定填充字符时默认取 `0`。

显示结果固定向右对齐，暂不支持其他对齐方式。

如果某个时间的十进制数值长度超过了指定的宽度限制，则会用与宽度相等的 `#` 字符替换数值。

指定了零宽度、或者传递一个空的格式字符串，会固定输出一个 `?` 字符。

只有最大的单位不会产生进位，其他单位会正常对 60 取模：

```txt
%H:%M:%S -> 01:01:01
%M:%S    -> 61:01
%:4S     -> 3661
```

如果传递的格式字符串不满足以上语法，或者包含重复时间单位，或者一个时间单位都没有，会抛出异常 `pace::exception::InvalidArgument`。

## 范围迭代支持
在处理一些可迭代类型、或者是数值范围的迭代任务时，pace 可以使用 `iterate` 方法将进度条接入到这些场景里。

`iterate` 的使用与 Python 中的 `range` 函数类似，它可以同时在由数值指定的范围上进行遍历；进度条对象的任务数量会由 `iterate` 自动配置。

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

除此之外，进度条对象也可以与满足概念 `std::ranges::sized_range`（不要求使用 C++20）的类型进行交互，例如 `std::vector` 和原始数组。

```cxx
pace::ProgressBar<> bar;

std::vector<int> arr1 {
  0, 1, 2, 3, 4, 5, 6,
};
int arr2[] { 100, 99, 98, 97, 96, 95, 94, 93, 92, 91 };

for ( auto& ele : bar.iterate( arr1.begin(), arr1.end() ) ) {
  ele += 1; // 此处的 ele 是对 vector 内元素的引用
  std::this_thread::sleep_for( 300ms );
}
// 逆序遍历
bar.iterate( arr2, []( int& ) { std::this_thread::sleep_for( 300ms ); } );
```

如果满足 `std::ranges::sized_range` 约束，且使用 C++20 标准，那么 `iterate` 能够正确处理满足概念 `std::ranges::view` 的视图类型的引用生命周期。

## 进度条模板参数
pace 的所有进度条类型都是模板类型，它们需要三个模板参数：`pace::Channel`、`pace::Policy` 和 `pace::Region`。
### 输出流
`pace::Channel` 指定了进度条的输出方向；目前 pace 仅允许在 `pace::Channel::Stderr` 和 `pace::Channel::Stdout` 之间选择，且默认取 `Stderr`。

```cxx
static_assert( std::is_same<pace::ProgressBar<>,
                            pace::ProgressBar<pace::Channel::Stderr>>::value,
                "" );

pace::ProgressBar<pace::Channel::Stdout> bar; // 绑定到 stdout 上
```

pace 会在运行时动态检查指定的输出流是否真实地绑定在一个终端设备上；如果某个输出流并不导向终端，pace 会关闭字符渲染效果以及终端光标操纵。

具体来说，当 pace 发现指定的输出流并不指向一个终端时（可以使用 `pace::config::intty( pace::Channel )` 函数检查），首先会在渲染时关闭所有终端光标操纵序列，然后检查 `pace::config::auto_style_off()` 函数的返回值。若 `pace::config::auto_style_off()` 返回 `true`，pace 会关闭字符渲染效果，此时输出的进度条就是一个纯文本；否则输出的进度条依然会保留为终端渲染设计的字符渲染序列。

默认情况下 `pace::config::auto_style_off()` 总是返回 `true`，但可以调用 `pace::config::auto_style_off( bool )` 修改其返回值。

特别的，在 include pace 库之前，如果定义了宏 `PACE_INTTY`，则会强制令 pace 认为任何输出流都绑定到终端上；如果定义了宏 `PACE_NOSTYLE`，则会强制令 pace 关闭所有字符渲染效果。

需要注意的是，绑定到相同输出流上的所有进度条对象，在同一时刻至多只允许一个在运行，否则会抛出异常 `pace::exception::InvalidState`。

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

如果需要在同一个输出流上渲染多个进度条，请使用 `pace::MultiBar` 或 `pace::DynamicBar`。
### 渲染策略
`pace::Policy` 决定了进度条的渲染策略方式；目前 pace 有三个 `Policy` 参数：`pace::Policy::Async`、`pace::Policy::Signal` 和 `pace::Policy::Sync`，不同的渲染策略决定了由哪个线程负责执行渲染行为。

在 `pace::Policy::Async` 模式下，所有渲染行为都由渲染器的后台线程执行，渲染行为与前台线程完全脱离，且每次渲染输出后该线程都会休眠一定时间，这个休眠的时间可以通过 `pace::config::refresh_interval()` 查看，使用 `pace::config::refresh_interval( pace::details::types::Tempus )` 修改。

在 `pace::Policy::Signal` 模式下，每次调用 `tick()` 或 `tick_to()` 时，调用该方法的线程会向渲染器提交一个渲染请求，每个渲染任务依然由后台线程执行，但每次执行渲染任务后不会休眠。如果短时间内有多个任务同时提交给渲染器，那么渲染器会尽最大可能快速消耗这些任务请求；但由于异步执行性质，每个渲染行为的实际执行时间会略微晚于调用 `tick()` 或 `tick_to()` 的时间。

如果在任务数量消耗完之前，进度条触发了 `reset()` 或 `abort()`，那么剩余渲染任务会被丢弃（但不会影响实际渲染效果）。即 `pace::Policy::Signal` 只保证渲染次数不大于调用 `tick()` 或 `tick_to()` 的次数。

在 `pace::Policy::Sync` 模式下，渲染动作由每次调用 `tick()` 或 `tick_to()` 方法的线程直接执行。也即每次调用 `tick()` 不仅会更新进度状态，也会立即将最新进度条输出到终端。此时进度条的每一次渲染都严格匹配 `tick()` 或 `tick_to()` 的调用。

```cxx
static_assert( std::is_same<pace::ProgressBar<>,
                            pace::ProgressBar<pace::Channel::Stderr, pace::Policy::Async>>::value,
                "" );

pace::ProgressBar<pace::Channel::Stderr, pace::Policy::Sync> bar; // 使用同步渲染
```
### 渲染位置
`pace::Region` 决定了进度条在终端上的渲染位置；目前 pace 仅允许在 `pace::Region::Fixed` 和 `pace::Region::Relative` 之间选择，且默认取 `Fixed`。

`pace::Region::Fixed` 会在首次渲染时保存当前终端光标位置，并始终相对该位置刷新进度条；此时同一输出流上的其他内容都会被进度条刷新覆盖。

`pace::Region::Relative` 会根据上一次渲染输出的行数、回退并覆盖旧进度条内容；此时向同一输出流写入信息后，若额外添加适当数量的换行符，那么写入的额外信息能够得到保留。

但如果进度条字符串宽度过长，使用 `pace::Region::Relative` 会导致终端渲染异常。

> 任意一个进度条的渲染结构共包含两个终端行（假定进度条长度不超过单行字符上限）：一行是进度条本身，一行则是空行；

使用 `pace::Region::Relative` 模式同时输出进度条以及自定义信息时，一般必须配合使用 `pace::Policy::Signal` 或 `pace::Policy::Sync`，否则会因为异步渲染机制导致终端滚动。

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

## 回调系统
pace 的所有进度条类型都提供了一个 `action` 方法，该方法可以接收或清除一个 `void()` 或 `void( /* Bar Type */& )` 类型的函数回调。

这个回调会在进度条对象调用 `reset()` 方法时、终止进度条渲染（`active()` 函数返回值从 `true` 切换为 `false`）**之前**被调用。

同样的，如果进度条正常运行且正常终止，那么进度条类型会在内部自己调用 `reset()` 方法，此时回调函数依然会在进度条终止之前被调用。

支持 `action` 的进度条对象都包含了 `operator|` 和 `operator|=` 重载，可以使用该运算符直接传递回调。

```cxx
pace::ProgressBar<> bar;
bool flag = true;
auto callback = [&]( pace::ProgressBar<>& self ) {
  if ( flag )
    self.config().prefix( "✔ Mission Accomplished" ).prefix_forecolor( pace::Color::Green );
  else
    self.config().prefix( "❌ Mission failed" ).prefix_forecolor( pace::Color::Red );
};

bar.action( callback );
// or
bar |= callback;
// or
bar | callback;
```

传递的回调函数类型必须满足 `std::is_move_constructible`；而且**绝对不应该**在回调内部调用*修改进度条对象自身状态*的方法（例如 `tick` 或 `reset`，但 `config` 除外），否则会导致*死锁*。

如果希望手动终止对象运行、并且跳过回调函数的执行，可以调用对象的 `abort` 方法；而且对象析构导致的渲染终止也不会执行预设的回调函数。

## 隐藏进度条
pace 允许自动隐藏已经完成的进度条字符串。当函数 `pace::config::hide_completed()` 返回 `true` 时，只要进度条的 `active()` 方法从 `true` 变为 `false`，且指定输出流绑定到终端上，那么这个已完成的进度条会被立即隐藏。

可以调用 `pace::config::hide_completed( bool )` 修改实际行为，默认值是 `false`。

# 组件差异
## ProgressBar
在运行期行为上，当 `config().quota()` 为 0 而试图调用 `tick` 方法，`pace::ProgressBar` 会抛出 `pace::exception::InvalidState` 异常。
### 元素构成
`pace::ProgressBar` 由以下几种元素组成：

```text
{LeftBorder}{Prefix}{Percent}{Starting}{Filler}{Lead}{Remain}{Ending}{Counter}{Speed}{Elapsed}{ETA}{Postfix}{RightBorder}
```

其中可以自定义的部分有：`LeftBorder`、`Prefix`、`Starting`、`Filler`、`Lead`、`Remain`、`Ending`、`Speed`、`Postfix` 和 `RightBorder`，它们的功能与名字相同。

这些元素可以直接在 `pace::option` 中找到对应的包装类型：

```cxx
pace::option::Colored;       // 颜色效果
pace::option::FontBold;      // 字体加粗
pace::option::FontFaint;     // 字体暗淡
pace::option::FontItalic;    // 斜体效果
pace::option::FontUnderline; // 字体下划线
pace::option::FontInverse;   // 字体反显
pace::option::FontHidden;    // 隐藏字体
pace::option::FontCrossed;   // 删除线

pace::option::LeftBorder;  // 修改整个进度条左侧的起始边框
pace::option::RightBorder; // 修改整个进度条右侧的终止边框

pace::option::Prefix;      // 修改前置描述信息
pace::option::Postfix;     // 修改尾随描述信息

pace::option::Starting;  // 修改进度条块左侧、Percent 右侧的元素
pace::option::Ending;    // 修改进度条块右侧、Counter 左侧的元素
pace::option::Filler;    // 修改已迭代部分的填充字符
pace::option::Lead;      // 修改可变动画部分的各个帧
pace::option::Remain;    // 修改未迭代部分的填充字符
pace::option::Reversed;  // 调整进度条的增长方向（false 表示从左到右）
pace::option::Shift;     // 调整动画部分（Lead）的动画速度
pace::option::BarWidth;  // 调整进度条的宽度

pace::option::SpeedUnit; // 修改 Speed 部分的单位
pace::option::Magnitude; // 调整 Speed 部分的进位倍率

pace::option::Quota;   // 调整任务数量
pace::option::Divider; // 修改位于两个元素之间的间隔符

pace::option::PrefixForecolor;  // 修改 Prefix 的前景色
pace::option::PrefixBackcolor;  // 修改 Prefix 的背景色
pace::option::PostfixForecolor; // 修改 Postfix 的前景色
pace::option::PostfixBackcolor; // 修改 Postfix 的背景色
pace::option::StartForecolor;   // 修改 Starting 的前景色
pace::option::StartBackcolor;   // 修改 Starting 的背景色
pace::option::EndForecolor;     // 修改 Ending 的前景色
pace::option::EndBackcolor;     // 修改 Ending 的背景色
pace::option::FillerForecolor;  // 修改 Filler 的前景色
pace::option::FillerBackcolor;  // 修改 Filler 的背景色
pace::option::RemainForecolor;  // 修改 Remain 的前景色
pace::option::RemainBackcolor;  // 修改 Remain 的背景色
pace::option::LeadForecolor;    // 修改 Lead 的前景色
pace::option::LeadBackcolor;    // 修改 Lead 的背景色
pace::option::InfoForecolor;    // 修改 Divider、Percent、Counter、Speed、Elapsed 和 ETA 的前景色
pace::option::InfoBackcolor;    // 修改 Divider、Percent、Counter、Speed、Elapsed 和 ETA 的背景色
```

且配置类型具有同名但命名风格不同的方法。
### 可变的进度条长度
在元素 `Starting` 和 `Ending` 中间部分的是被称作 `CharPlot` 的进度指示器（不包括 `Starting` 和 `Ending`），这个进度指示器的宽度是可变的。

每个进度条都有一个默认的初始宽度（30 字符），如果希望进度条能够填满一个终端行，或者进度条太长需要缩窄，就需要使用到 `bar_width()` 方法或 `pace::option::BarWidth` 包装器更改进度指示器的宽度。

如果希望进度条能够恰好占满一整个终端行，pace 的进度条类型提供了 `config().fixed_width()` 方法提供除了进度指示器之外部分的宽度。

> 请不要直接调用一个**裸的**配置对象的 `fixed_width()` 方法，具体原因请参照代码注释（[BasicConfig.hpp](../include/pace/prefab/BasicConfig.hpp#L284)）。

```cxx
pace::config::Line cfg;
// cfg.fixed_width(); Never do this!

pace::ProgressBar<> bar;
assert( bar.config().bar_width() == 30 );  // 默认值
assert( bar.config().fixed_width() != 0 ); // 具体值取决于数据成员的内容
```

具体的终端行宽度（以字符为单位）可以使用 `pace::config::terminal_width()` 获取；如果传递的输出流不指向实际终端设备，那么返回值为 0。

> 如果运行平台既不是 `Windows` 也不是 `unix-like`，那么该函数只会返回一个固定值 0。

```cxx
pace::ProgressBar<> bar;

assert( pace::config::terminal_width( pace::Channel::Stderr ) > bar.config().fixed_width() );
bar.config().bar_width( pace::config::terminal_width( pace::Channel::Stderr )
                          - bar.config().fixed_width() );
```

需要注意，如果打开了 `pace::facade::Counter` 组件，那么进度条的长度还会受到当前任务数量影响；此时必须先配置了任务数量，才能得到一个正确的进度条长度。

## BlockBar
在运行期行为上，当 `config().quota()` 为 0 而试图调用 `tick` 方法，`pace::BlockBar` 会抛出 `pace::exception::InvalidState` 异常。
### 元素构成
`pace::BlockBar` 由以下几种元素组成：

```text
{LeftBorder}{Prefix}{Percent}{Starting}{Filler}{Lead}{Remain}{Ending}{Counter}{Speed}{Elapsed}{ETA}{Postfix}{RightBorder}
```

其中可以自定义的部分有：`LeftBorder`、`Prefix`、`Starting`、`Lead`、`Filler`、`Remain`、`Ending`、`Speed`、`Postfix` 和 `RightBorder`，它们的功能与名字相同。

这些元素可以直接在 `pace::option` 中找到对应的包装类型：

```cxx
pace::option::Colored;       // 颜色效果
pace::option::FontBold;      // 字体加粗
pace::option::FontFaint;     // 字体暗淡
pace::option::FontItalic;    // 斜体效果
pace::option::FontUnderline; // 字体下划线
pace::option::FontInverse;   // 字体反显
pace::option::FontHidden;    // 隐藏字体
pace::option::FontCrossed;   // 删除线

pace::option::LeftBorder;  // 修改整个进度条左侧的起始边框
pace::option::RightBorder; // 修改整个进度条右侧的终止边框

pace::option::Prefix;      // 修改前置描述信息
pace::option::Postfix;     // 修改尾随描述信息

pace::option::Starting;  // 修改进度条块左侧、Percent 右侧的元素
pace::option::Ending;    // 修改进度条块右侧、Counter 左侧的元素
pace::option::Lead;      // 修改可变动画部分的各个帧
pace::option::Filler;    // 修改已迭代部分的填充字符
pace::option::Remain;    // 修改未迭代部分的填充字符
pace::option::Reversed;  // 调整进度条的增长方向（false 表示从左到右）
pace::option::BarWidth;  // 调整进度条的宽度

pace::option::SpeedUnit; // 修改 Speed 部分的单位
pace::option::Magnitude; // 调整 Speed 部分的进位倍率

pace::option::Quota;   // 调整任务数量
pace::option::Divider; // 修改位于两个元素之间的间隔符

pace::option::PrefixForecolor;  // 修改 Prefix 的前景色
pace::option::PrefixBackcolor;  // 修改 Prefix 的背景色
pace::option::PostfixForecolor; // 修改 Postfix 的前景色
pace::option::PostfixBackcolor; // 修改 Postfix 的背景色
pace::option::StartForecolor;   // 修改 Starting 的前景色
pace::option::StartBackcolor;   // 修改 Starting 的背景色
pace::option::EndForecolor;     // 修改 Ending 的前景色
pace::option::EndBackcolor;     // 修改 Ending 的背景色
pace::option::FillerForecolor;  // 修改 Filler 的前景色
pace::option::FillerBackcolor;  // 修改 Filler 的背景色
pace::option::RemainForecolor;  // 修改 Remain 的前景色
pace::option::RemainBackcolor;  // 修改 Remain 的背景色
pace::option::LeadForecolor;    // 修改 Lead 的前景色
pace::option::LeadBackcolor;    // 修改 Lead 的背景色
pace::option::InfoForecolor;    // 修改 Divider、Percent、Counter、Speed、Elapsed 和 ETA 的前景色
pace::option::InfoBackcolor;    // 修改 Divider、Percent、Counter、Speed、Elapsed 和 ETA 的背景色
```

且配置类型具有同名但命名风格不同的方法。
### 可变的进度条长度
在元素 `Starting` 和 `Ending` 中间部分的是被称作 `BlockPlot` 的进度指示器（不包括 `Starting` 和 `Ending`），这个进度指示器的宽度是可变的。

`pace::BlockBar` 的这部分行为表现与 `pace::ProgressBar` 一致，可以参阅之前的章节。

## SpinBar
在运行期行为上，`pace::SpinBar` 不需要总是配置任务数量，反之可以直接调用 `tick` 立即开始运行。
### 元素构成
`SpinBar` 由以下几种元素组成：

```text
{LeftBorder}{Prefix}{Lead}{Percent}{Counter}{Speed}{Elapsed}{ETA}{Postfix}{RightBorder}
```

其中可以自定义的部分有：`LeftBorder`、`Prefix`、`Lead`、`Speed`、`Postfix` 和 `RightBorder`，它们的功能与名字相同。

这些元素可以直接在 `pace::option` 中找到对应的包装类型：

```cxx
pace::option::Colored;       // 颜色效果
pace::option::FontBold;      // 字体加粗
pace::option::FontFaint;     // 字体暗淡
pace::option::FontItalic;    // 斜体效果
pace::option::FontUnderline; // 字体下划线
pace::option::FontInverse;   // 字体反显
pace::option::FontHidden;    // 隐藏字体
pace::option::FontCrossed;   // 删除线

pace::option::LeftBorder;  // 修改整个进度条左侧的起始边框
pace::option::RightBorder; // 修改整个进度条右侧的终止边框

pace::option::Prefix;      // 修改前置描述信息
pace::option::Postfix;     // 修改尾随描述信息

pace::option::Lead;      // 修改可变动画部分的各个帧
pace::option::Shift;     // 调整动画部分（Lead）的动画速度

pace::option::SpeedUnit; // 修改 Speed 部分的单位
pace::option::Magnitude; // 调整 Speed 部分的进位倍率

pace::option::Quota;   // 调整任务数量
pace::option::Divider; // 修改位于两个元素之间的间隔符

pace::option::PrefixForecolor;  // 修改 Prefix 的前景色
pace::option::PrefixBackcolor;  // 修改 Prefix 的背景色
pace::option::PostfixForecolor; // 修改 Postfix 的前景色
pace::option::PostfixBackcolor; // 修改 Postfix 的背景色
pace::option::LeadForecolor;    // 修改 Lead 的前景色
pace::option::LeadBackcolor;    // 修改 Lead 的背景色
pace::option::InfoForecolor;    // 修改 Divider、Percent、Counter、Speed、Elapsed 和 ETA 的前景色
pace::option::InfoBackcolor;    // 修改 Divider、Percent、Counter、Speed、Elapsed 和 ETA 的背景色
```

且配置类型具有同名但命名风格不同的方法。

`pace::SpinBar` 不包含进度条。

## SweepBar
在运行期行为上，`pace::SweepBar` 不需要总是配置任务数量，反之可以直接调用 `tick` 立即开始运行。
### 元素构成
`SweepBar` 由以下几种元素组成：

```text
{LeftBorder}{Prefix}{Percent}{Starting}{Filler}{Lead}{Filler}{Ending}{Counter}{Speed}{Elapsed}{ETA}{Postfix}{RightBorder}
```

其中可以自定义的部分有：`LeftBorder`、`Prefix`、`Starting`、`Filler`、`Lead`、`Ending`、`Speed`、`Postfix` 和 `RightBorder`，它们的功能与名字相同。

这些元素可以直接在 `pace::option` 中找到对应的包装类型：

```cxx
pace::option::Colored;       // 颜色效果
pace::option::FontBold;      // 字体加粗
pace::option::FontFaint;     // 字体暗淡
pace::option::FontItalic;    // 斜体效果
pace::option::FontUnderline; // 字体下划线
pace::option::FontInverse;   // 字体反显
pace::option::FontHidden;    // 隐藏字体
pace::option::FontCrossed;   // 删除线

pace::option::LeftBorder;  // 修改整个进度条左侧的起始边框
pace::option::RightBorder; // 修改整个进度条右侧的终止边框

pace::option::Prefix;      // 修改前置描述信息
pace::option::Postfix;     // 修改尾随描述信息

pace::option::Starting;  // 修改进度条块左侧、Percent 右侧的元素
pace::option::Ending;    // 修改进度条块右侧、Counter 左侧的元素
pace::option::Filler;    // 修改进度条背景的填充字符
pace::option::Lead;      // 修改可变动画部分的各个帧
pace::option::Shift;     // 调整动画部分（Lead）的动画速度
pace::option::BarWidth;  // 调整进度条的宽度

pace::option::SpeedUnit; // 修改 Speed 部分的单位
pace::option::Magnitude; // 调整 Speed 部分的进位倍率

pace::option::Quota;   // 调整任务数量
pace::option::Divider; // 修改位于两个元素之间的间隔符

pace::option::PrefixForecolor;  // 修改 Prefix 的前景色
pace::option::PrefixBackcolor;  // 修改 Prefix 的背景色
pace::option::PostfixForecolor; // 修改 Postfix 的前景色
pace::option::PostfixBackcolor; // 修改 Postfix 的背景色
pace::option::StartForecolor;   // 修改 Starting 的前景色
pace::option::StartBackcolor;   // 修改 Starting 的背景色
pace::option::EndForecolor;     // 修改 Ending 的前景色
pace::option::EndBackcolor;     // 修改 Ending 的背景色
pace::option::FillerForecolor;  // 修改 Filler 的前景色
pace::option::FillerBackcolor;  // 修改 Filler 的背景色
pace::option::LeadForecolor;    // 修改 Lead 的前景色
pace::option::LeadBackcolor;    // 修改 Lead 的背景色
pace::option::InfoForecolor;    // 修改 Divider、Percent、Counter、Speed、Elapsed 和 ETA 的前景色
pace::option::InfoBackcolor;    // 修改 Divider、Percent、Counter、Speed、Elapsed 和 ETA 的背景色
```

且配置类型具有同名但命名风格不同的方法。
### 可变的进度条长度
在元素 `Starting` 和 `Ending` 中间部分的是被称作 `SweepPlot` 的扫描进度条（不包括 `Starting` 和 `Ending`），这个扫描进度条的宽度是可变的。

`pace::SweepBar` 的这部分行为表现与 `pace::ProgressBar` 一致，可以参阅之前的章节。

## FlowBar
在运行期行为上，`pace::SweepBar` 不需要总是配置任务数量，反之可以直接调用 `tick` 立即开始运行。
### 元素构成
`FlowBar` 由以下几种元素组成：

```text
{LeftBorder}{Prefix}{Percent}{Starting}{Filler}{Lead}{Filler}{Ending}{Counter}{Speed}{Elapsed}{ETA}{Postfix}{RightBorder}
```

其中可以自定义的部分有：`LeftBorder`、`Prefix`、`Starting`、`Filler`、`Lead`、`Ending`、`Speed`、`Postfix` 和 `RightBorder`，它们的功能与名字相同。

这些元素可以直接在 `pace::option` 中找到对应的包装类型：

```cxx
pace::option::Colored;       // 颜色效果
pace::option::FontBold;      // 字体加粗
pace::option::FontFaint;     // 字体暗淡
pace::option::FontItalic;    // 斜体效果
pace::option::FontUnderline; // 字体下划线
pace::option::FontInverse;   // 字体反显
pace::option::FontHidden;    // 隐藏字体
pace::option::FontCrossed;   // 删除线

pace::option::LeftBorder;  // 修改整个进度条左侧的起始边框
pace::option::RightBorder; // 修改整个进度条右侧的终止边框

pace::option::Prefix;      // 修改前置描述信息
pace::option::Postfix;     // 修改尾随描述信息

pace::option::Starting;  // 修改进度条块左侧、Percent 右侧的元素
pace::option::Ending;    // 修改进度条块右侧、Counter 左侧的元素
pace::option::Filler;    // 修改进度条背景的填充字符
pace::option::Lead;      // 修改可变动画部分的各个帧
pace::option::Shift;     // 调整动画部分（Lead）的动画速度
pace::option::BarWidth;  // 调整进度条的宽度

pace::option::SpeedUnit; // 修改 Speed 部分的单位
pace::option::Magnitude; // 调整 Speed 部分的进位倍率

pace::option::Quota;   // 调整任务数量
pace::option::Divider; // 修改位于两个元素之间的间隔符

pace::option::PrefixForecolor;  // 修改 Prefix 的前景色
pace::option::PrefixBackcolor;  // 修改 Prefix 的背景色
pace::option::PostfixForecolor; // 修改 Postfix 的前景色
pace::option::PostfixBackcolor; // 修改 Postfix 的背景色
pace::option::StartForecolor;   // 修改 Starting 的前景色
pace::option::StartBackcolor;   // 修改 Starting 的背景色
pace::option::EndForecolor;     // 修改 Ending 的前景色
pace::option::EndBackcolor;     // 修改 Ending 的背景色
pace::option::FillerForecolor;  // 修改 Filler 的前景色
pace::option::FillerBackcolor;  // 修改 Filler 的背景色
pace::option::LeadForecolor;    // 修改 Lead 的前景色
pace::option::LeadBackcolor;    // 修改 Lead 的背景色
pace::option::InfoForecolor;    // 修改 Divider、Percent、Counter、Speed、Elapsed 和 ETA 的前景色
pace::option::InfoBackcolor;    // 修改 Divider、Percent、Counter、Speed、Elapsed 和 ETA 的背景色
```

且配置类型具有同名但命名风格不同的方法。
### 可变的进度条长度
在元素 `Starting` 和 `Ending` 中间部分的是被称作 `FlowPlot` 的扫描进度条（不包括 `Starting` 和 `Ending`），这个扫描进度条的宽度是可变的。

`pace::FlowBar` 的这部分行为表现与 `pace::ProgressBar` 一致，可以参阅之前的章节。

# 独立组件
## MultiBar
`pace::MultiBar` 是一个 tuple-like 类型，它并不是一个实际的进度条类型，而必须接收多个不同类型的进度条对象，将它们组合起来，才能实现多进度条的输出。
### 快速上手
`pace::MultiBar` 要求它持有的所有对象都必须具有相同的模板参数，但配置类型可以不同。

```cxx
pace::MultiBar<pace::ProgressBar<>, pace::ProgressBar<>, pace::BlockBar<>> mbar1;
// or
pace::MultiBar<pace::ProgressBar<pace::Channel::Stdout>,
                pace::ProgressBar<pace::Channel::Stdout>,
                pace::ProgressBar<pace::Channel::Stdout>>
  mbar2;

// 如果需要构造包含多个重复进度条类型的 MultiBar，可以使用 MultiBar_t
pace::MultiBar_t<pace::ProgressBar<pace::Channel::Stdout>, 3> mbar3;
static_assert( std::is_same_v<decltype( mbar2 ), decltype( mbar3 )> );

mbar1.config<0>().quota( 100 );
mbar1.config<1>().quota( 200 );
mbar1.config<2>().quota( 300 );

// 可以直接访问对应的进度条对象
mbar1.at<0>().tick();
// 也可以间接访问
mbar1.tick<1>();
// 还可以使用无名称限定的 get() 调用访问
using std::get;
get<2>( mbar1 );

// 不带模板参数的方法表示访问 MultiBar 对象本身
assert( mbar1.active() );

// do tasks...
```

`pace::MultiBar` 的构造函数可以接受单独的进度条对象，也可以接受这些进度条对象的配置类型。如果使用的 C++ 标准高于 C++17，pace 还为 `pace::MultiBar` 添加了一个类型模板推导指引。

```cxx
// MultiBar 要求它的模板参数列表中的所有类型，必须具有相同输出流属性和执行策略
pace::ProgressBar<> bar1;
pace::BlockBar<> bar2, bar3;

// 由于 bar 是 move-only 对象，因此此处必须使用 std::move
auto mbar1 = pace::MultiBar<pace::ProgressBar<>, pace::BlockBar<>, pace::BlockBar<>>( std::move( bar1 ),
                                                                                      std::move( bar2 ),
                                                                                      std::move( bar3 ) );
auto mbar2 =
  pace::MultiBar<pace::ProgressBar<>, pace::BlockBar<>, pace::ProgressBar<>>( pace::config::Line(),
                                                                              pace::config::Block(),
                                                                              pace::config::Line() );

#if __cplusplus >= 201703L
// 如果在 C++17 之后，以下语句将是合法的
auto mbar3 = pace::MultiBar( pace::config::Line(), pace::config::Block(), pace::config::Line() );
// 这个对象的类型将会是指向 pace::Channel::Stderr 的 MultiBar

static_assert( std::is_same<decltype( mbar3 ), decltype( mbar2 )>::value );
#endif
```

进度条类型的所有方法都可以在 `pace::MultiBar` 中以模板函数的方式访问；某种程度上来说，`pace::MultiBar` 更像是一个容器而非进度条类型。

与独立进度条类型相同，`pace::MultiBar` 也是一个 movable 且 swappable 的类型；并且也同样不应该在 `pace::MultiBar` 运行过程中移动或交换它。

同时在多个线程中尝试 move 或 swap `pace::MultiBar`、或者在某个线程中 move 或 swap 而在其他线程中调用 `pace::MultiBar` 的其他方法，是线程不安全的。
### 工厂函数
pace 提供了多个名为 `make_multi` 的重载函数，以简化构造 `pace::MultiBar` 的构造操作。

这些函数及其作用分别是：

```cxx
// 创建与参数数量相同大小的 MultiBar
auto bar1 = pace::make_multi<pace::Channel::Stdout>( pace::config::Line(), pace::config::Block() );
auto bar2 = pace::make_multi<>( pace::ProgressBar<>(), pace::BlockBar<>() );

// 创建一个固定长度、所有进度条类型都相同的 MultiBar，并使用参数提供的配置对象初始化内部所有进度条对象
auto bar3 = pace::make_multi<6, pace::Channel::Stdout>( pace::config::Spin() );
auto bar4 = pace::make_multi<6>( pace::SpinBar<pace::Channel::Stdout>() );
// bar3 和 bar4 内部的所有进度条的配置数据都是相同的

// 创建一个固定长度、所有进度条类型都相同的 MultiBar，提供的参数会按顺序作用在内部的进度条对象上
auto bar5 = pace::make_multi<pace::config::Sweep, 3>( pace::config::Sweep() );
auto bar6 =
  pace::make_multi<pace::SweepBar<pace::Channel::Stdout>, 3>( pace::SweepBar<pace::Channel::Stdout>() );
// bar5 和 bar6 只有第一个进度条对象被初始化为参数指定的内容，其他两个进度条均被默认初始化
```
### 渲染行为
`pace::MultiBar` 的渲染策略与其他进度条相同，但渲染行为略有差异。

因为 `pace::MultiBar` 会在多行同时渲染多个进度条，使用 `pace::Region::Relative` 时，进度条的渲染结构所占的行数将会由 `pace::MultiBar` 容纳的进度条类型数量决定。

`pace::MultiBar` 容纳的进度条数量可由 `active_count()` 方法得到，而渲染结构所占行数将会是该方法返回的数量 +1。

示例：

```cxx
// Since the newline character is output successively here,
// the scheduling strategy has chosen synchronization to avoid inconsistent output behavior
auto bar = pace::make_multi<pace::Channel::Stderr, pace::Policy::Sync, pace::Region::Relative>(
  pace::config::Line( pace::option::Quota( 100 ) ),
  pace::config::Line( pace::option::Quota( 150 ) ),
  pace::config::Line( pace::option::Quota( 200 ) ) );

for ( size_t i = 0; i < 95; ++i ) {
  bar.tick<0>();
  bar.tick<1>();
}

std::cerr << "Extra log information";
// Notice: At least `active_count() + 1` nextline must be inserted after the output information
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
### 元组协议
`pace::MultiBar` 为标准库的 `std::tuple_element` 和 `std::tuple_size` 提供了特化实现，同时提供了 `get` 的重载版本；因此可以将 `pace::MultiBar` 视作是一个特殊版本的 `std::tuple`。

在 C++17 以上标准时，可以对 `pace::MultiBar` 使用结构化绑定：

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
与 `pace::MultiBar` 相反，`pace::DynamicBar` 是一个工厂类型，它几乎不持有任何数据，只负责在不同的进度条类型之间建立起一定的生命周期关系。
### 快速上手
`pace::DynamicBar` 与其他类型不同，它接收进度条类型或配置类类型，并返回一个指向对应进度条类型的 `std::unique_ptr` 对象；所有调用进度条方法的行为都需要解引用这个返回的指针对象。`pace::DynamicBar` 所返回的每一个 `std::unique_ptr` 都可以开启终端的进度条渲染；但只有所有的 `std::unique_ptr` 都被析构或者停止运行，终端渲染工作才会停止。

`pace::DynamicBar` 可以在已创建多个 `std::unique_ptr` 的情况下被析构，这只会导致不能再查看这个 `pace::DynamicBar` 是否正在运行，并且也不能经由这个 `pace::DynamicBar` 关闭所有由它创建的 `std::unique_ptr` 指向的进度条对象。

如果 `pace::DynamicBar` 返回的 `std::unique_ptr` 对象被析构，那么如果此时该指针指向的进度条对象正在运行中，`pace::DynamicBar` 能够识别出对象失效并将它移除出渲染列表。

综上所述，`pace::DynamicBar` 可以在运行时接收任意多的进度条对象，在后台协调它们向终端渲染的顺序；进度条的输出顺序将取决于它们被启动时的时间，越晚启动的进度条会出现在终端更下方。

此外，同时在多个线程中尝试 move 或 swap `pace::DynamicBar`、或者在某个线程中 move 或 swap 而在其他线程中调用 `pace::DynamicBar` 的其他方法，是线程不安全的。

同理，`pace::DynamicBar` 也要求所有传入的进度条类型必须具有相同的模板参数。

```cxx
std::vector<std::thread> pool;
{
  pace::DynamicBar<> dbar;

  auto bar1 = dbar.insert<pace::ProgressBar<>>();
  // bar1, bar2 都是 std::unique_ptr</* ProgressBar */> 类型的对象
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

    // "No.3" 将会重新出现在终端的底部
    for ( int i = 0; i < 400; ++i ) {
      bar->tick();
      std::this_thread::sleep_for( std::chrono::milliseconds( 5 ) );
    }
    // 让 bar 被析构
  } );

  std::this_thread::sleep_for( std::chrono::seconds( 1 ) );
  assert( dbar.active() );
} // dbar 在这里被析构了，但这是安全的

for ( auto& td : pool )
  td.join();
```
### 辅助函数
正由于 `pace::DynamicBar` 只是一个工厂类型，所以 pace 提供了一个 `make_dynamic` 函数，这个函数可以在不关心 `pace::DynamicBar` 的情况下返回多个包裹在 `std::unique_ptr` 当中的进度条对象。

注意：这个函数返回的对象类型是 `std::unique_ptr`，而 `std::unique_ptr` 指向的对象本身**没有反向判断自身与另一个对象是否归属于同一个 `pace::DynamicBar`**的功能；所以多个不同的 `pace::DynamicBar` 所返回的 `std::unique_ptr` 在类型角度上看是相等的。

混用不同来源的 `std::unique_ptr` 往往会抛出异常 `pace::exception::InvalidState`，告知已经存在一个正在运行的进度条实例。

```cxx
// 获得与参数数量相同的 std::unique_ptr
auto bars1 = pace::make_dynamic<pace::Channel::Stdout>( pace::config::Line(), pace::config::Block() );
auto bars2 = pace::make_dynamic<>( pace::ProgressBar<>(), pace::BlockBar<>() );
// 为了存储不同进度条类型，bars1 和 bars2 都是 std::tuple 类型，内含多个 std::unique_ptr 对象

// 创建一个所有进度条类型都相同的 std::vector<std::unique_ptr</* Bar Type */>>
// 并使用参数提供的配置对象初始化内部所有进度条对象
auto bar3 = pace::make_dynamic<pace::Channel::Stdout>( pace::config::Spin(), 6 );
auto bar4 = pace::make_dynamic( pace::SpinBar<pace::Channel::Stdout>(), 6 );
// bar3 和 bar4 内部的所有进度条的配置数据都是相同的

// 创建一个所有进度条类型都相同的 std::vector<std::unique_ptr</* Bar Type */>>
// 提供的参数会按顺序作用在内部的进度条对象上
auto bar5 = pace::make_dynamic<pace::config::Sweep>( 3, pace::config::Sweep() );
auto bar6 =
  pace::make_dynamic<pace::SweepBar<pace::Channel::Stdout>>( 3, pace::SweepBar<pace::Channel::Stdout>() );
// bar5 和 bar6 只有第一个进度条对象被初始化为参数指定的内容，其他两个进度条均被默认初始化

// 对于最后两个函数，如果传入的数值和给定的对象数量不一致，会抛出异常 pace::exception::InvalidArgument
try {
  auto _ = pace::make_dynamic<pace::config::Sweep>( 2,
                                                    pace::config::Sweep(),
                                                    pace::config::Sweep(),
                                                    pace::config::Sweep() );
} catch ( const pace::exception::InvalidArgument& e ) {
  std::cerr << "Oops! " << e.what() << std::endl;
}
```
### 渲染行为
`pace::DynamicBar` 的渲染行为与 `pace::MultiBar` 类似：`pace::DynamicBar` 会在多行同时渲染多个进度条，因此使用 `pace::Region::Relative` 时，进度条的渲染结构所占的行数会由 `pace::DynamicBar` 中正在运行的进度条数量决定。

`pace::DynamicBar` 容纳的进度条数量可由 `active_count()` 方法得到，而渲染结构所占行数将会是该方法返回的数量 +1。

示例：

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
`pace::slice::NumericSpan` 是一个模板类型，它被用于表达一个数值范围的起始点、终止点和步长；该数值范围在数学上被表示为：`[start, end)`。

在构造过程、或者更改成员数值过程中，以下情况会导致异常 `pace::exception::InvalidArgument` 抛出：

1. 起点大于终点，而步长是正数；
2. 起点小于终点，而步长是负数；
3. 步长为零。
### 成员方法
`pace::slice::NumericSpan` 有以下几个方法：

```cxx
iterator begin() const noexcept; // 返回一个指向数值范围起点的迭代器
iterator end() const noexcept;   // 返回一个指向数值范围终点的迭代器

N front() const noexcept;           // 返回当前起始点数值
N back() const noexcept;            // 返回当前终止点数值
N step() const noexcept;            // 返回当前步长
/* size_t */ size() const noexcept; // 返回当前数值范围的步数

void swap( NumericSpan& ) noexcept; // 交换两个数值范围
```
### 迭代器类型
`pace::slice::NumericSpan::iterator` 属于前向迭代器，重载了包括但不限于 `operator++()`、`operator++( int )`、`operator+=()`、`operator*()` 和判等运算符在内的运算符函数。

迭代器的有效迭代次数与 `pace::slice::NumericSpan` 的方法 `size()` 返回的值相同；特别的，如果步长大于数值范围，那么迭代器在前进一步后的值将会超出数值范围的终止点。

## IteratorSpan
`pace::slice::IteratorSpan` 是一个模板类型，它被用于表达两个迭代器所划定的抽象范围；可以被视作是 `std::views::ref_view` 的极度简化版本。

`pace::slice::IteratorSpan` 要求传入的迭代器类型必须满足复制构造或移动构造，并且必须能够计算两个迭代器对象之间的距离，否则会导致编译失败。

如果传入的迭代器是一组非逆序类型的逆序迭代器，那么会抛出异常 `pace::exception::InvalidArgument`。

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
### 成员方法
`pace::slice::IteratorSpan` 有以下几个方法：

```cxx
iterator begin() const noexcept; // 返回一个指向抽象范围起点的迭代器
iterator end() const noexcept;   // 返回一个指向抽象范围终点的迭代器

/* reference */ front() const noexcept; // 返回抽象范围起点迭代器指向的元素的引用
/* reference */ back() const noexcept;  // 返回抽象范围终点迭代器指向的前一个元素的引用
/* size_t */ step() const noexcept;     // 返回当前步长，通常是编译期常数 1
/* size_t */ size() const noexcept;     // 返回当前抽象范围的大小

void swap( IteratorSpan& ) noexcept; // 交换两个抽象范围
```
### 迭代器类型
`pace::slice::IteratorSpan::iterator` 属于前向迭代器，重载了包括但不限于 `operator++()`、`operator++( int )`、`operator+=()`、`operator*()` 和判等运算符在内的运算符函数。

由于是前向迭代器且没有提供自减运算符，因此所有逆序操作都依赖于迭代器类型实现。

## SizedSpan
`pace::slice::SizedSpan` 是一个模板类型，它用于表示满足概念 `std::ranges::sized_range` 且不满足概念 `std::ranges::view` 的迭代范围。

简单来说，`pace::slice::SizedSpan` 可以看作是 `std::ranges::ref_view` 的简化版本；它是对容器类型以及数组类型的**视图**。
### 成员方法
`pace::slice::SizedSpan` 有以下几个方法：
```cxx
/* iterator */ begin() const; // 返回一个指向抽象范围起点的迭代器
/* sentinel */ end() const;   // 返回一个指向抽象范围终点的迭代器

/* reference */ front() const;      // 返回抽象范围起点迭代器指向的元素的引用
/* reference */ back() const;       // 返回抽象范围终点迭代器指向的前一个元素的引用
/* size_t */ step() const noexcept; // 返回当前步长，通常是编译期常数 1
/* size_t */ size() const;          // 返回当前抽象范围的大小

void swap( SizedSpan& ) noexcept; // 交换两个抽象范围
```
### 迭代器类型
`pace::slice::SizedSpan::iterator` 的迭代器类型等价于其底层范围的迭代器类型。

## TrackedSpan
`pace::slice::TrackedSpan` 是一个模板类型，它用于表达某个进度条类型的迭代范围。

`pace::slice::TrackedSpan` 只能接受一个满足概念 `std::ranges::sized_range` 的*视图类型*对象，和一个进度条对象；它的作用是为了简化进度条实例与 Enhanced-for 等需要使用迭代器的场景的交互。

这是一个 move-only 的特殊类型，它只应该被工厂函数，如进度条的 `iterate` 方法，构造并返回，而不应该手动构造。

调用 `pace::slice::TrackedSpan` 的 `begin` 方法会导致副作用：`pace::slice::TrackedSpan` 对象将尝试根据内部的抽象范围大小，对其引用的进度条实例设置任务数量。
### 成员方法
`pace::slice::TrackedSpan` 有以下几个方法：

```cxx
/* iterator */ begin() &;          // 为内部的进度条实例赋值，并返回起始迭代器
/* sentinel */ end() const;        // 返回终止迭代器
bool empty() const noexcept;       // 检查当前对象是否指向了一个有效的进度条实例
explicit operator bool() noexcept; // 检查当前对象是否非空

void swap( TrackedSpan& ) noexcept; // 交换两个代理范围
```
### 迭代器类型
`pace::slice::TrackedSpan::iterator` 属于前向迭代器，该迭代器的自增运算符会尝试调用与之绑定的进度条实例的 `tick()` 方法，因此会在意料之外的场景触发副作用。

## iterate
`iterate` 是一系列模板函数的重载名称，它是 pace 的进度条类型的 `iterate` 方法的包装接口。

这个函数允许不显式构造一个进度条对象，同时使用进度条的 `iterate` 方法可视化一个迭代进度过程。

使用方法与 `iterate` 方法相同；但 `iterate` 函数允许传入任意数量的额外参数以定制进度条的样式。

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


# 组合模型
实际上，pace 库的所有进度条类型以及配置类型都是在编译期生成的。
## 模块化组件
pace 将进度条拆分为三个部分：外观组件 `pace::facade`，进度条对象行为 `pace::details::behaviors`，以及支撑外观组件实现的功能组件 `pace::details::aspects`。

这三个部分会使用多个注册结构联系到一起：进度条对象行为组件通过 `pace::details::traits::InheritOrder` 结构声明不同功能之间的依赖关系；功能组件同样通过同一个结构声明不同功能组件的依赖关系；外观组件也通过该结构声明对功能组件的依赖关系，并通过 `pace::details::aspects::EntailOf` 结构声明外观组件对进度条对象行为组件的依赖关系。

`pace::prefab::BasicConfig` 的模板参数能接收多个外观组件，并且会使用某个依赖解析算法收集外观组件的功能组件依赖关系，得到目标配置类型。

配置类型会被注入给 `pace::prefab::BasicBar` 以得到目标进度条类型。

为了解析 `pace::prefab::BasicConfig` 的数据并且渲染为终端的字符串，pace 定义了一个同样是编译期生成的渲染引擎 `pace::details::render::Assembler` 和 `pace::details::render::Builder`，它们会按照 `pace::prefab::BasicConfig` 模板参数中依赖的外观组件顺序，渲染生成目标进度条字符串。

以上行为完全发生在编译期。

## 拼接新的配置类型
得益于模块化设计，pace 允许在编译期创建一个与默认提供的进度条类型截然不同的新的进度条类型。

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

如果去掉上述代码的 `config()` 部分，会发现这个编译期生成的进度条什么都没有显示；这是因为没有为这个进度条配置默认参数。

当默认构造一个进度条类型和一个配置类型时，配置类型会尝试访问 `pace::config::ProvideFor` 结构获取一个默认值；在没有特殊配置的情况下，这些该结构会返回某个包装器类型的默认构造结果。

pace 允许特化 `pace::config::ProvideFor` 以提供非空默认值：

```cxx
using AnotherConfig = pace::prefab::BasicConfig<pace::facade::Elapsed, pace::facade::ETA>;

template<>
struct pace::config::ProvideFor<AnotherConfig, pace::option::Divider> {
  static constexpr pace::option::Divider provide() { return { " | " }; }
};
// 组件的默认开关配置会比较复杂
template<>
struct pace::config::ProvideFor<AnotherConfig, pace::option::Projection> {
  static constexpr pace::option::Projection provide()
  { // 由于 Only 和 Except 是一个编译期可变组件，它们不可能被注入到 ProvideFor 中
    // 为了让默认值与具体类型无关，必须借助 BasicConfig 提供的静态方法剥离 Only 或 Except 的参数列表
    // 从而得到一个不需要模板参数、可以作为默认值的 Projection 类型
    return AnotherConfig::bake( pace::option::Except<>() );
  }
};

// 如果使用 C++14 及之后的标准，可以利用 lambda 特化变量模板 ProvideFor_v
// 此时不需要特化整个 ProvideFor 类型
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
  // 一般来说不建议为任务数量配置一个默认值
  another.config().quota( 100 );

  for ( int i = 0; i < 100; ++i ) {
    another.tick();
    std::this_thread::sleep_for( std::chrono::milliseconds( 500 ) );
  }
}

```

虽然 pace 支持混用不同的进度条指示器（以 Plot 结尾的外观组件），但由于依赖解析算法的性质，这些指示器实际上共享同一组功能组件 `pace::details::aspects`，同时混用这些指示器会导致进度条的外观区分度下降，而且可能产生奇怪的渲染效果。

实际上并不建议混合声明一个以上的进度条渲染器。

## 自定义组件
以下内容会依赖 pace 的内部组件，部分内部组件不具有更新稳定性；具体操作应该以库代码为准。
### 自定义 facade
pace 允许在外部构造一个新的 facade，并且注入到 `pace::prefab::BasicConfig` 当中。

以一个显示时间的时钟为例，这个时钟包含 24 小时和 12 小时制的切换，同时可以有不同着色效果：

```cxx
struct TimeFormat {
  bool data_;

  TimeFormat() = default;
  TimeFormat( bool data ) noexcept : data_ { data } {}
};

struct ClockColor {
  pace::details::console::TrueColor color_;

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
  { // require C++20
    const auto local =
      std::chrono::zoned_time { std::chrono::current_zone(),
                                std::chrono::floor<std::chrono::seconds>( std::chrono::system_clock::now() ) }
        .get_local_time();
    std::chrono::hh_mm_ss time_of_day { local - std::chrono::floor<std::chrono::days>( local ) };
    auto hours = time_of_day.hours().count();

    // 在渲染时钟之前先嵌入着色效果
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

  uint64_t fixed_length() const noexcept { return 8 + ( format_ ? 2 : 0 ); }

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

// 如果外观组件需要依赖 aspects 的内容，则需要在此声明依赖关系
// 且需要支持着色效果则必须声明依赖于 RenderRule
template<>
struct pace::details::traits::InheritOrder<Clock> {
  // Clock 必须在第一个位置上
  using type = pace::details::traits::Relation<Clock, pace::details::aspects::RenderRule>;
};

template<>
struct pace::details::aspects::EntailOf<Clock> {
  // 如果外观组件不要求必须配置任务数量，那么只需要声明对这两个类型的依赖
  // 否则必须声明对于 pace::details::behaviors::Determinate 的依赖
  // 而且若外观组件需要依赖帧计数器，则应该声明对于 pace::details::behaviors::Fancy 的依赖
  // 更多详细操作可以参阅 pace::details::aspects 的代码实现
  using type = pace::details::traits::Relation<pace::details::behaviors::Indeterminate,
                                               pace::details::behaviors::Plain>;
};

// 所有组件会按声明顺序渲染
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
### 自定义渲染
同理，渲染引擎是一个模板类型，它可以被特化重写，但是不能被替换。

具体实现代码可以参阅 [pace/SpinBar.hpp](../include/pace/SpinBar.hpp) 的实现。

而且如果有实际需求，完全可以另起炉灶编写一个不同的 `BasicConfig`，或者提供某些偏特化实现；只需要遵循原来 `BasicConfig` 定义的接口即可。

但是不同的是 `pace::prefab::BasicBar` 并不支持重写，首先是因为 `pace::prefab::BasicBar` 遵循最小依赖原则，它不关心配置类型的实现、也不关心配置类型有什么数据，更不关心自己继承自哪些 `pace::prefab::behaviors`，且只收集 `pace::details::aspects::EntailOf` 的依赖信息并借助解析依赖算法构造一个基类，重写它不会有任何收益。

其次 `pace::MultiBar` 和 `pace::DynamicBar` 显式依赖了 `pace::prefab::BasicBar` 的名称，重写新的 `BasicBar` 会无法接入现有的多进度条渲染体系。

虽然 `pace::prefab::behaviors` 的组件也是可替换重写的，但这必须建立在实现了 `pace::MultiBar` 和 `pace::DynamicBar` 所依赖的内部接口之上，而且必须提供对于 `pace::details::aspects::EntailOf` 的特化支持。

# 设计说明
## 断言检查
pace 使用 `<cassert>` 中的 `assert` 在代码中插入了多处断言检查，这些断言仅会在定义宏 `PACE_DEBUG`、且开启标准库断言时生效。

绝大多数的断言都是为了在内部组件中确认某些参数的有效性，仅有少数断言会被放置在诸如构造函数和赋值运算符等位置，这些断言用于检查当前对象状态是否符合预期。

例如，pace 不允许任何进度条对象在其方法 `active()` 返回 `true` 时调用 `operator=()` 或 `swap()` 函数，因此这些位置的断言有助于检查程序中是否存在这样的非法情况。

自赋值操作同样会被断言检查并拒绝。

## 更新计数与任务总数一致性
pace 中的进度条类型会在调用任意一个 `tick` 方法时启动，在调用 `tick` 所产生的已完成任务数量恰好达到预定任务数量时，进度条类型就会自动停止。

同时，pace 保证在同一个任务周期内（从第一个 `tick` 开始到最后一个 `tick` 为止），所有对 `tick`、`reset` 方法的调用都是线程安全的。

但如果总计调用 `tick` 的次数超过了任务总数，这一保证会失效：

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

在上述代码中，两个线程累计调用 `tick` 的次数超过了预定任务数 `1000`，因此多余的 `( 500 + 700 + 1000 * 0.8 ) - 1000 = 1000` 次调用不受线程安全保障，它们既可能被丢弃、也可能正常计入下一轮进度条运行时的计数器中。

此外，`tick_to` 只保证调用后，进度条的累计任务数**不低于**指定的百分比，它不保证跨线程的 `tick` 次数同步；也就是说，如果有多个线程*正在*同时调用 `tick`，此时调用 `tick_to` 有可能会丢失若干次 `tick`。

## 进度条对象的生命周期
每个进度条对象的生命周期都严格服从于 C++ 标准的对象生命周期机制：

- 在局部作用域创建的对象，会在控制流离开该作用域时被析构；
- 被动态创建的对象，它的生命周期从 `new` 开始，到 `delete` 终止。

这里之所以提及生命周期问题，是因为进度条在被析构过程中，会无视当前迭代进度立即终止运行。

这种强制性终止与调用 `reset()` 方法停止不同：`reset()` 方法允许进度条在即将停止之前，调用（可能）预先传递的回调函数；而析构导致的终止则不会执行这个过程，只会立即关闭与之关联的全局渲染器并清理资源。

> 实质上就等价于调用 `abort()` 方法。

因析构而停止的进度条不会向终端再追加任何信息，因此这可能会导致一定程度上的终端渲染混乱。

## Unicode 支持
pace 默认所有传入的字符串都以 UTF-8 格式编码；使用任何不以 UTF-8 编码的字符串都会有以下四种结果：

1. 被认为是不完整的 UTF-8 字符串，并抛出 `pace::exception::InvalidArgument` 异常；
2. 被认为是部分字节存在错误的破损 UTF-8 字符串，同样抛出 `pace::exception::InvalidArgument` 异常；
3. 被认为是非标准的 UTF-8 字符串，行为同上；
4. 被错误认为是 UTF-8 字符串，无异常抛出。

pace 仅处理有关字符类型的 Unicode 编码，并不会在运行时主动更改终端编码环境。

但如果程序运行在 Windows 上，那么 pace 会在输出流绑定到终端时、于输出字符串之前，将内部的 UTF-8 字符串经过 winapi 转换为对应终端代码页编码的字符，然后再输出；此时即使不改变终端编码环境也能看到正常的字符输出。

> pace 不保证对应 UTF-8 字符在该终端编码环境下有正确的字体映射。

如果使用的 C++ 标准在 C++20 以上，那么 pace 也能接受标准库的 `std::u8string` 和 `std::u8string_view` 等类型。

## 渲染器设计
pace 采用了多线程协作模式设计，因此渲染器实际上是一个在后台工作的子线程；并且 pace 的渲染器采用了单例模式设计。

具体来说，每个进度条类型的 `tick` 和 `tick_to` 等方法都会被视作是一次状态更新，并将状态变迁作用到进度条类型内部的原子量上；在每个进度条实例的全局第一次 `tick` 调用时，都会向 pace 的全局单例渲染器派发一个任务，并在迭代结束后清空这个任务。

> 使用同步渲染模式 `pace::Policy::Sync` 时*绝大多数情况下*都不会使用后台渲染线程执行渲染任务，而是由调用 `tick` 或 `tick_to` 的线程主动执行渲染任务。

派发任务后，进度条实例会启动渲染器，在这期间调用 `tick` 方法的线程会循环等待后台渲染线程启动；同理，当进度条实例关闭渲染器时也会等待后台渲染线程挂起。

进度条实例可以工作在不同的输出流上，所以全局单例的渲染器也被分为了指向 `stdout` 和 `stderr` 的两个单独实例；它们之间互不影响且不存在依赖关系。

在全局范围内，pace 要求同一时刻，指向同一个输出流的情况下，只能有一个进度条实例向全局渲染器派发任务。

如果在同一作用域内创建了多个进度条对象并先后发起任务，则最先派发任务的进度条会成功工作，后续尝试派发任务的进度条会因全局渲染器已被占用而在其任务调用处抛出 `pace::exception::InvalidState` 异常。

在多线程环境下，哪个线程是“最先派发任务”的线程要取决于具体的线程调度策略。

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

在这段代码中，首先创建了三个不同类型的进度条对象：

`bar1` 通过 `bar1.config().quota( 100 )` 成功配置任务数量，并调用 `bar1.tick()` 向全局渲染器派发任务。

接着，`bar2` 调用 `bar2.tick()` 时，由于全局渲染器已被 `bar1` 占用，因而触发了异常。

而 `bar3` 则能正常调用 `bar3.tick()`，这是因为它所使用的输出流与前两个进度条不同，不会与已被占用的全局渲染器产生冲突。

此外，当代码块结束后，之前占用全局渲染器的进度条对象被销毁，随后在全局范围内新建的 `pace::ProgressBar` 对象再次能够正常派发任务；也就是说，全局渲染器在前一个进度条生命周期结束后恢复为可用状态。

## 异常传播机制
pace 内涉及了非常多的动态内存分配申请，因此在大部分复制拷贝/构造和默认初始化过程中，标准库的异常都有可能被抛出。

pace 会在内部自行处理 IO 过程，所以在不同平台下也会有一些不同的异常检查机制。

如果在 Windows 平台下，pace 无法获取到当前进程的标准输出流 Handle，那么会抛出一个本地系统错误异常 `pace::exception::SystemError`。

## 编译时长问题
因为 pace 内部使用了大量模板元编程技巧实现更复杂的抽象能力，所以在使用一些“更静态”的类型（如 `pace::MultiBar`）时会产生相当多的模板计算工作，进而严重降低编译速度。

pace 而且的所有进度条类型都完全依赖模板元编程在编译期生成它自身，而不是直接编码在代码文件中，所以编译时长较长的这一缺点目前没有很好的解决方法。

## 内部设计
### 基础数据结构设计
从性能角度出发，pace 在内部编写了许多针对性优化的数据结构，包括但不限于模板元编程组件、简化版的 `std::move_only_function`、根据 C++ 标准提供不同实现的数值格式化函数、绕过标准库缓冲区的 IO 函数等。

这些组件只适用于 pace 自身，pace 不对外做任何可用性和兼容性保证。
### 进度条类型设计
根据[这篇文章](https://zhuanlan.zhihu.com/p/1956112462068815023)的原理，pace 的进度条类型使用了 Mixin 模式组合继承自内部的多个不同模板基类，这些基类都采用 CRTP 设计，因此在配置类型中可以以链式调用的形式顺序访问不同的配置方法。

为了避免 Mixin 组合继承时引入多继承导致基类构造顺序不确定、且 C++ 虚继承情况下不允许使用 CRTP 设计的问题，pace 使用了一个编译期 C3 线性化算法生成最终的组合继承结构。

这个线性化算法就是 Python 等语言使用的 C3 线性化算法。
