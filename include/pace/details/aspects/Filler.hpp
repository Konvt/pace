#ifndef PACE_FILLER
#define PACE_FILLER

#include "../traits/C3.hpp"
#include "../wrappers/OptionPacket.hpp"
#include "RenderRule.hpp"

namespace pace {
  namespace option {
    // A wrapper that stores the characters of the filler in the bar indicator.
    struct Filler : PACE__DERIVING_OPTION2( Filler, details::charcodes::U8Raw, _filler );

    // A wrapper that stores the color of the filler in the bar indicator.
    struct FillerColor
      : PACE__DERIVING_OPTION1( FillerColor, details::console::escodes::RGBColor, _filler_color );
  }

  namespace details {
    namespace aspects {
      template<typename Base, typename Derived>
      class Filler : public Base {
#define PACE__UNPAKING( OptionName, MemberName, Constexpr )                                         \
  friend PACE__FORCEINLINE Constexpr void unpack( Filler& self, option::OptionName&& val ) noexcept \
  {                                                                                                 \
    self.MemberName = std::move( val.value() );                                                     \
  }
        PACE__UNPAKING( Filler, filler_, PACE__CXX20_CNSTXPR )
        PACE__UNPAKING( FillerColor, filler_col_, )
#undef PACE__UNPAKING

      protected:
        charcodes::U8Raw filler_;
        console::escodes::RGBColor filler_col_;

        template<typename... Options>
        PACE__CXX20_CNSTXPR Filler( traits::TypeSet<Options...> tag ) : Base( tag )
        {
          using OptionSet = traits::TypeSet<Options...>;
          if PACE__CXX17_CNSTXPR ( !traits::TpContain<OptionSet, option::Filler>::value )
            unpack( *this, config::provide_for<Derived, option::Filler>() );
          if PACE__CXX17_CNSTXPR ( !traits::TpContain<OptionSet, option::FillerColor>::value )
            unpack( *this, config::provide_for<Derived, option::FillerColor>() );
        }

        PACE__CXX20_CNSTXPR Filler() = default;
        PACE__SPECIAL_MEMBERS_CX( Filler, PACE__CXX20_CNSTXPR );

      public:
#define PACE__METHOD( OptionName, ParamName, ReturnType, Operation ) \
  std::lock_guard<concurrent::SharedMutex> lock { this->rw_mtx_ };   \
  unpack( *this, option::OptionName( Operation( ParamName ) ) );     \
  return static_cast<ReturnType>( *this )

        /// @throw exception::InvalidArgument If the passed parameters are not coding in UTF-8.
        Derived& filler( types::String _filler ) &
        {
          PACE__METHOD( Filler, _filler, Derived&, std::move );
        }
        Derived&& filler( types::String _filler ) &&
        {
          PACE__METHOD( Filler, _filler, Derived&&, std::move );
        }
#ifdef __cpp_lib_char8_t
        Derived& filler( types::LitU8 _filler ) &
        {
          PACE__METHOD( Filler, _filler, Derived&, );
        }
        Derived&& filler( types::LitU8 _filler ) &&
        {
          PACE__METHOD( Filler, _filler, Derived&&, );
        }
#endif

        /// @throw exception::InvalidArgument If the passed parameters is not a valid RGB color string.
        Derived& filler_color( console::escodes::RGBColor _filler_color ) &
        {
          PACE__METHOD( FillerColor, _filler_color, Derived&, std::move );
        }
        Derived&& filler_color( console::escodes::RGBColor _filler_color ) &&
        {
          PACE__METHOD( FillerColor, _filler_color, Derived&&, std::move );
        }

#undef PACE__METHOD

        PACE__CXX20_CNSTXPR void swap( Filler& other ) noexcept
        {
          filler_.swap( other.filler_ );
          filler_col_.swap( other.filler_col_ );
          Base::swap( other );
        }
      };
    } // namespace aspects

    PACE__INHERIT_REGISTER( aspects::Filler, aspects::RenderRule );

    PACE__OPTION_REGISTER( aspects::Filler, option::Filler, option::FillerColor );
  } // namespace details
} // namespace pace

#endif
