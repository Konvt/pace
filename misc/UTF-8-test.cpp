#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>

/**
 * This file is a minimal implementation of the pace::details::charcodes::U8String component,
 * used to demonstrate the char_width function with character encoding mappings
 * from the Unicode CodeCharts.
 */

namespace pace {
  namespace details {
    namespace types {
      using Size       = std::size_t;
      using String     = std::string;
      using ROStr      = std::string_view;
      using UCodePoint = char32_t;
    }

    namespace charcodes {
      class U8String {
        using self = U8String;

        types::Size width_;
        std::string bytes_;

      public:
        /* nodiscard inline */ static constexpr types::Size char_width( types::UCodePoint codepoint ) noexcept
        {
          // The condition judgement here is taken directly from the standard CodeCharts file.

          if ( codepoint <= 0x19 || ( codepoint >= 0x7F && codepoint <= 0xA0 ) )
            return 0; // control characters,
          if ( codepoint == 0xAD || ( codepoint >= 0x300 && codepoint <= 0x36F ) )
            return 0; // combining characters
          if ( ( codepoint >= 0x2000 && codepoint <= 0x200F ) || codepoint == 0x2011
               || ( codepoint >= 0x2028 && codepoint <= 0x202F )
               || ( codepoint >= 0x205F && codepoint <= 0x206F ) )
            return 0; // General Punctuation
          if ( codepoint >= 0xFDD0 && codepoint <= 0xFDEF )
            return 0; // the standard said they aren't characters
          if ( codepoint >= 0xFE00 && codepoint <= 0xFE0F )
            return 0; // Variation Selectors
          if ( codepoint >= 0xFE20 && codepoint <= 0xFE2F )
            return 0; // Combining Half Marks
          if ( codepoint == 0xFEFF )
            return 0; // Zero width space
          if ( ( codepoint >= 0x1FF80 && codepoint <= 0x1FFFF )
               /*|| ( codepoint >= 0x2FF80 && codepoint <= 0x2FFFF )
               || ( codepoint >= 0x3FF80 && codepoint <= 0x3FFFF )*/
               || ( codepoint >= 0xEFF80 && codepoint <= 0xEFFFF ) )
            return 0; // Unassigned
          if ( codepoint >= 0xE0000 && codepoint <= 0xE007F )
            return 0; // Tags
          if ( codepoint >= 0xE0100 && codepoint <= 0xE01EF )
            return 0; // Variation Selectors Supplement

          if ( codepoint >= 0x20 && codepoint <= 0x7E )
            return 1; // ASCII
          if ( codepoint >= 0xA1 && codepoint <= 0x2FF && codepoint != 0xAD )
            return 1; // Latin Extended
          if ( ( codepoint >= 0x370 && codepoint <= 0x1FFF ) || codepoint == 0x2010
               || ( codepoint >= 0x2012 && codepoint <= 0x2027 ) // These are General Punctuation
               || ( codepoint >= 0x2030 && codepoint <= 0x205E )
               || ( codepoint >= 0x2070 && codepoint <= 0x2E7F ) )
            return 1; // other languages' characters and reserved characters
          // I believe they are rendered to 1 character width (not pretty sure).
          if ( codepoint >= 0xA4D0 && codepoint <= 0xA95F )
            return 1; // Lisu, Vai, Cyrillic Extended and other characters with 1 width
          if ( codepoint >= 0xA980 && codepoint <= 0xABFF )
            return 1; // Javanese, not that Java run on JVM; and other characters
          if ( ( codepoint >= 0xFB00 && codepoint <= 0xFDCF ) // Alphabetic Presentation Forms
               || ( codepoint >= 0xFDF0 && codepoint <= 0xFDFF ) )
            return 1; // Arabic Presentation Forms-A
          if ( codepoint >= 0xFE70 && codepoint <= 0xFEFE )
            return 1; // Arabic Presentation Forms-B
          if ( ( codepoint >= 0xFF61 && codepoint <= 0xFFDF )
               || ( codepoint >= 0xFFE7 && codepoint <= 0xFFEF ) )
            return 1; // Halfwidth Forms
          if ( codepoint >= 0xFFF0 && codepoint <= 0xFFFF )
            return 1; // Specials

          if ( codepoint >= 0x2E80 && codepoint <= 0xA4CF )
            return 2; // CJK characters, phonetic scripts and reserved characters
          // including many other symbol characters
          if ( codepoint >= 0xA960 && codepoint <= 0xA97F )
            return 2; // Hangul Jamo Extended
          if ( codepoint >= 0xAC00 && codepoint <= 0xD7FF )
            return 2; // Hangul Syllables and its extended block
          // U+D800 to U+DFFF is Unicode Surrogate Range,
          if ( codepoint >= 0xF900 && codepoint <= /*0xFAD9*/ 0xFAFF )
            return 2; // CJK Compatibility Ideographs
          if ( codepoint >= 0xFE10 && codepoint <= 0xFE1F )
            return 2; // Vertical Forms
          if ( codepoint >= 0xFE30 && codepoint <= 0xFE6F )
            return 2; // CJK Compatibility Forms and Small Form Variants
          if ( ( codepoint >= 0xFF00 && codepoint <= 0xFF60 )
               || ( codepoint >= 0xFFE0 && codepoint <= 0xFFE6 ) )
            return 2; // Fullwidth Forms
          if ( codepoint >= 0x10000 && codepoint <= 0x1F8FF )
            return 2; // Some complex characters, including emojis
          /*if ( ( codepoint >= 0x20000 && codepoint <= 0x2A6DF )      // B
               || ( codepoint >= 0x2A700 && codepoint <= 0x2B81D )   // C and D
               || ( codepoint >= 0x2B820 && codepoint <= 0x2CEA1 )   // E
               || ( codepoint >= 0x2CEB0 && codepoint <= 0x2EBE0 )   // F
               || ( codepoint >= 0x2EBF0 && codepoint <= 0x2EE5D ) ) // I
            return 2; // CJK Unified Ideographs Extension, B to I
          if ( codepoint >= 0x2F800 && codepoint <= 0x2FA1D )
            return 2; // CJK Compatibility Ideographs Supplement
          if ( ( codepoint >= 0x30000 && codepoint <= 0x3134A )      // G
               || ( codepoint >= 0x31350 && codepoint <= 0x323AF ) ) // H
            return 2; // CJK Unified Ideographs Extension, G to H*/
          if ( codepoint >= 0x20000 && codepoint <= 0x3FFFD )
            return 2; // But EastAsianWidth said the width of this range is 'W'.

          if ( ( codepoint >= 0xE000 && codepoint <= 0xF8FF )
               || ( codepoint >= 0xFFF80 && codepoint <= 0xFFFFF )
               || ( codepoint >= 0x10FF80 && codepoint <= 0x10FFFF ) )
            return 2; // Private Use Area and its Supplementary

          if ( codepoint >= 0x1F900 && codepoint <= 0x1FBFF )
            return 3; // new emojis

          return 1; // Default fallback
        }
        /* nodiscard inline */ static types::Size render_width( types::ROStr u8_str )
        {
          types::Size width = 0;
          for ( types::Size i = 0; i < u8_str.size(); ) {
            const auto start_point = u8_str.data() + i;
            // After RFC 3629, the maximum length of each standard UTF-8 character is 4 bytes.
            const auto first_byte  = static_cast<types::UCodePoint>( *start_point );
            auto integrity_checker = [start_point, &u8_str]( types::Size expected_len ) -> void {
              // __PACE_ASSERT( start_point >= u8_str.data() );
              if ( u8_str.size() - ( start_point - u8_str.data() ) < expected_len )
                throw std::invalid_argument( "pace: incomplete UTF-8 string" );

              for ( types::Size i = 1; i < expected_len; ++i )
                if ( ( start_point[i] & 0xC0 ) != 0x80 )
                  throw std::invalid_argument( "pace: broken UTF-8 character" );
            };

            types::UCodePoint utf_codepoint = {};
            if ( ( first_byte & 0x80 ) == 0 ) {
              utf_codepoint = static_cast<types::UCodePoint>( first_byte );
              i += 1;
            } else if ( ( ( first_byte & 0xE0 ) == 0xC0 ) ) {
              integrity_checker( 2 );
              utf_codepoint = ( ( static_cast<types::UCodePoint>( first_byte ) & 0x1F ) << 6 )
                            | ( static_cast<types::UCodePoint>( start_point[1] ) & 0x3F );
              i += 2;
            } else if ( ( first_byte & 0xF0 ) == 0xE0 ) {
              integrity_checker( 3 );
              utf_codepoint = ( ( static_cast<types::UCodePoint>( first_byte ) & 0xF ) << 12 )
                            | ( ( static_cast<types::UCodePoint>( start_point[1] ) & 0x3F ) << 6 )
                            | ( static_cast<types::UCodePoint>( start_point[2] ) & 0x3F );
              i += 3;
            } else if ( ( first_byte & 0xF8 ) == 0xF0 ) {
              integrity_checker( 4 );
              utf_codepoint = ( ( static_cast<types::UCodePoint>( first_byte ) & 0x7 ) << 18 )
                            | ( ( static_cast<types::UCodePoint>( start_point[1] ) & 0x3F ) << 12 )
                            | ( ( static_cast<types::UCodePoint>( start_point[2] ) & 0x3F ) << 6 )
                            | ( static_cast<types::UCodePoint>( start_point[3] ) & 0x3F );
              i += 4;
            } else
              throw std::invalid_argument( "pace: not a standard UTF-8 string" );

            width += char_width( utf_codepoint );
          }
          return width;
        }

        /* cxx20constexpr */ U8String() noexcept : width_ { 0 } {}
        explicit U8String( types::String bytes ) : U8String()
        {
          width_ = render_width( bytes );
          bytes_ = std::move( bytes );
        }

        /* cxx20constexpr */ self& operator=( const self& ) &     = default;
        /* cxx20constexpr */ self& operator=( self&& ) & noexcept = default;
        /* cxx20constexpr */ ~U8String() noexcept                 = default;

        /* cxx20constexpr */ constexpr types::Size size() const noexcept { return width_; }

        /* cxx20constexpr */ void swap( self& lhs ) noexcept
        {
          std::swap( width_, lhs.width_ );
          bytes_.swap( lhs.bytes_ );
        }
        /* cxx20constexpr */ friend void swap( self& a, self& b ) noexcept { a.swap( b ); }

        /* cxx20constexpr */ explicit operator types::String() & { return bytes_; }
        /* cxx20constexpr */ explicit operator types::String() const& { return bytes_; }
        /* cxx20constexpr */ explicit operator types::String() && noexcept { return std::move( bytes_ ); }
        /* cxx20constexpr */ operator types::ROStr() const noexcept { return bytes_; }
      };
    } // namespace charcodes
  } // namespace details
} // namespace pace

int main()
{
#ifdef __WIN32
  system( "chcp 65001" );
#endif

  using namespace pace::details;
  std::cout << "🇫🇪" << ": " << charcodes::U8String::render_width( "🇫🇪" ) << std::endl;
  std::cout << "👨‍👩‍👧‍👦" << ": "
            << charcodes::U8String::render_width( "👨‍👩‍👧‍👦" ) << std::endl;
  std::cout << "你好" << ": " << charcodes::U8String::render_width( "你好" ) << std::endl;
  std::cout << "お幸せに" << ": " << charcodes::U8String::render_width( "お幸せに" ) << std::endl;
  std::cout << "🥳" << ": " << charcodes::U8String::render_width( "🥳" ) << std::endl;
  std::cout << "█" << ": " << charcodes::U8String::render_width( "█" ) << std::endl;
}
