#ifndef PGBAR_TYPES
#define PGBAR_TYPES

#include <chrono>
#include <cstddef>
#include <string>
#ifdef __cpp_lib_string_view
# include <string_view>
#else
# include <type_traits>
#endif

namespace pgbar {
  namespace details {
    namespace types {
      using Size   = std::size_t;
      using String = std::string;
      using Char   = char;
#ifdef __cpp_lib_string_view
      using ROStr  = std::string_view;
      using LitStr = ROStr; // literal strings
#else
      using ROStr  = typename std::add_lvalue_reference<typename std::add_const<String>::type>::type;
      using LitStr = typename std::add_pointer<typename std::add_const<Char>::type>::type;
#endif
#ifdef __cpp_lib_char8_t
      using LitU8 = std::u8string_view;
#else
      using LitU8 = LitStr;
#endif
#ifdef __cpp_lib_byte
      using Byte = std::byte; // addressable Byte type
#else
      using Byte = unsigned char;
#endif
      using HexRGB     = std::uint32_t;
      using CodePoint  = char32_t; // Unicode code point
      using Float      = double;
      using Bit8       = std::uint8_t; // a computable and addressable Byte type
      using GlyphWidth = std::uint8_t; // value is between [0, 3]
      using Tempus     = std::chrono::nanoseconds;
      // ETA requires nanosecond resolution;
      // microseconds may introduce significant precision loss
      // for fast-updating progress bars.
    } // namespace types
  } // namespace details

  // A enum that specifies the type of the output stream.
  enum class Channel : int { Stdout = 1, Stderr = 2 };
  enum class Policy : std::uint8_t { Async, Signal, Sync };
  enum class Region : bool { Fixed, Relative };

#define PGBAR__DEFAULT 0xC105EA11 // C1O5E -> ClOSE, A11 -> All
#define PGBAR__BLACK   0x000000
#define PGBAR__RED     0xFF0000
#define PGBAR__GREEN   0x00FF00
#define PGBAR__BLUE    0x0000FF
#define PGBAR__YELLOW  0xFFFF00
#define PGBAR__MAGENTA 0x800080
#define PGBAR__CYAN    0x00FFFF
#define PGBAR__WHITE   0xFFFFFF

  enum class Color : std::uint32_t {
    None    = PGBAR__DEFAULT,
    Black   = PGBAR__BLACK,
    Red     = PGBAR__RED,
    Green   = PGBAR__GREEN,
    Blue    = PGBAR__BLUE,
    Yellow  = PGBAR__YELLOW,
    Magenta = PGBAR__MAGENTA,
    Cyan    = PGBAR__CYAN,
    White   = PGBAR__WHITE,
  };
} // namespace pgbar

#endif
