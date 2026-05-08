#ifndef PGBAR_GLYPH
#define PGBAR_GLYPH

#include "../types/Types.hpp"

namespace pgbar {
  namespace details {
    namespace charcodes {
      // Represents a displayable character cell on the rendered surface.
      // `offset_` is the byte position of this character in the original encoded buffer,
      // not its visual index on screen.
      struct Font {
        // The starting offset (in byte) and the rendered width (in character) of the encoded character.
        types::Size offset_;
        types::GlyphWidth width_;

        constexpr Font( types::Size offset, types::GlyphWidth width ) noexcept
          : offset_ { offset }, width_ { width }
        {}
      };
    } // namespace charcodes
  } // namespace details
} // namespace pgbar

#endif
