#ifndef PACE_U8CHAR
#define PACE_U8CHAR

#include "../../exception/Error.hpp"
#include "CodeChart.hpp"
#include "StringView.hpp"
#include <algorithm>

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

        PACE__NODISCARD const types::Char* as_bytes() const noexcept { return byte_.data(); }

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
