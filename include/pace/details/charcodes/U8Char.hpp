#ifndef PACE_U8CHAR
#define PACE_U8CHAR

#include "../../exception/Error.hpp"
#include "CodeChart.hpp"
#include "StringView.hpp"
#include <algorithm>
#include <array>

namespace pace {
  namespace details {
    namespace charcodes {
      class U8Char {
        std::array<types::Char, 4> byte_;
        std::uint8_t length_;
        std::uint8_t width_;

      public:
        /// @return The utf codepoint and the number of byte of the utf-8 character.
        static PACE__CXX20_CNSTXPR std::pair<types::CodePoint, std::uint8_t> next_codepoint(
          charcodes::StringView raw_u8_str )
        {
          // After RFC 3629, the maximum length of each standard UTF-8 character is 4 bytes.
          const auto first_byte = raw_u8_str.front();
          auto validator        = [=]( types::Size expected_len ) -> types::CodePoint {
            if ( expected_len > raw_u8_str.size() )
              PACE__UNLIKELY throw exception::InvalidArgument( "pace: incomplete UTF-8 sequence"_cow );
            for ( types::Size i = 1; i < expected_len; ++i ) {
              if ( ( raw_u8_str[i] & 0xC0 ) != 0x80 )
                PACE__UNLIKELY throw exception::InvalidArgument(
                  "pace: invalid UTF-8 continuation byte"_cow );
            }

            types::CodePoint ret, overlong;
            switch ( expected_len ) {
            case 2:
              ret      = ( ( first_byte & 0x1F ) << 6 ) | ( raw_u8_str[1] & 0x3F );
              overlong = 0x80;
              break;
            case 3:
              ret =
                ( ( first_byte & 0xF ) << 12 ) | ( ( raw_u8_str[1] & 0x3F ) << 6 ) | ( raw_u8_str[2] & 0x3F );
              overlong = 0x800;
              break;
            case 4:
              ret      = ( ( first_byte & 0x7 ) << 18 ) | ( ( raw_u8_str[1] & 0x3F ) << 12 )
                       | ( ( raw_u8_str[2] & 0x3F ) << 6 ) | ( raw_u8_str[3] & 0x3F );
              overlong = 0x10000;
              break;
            default: utils::unreachable();
            }
            if ( ret < overlong )
              PACE__UNLIKELY throw exception::InvalidArgument( "pace: overlong UTF-8 sequence"_cow );
            return ret;
          };

          if ( ( first_byte & 0x80 ) == 0 )
            return { first_byte, 1 };
          else if ( ( ( first_byte & 0xE0 ) == 0xC0 ) )
            return { validator( 2 ), 2 };
          else if ( ( first_byte & 0xF0 ) == 0xE0 ) {
            const auto codepoint = validator( 3 );
            if ( codepoint >= 0xD800 && codepoint <= 0xDFFF )
              PACE__UNLIKELY throw exception::InvalidArgument( "pace: UTF-8 surrogate code point"_cow );
            return { codepoint, 3 };
          } else if ( ( first_byte & 0xF8 ) == 0xF0 ) {
            const auto codepoint = validator( 4 );
            if ( codepoint > 0x10FFFF )
              PACE__UNLIKELY throw exception::InvalidArgument( "pace: UTF-8 code point out of range"_cow );
            return { codepoint, 4 };
          } else
            PACE__UNLIKELY throw exception::InvalidArgument( "pace: illegal UTF-8 leading byte"_cow );
        }

        // See the Unicode CodeCharts documentation for complete code points.
        // Also can see the `if-else` version in misc/UTF-8-test.cpp
        static PACE__CNSTEVAL std::array<CodeChart, 47> code_chart() noexcept
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

        PACE__NODISCARD static PACE__CXX20_CNSTXPR types::GlyphWidth glyph_width(
          types::CodePoint codepoint ) noexcept
        {
          constexpr auto chart = code_chart();
          PACE__ASSERT( std::is_sorted( chart.cbegin(), chart.cend() ) );
          // Compare with the `if-else` version, here we can search for code points with O(logn).
          const auto itr = std::lower_bound( chart.cbegin(), chart.cend(), codepoint );
          if ( itr != chart.cend() && itr->contains( codepoint ) )
            return itr->width();

          return 1; // Default fallback
        }

        PACE__NODISCARD static PACE__CXX20_CNSTXPR U8Char from_bytes( charcodes::StringView bytes )
        {
          auto parsed = next_codepoint( bytes );
          U8Char ret;
          ret.width_ = glyph_width( parsed.first );
          std::copy( bytes.data(), bytes.data() + parsed.second, ret.byte_.begin() );
          ret.length_ = static_cast<std::uint8_t>( parsed.second );
          return ret;
        }

        constexpr U8Char() noexcept : byte_ {}, length_ { 0 }, width_ { 0 } {}
        PACE__CXX20_CNSTXPR U8Char( types::Char single_u8char ) noexcept
          : byte_ {}, length_ { 1 }, width_ { 1 }
        { byte_.front() = single_u8char; }

        PACE__CXX14_CNSTXPR U8Char( const U8Char& )              = default;
        PACE__CXX14_CNSTXPR U8Char& operator=( const U8Char& ) & = default;
        PACE__CXX20_CNSTXPR ~U8Char()                            = default;

        PACE__NODISCARD const types::Char* as_bytes() const noexcept { return byte_.begin(); }

        PACE__NODISCARD constexpr std::uint8_t size() const noexcept { return length_; }
        PACE__NODISCARD constexpr std::uint8_t width() const noexcept { return width_; }

        PACE__NODISCARD constexpr bool empty() const noexcept { return length_ == 0; }

        PACE__NODISCARD PACE__CXX17_CNSTXPR operator StringView() const noexcept
        { return { byte_.data(), length_ }; }

        PACE__CXX20_CNSTXPR void swap( U8Char& other ) noexcept
        {
          std::swap( byte_, other.byte_ );
          std::swap( length_, other.length_ );
          std::swap( width_, other.width_ );
        }
        friend PACE__CXX20_CNSTXPR void swap( U8Char& a, U8Char& b ) noexcept { a.swap( b ); }
      };
    } // namespace charcodes
  } // namespace details
} // namespace pace

#endif
