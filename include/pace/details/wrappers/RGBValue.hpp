#ifndef PACE_RGB_VALUE
#define PACE_RGB_VALUE

#include "../../exception/Error.hpp"

namespace pace {
  namespace details {
    namespace wrappers {
      class RGBValue {
      public:
        enum Tonality : std::uint8_t { None, Fixed, Hex };

      private:
        union Tint {
          types::HexRGB hex_;
          Color enum_;

          constexpr Tint() noexcept : hex_ {} {}
        };
        Tint value_;
        Tonality ton_;

        PACE__CXX20_CNSTXPR void from_str( const types::Char* hex_str, types::Size length ) &
        {
          if ( ( length != 7 && length != 4 ) || *hex_str != '#' )
            throw exception::InvalidArgument( charcodes::make_literal( "pace: invalid hex color format" ) );

          for ( types::Size i = 1; i < length; i++ ) {
            if ( ( hex_str[i] < '0' || hex_str[i] > '9' ) && ( hex_str[i] < 'A' || hex_str[i] > 'F' )
                 && ( hex_str[i] < 'a' || hex_str[i] > 'f' ) ) {
              charcodes::CoWString message = charcodes::make_literal( "pace: invalid hexadecimal letter (" );
              message.append( 1, hex_str[i] ).append( 1, ')' );
              throw exception::InvalidArgument( std::move( message ) );
            }
          }

#ifdef PACE_NOSTYLE
          ton_ = Tonality::None;
#else
          ton_        = Tonality::Hex;
          value_.hex_ = 0;
          if ( length == 4 ) {
            for ( types::Size i = 1; i < length; ++i ) {
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
            for ( types::Size i = 1; i < length; ++i ) {
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
        constexpr RGBValue() noexcept : ton_ { Tonality::None } {}

        PACE__CXX14_CNSTXPR RGBValue( types::HexRGB hex_val ) noexcept : ton_ { Tonality::Hex }
        { value_.hex_ = hex_val; }
        PACE__CXX14_CNSTXPR
        RGBValue( Color enum_val ) noexcept : ton_ { Tonality::Fixed } { value_.enum_ = enum_val; }
        PACE__CXX20_CNSTXPR RGBValue( types::ROStr hex_str ) : RGBValue()
        { from_str( hex_str.data(), hex_str.size() ); }
        template<types::Size N>
        PACE__CXX20_CNSTXPR RGBValue( const types::Char ( &hex_str )[N] ) : RGBValue()
        { from_str( hex_str, N - 1 ); }

        constexpr RGBValue( const RGBValue& other )                        = default;
        PACE__CXX14_CNSTXPR RGBValue& operator=( const RGBValue& other ) & = default;

        PACE__CXX14_CNSTXPR RGBValue& operator=( types::HexRGB hex_val ) & noexcept
        {
          value_.hex_ = hex_val;
          return *this;
        }
        PACE__CXX14_CNSTXPR RGBValue& operator=( Color enum_val ) & noexcept
        {
          ton_         = Tonality::Fixed;
          value_.enum_ = enum_val;
          return *this;
        }
        PACE__CXX20_CNSTXPR RGBValue& operator=( types::ROStr hex_str ) &
        {
          from_str( hex_str.data(), hex_str.size() );
          return *this;
        }
        template<types::Size N>
        PACE__CXX20_CNSTXPR RGBValue& operator=( const types::Char ( &hex_str )[N] ) &
        {
          from_str( hex_str, N - 1 );
          return *this;
        }

        PACE__NODISCARD constexpr Tonality tonality() const noexcept { return ton_; }

        PACE__NODISCARD constexpr std::uint8_t r() const noexcept { return ( value_.hex_ >> 16 ) & 0xFF; }
        PACE__NODISCARD constexpr std::uint8_t g() const noexcept { return ( value_.hex_ >> 8 ) & 0xFF; }
        PACE__NODISCARD constexpr std::uint8_t b() const noexcept { return value_.hex_ & 0xFF; }

        PACE__NODISCARD constexpr Color color() const noexcept { return value_.enum_; }

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
