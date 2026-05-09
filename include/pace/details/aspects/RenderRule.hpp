#ifndef PACE_RENDER_RULE
#define PACE_RENDER_RULE

#include "../../config/Provider.hpp"
#include "../concurrent/SharedLock.hpp"
#include "../concurrent/SharedMutex.hpp"
#include "../console/escodes/Escodes.hpp"
#include "../wrappers/Brush.hpp"
#include "../wrappers/OptionPacket.hpp"
#include <bitset>
#include <mutex>

namespace pace {
  namespace option {
    // A wrapper that stores the value of the color effect setting.
    struct Colored : PACE__DERIVING_OPTION1( Colored, bool, _enable );

    // A wrapper that stores the value of the font boldness setting.
    struct Bolded : PACE__DERIVING_OPTION1( Bolded, bool, _enable );
  } // namespace option

  namespace details {
    namespace aspects {
      template<typename Base, typename Derived>
      class RenderRule : public Base {
#define PACE__UNPAKING( OptionName, MemberName )                                                \
  friend PACE__FORCEINLINE PACE__CXX14_CNSTXPR void unpack( RenderRule& self,                   \
                                                            option::OptionName&& val ) noexcept \
  { self.rules_[utils::to_underlying( Chroma::OptionName )] = val.value(); }
        PACE__UNPAKING( Colored, colored_ )
        PACE__UNPAKING( Bolded, bolded_ )
#undef PACE__UNPAKING

        enum class Chroma : std::uint8_t { Colored = 0, Bolded };
        std::bitset<2> rules_;

      protected:
        PACE__NODISCARD PACE__FORCEINLINE PACE__CXX20_CNSTXPR wrappers::Brush<console::escodes::RGBColor>
          with_dye( const console::escodes::RGBColor& rgb, bool style_off ) const
        {
#ifdef PACE_NOSTYLE
          (void)rgb;
          (void)style_off;
#else
          if ( !style_off && rules_[utils::to_underlying( Chroma::Colored )] )
            return { &rgb };
#endif
          return { nullptr };
        }
        PACE__NODISCARD PACE__FORCEINLINE PACE__CXX20_CNSTXPR
          wrappers::Brush<console::escodes::RGBColor,
                          wrappers::Brush_t<decltype( console::escodes::fontbold )>>
          with_style( const console::escodes::RGBColor& rgb, bool style_off ) const
        {
#ifdef PACE_NOSTYLE
          (void)rgb;
          (void)style_off;
          return { nullptr, { nullptr } };
#else
          // The types::LitU8 changes between different C++ standard,
          // thus we need a decltype to generate the correct wrapper.
          if ( !style_off && rules_[utils::to_underlying( Chroma::Bolded )] )
            return { with_dye( rgb, style_off ).effect_, &console::escodes::fontbold };
          return { with_dye( rgb, style_off ).effect_, { nullptr } };
#endif
        }

        PACE__NODISCARD PACE__FORCEINLINE PACE__CXX20_CNSTXPR
          wrappers::Brush_t<decltype( console::escodes::fontbold )>
          with_reset( bool style_off ) const
        {
#ifdef PACE_NOSTYLE
          (void)style_off;
#else
          if ( !style_off && rules_.any() )
            return { &console::escodes::fontreset };
#endif
          return { nullptr };
        }
        PACE__NODISCARD PACE__FORCEINLINE PACE__CXX20_CNSTXPR
          wrappers::Brush_t<decltype( console::escodes::fontbold ),
                            wrappers::Brush<console::escodes::RGBColor>>
          reset_then_dye( const console::escodes::RGBColor& rgb, bool style_off ) const
        {
#ifndef PACE_NOSTYLE
          if ( !style_off && rules_.any() )
            return { &console::escodes::fontreset, with_dye( rgb, style_off ) };
#endif
          return { nullptr, with_dye( rgb, style_off ) };
        }
        PACE__NODISCARD PACE__FORCEINLINE PACE__CXX20_CNSTXPR
          wrappers::Brush_t<decltype( console::escodes::fontbold ),
                            wrappers::Brush<console::escodes::RGBColor,
                                            wrappers::Brush_t<decltype( console::escodes::fontbold )>>>
          reset_then_style( const console::escodes::RGBColor& rgb, bool style_off ) const
        {
#ifndef PACE_NOSTYLE
          if ( rules_.any() )
            return { &console::escodes::fontreset, with_style( rgb, style_off ) };
#endif
          return { nullptr, with_style( rgb, style_off ) };
        }

        template<typename... Options>
        PACE__CXX14_CNSTXPR RenderRule( traits::TypeSet<Options...> tag ) : Base( tag )
        {
          using OptionSet = traits::TypeSet<Options...>;
          if PACE__CXX17_CNSTXPR ( !traits::TpContain<OptionSet, option::Colored>::value )
            unpack( *this, config::provide_for<Derived, option::Colored>() );
          if PACE__CXX17_CNSTXPR ( !traits::TpContain<OptionSet, option::Bolded>::value )
            unpack( *this, config::provide_for<Derived, option::Bolded>() );
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
        Derived& bolded( bool _enable ) & noexcept
        { PACE__METHOD( Bolded, Derived& ); }
        Derived&& bolded( bool _enable ) && noexcept
        { PACE__METHOD( Bolded, Derived&& ); }

#undef PACE__METHOD
#define PACE__METHOD( Offset )                                            \
  concurrent::SharedLock<concurrent::SharedMutex> lock { this->rw_mtx_ }; \
  return rules_[utils::to_underlying( Chroma::Offset )]

        // Check whether the color effect is enabled.
        PACE__NODISCARD bool colored() const noexcept
        { PACE__METHOD( Colored ); }
        // Check whether the bold effect is enabled.
        PACE__NODISCARD bool bolded() const noexcept
        { PACE__METHOD( Bolded ); }

#undef PACE__METHOD

        PACE__CXX14_CNSTXPR void swap( RenderRule& other ) noexcept
        { std::swap( rules_, other.rules_ ); }
      };
    } // namespace aspects

    PACE__OPTION_REGISTER( aspects::RenderRule, option::Colored, option::Bolded );
  } // namespace details
} // namespace pace

#endif
