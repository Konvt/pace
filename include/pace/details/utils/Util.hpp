#ifndef PACE_UTILS_UTIL
#define PACE_UTILS_UTIL

#include "../core/Core.hpp"
#include "../traits/Backport.hpp"
#include <cmath>
#include <tuple>
#ifdef __cpp_lib_to_chars
# include <charconv>
#endif
#ifdef PACE_DEBUG
# include <limits>
#endif

namespace pace {
  namespace details {
    namespace utils {
      // Perfectly forward the I-th element of a tuple, constructing one by default if it's out of bound.
      template<types::Size I, typename T, typename Tuple>
      PACE__FORCEINLINE constexpr auto pick_or( Tuple&& tup ) noexcept ->
        typename std::enable_if<std::is_default_constructible<T>::value,
                                decltype( std::get<I>( std::forward<Tuple>( tup ) ) )>::type
      {
        static_assert( std::is_convertible<typename std::tuple_element<I, Tuple>::type, T>::value,
                       "incompatible type" );
        return std::get<I>( std::forward<Tuple>( tup ) );
      }
      template<types::Size I, typename T, typename Tuple>
      PACE__FORCEINLINE constexpr auto pick_or( Tuple&& )
        noexcept( std::is_nothrow_default_constructible<T>::value ) -> typename std::enable_if<
          traits::AllOf<
            traits::BoolConstant<( I >= std::tuple_size<typename std::decay<Tuple>::type>::value )>,
            std::is_default_constructible<T>>::value,
          T>::type
      { return T(); }

      template<typename Numeric>
      PACE__NODISCARD PACE__FORCEINLINE PACE__CXX14_CNSTXPR
        typename std::enable_if<std::is_unsigned<Numeric>::value, types::Size>::type
        count_digits( Numeric val ) noexcept
      {
        types::Size digits = val == 0;
        for ( ; val > 0; val /= 10 )
          ++digits;
        return digits;
      }
      template<typename Numeric>
      PACE__NODISCARD PACE__FORCEINLINE PACE__CXX14_CNSTXPR
        typename std::enable_if<std::is_signed<Numeric>::value, types::Size>::type
        count_digits( Numeric val ) noexcept
      { return count_digits( static_cast<std::uint64_t>( val < 0 ? -val : val ) ); }

      // Format an integer number.
      template<typename Integer>
      PACE__NODISCARD PACE__FORCEINLINE
        typename std::enable_if<std::is_integral<Integer>::value, types::String>::type
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
      PACE__NODISCARD typename std::enable_if<std::is_floating_point<Floating>::value, types::String>::type
        format( Floating val, int precision = 3 )
      {
        /* Unlike the integer version,
         * the std::to_string in the standard library does not provide a precision limit
         * on floating-point numbers;

         * So the implementation here is provided manually. */
        PACE__ASSERT( std::isfinite( val ) );
        PACE__TRUST( precision >= 0 );
#ifdef __cpp_lib_to_chars
        const auto abs_rounded_val = std::round( std::abs( val ) );
        const auto int_digits      = count_digits( abs_rounded_val );

        types::String formatted;
# ifdef __cpp_lib_string_resize_and_overwrite
        formatted.resize_and_overwrite(
          int_digits + precision + 2,
          [val, precision]( types::Char* buf, types::Size n ) noexcept {
            const auto result = std::to_chars( buf, buf + n, val, std::chars_format::fixed, precision );
            PACE__TRUST( result.ec == std::errc() );
            PACE__TRUST( result.ptr >= buf );
            return static_cast<types::Size>( result.ptr - buf );
          } );
# else
        formatted.resize( int_digits + precision + 2 );
        // The extra 2 is left for the decimal point and carry.
        const auto result = std::to_chars( formatted.data(),
                                           formatted.data() + formatted.size(),
                                           val,
                                           std::chars_format::fixed,
                                           precision );
        PACE__TRUST( result.ec == std::errc() );
        PACE__ASSERT( result.ptr >= formatted.data() );
        formatted.resize( result.ptr - formatted.data() );
# endif
#else
        const auto scale = static_cast<std::uint64_t>( std::pow( 10, precision ) );
        PACE__ASSERT( scale <= ( std::numeric_limits<std::uint64_t>::max )() );
        const auto scaled = static_cast<std::uint64_t>( std::round( scale * std::abs( val ) ) );
        PACE__ASSERT( scaled <= ( std::numeric_limits<std::uint64_t>::max )() );
        const auto integer  = scaled / scale;
        const auto fraction = scaled % scale;
        const auto sign     = std::signbit( val );

        auto formatted = types::String( sign, '-' );
        formatted.reserve( count_digits( integer ) + sign + precision );
        formatted.append( format( integer ) );
        if ( precision > 0 ) {
          formatted.push_back( '.' );
          const auto fract_digits = count_digits( fraction );
          PACE__TRUST( fract_digits <= static_cast<types::Size>( precision ) );
          formatted.append( precision - fract_digits, '0' ).append( format( fraction ) );
        }
#endif
        return formatted;
      }

      enum class TxtAlign { Left, Right, Center }; // text layout
      // Format the `str`.
      template<TxtAlign Alignment, typename String>
      PACE__NODISCARD PACE__FORCEINLINE PACE__CXX20_CNSTXPR
        typename std::enable_if<std::is_same<typename std::decay<String>::type, types::String>::value,
                                types::String>::type
        format_as( String&& str, types::Size width, types::Char padding = ' ' )
      {
        if ( width == 0 )
          PACE__UNLIKELY return {};
        if ( str.size() >= width )
          return str;
        if PACE__CXX17_CNSTXPR ( Alignment == TxtAlign::Right )
          return types::String( width - str.size(), padding ) + std::forward<String>( str );
        else if PACE__CXX17_CNSTXPR ( Alignment == TxtAlign::Left ) {
#if PACE__CXX17
          if PACE__CXX17_CNSTXPR ( std::is_rvalue_reference_v<String&&> && !std::is_const_v<String> ) {
            str.append( width - str.size(), padding );
            return str;
          } else
#endif
            return std::forward<String>( str ) + types::String( width - str.size(), padding );
        } else {
          width -= str.size();
          const types::Size left_align = width / 2;
          return types::String( left_align, padding ) + std::forward<String>( str )
               + types::String( width - left_align, padding );
        }
      }
    } // namespace utils
  } // namespace details
} // namespace pace

#endif
