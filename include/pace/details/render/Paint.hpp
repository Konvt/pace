#ifndef PACE_PAINT
#define PACE_PAINT

#include <cstdint>

namespace pace {
  namespace details {
    namespace render {
      // Used for parsing TrueColor and RGBValue.
      enum Paint : std::uint8_t { None, Csi8, Xterm24bit };
    }
  }
} // namespace pace

#endif
