#ifndef PACE_RGB_VALUE
#define PACE_RGB_VALUE

#include "../../exception/Error.hpp"
#include "../charcodes/StringView.hpp"
#include "../core/Types.hpp"
#include "../render/Paint.hpp"

namespace pace {
  namespace details {
    namespace wrappers {
      class RGBValue {
        union Ink {
          types::HexRGB hex_;
          Color ansi_;

          constexpr Ink() noexcept : hex_ {} {}
        } value_;
        render::Paint encoding_;

        PACE__CXX20_CNSTXPR void from_str( charcodes::StringView hex_str ) &
        {
          if ( hex_str.empty() ) {
            encoding_ = render::Paint::None;
            return;
          } else if ( ( hex_str.size() != 7 && hex_str.size() != 4 ) || hex_str.front() != '#' )
            throw exception::InvalidArgument( charcodes::make_literal( "pace: invalid hex color format" ) );

          for ( std::size_t i = 1; i < hex_str.size(); i++ ) {
            if ( ( hex_str[i] < '0' || hex_str[i] > '9' ) && ( hex_str[i] < 'A' || hex_str[i] > 'F' )
                 && ( hex_str[i] < 'a' || hex_str[i] > 'f' ) ) {
              charcodes::SharedString message =
                charcodes::make_literal( "pace: invalid hexadecimal letter (" );
              message.append( 1, hex_str[i] ).append( 1, ')' );
              throw exception::InvalidArgument( std::move( message ) );
            }
          }

#ifdef PACE_NOSTYLE
          encoding_ = render::Paint::None;
#else
          encoding_   = render::Paint::Xterm24bit;
          value_.hex_ = 0;
          if ( hex_str.size() == 4 ) {
            for ( std::size_t i = 1; i < hex_str.size(); ++i ) {
              value_.hex_ <<= 4;
              if ( hex_str[i] >= '0' && hex_str[i] <= '9' )
                value_.hex_ = ( ( value_.hex_ | ( hex_str[i] - '0' ) ) << 4 ) | ( hex_str[i] - '0' );
              else if ( hex_str[i] >= 'A' && hex_str[i] <= 'F' )
                value_.hex_ =
                  ( ( value_.hex_ | ( hex_str[i] - 'A' + 10 ) ) << 4 ) | ( hex_str[i] - 'A' + 10 );
              else // no need to check whether it's valid or not
                value_.hex_ =
                  ( ( value_.hex_ | ( hex_str[i] - 'a' + 10 ) ) << 4 ) | ( hex_str[i] - 'a' + 10 );
            }
          } else {
            for ( std::size_t i = 1; i < hex_str.size(); ++i ) {
              value_.hex_ <<= 4;
              if ( hex_str[i] >= '0' && hex_str[i] <= '9' )
                value_.hex_ |= hex_str[i] - '0';
              else if ( hex_str[i] >= 'A' && hex_str[i] <= 'F' )
                value_.hex_ |= hex_str[i] - 'A' + 10;
              else
                value_.hex_ |= hex_str[i] - 'a' + 10;
            }
          }
#endif
        }

      public:
        constexpr RGBValue() noexcept : encoding_ { render::Paint::None } {}

        PACE__CXX14_CNSTXPR RGBValue( types::HexRGB hex_val ) noexcept
          : encoding_ { render::Paint::Xterm24bit }
        { value_.hex_ = hex_val; }
        PACE__CXX14_CNSTXPR
        RGBValue( Color enum_val ) noexcept : encoding_ { render::Paint::Csi8 } { value_.ansi_ = enum_val; }
        template<
          typename SV,
          typename = typename std::enable_if<std::is_convertible<SV, charcodes::StringView>::value>::type>
        PACE__CXX20_CNSTXPR RGBValue( SV&& hex_str ) : RGBValue()
        { from_str( std::forward<SV>( hex_str ) ); }

        constexpr RGBValue( const RGBValue& )                        = default;
        PACE__CXX14_CNSTXPR RGBValue& operator=( const RGBValue& ) & = default;

        PACE__CXX14_CNSTXPR RGBValue& operator=( types::HexRGB hex_val ) & noexcept
        {
          value_.hex_ = hex_val;
          return *this;
        }
        PACE__CXX14_CNSTXPR RGBValue& operator=( Color enum_val ) & noexcept
        {
          encoding_    = render::Paint::Csi8;
          value_.ansi_ = enum_val;
          return *this;
        }
        template<typename SV>
        PACE__CXX20_CNSTXPR
          typename std::enable_if<std::is_convertible<SV, charcodes::StringView>::value, RGBValue&>::type
          operator=( SV&& hex_str ) &
        {
          from_str( std::forward<SV>( hex_str ) );
          return *this;
        }

        PACE__NODISCARD constexpr render::Paint encoding() const noexcept { return encoding_; }

        PACE__NODISCARD constexpr std::uint8_t r() const noexcept { return ( value_.hex_ >> 16 ) & 0xFF; }
        PACE__NODISCARD constexpr std::uint8_t g() const noexcept { return ( value_.hex_ >> 8 ) & 0xFF; }
        PACE__NODISCARD constexpr std::uint8_t b() const noexcept { return value_.hex_ & 0xFF; }

        PACE__NODISCARD constexpr Color color() const noexcept { return value_.ansi_; }

        PACE__CXX20_CNSTXPR void swap( RGBValue& other ) noexcept
        {
#ifdef PACE_NOSTYLE
          (void)other;
#else
          std::swap( value_, other.value_ );
#endif
        }
        friend PACE__CXX20_CNSTXPR void swap( RGBValue& a, RGBValue& b ) noexcept { a.swap( b ); }
      };
    } // namespace wrappers
  } // namespace details
} // namespace pace

#endif
