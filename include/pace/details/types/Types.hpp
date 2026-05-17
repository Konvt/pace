#ifndef PACE_TYPES
#define PACE_TYPES

#include <chrono>
#include <cstddef>
#include <string>

namespace pace {
  namespace details {
    namespace types {
      using Char      = char;
      using CodePoint = char32_t; // Unicode code point

      using Size  = std::size_t;
      using Float = double;

      using Byte =
#ifdef __cpp_lib_byte
        std::byte; // addressable Byte type
#else
        unsigned char;
#endif
      using Bit8   = std::uint8_t; // a computable and addressable Byte type
      using String = std::basic_string<Char>;
      using Tempus = std::chrono::nanoseconds;

      using HexRGB     = std::uint32_t;
      using GlyphWidth = std::uint8_t; // value is between [0, 3]
    } // namespace types
  } // namespace details

  // A enum that specifies the type of the output stream.
  enum class Channel : int { Stdout = 1, Stderr = 2 };
  enum class Policy : std::uint8_t { Async, Signal, Sync };
  enum class Region : bool { Fixed, Relative };

  // The ANSI 8 colors
  enum class Color : std::uint8_t {
    Black = 30,
    Red,
    Green,
    Blue,
    Yellow,
    Magenta,
    Cyan,
    White,
  };
} // namespace pace

#endif
