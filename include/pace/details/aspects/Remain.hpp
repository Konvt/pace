#ifndef PACE_REMAIN
#define PACE_REMAIN

#include "../traits/C3.hpp"
#include "../wrappers/OptionPacket.hpp"
#include "RenderRule.hpp"

namespace pace {
  namespace option { // A wrapper that stores the characters of the remain in the bar indicator.
    struct Remain : PACE__DERIVING_OPTION2( Remain, details::charcodes::U8Raw, _remain );

    // A wrapper that stores the color of the remain in the bar indicator.
    struct RemainColor
      : PACE__DERIVING_OPTION1( RemainColor, details::console::escodes::RGBColor, _remain_color );
  }

  namespace details {
    namespace aspects {
      template<typename Base, typename Derived>
      class Remain : public Base {
#define PACE__UNPAKING( OptionName, MemberName, Constexpr )                                         \
  friend PACE__FORCEINLINE Constexpr void unpack( Remain& self, option::OptionName&& val ) noexcept \
  { self.MemberName = std::move( val.value() ); }
        PACE__UNPAKING( Remain, remain_, PACE__CXX20_CNSTXPR )
        PACE__UNPAKING( RemainColor, remain_col_, )
#undef PACE__UNPAKING

      protected:
        charcodes::U8Raw remain_;
        console::escodes::RGBColor remain_col_;

        template<typename... Options>
        PACE__CXX20_CNSTXPR Remain( traits::TypeSet<Options...> tag ) : Base( tag )
        {
          using OptionSet = traits::TypeSet<Options...>;
          if PACE__CXX17_CNSTXPR ( !traits::TpContain<OptionSet, option::Remain>::value )
            unpack( *this, config::provide_for<Derived, option::Remain>() );
          if PACE__CXX17_CNSTXPR ( !traits::TpContain<OptionSet, option::RemainColor>::value )
            unpack( *this, config::provide_for<Derived, option::RemainColor>() );
        }

        PACE__CXX20_CNSTXPR Remain() = default;
        PACE__SPECIAL_MEMBERS_CX( Remain, PACE__CXX20_CNSTXPR );

      public:
#define PACE__METHOD( OptionName, ParamName, ReturnType, Operation ) \
  std::lock_guard<concurrent::SharedMutex> lock { this->rw_mtx_ };   \
  unpack( *this, option::OptionName( Operation( ParamName ) ) );     \
  return static_cast<ReturnType>( *this )

        /// @throw exception::InvalidArgument If the passed parameters is not a valid RGB color string.
        Derived& remain_color( console::escodes::RGBColor _remain_color ) &
        { PACE__METHOD( RemainColor, _remain_color, Derived&, std::move ); }
        Derived&& remain_color( console::escodes::RGBColor _remain_color ) &&
        { PACE__METHOD( RemainColor, _remain_color, Derived&&, std::move ); }

        /// @throw exception::InvalidArgument If the passed parameters are not coding in UTF-8.
        Derived& remain( types::String _remain ) &
        { PACE__METHOD( Remain, _remain, Derived&, std::move ); }
        Derived&& remain( types::String _remain ) &&
        { PACE__METHOD( Remain, _remain, Derived&&, std::move ); }
#ifdef __cpp_lib_char8_t
        Derived& remain( types::LitU8 _remain ) &
        { PACE__METHOD( Remain, _remain, Derived&, ); }
        Derived&& remain( types::LitU8 _remain ) &&
        { PACE__METHOD( Remain, _remain, Derived&&, ); }
#endif

#undef PACE__METHOD

        PACE__CXX20_CNSTXPR void swap( Remain& other ) noexcept
        {
          remain_col_.swap( other.remain_col_ );
          remain_.swap( other.remain_ );
          Base::swap( other );
        }
      };
    } // namespace aspects

    PACE__INHERIT_REGISTER( aspects::Remain, aspects::RenderRule );

    PACE__OPTION_REGISTER( aspects::Remain, option::Remain, option::RemainColor );
  } // namespace details
} // namespace pace

#endif
