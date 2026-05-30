#ifndef PACE_TEXT_ALIGN
#define PACE_TEXT_ALIGN

#include "../charcodes/StringView.hpp"
#ifdef __cpp_lib_format
# include <format>
#else
# include "../utils/Format.hpp"
# include "../utils/Util.hpp"
#endif

namespace pace {
  namespace details {
    namespace render {
      // text layout
      enum class TextAlign : std::uint8_t { Left, Right, Center };

      template<TextAlign Style, typename Out>
      PACE__FORCEINLINE PACE__CXX20_CNSTXPR Out align_to( Out itr,
                                                          charcodes::StringView str,
                                                          types::Size width )
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
          const types::Size left = pad / 2;
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
        align_to( Out itr, Integer val, types::Size width )
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
          const types::Size left = pad / 2;
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
        align_to( Out itr, Floating val, types::Size width, int precision = 3 )
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

      template<TextAlign Style, typename String>
      PACE__NODISCARD PACE__FORCEINLINE PACE__CXX20_CNSTXPR
        typename std::enable_if<std::is_same<typename std::decay<String>::type, types::String>::value,
                                types::String>::type
        align( String&& str, types::Size width )
      {
        if ( str.size() >= width )
          return str;

#if PACE__CXX17
        if PACE__CXX17_CNSTXPR ( Style == TextAlign::Left
                                 && std::is_rvalue_reference_v<String&&> && !std::is_const_v<String> ) {
          str.append( width - str.size(), ' ' );
          return str;
        }
#endif
        types::String buffer;
        buffer.reserve( width );
        align_to<Style>( std::back_inserter( buffer ), str, width, ' ' );
        return buffer;
      }
    } // namespace render
  } // namespace details
} // namespace pace

#endif
