#ifndef PACE_RENDER_RULE
#define PACE_RENDER_RULE

#include "../../config/Provider.hpp"
#include "../concurrent/SharedLock.hpp"
#include "../concurrent/SharedMutex.hpp"
#include "../console/Colorize.hpp"
#include "../console/Escode.hpp"
#include "../wrappers/Brush.hpp"
#include "../wrappers/OptionPacket.hpp"
#include <bitset>
#include <mutex>

namespace pace {
  namespace option {
    // A wrapper that stores the value of the color effect setting.
    struct Colored : PACE__DERIVING_OPTION2( Colored, bool, _enable );

    // A wrapper that stores the value of the font effect boldness setting.
    struct FontBold : PACE__DERIVING_OPTION2( FontBold, bool, _enable );

    // A wrapper that stores the value of the font effect faint setting.
    struct FontFaint : PACE__DERIVING_OPTION2( FontFaint, bool, _enable );

    // A wrapper that stores the value of the font effect italic setting.
    struct FontItalic : PACE__DERIVING_OPTION2( FontItalic, bool, _enable );

    // A wrapper that stores the value of the font effect underline setting.
    struct FontUnderline : PACE__DERIVING_OPTION2( FontUnderline, bool, _enable );

    // A wrapper that stores the value of the font effect inverse setting.
    struct FontInverse : PACE__DERIVING_OPTION2( FontInverse, bool, _enable );

    // A wrapper that stores the value of the font effect hidden setting.
    struct FontHidden : PACE__DERIVING_OPTION2( FontHidden, bool, _enable );

    // A wrapper that stores the value of the font effect  setting.
    struct FontCrossed : PACE__DERIVING_OPTION2( FontCrossed, bool, _enable );
  } // namespace option

  namespace details {
    namespace aspects {
      template<typename Base, typename Derived>
      class RenderRule : public Base {
#define PACE__UNPAKING( OptionName, EnumName, MemberName )                                      \
  friend PACE__FORCEINLINE PACE__CXX14_CNSTXPR void unpack( RenderRule& self,                   \
                                                            option::OptionName&& val ) noexcept \
  { self.rules_[utils::to_underlying( Chroma::EnumName )] = val.value(); }
        PACE__UNPAKING( Colored, Colored, colored_ )
        PACE__UNPAKING( FontBold, Bold, bold_ )
        PACE__UNPAKING( FontFaint, Faint, faint_ )
        PACE__UNPAKING( FontItalic, Italic, italic_ )
        PACE__UNPAKING( FontUnderline, Underline, underline_ )
        PACE__UNPAKING( FontInverse, Inverse, inverse_ )
        PACE__UNPAKING( FontHidden, Hidden, hidden_ )
        PACE__UNPAKING( FontCrossed, Crossed, crossed_ )
#undef PACE__UNPAKING

        enum class Chroma : std::uint8_t {
          Colored = 0,
          Bold,
          Faint,
          Italic,
          Underline,
          Inverse,
          Hidden,
          Crossed
        };
        std::bitset<8> rules_;

      protected:
        PACE__FORCEINLINE PACE__CXX20_CNSTXPR io::CharPipeline& font_effect( io::CharPipeline& pipeline,
                                                                             bool style_off ) const
        { // This method should only be called by the rendering engine
#ifdef PACE_NOSTYLE
          (void)style_off;
#else
          if ( style_off )
            return pipeline;
          if ( rules_[utils::to_underlying( Chroma::Bold )] )
            pipeline << console::fontbold;
          if ( rules_[utils::to_underlying( Chroma::Faint )] )
            pipeline << console::fontfaint;
          if ( rules_[utils::to_underlying( Chroma::Italic )] )
            pipeline << console::fontitalic;
          if ( rules_[utils::to_underlying( Chroma::Underline )] )
            pipeline << console::fontunderline;
          if ( rules_[utils::to_underlying( Chroma::Inverse )] )
            pipeline << console::fontinverse;
          if ( rules_[utils::to_underlying( Chroma::Hidden )] )
            pipeline << console::fonthidden;
          if ( rules_[utils::to_underlying( Chroma::Crossed )] )
            pipeline << console::fontcrossed;
#endif
          return pipeline;
        }
        PACE__FORCEINLINE PACE__CXX20_CNSTXPR io::CharPipeline& reset_style( io::CharPipeline& pipeline,
                                                                             bool style_off ) const
        { // This method should only be called by the rendering engine
#ifdef PACE_NOSTYLE
          (void)style_off;
#else
          if ( !style_off && rules_.any() )
            pipeline << console::stylereset;
#endif
          return pipeline;
        }

        template<typename Op>
        PACE__NODISCARD PACE__FORCEINLINE PACE__CXX20_CNSTXPR
          typename std::enable_if<traits::AnyOf<std::is_same<Op, console::Forecolor>,
                                                std::is_same<Op, console::Backcolor>,
                                                std::is_same<Op, console::Dualcolor>>::value,
                                  wrappers::Brush<Op>>::type
          with_dye( Op rgb, bool style_off ) const
        {
#ifdef PACE_NOSTYLE
          (void)rgb;
          (void)style_off;
#else
          if ( !style_off && rules_[utils::to_underlying( Chroma::Colored )] )
            return { std::move( rgb ) };
#endif
          return {};
        }

        PACE__NODISCARD PACE__FORCEINLINE PACE__CXX20_CNSTXPR
          wrappers::Brush<std::reference_wrapper<const console::Escode>,
                          wrappers::Brush<std::reference_wrapper<const console::Escode>>>
          with_clear( bool style_off ) const
        {
#ifdef PACE_NOSTYLE
          (void)style_off;
#else
          if ( !style_off && rules_[utils::to_underlying( Chroma::Colored )] )
            return { std::cref( console::fgcolorrest ), { std::cref( console::bgcolorrest ) } };
#endif
          return { {} };
        }

        template<typename Op>
        PACE__NODISCARD PACE__FORCEINLINE PACE__CXX20_CNSTXPR typename std::enable_if<
          traits::AnyOf<std::is_same<Op, console::Forecolor>,
                        std::is_same<Op, console::Backcolor>,
                        std::is_same<Op, console::Dualcolor>>::value,
          wrappers::Brush<
            std::reference_wrapper<const console::Escode>,
            wrappers::Brush<std::reference_wrapper<const console::Escode>, wrappers::Brush<Op>>>>::type
          clear_then_dye( Op rgb, bool style_off ) const
        {
#ifndef PACE_NOSTYLE
          if ( !style_off && rules_[utils::to_underlying( Chroma::Colored )] )
            return with_clear( style_off ).append( std::move( rgb ) );
#endif
          return { { {} } };
        }

        template<typename... Options>
        PACE__CXX14_CNSTXPR RenderRule( traits::TypeSet<Options...> tag ) : Base( tag )
        {
          using OptionSet = traits::TypeSet<Options...>;
          if PACE__CXX17_CNSTXPR ( !traits::TpContain<OptionSet, option::Colored>::value )
            unpack( *this, config::provide_for<Derived, option::Colored>() );
          if PACE__CXX17_CNSTXPR ( !traits::TpContain<OptionSet, option::FontBold>::value )
            unpack( *this, config::provide_for<Derived, option::FontBold>() );
        }

        PACE__SPECIAL_MEMBERS( RenderRule );

      public:
#define PACE__METHOD( OptionName, ReturnType )                     \
  std::lock_guard<concurrent::SharedMutex> lock { this->rw_mtx_ }; \
  unpack( *this, option::OptionName( _enable ) );                  \
  return static_cast<ReturnType>( *this )

        // Enable or disable the color effect.
        Derived& colored( bool _enable ) & noexcept
        { PACE__METHOD( Colored, Derived& ); }
        Derived&& colored( bool _enable ) && noexcept
        { PACE__METHOD( Colored, Derived&& ); }
        // Enable or disable the bold effect.
        Derived& font_bold( bool _enable ) & noexcept
        { PACE__METHOD( FontBold, Derived& ); }
        Derived&& font_bold( bool _enable ) && noexcept
        { PACE__METHOD( FontBold, Derived&& ); }
        // Enable or disable the faint effect.
        Derived& font_faint( bool _enable ) & noexcept
        { PACE__METHOD( FontFaint, Derived& ); }
        Derived&& font_faint( bool _enable ) && noexcept
        { PACE__METHOD( FontFaint, Derived&& ); }
        // Enable or disable the italic effect.
        Derived& font_italic( bool _enable ) & noexcept
        { PACE__METHOD( FontItalic, Derived& ); }
        Derived&& font_( bool _enable ) && noexcept
        { PACE__METHOD( FontItalic, Derived&& ); }
        // Enable or disable the underline effect.
        Derived& font_underline( bool _enable ) & noexcept
        { PACE__METHOD( FontUnderline, Derived& ); }
        Derived&& font_underline( bool _enable ) && noexcept
        { PACE__METHOD( FontUnderline, Derived&& ); }
        // Enable or disable the inverse effect.
        Derived& font_inverse( bool _enable ) & noexcept
        { PACE__METHOD( FontInverse, Derived& ); }
        Derived&& font_inverse( bool _enable ) && noexcept
        { PACE__METHOD( FontInverse, Derived&& ); }
        // Enable or disable the hidden effect.
        Derived& font_hidden( bool _enable ) & noexcept
        { PACE__METHOD( FontHidden, Derived& ); }
        Derived&& font_hidden( bool _enable ) && noexcept
        { PACE__METHOD( FontHidden, Derived&& ); }
        // Enable or disable the crossed effect.
        Derived& font_crossed( bool _enable ) & noexcept
        { PACE__METHOD( FontCrossed, Derived& ); }
        Derived&& font_crossed( bool _enable ) && noexcept
        { PACE__METHOD( FontCrossed, Derived&& ); }

#undef PACE__METHOD
#define PACE__METHOD( Offset )                                            \
  concurrent::SharedLock<concurrent::SharedMutex> lock { this->rw_mtx_ }; \
  return rules_[utils::to_underlying( Chroma::Offset )]

        // Check whether the color effect is enabled.
        PACE__NODISCARD bool colored() const noexcept
        { PACE__METHOD( Colored ); }
        // Check whether the bold effect is enabled.
        PACE__NODISCARD bool font_bold() const noexcept
        { PACE__METHOD( Bold ); }
        // Check whether the faint effect is enabled.
        PACE__NODISCARD bool font_faint() const noexcept
        { PACE__METHOD( Faint ); }
        // Check whether the italic effect is enabled.
        PACE__NODISCARD bool font_italic() const noexcept
        { PACE__METHOD( Italic ); }
        // Check whether the underline effect is enabled.
        PACE__NODISCARD bool font_underline() const noexcept
        { PACE__METHOD( Underline ); }
        // Check whether the inverse effect is enabled.
        PACE__NODISCARD bool font_inverse() const noexcept
        { PACE__METHOD( Inverse ); }
        // Check whether the hidden effect is enabled.
        PACE__NODISCARD bool font_hidden() const noexcept
        { PACE__METHOD( Hidden ); }
        // Check whether the crossed effect is enabled.
        PACE__NODISCARD bool font_crossed() const noexcept
        { PACE__METHOD( Crossed ); }

#undef PACE__METHOD

        PACE__CXX14_CNSTXPR void swap( RenderRule& other ) noexcept
        { std::swap( rules_, other.rules_ ); }
      };
    } // namespace aspects

    PACE__OPTION_REGISTER( aspects::RenderRule, option::Colored, option::FontBold );
  } // namespace details
} // namespace pace

#endif
