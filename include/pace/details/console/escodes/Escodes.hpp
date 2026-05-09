#ifndef PACE_ESCODES
#define PACE_ESCODES

#include "../../io/CharPipeline.hpp"
#include "../../utils/Backport.hpp"
#ifdef __cpp_lib_to_chars
# include <charconv>
#endif

namespace pace {
  namespace details {
    namespace console {
      namespace escodes {
#ifdef PACE_NOSTYLE
        PACE__CXX17_INLINE constexpr auto& fontreset = u8"";
        PACE__CXX17_INLINE constexpr auto& fontbold  = u8"";
#else
        PACE__CXX17_INLINE constexpr auto& fontreset = u8"\x1B[0m";
        PACE__CXX17_INLINE constexpr auto& fontbold  = u8"\x1B[1m";
#endif
        PACE__CXX17_INLINE constexpr auto& savecursor      = u8"\x1B[s";
        PACE__CXX17_INLINE constexpr auto& resetcursor     = u8"\x1B[u";
        PACE__CXX17_INLINE constexpr auto& linewipe        = u8"\x1B[K";
        PACE__CXX17_INLINE constexpr auto& prevline        = u8"\x1b[A";
        PACE__CXX17_INLINE constexpr types::Char nextline  = '\n';
        PACE__CXX17_INLINE constexpr types::Char linestart = '\r';

        class RGBColor {
#ifndef PACE_NOSTYLE
          std::array<types::Char, 17> sgr_; // Select Graphic Rendition
          std::uint8_t length_;
#endif

          static PACE__CXX23_CNSTXPR types::Char* to_char( types::Char* first,
                                                           types::Char* last,
                                                           std::uint8_t value ) noexcept
          {
#ifdef __cpp_lib_to_chars
            auto result = std::to_chars( first, last, value );
            PACE__TRUST( result.ec == std::errc() );
            return result.ptr;
#else
            types::Size offset = 1;
            if ( value >= 100 ) {
              PACE__TRUST( last - first >= 3 );
              first[0] = '0' + value / 100;
              first[1] = '0' + ( value / 10 % 10 );
              first[2] = '0' + ( value % 10 );
              offset += 2;
            } else if ( value >= 10 ) {
              PACE__TRUST( last - first >= 2 );
              first[0] = '0' + value / 10;
              first[1] = '0' + ( value % 10 );
              offset += 1;
            } else
              first[0] = '0' + value;
            PACE__TRUST( last - first >= 1 );
            return first + offset;
#endif
          }

          PACE__CXX23_CNSTXPR void from_hex( types::HexRGB hex_val ) & noexcept
          {
#ifdef PACE_NOSTYLE
            (void)hex_val;
#else
            length_ = 1;
            if ( hex_val == PACE__DEFAULT ) {
              sgr_[0] = '0';
              return;
            }

            length_ = 2;
            sgr_[0] = '3';
            switch ( hex_val & 0x00FFFFFF ) { // discard the high 8 bits
            case PACE__BLACK:   sgr_[1] = '0'; break;
            case PACE__RED:     sgr_[1] = '1'; break;
            case PACE__GREEN:   sgr_[1] = '2'; break;
            case PACE__YELLOW:  sgr_[1] = '3'; break;
            case PACE__BLUE:    sgr_[1] = '4'; break;
            case PACE__MAGENTA: sgr_[1] = '5'; break;
            case PACE__CYAN:    sgr_[1] = '6'; break;
            case PACE__WHITE:   sgr_[1] = '7'; break;
            default:            {
              sgr_[1] = '8', sgr_[2] = ';', sgr_[3] = '2', sgr_[4] = ';';
              auto tail = to_char( sgr_.data() + 5, sgr_.data() + sgr_.size(), ( hex_val >> 16 ) & 0xFF );
              *tail     = ';';
              tail      = to_char( tail + 1, sgr_.data() + sgr_.size(), ( hex_val >> 8 ) & 0xFF );
              *tail     = ';';
              tail      = to_char( tail + 1, sgr_.data() + sgr_.size(), hex_val & 0xFF );
              length_   = static_cast<std::uint8_t>( tail - sgr_.data() );
            } break;
            }
#endif
          }
          PACE__CXX23_CNSTXPR void from_str( const types::Char* hex_str, types::Size length ) &
          {
            if ( ( length != 7 && length != 4 ) || *hex_str != '#' )
              throw exception::InvalidArgument( charcodes::make_literal( "pace: invalid hex color format" ) );

            for ( types::Size i = 1; i < length; i++ ) {
              if ( ( hex_str[i] < '0' || hex_str[i] > '9' ) && ( hex_str[i] < 'A' || hex_str[i] > 'F' )
                   && ( hex_str[i] < 'a' || hex_str[i] > 'f' ) ) {
                charcodes::CoWString message =
                  charcodes::make_literal( "pace: invalid hexadecimal letter (" );
                message.append( 1, hex_str[i] ).append( 1, ')' );
                throw exception::InvalidArgument( std::move( message ) );
              }
            }

#ifndef PACE_NOSTYLE
            std::uint32_t hex_val = 0;
            if ( length == 4 ) {
              for ( types::Size i = 1; i < length; ++i ) {
                hex_val <<= 4;
                if ( hex_str[i] >= '0' && hex_str[i] <= '9' )
                  hex_val = ( ( hex_val | ( hex_str[i] - '0' ) ) << 4 ) | ( hex_str[i] - '0' );
                else if ( hex_str[i] >= 'A' && hex_str[i] <= 'F' )
                  hex_val = ( ( hex_val | ( hex_str[i] - 'A' + 10 ) ) << 4 ) | ( hex_str[i] - 'A' + 10 );
                else // no need to check whether it's valid or not
                  hex_val = ( ( hex_val | ( hex_str[i] - 'a' + 10 ) ) << 4 ) | ( hex_str[i] - 'a' + 10 );
              }
            } else {
              for ( types::Size i = 1; i < length; ++i ) {
                hex_val <<= 4;
                if ( hex_str[i] >= '0' && hex_str[i] <= '9' )
                  hex_val |= hex_str[i] - '0';
                else if ( hex_str[i] >= 'A' && hex_str[i] <= 'F' )
                  hex_val |= hex_str[i] - 'A' + 10;
                else
                  hex_val |= hex_str[i] - 'a' + 10;
              }
            }
            from_hex( hex_val );
#endif
          }

        public:
          PACE__CXX20_CNSTXPR RGBColor() noexcept { clear(); }

          PACE__CXX23_CNSTXPR RGBColor( types::HexRGB hex_val ) noexcept : RGBColor() { from_hex( hex_val ); }
          PACE__CXX23_CNSTXPR RGBColor( Color enum_val ) noexcept
            : RGBColor( utils::to_underlying( enum_val ) )
          {}
          PACE__CXX23_CNSTXPR RGBColor( types::ROStr hex_str ) : RGBColor()
          { from_str( hex_str.data(), hex_str.size() ); }
          template<types::Size N>
          PACE__CXX23_CNSTXPR RGBColor( const types::Char ( &hex_str )[N] ) : RGBColor()
          { from_str( hex_str, N - 1 ); }

          PACE__CXX20_CNSTXPR RGBColor( const RGBColor& other )              = default;
          PACE__CXX20_CNSTXPR RGBColor& operator=( const RGBColor& other ) & = default;

          PACE__CXX23_CNSTXPR RGBColor& operator=( types::HexRGB hex_val ) & noexcept
          {
            from_hex( hex_val );
            return *this;
          }
          PACE__CXX23_CNSTXPR RGBColor& operator=( Color enum_val ) & noexcept
          {
            from_hex( utils::to_underlying( enum_val ) );
            return *this;
          }
          PACE__CXX23_CNSTXPR RGBColor& operator=( types::ROStr hex_str ) &
          {
            from_str( hex_str.data(), hex_str.size() );
            return *this;
          }
          template<types::Size N>
          PACE__CXX23_CNSTXPR RGBColor& operator=( const types::Char ( &hex_str )[N] ) &
          {
            from_str( hex_str, N );
            return *this;
          }

          PACE__FORCEINLINE PACE__CXX20_CNSTXPR void clear() noexcept
          {
#ifndef PACE_NOSTYLE
            std::fill( sgr_.begin(), sgr_.end(), '\0' );
            length_ = 0;
#endif
          }

          PACE__CXX20_CNSTXPR void swap( RGBColor& other ) noexcept
          {
#ifdef PACE_NOSTYLE
            (void)other;
#else
            std::swap( sgr_, other.sgr_ );
            std::swap( length_, other.length_ );
#endif
          }
          friend PACE__CXX20_CNSTXPR void swap( RGBColor& a, RGBColor& b ) noexcept { a.swap( b ); }

          friend PACE__FORCEINLINE io::CharPipeline& operator<<( io::CharPipeline& pipeline,
                                                                 const RGBColor& col )
          {
#ifdef PACE_NOSTYLE
            (void)col;
#else
            if ( col.length_ > 0 ) {
              pipeline.append( '\x1B' )
                .append( '[' )
                .append( col.sgr_.data(), col.sgr_.data() + col.length_ )
                .append( 'm' );
            }
#endif
            return pipeline;
          }
        };
      } // namespace escodes
    } // namespace console
  } // namespace details
} // namespace pace

#endif
