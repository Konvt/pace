#ifndef PACE_CODE_CHART
#define PACE_CODE_CHART

#include "../core/Core.hpp"
#include "../core/Types.hpp"
#include <array>

namespace pace {
  namespace details {
    namespace charcodes {
      // A type of wrapper that stores the mapping between Unicode code chart and character width.
      class CodeChart final {
        char32_t start_, end_;
        types::GlyphWidth width_;

      public:
        constexpr CodeChart( char32_t start, char32_t end, types::GlyphWidth width ) noexcept
          : start_ { start }, end_ { end }, width_ { width }
        {       // This is an internal component, so we assume the arguments are always valid.
#if PACE__CXX14 // C++11 requires the constexpr ctor should have an empty function body.
          PACE__TRUST( start_ <= end_ );
#endif
        }
        constexpr CodeChart( const CodeChart& )                        = default;
        PACE__CXX14_CNSTXPR CodeChart& operator=( const CodeChart& ) & = default;
        PACE__CXX20_CNSTXPR ~CodeChart()                               = default;

        // Check whether the Unicode code point is within this code chart.
        PACE__NODISCARD constexpr bool contains( char32_t codepoint ) const noexcept
        { return start_ <= codepoint && codepoint <= end_; }
        // Return the character width of this Unicode code chart.
        PACE__NODISCARD constexpr types::GlyphWidth width() const noexcept { return width_; }
        // Return the size of this range of Unicode code chart.
        PACE__NODISCARD constexpr char32_t size() const noexcept { return end_ - start_ + 1; }
        // Return the start Unicode code point of this code chart.
        PACE__NODISCARD constexpr char32_t head() const noexcept { return start_; }
        // Return the end Unicode code point of this code chart.
        PACE__NODISCARD constexpr char32_t tail() const noexcept { return end_; }

        PACE__NODISCARD friend constexpr bool operator<( const CodeChart& a, const CodeChart& b ) noexcept
        { return a.end_ < b.start_; }
        PACE__NODISCARD friend constexpr bool operator>( const CodeChart& a, const CodeChart& b ) noexcept
        { return a.start_ > b.end_; }
        PACE__NODISCARD friend constexpr bool operator>( const CodeChart& a, const char32_t& b ) noexcept
        { return a.start_ > b; }
        PACE__NODISCARD friend constexpr bool operator<( const CodeChart& a, const char32_t& b ) noexcept
        { return a.end_ < b; }
        PACE__NODISCARD friend constexpr bool operator>( const char32_t& a, const CodeChart& b ) noexcept
        { return b < a; }
        PACE__NODISCARD friend constexpr bool operator<( const char32_t& a, const CodeChart& b ) noexcept
        { return b > a; }
      };

      // The content is from the Unicode 17.0 standard CodeChart pdf.
      inline PACE__CNSTEVAL std::array<CodeChart, 47> code_chart() noexcept
      {
        return {
          { { 0x0, 0x19, 0 },        { 0x20, 0x7E, 1 },        { 0x7F, 0xA0, 0 },
           { 0xA1, 0xAC, 1 },       { 0xAD, 0xAD, 0 },        { 0xAE, 0x2FF, 1 },
           { 0x300, 0x36F, 0 },     { 0x370, 0x1FFF, 1 },     { 0x2000, 0x200F, 0 },
           { 0x2010, 0x2010, 1 },   { 0x2011, 0x2011, 0 },    { 0x2012, 0x2027, 1 },
           { 0x2028, 0x202F, 0 },   { 0x2030, 0x205E, 1 },    { 0x205F, 0x206F, 0 },
           { 0x2070, 0x2E7F, 1 },   { 0x2E80, 0xA4CF, 2 },    { 0xA4D0, 0xA95F, 1 },
           { 0xA960, 0xA97F, 2 },   { 0xA980, 0xABFF, 1 },    { 0xAC00, 0xD7FF, 2 },
           { 0xE000, 0xF8FF, 2 },   { 0xF900, 0xFAFF, 2 },    { 0xFB00, 0xFDCF, 1 },
           { 0xFDD0, 0xFDEF, 0 },   { 0xFDF0, 0xFDFF, 1 },    { 0xFE00, 0xFE0F, 0 },
           { 0xFE10, 0xFE1F, 2 },   { 0xFE20, 0xFE2F, 0 },    { 0xFE30, 0xFE6F, 2 },
           { 0xFE70, 0xFEFE, 1 },   { 0xFEFF, 0xFEFF, 0 },    { 0xFF00, 0xFF60, 2 },
           { 0xFF61, 0xFFDF, 1 },   { 0xFFE0, 0xFFE6, 2 },    { 0xFFE7, 0xFFEF, 1 },
           { 0xFFF0, 0xFFFF, 1 },   { 0x10000, 0x1F8FF, 2 },  { 0x1F900, 0x1FBFF, 3 },
           { 0x1FF80, 0x1FFFF, 0 }, { 0x20000, 0x3FFFD, 2 },  { 0x3FFFE, 0x3FFFF, 0 },
           { 0xE0000, 0xE007F, 0 }, { 0xE0100, 0xE01EF, 0 },  { 0xEFF80, 0xEFFFF, 0 },
           { 0xFFF80, 0xFFFFF, 2 }, { 0x10FF80, 0x10FFFF, 2 } }
        };
      }

      PACE__NODISCARD inline PACE__CXX20_CNSTXPR types::GlyphWidth glyph_width( char32_t codepoint ) noexcept
      {
        constexpr auto chart = code_chart();
        PACE__ASSERT( std::is_sorted( chart.cbegin(), chart.cend() ) );
        // Compare with the `if-else` version, here we can search for code points with O(logn).
        const auto itr = std::lower_bound( chart.cbegin(), chart.cend(), codepoint );
        if ( itr != chart.cend() && itr->contains( codepoint ) )
          return itr->width();

        // Returning 2 is more appropriate than returning 1
        // because overestimating in layout will only result in extra spaces,
        // but underestimating will cause font distortion.
        return 2;
      }
    } // namespace charcodes
  } // namespace details
} // namespace pace

#endif
