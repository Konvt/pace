#ifndef PACE_U8_RAW
#define PACE_U8_RAW

#include "../../exception/Error.hpp"
#include "../utils/Backport.hpp"
#include "../utils/Util.hpp"
#include "CodeChart.hpp"
#include <algorithm>
#include <array>
#include <utility>

namespace pace {
  namespace details {
    namespace charcodes {
      // A simple UTF-8 string implementation, but it does not provide specific utf-8 codec operations.
      class U8Raw {
      protected:
        types::Size width_;
        types::String bytes_;

        /// @return The utf codepoint and the number of byte of the utf-8 character.
        static PACE__CXX23_CNSTXPR std::pair<types::CodePoint, types::Size> next_codepoint(
          const types::Char* raw_u8_str,
          types::Size str_length )
        {
          // After RFC 3629, the maximum length of each standard UTF-8 character is 4 bytes.
          const auto first_byte = *raw_u8_str;
          auto validator        = [=]( types::Size expected_len ) -> types::CodePoint {
            if ( expected_len > str_length )
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
              ret = ( ( first_byte & 0x7 ) << 18 ) | ( ( raw_u8_str[1] & 0x3F ) << 12 )
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

      public:
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
        /**
         * @throw exception::InvalidArgument
         *
         * If the parameter `u8_str` isn't a valid UTF-8 string.
         *
         * @return Returns the render width of the given string.
         */
        PACE__NODISCARD static PACE__CXX20_CNSTXPR types::Size text_width( types::ROStr u8_str )
        {
          types::Size width     = 0;
          const auto raw_u8_str = u8_str.data();
          for ( types::Size i = 0; i < u8_str.size(); ) {
            const auto startpoint = raw_u8_str + i;
            PACE__TRUST( startpoint >= raw_u8_str );
            auto parsed = next_codepoint( startpoint, u8_str.size() - i );
            width += static_cast<types::Size>( glyph_width( parsed.first ) );
            i += parsed.second;
          }
          return width;
        }

        PACE__CXX20_CNSTXPR U8Raw() noexcept( std::is_nothrow_default_constructible<types::String>::value )
          : width_ { 0 }
        {}
        explicit PACE__CXX20_CNSTXPR U8Raw( types::String u8_bytes ) : U8Raw()
        {
          width_ = text_width( u8_bytes );
          bytes_ = std::move( u8_bytes );
        }
        PACE__CXX20_CNSTXPR U8Raw( const U8Raw& )              = default;
        PACE__CXX20_CNSTXPR U8Raw( U8Raw&& )                   = default;
        PACE__CXX20_CNSTXPR U8Raw& operator=( const U8Raw& ) & = default;
        PACE__CXX20_CNSTXPR U8Raw& operator=( U8Raw&& ) &      = default;
        PACE__CXX20_CNSTXPR ~U8Raw()                           = default;

        PACE__CXX20_CNSTXPR U8Raw& operator=( types::ROStr u8_bytes ) &
        {
          const auto new_width = text_width( u8_bytes );
          auto new_bytes       = types::String( u8_bytes );
          bytes_.swap( new_bytes );
          width_ = new_width;
          return *this;
        }
        PACE__CXX20_CNSTXPR U8Raw& operator=( types::String u8_bytes ) &
        {
          width_ = text_width( u8_bytes );
          bytes_.swap( u8_bytes );
          return *this;
        }

        PACE__NODISCARD PACE__CXX20_CNSTXPR bool empty() const noexcept { return bytes_.empty(); }
        PACE__NODISCARD PACE__CXX20_CNSTXPR types::Size size() const noexcept { return bytes_.size(); }
        PACE__NODISCARD PACE__CXX20_CNSTXPR types::Size width() const noexcept { return width_; }

        PACE__CXX20_CNSTXPR const types::Char* data() const noexcept { return bytes_.data(); }
        PACE__CXX20_CNSTXPR types::ROStr str() & noexcept { return bytes_; }
        PACE__CXX20_CNSTXPR types::ROStr str() const& noexcept { return bytes_; }
        PACE__CXX20_CNSTXPR types::String&& str() && noexcept { return std::move( bytes_ ); }

        PACE__CXX20_CNSTXPR void clear() noexcept( noexcept( bytes_.clear() ) )
        {
          bytes_.clear();
          width_ = 0;
        }
        PACE__CXX20_CNSTXPR void shrink_to_fit() noexcept( noexcept( bytes_.shrink_to_fit() ) )
        { // The standard does not seem to specify whether the function is noexcept,
          // so let's make a judgment here.
          // At least I didn't see it on cppreference.
          bytes_.shrink_to_fit();
        }

        PACE__CXX20_CNSTXPR void swap( U8Raw& other ) noexcept
        {
          std::swap( width_, other.width_ );
          bytes_.swap( other.bytes_ );
        }
        friend PACE__CXX20_CNSTXPR void swap( U8Raw& a, U8Raw& b ) noexcept { a.swap( b ); }

        explicit PACE__CXX20_CNSTXPR operator types::String() & { return bytes_; }
        explicit PACE__CXX20_CNSTXPR operator types::String() const& { return bytes_; }
        explicit PACE__CXX20_CNSTXPR operator types::String&&() && noexcept { return std::move( bytes_ ); }
        PACE__CXX20_CNSTXPR operator types::ROStr() const noexcept { return str(); }

        PACE__NODISCARD friend PACE__FORCEINLINE PACE__CXX20_CNSTXPR types::String operator+( U8Raw&& a,
                                                                                              const U8Raw& b )
        {
          return std::move( a.bytes_ ) + b.bytes_;
        }
        PACE__NODISCARD friend PACE__FORCEINLINE PACE__CXX20_CNSTXPR types::String operator+( const U8Raw& a,
                                                                                              const U8Raw& b )
        {
          return a.bytes_ + b.bytes_;
        }
        PACE__NODISCARD friend PACE__FORCEINLINE PACE__CXX20_CNSTXPR types::String operator+(
          types::String&& a,
          const U8Raw& b )
        {
          return std::move( a ) + b.bytes_;
        }
        template<types::Size N>
        PACE__NODISCARD friend PACE__FORCEINLINE PACE__CXX20_CNSTXPR types::String operator+(
          const char ( &a )[N],
          const U8Raw& b )
        {
          return a + b.bytes_;
        }
        PACE__NODISCARD friend PACE__FORCEINLINE PACE__CXX20_CNSTXPR types::String operator+( const char* a,
                                                                                              const U8Raw& b )
        {
          return a + b.bytes_;
        }
        PACE__NODISCARD friend PACE__FORCEINLINE PACE__CXX20_CNSTXPR types::String operator+( types::ROStr a,
                                                                                              const U8Raw& b )
        {
          return types::String( a ) + b;
        }
        PACE__NODISCARD friend PACE__FORCEINLINE PACE__CXX20_CNSTXPR types::String operator+( U8Raw&& a,
                                                                                              types::ROStr b )
        {
          a.bytes_.append( b );
          return std::move( a.bytes_ );
        }
        PACE__NODISCARD friend PACE__FORCEINLINE PACE__CXX20_CNSTXPR types::String operator+( const U8Raw& a,
                                                                                              types::ROStr b )
        {
          return a.bytes_ + types::String( b );
        }

#ifdef __cpp_char8_t
        static_assert( sizeof( char8_t ) == sizeof( char ), "unexpected type size mismatch" );
#endif
#ifdef __cpp_lib_char8_t
        explicit PACE__CXX20_CNSTXPR U8Raw( types::LitU8 u8_sv ) : U8Raw()
        {
          auto new_bytes = types::String( u8_sv.size(), '\0' );
          std::copy( u8_sv.cbegin(), u8_sv.cend(), new_bytes.begin() );
          width_ = text_width( new_bytes );
          bytes_ = std::move( new_bytes );
        }

        explicit PACE__CXX20_CNSTXPR operator std::u8string() const
        {
          std::u8string ret;
          ret.resize( bytes_.size() );
          std::copy( bytes_.cbegin(), bytes_.cend(), ret.begin() );
          return ret;
        }

        PACE__NODISCARD friend PACE__FORCEINLINE PACE__CXX20_CNSTXPR types::String operator+( types::LitU8 a,
                                                                                              const U8Raw& b )
        {
          types::String tmp;
          tmp.reserve( a.size() );
          std::copy( a.cbegin(), a.cend(), std::back_inserter( tmp ) );
          return std::move( tmp ) + b.bytes_;
        }
        PACE__NODISCARD friend PACE__FORCEINLINE PACE__CXX20_CNSTXPR types::String operator+( U8Raw&& a,
                                                                                              types::LitU8 b )
        {
          a.bytes_.reserve( a.bytes_.size() + b.size() );
          std::copy( b.cbegin(), b.cend(), std::back_inserter( a.bytes_ ) );
          return std::move( a.bytes_ );
        }
        PACE__NODISCARD friend PACE__FORCEINLINE PACE__CXX20_CNSTXPR types::String operator+( const U8Raw& a,
                                                                                              types::LitU8 b )
        {
          auto tmp = a.bytes_;
          tmp.reserve( a.bytes_.size() + b.size() );
          std::copy( b.cbegin(), b.cend(), std::back_inserter( tmp ) );
          return tmp;
        }
#endif
      };
    } // namespace charcodes

    namespace utils {
      template<TxtLayout Style>
      PACE__NODISCARD PACE__FORCEINLINE PACE__CXX20_CNSTXPR types::String format(
        types::Size width,
        const charcodes::U8Raw& str )
      {
        return format<Style>( width, str.str() );
      }
      template<TxtLayout Style>
      PACE__NODISCARD PACE__FORCEINLINE PACE__CXX20_CNSTXPR types::String format( types::Size width,
                                                                                  charcodes::U8Raw&& str )
      {
        return format<Style>( width, std::move( str ).str() );
      }
    } // namespace utils
  } // namespace details
} // namespace pace

#endif
