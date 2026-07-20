#ifndef PACE_FORMAT
#define PACE_FORMAT

#include "../core/Core.hpp"
#include <array>
#include <cmath>
#include <string>
#include <type_traits>
#ifdef __cpp_lib_format
# include <format>
#elif defined( __cpp_lib_to_chars )
# include <charconv>
# include <limits>
#endif

namespace pace {
  namespace details {
    namespace utils {
      template<typename Out, typename Integer>
      typename std::enable_if<std::is_integral<Integer>::value, Out>::type format_to( Out itr,
                                                                                      Integer val ) noexcept
      {
#ifdef __cpp_lib_format
        return std::format_to( itr, "{}", val );
#else
        std::array<char, std::numeric_limits<Integer>::digits10 + 1> buffer;
# ifdef __cpp_lib_to_chars
        auto result = std::to_chars( buffer.data(), buffer.data() + buffer.size(), val );
        PACE__TRUST( result.ec == std::errc() );
        return std::copy( buffer.data(), result.ptr, itr );
# else
        auto pos        = buffer.size();
        const bool sign = std::is_signed<Integer>::value && val < 0;
        if ( sign )
          val = ( Integer( 0 ) - ( val + 1 ) ) + 1;
        do {
          buffer[--pos] = static_cast<char>( '0' + ( val % 10 ) );
          val /= 10;
        } while ( val != 0 );
        if ( sign )
          *( itr++ ) = '-';
        return std::copy( buffer.begin() + pos, buffer.end(), itr );
# endif
#endif
      }

      template<typename Out, typename Floating>
      typename std::enable_if<std::is_floating_point<Floating>::value, Out>::type
        format_to( Out itr, Floating val, int precision = 3 ) noexcept
      {
        PACE__ASSERT( std::isfinite( val ) );
        PACE__TRUST( precision >= 0 );
#ifdef __cpp_lib_format
        return std::format_to( itr, "{:.{}f}", val, precision );
#else
        constexpr auto integral_part = std::numeric_limits<Floating>::max_exponent10 + 1;
        constexpr auto fraction_part =
          ( -std::numeric_limits<Floating>::min_exponent10 ) + std::numeric_limits<Floating>::max_digits10;
# ifdef __cpp_lib_to_chars
        std::array<char, integral_part + fraction_part + 1> buffer;
        auto result = std::to_chars( buffer.data(),
                                     buffer.data() + buffer.size(),
                                     val,
                                     std::chars_format::fixed,
                                     precision );
        PACE__TRUST( result.ec == std::errc() );
        return std::copy( buffer.data(), result.ptr, itr );
# else
        std::array<char, ( integral_part > fraction_part ? integral_part : fraction_part )> buffer;

        const auto scale = static_cast<std::uint64_t>( std::pow( 10, precision ) );
        PACE__ASSERT( scale <= ( std::numeric_limits<std::uint64_t>::max )() );
        const auto scaled = static_cast<std::uint64_t>( std::round( scale * std::abs( val ) ) );
        PACE__ASSERT( scaled <= ( std::numeric_limits<std::uint64_t>::max )() );
        auto integer  = scaled / scale;
        auto fraction = scaled % scale;
        auto pos      = buffer.size();

        do {
          buffer[--pos] = static_cast<char>( '0' + integer % 10 );
          integer /= 10;
        } while ( integer != 0 );
        if ( std::signbit( val ) )
          ( *itr++ ) = '-';
        itr = std::copy( buffer.begin() + pos, buffer.end(), itr );
        pos = buffer.size();

        if ( precision > 0 ) {
          ( *itr++ ) = '.';
          while ( precision-- > 0 ) {
            buffer[--pos] = static_cast<char>( '0' + fraction % 10 );
            fraction /= 10;
          }
          itr = std::copy( buffer.begin() + pos, buffer.end(), itr );
        }
        return itr;
# endif
#endif
      }

      // Format an integer number.
      template<typename Integer>
      PACE__NODISCARD PACE__FORCEINLINE
        typename std::enable_if<std::is_integral<Integer>::value, std::string>::type
        format( Integer val ) noexcept( noexcept( std::to_string( val ) ) )
      {
        /* In some well-designed standard libraries,
         * integer std::to_string has specialized implementations for different bit-length types;

         * This includes directly constructing the destination string using the SOO/SSO nature of the string,
         * optimizing the memory management strategy using internally private resize_and_overwrite, etc.

         * Therefore, the functions of the standard library are called directly here,
         * rather than providing a manual implementation like the other functions. */
        return std::to_string( val );
        // Although, unfortunately, std::to_string is not labeled constexpr.
      }

      // Format a finite floating point number.
      template<typename Floating>
      PACE__NODISCARD typename std::enable_if<std::is_floating_point<Floating>::value, std::string>::type
        format( Floating val, int precision = 3 )
      {
        /* Unlike the integer version,
         * the std::to_string in the standard library does not provide a precision limit
         * on floating-point numbers;

         * So the implementation here is provided manually. */
        std::string formatted;
        format_to( std::back_inserter( formatted ), val, precision );
        return formatted;
      }
    } // namespace utils
  } // namespace details
} // namespace pace

#endif
