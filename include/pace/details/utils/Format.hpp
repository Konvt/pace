#ifndef PACE_FORMAT
#define PACE_FORMAT

#include "../charcodes/StringView.hpp"
#include "../core/Core.hpp"
#include "../io/CharPipeline.hpp"
#include "../io/Monoid.hpp"
#include "Backport.hpp"
#include <string>
#include <type_traits>
#ifdef __cpp_lib_format
# include <format>
#else
# include "Util.hpp"
# include <array>
# ifdef __cpp_lib_to_chars
#  include <charconv>
#  include <limits>
# endif
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
        const auto num_written = [&]() {
          static_assert( std::is_same<Floating, long double>::value == false,
                         "we are not currently using long double" );
          // if PACE__CXX17_CNSTXPR ( std::is_same<Floating, long double>::value )
          //   return std::snprintf( buffer.data(), buffer.size(), "%.*Lf", precision, val );
          return std::snprintf( buffer.data(), buffer.size(), "%.*f", precision, val );
        }();
        PACE__TRUST( num_written > 0 );
        return std::copy( buffer.cbegin(), buffer.cbegin() + num_written, itr );
# endif
#endif
      }

      // Format a finite floating point number.
      template<typename Floating>
      PACE__NODISCARD typename std::enable_if<std::is_floating_point<Floating>::value, std::string>::type
        format( Floating val, int precision = 3 )
      {
        std::string formatted;
        format_to( std::back_inserter( formatted ), val, precision );
        return formatted;
      }

      // text layout
      enum class TextAlign : std::uint8_t { Left, Right, Center };

      template<TextAlign Style, typename Out>
      PACE__FORCEINLINE PACE__CXX20_CNSTXPR Out align_to( Out itr,
                                                          charcodes::StringView str,
                                                          std::size_t width )
      {
        if ( str.size() >= width )
          return std::copy( str.cbegin(), str.cend(), itr );
        if PACE__CXX17_CNSTXPR ( Style == TextAlign::Right ) {
          if ( width > str.size() )
            itr = std::fill_n( itr, width - str.size(), ' ' );
          itr = std::copy( str.cbegin(), str.cend(), itr );
        } else if PACE__CXX17_CNSTXPR ( Style == TextAlign::Left ) {
          itr = std::copy( str.cbegin(), str.cend(), itr );
          if ( width > str.size() )
            itr = std::fill_n( itr, width - str.size(), ' ' );
        } else {
          const auto pad         = width - str.size();
          const std::size_t left = pad / 2;
          if ( width > str.size() )
            itr = std::fill_n( itr, left, ' ' );
          itr = std::copy( str.cbegin(), str.cend(), itr );
          if ( width > str.size() )
            itr = std::fill_n( itr, pad - left, ' ' );
        }
        return itr;
      }

      template<TextAlign Style, typename Out, typename Integer>
      PACE__FORCEINLINE PACE__CXX20_CNSTXPR
        typename std::enable_if<std::is_integral<Integer>::value, Out>::type
        align_to( Out itr, Integer val, std::size_t width )
      {
#ifdef __cpp_lib_format
        if PACE__CXX17_CNSTXPR ( Style == TextAlign::Left )
          return std::format_to( itr, "{: <{}}", val, width );
        else if PACE__CXX17_CNSTXPR ( Style == TextAlign::Center )
          return std::format_to( itr, "{: ^{}}", val, width );
        else
          return std::format_to( itr, "{: >{}}", val, width );
#else
        const auto num_digits = utils::count_digits( val );
        if ( num_digits >= width )
          return utils::format_to( itr, val );
        if PACE__CXX17_CNSTXPR ( Style == TextAlign::Right ) {
          if ( width > num_digits )
            itr = std::fill_n( itr, width - num_digits, ' ' );
          itr = utils::format_to( itr, val );
        } else if PACE__CXX17_CNSTXPR ( Style == TextAlign::Left ) {
          itr = utils::format_to( itr, val );
          if ( width > num_digits )
            itr = std::fill_n( itr, width - num_digits, ' ' );
        } else {
          const auto pad         = width - num_digits;
          const std::size_t left = pad / 2;
          if ( width > num_digits )
            itr = std::fill_n( itr, left, ' ' );
          itr = utils::format_to( itr, val );
          if ( width > num_digits )
            itr = std::fill_n( itr, pad - left, ' ' );
        }
        return itr;
#endif
      }

      template<TextAlign Style, typename Out, typename Floating>
      PACE__FORCEINLINE PACE__CXX20_CNSTXPR
        typename std::enable_if<std::is_floating_point<Floating>::value, Out>::type
        align_to( Out itr, Floating val, std::size_t width, int precision = 3 )
      {
#ifdef __cpp_lib_format
        if PACE__CXX17_CNSTXPR ( Style == TextAlign::Left )
          return std::format_to( itr, "{: <{}.{}f}", val, width, precision );
        else if PACE__CXX17_CNSTXPR ( Style == TextAlign::Center )
          return std::format_to( itr, "{: ^{}.{}f}", val, width, precision );
        else
          return std::format_to( itr, "{: >{}.{}f}", val, width, precision );
#else
        return align_to<Style>( itr, utils::format( val, precision ), width );
#endif
      }
    } // namespace utils

    namespace io {
      template<typename Value, typename... Args>
      struct Format {
        static_assert( traits::Not<traits::AnyOf<std::is_rvalue_reference<Value>,
                                                 std::is_rvalue_reference<Args>...>>::value,
                       "dangling reference" );

        std::tuple<Value, Args...> value;

        PACE__FORCEINLINE friend PACE__CXX23_CNSTXPR io::CharPipeline& operator<<( io::CharPipeline& pipeline,
                                                                                   const Format& self )
        {
          utils::apply(
            [&]( const Value& val, const Args&... args ) {
              using utils::format_to;
              format_to( std::back_inserter( pipeline ), val, args... );
            },
            self.value );
          return pipeline;
        }
        PACE__FORCEINLINE friend PACE__CXX23_CNSTXPR io::CharPipeline& operator<<( io::CharPipeline& pipeline,
                                                                                   Format&& self )
        {
          utils::apply(
            [&]( Value&& val, Args&&... args ) {
              using utils::format_to;
              format_to( std::back_inserter( pipeline ),
                         std::forward<Value>( val ),
                         std::forward<Args>( args )... );
            },
            std::move( self.value ) );
          return pipeline;
        }
      };
      template<typename Value, typename... Args>
      PACE__NODISCARD PACE__FORCEINLINE constexpr Format<Value, Args...> format( Value&& value,
                                                                                 Args&&... args )
      {
        return {
          { std::forward<Value>( value ), std::forward<Args>( args )... }
        };
      }

      template<typename Style, typename Value, typename... Args>
      struct Align;
      template<utils::TextAlign Style, typename Value, typename... Args>
      struct Align<std::integral_constant<utils::TextAlign, Style>, Value, Args...> {
        static_assert( traits::Not<traits::AnyOf<std::is_rvalue_reference<Value>,
                                                 std::is_rvalue_reference<Args>...>>::value,
                       "dangling reference" );

        std::tuple<std::size_t, Value, Args...> value;

        PACE__FORCEINLINE friend PACE__CXX23_CNSTXPR io::CharPipeline& operator<<( io::CharPipeline& pipeline,
                                                                                   const Align& self )
        {
          utils::apply(
            [&]( std::size_t width, const Value& val, const Args&... args ) {
              using utils::align_to;
              align_to<Style>( std::back_inserter( pipeline ), val, width, args... );
            },
            self.value );
          return pipeline;
        }
        PACE__FORCEINLINE friend PACE__CXX23_CNSTXPR io::CharPipeline& operator<<( io::CharPipeline& pipeline,
                                                                                   Align&& self )
        {
          utils::apply(
            [&]( std::size_t width, Value&& val, Args&&... args ) {
              using utils::align_to;
              align_to<Style>( std::back_inserter( pipeline ),
                               std::forward<Value>( val ),
                               width,
                               std::forward<Args>( args )... );
            },
            std::move( self.value ) );
          return pipeline;
        }
      };

      template<utils::TextAlign Style>
      struct Currying<Align, std::integral_constant<utils::TextAlign, Style>, std::size_t> {
        std::tuple<std::size_t> capture;

        template<typename Value, typename... Args>
        PACE__NODISCARD
          PACE__FORCEINLINE constexpr Align<std::integral_constant<utils::TextAlign, Style>, Value, Args...>
          operator()( Value&& val, Args&&... args ) const
        {
          return {
            { std::get<0>( capture ), std::forward<Value>( val ), std::forward<Args>( args )... }
          };
        }
      };

      template<utils::TextAlign Style>
      PACE__NODISCARD PACE__FORCEINLINE
        io::Currying<Align, std::integral_constant<utils::TextAlign, Style>, std::size_t>
        align( std::size_t width ) noexcept
      { return { { width } }; }
      template<utils::TextAlign Style, typename Value, typename... Args>
      PACE__NODISCARD
        PACE__FORCEINLINE constexpr Align<std::integral_constant<utils::TextAlign, Style>, Value, Args...>
        align( std::size_t width, Value&& val, Args&&... args )
      {
        return {
          { width, std::forward<Value>( val ), std::forward<Args>( args )... }
        };
      }
    } // namespace io
  } // namespace details
} // namespace pace

#endif
