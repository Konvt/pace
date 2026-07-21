#ifndef PACE_TEXT_ALIGN
#define PACE_TEXT_ALIGN

#include "../charcodes/StringView.hpp"
#include "../io/Combinator.hpp"
#include "../utils/Backport.hpp"
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
    } // namespace render

    namespace io {
      template<typename Style, typename Value, typename... Args>
      struct Align;
      template<render::TextAlign Style, typename Value, typename... Args>
      struct Align<std::integral_constant<render::TextAlign, Style>, Value, Args...> {
        std::size_t width;
        std::tuple<Value, Args...> emission;

        friend PACE__FORCEINLINE CharPipeline& operator<<( CharPipeline& pipeline, const Align& self )
        {
          utils::apply(
            [&]( const typename std::remove_reference<Value>::type& val,
                 const typename std::remove_reference<Args>::type&... args ) {
              using render::align_to;
              align_to<Style>( std::back_inserter( pipeline ), val, self.width, args... );
            },
            self.emission );
          return pipeline;
        }
        friend PACE__FORCEINLINE CharPipeline& operator<<( CharPipeline& pipeline, Align&& self )
        {
          utils::apply(
            [&]( Value&& val, Args&&... args ) {
              using render::align_to;
              align_to<Style>( std::back_inserter( pipeline ),
                               std::forward<Value>( val ),
                               self.width,
                               std::forward<Args>( args )... );
            },
            std::move( self.emission ) );
          return pipeline;
        }
      };

      template<render::TextAlign Style>
      struct Currying<Align, std::integral_constant<render::TextAlign, Style>, std::size_t> {
        std::tuple<std::size_t> capture;

        template<typename Value, typename... Args>
        PACE__NODISCARD PACE__FORCEINLINE constexpr Align<std::integral_constant<render::TextAlign, Style>,
                                                          traits::PassAs_t<Value>,
                                                          traits::PassAs_t<Args>...>
          operator()( Value&& val, Args&&... args ) const
        {
          return {
            std::get<0>( capture ),
            { std::forward<Value>( val ), std::forward<Args>( args )... }
          };
        }
      };

      template<render::TextAlign Style>
      PACE__NODISCARD PACE__FORCEINLINE
        Currying<Align, std::integral_constant<render::TextAlign, Style>, std::size_t>
        align( std::size_t width ) noexcept
      { return { { width } }; }
      template<render::TextAlign Style, typename Value, typename... Args>
      PACE__NODISCARD PACE__FORCEINLINE constexpr Align<std::integral_constant<render::TextAlign, Style>,
                                                        traits::PassAs_t<Value>,
                                                        traits::PassAs_t<Args>...>
        align( std::size_t width, Value&& val, Args&&... args )
      {
        return {
          width,
          { std::forward<Value>( val ), std::forward<Args>( args )... }
        };
      }
    } // namespace io
  } // namespace details
} // namespace pace

#endif
