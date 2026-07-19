#ifndef PACE_GLYPH
#define PACE_GLYPH

#include "../core/Types.hpp"

namespace pace {
  namespace details {
    namespace charcodes {
      // Represents a displayable character cell on the rendered surface.
      // `offset_` is the byte position of this character in the original encoded buffer,
      // not its visual index on screen.
      struct Font {
        // The starting offset (in byte) and the rendered width (in character) of the encoded character.
        std::size_t offset_;
        types::GlyphWidth width_;

        constexpr Font( std::size_t offset, types::GlyphWidth width ) noexcept
          : offset_ { offset }, width_ { width }
        {}
      };
    } // namespace charcodes
  } // namespace details
} // namespace pace

#endif
