#ifndef PACE_REMAIN
#define PACE_REMAIN

#include "../charcodes/U8Raw.hpp"
#include "../traits/C3.hpp"
#include "../wrappers/OptionPacket.hpp"
#include "RenderRule.hpp"

namespace pace {
  namespace option { // A wrapper that stores the characters of the remain in the bar indicator.
    struct Remain : PACE__DERIVING_OPTION3( Remain, details::charcodes::U8Raw, _remain );

    // A wrapper that stores the foreground color of the remain in the bar indicator.
    struct RemainForecolor
      : PACE__DERIVING_OPTION1( RemainForecolor,
                                details::console::TrueColor,
                                details::wrappers::RGBValue,
                                _remain_forecolor );

    // A wrapper that stores the background color of the remain in the bar indicator.
    struct RemainBackcolor
      : PACE__DERIVING_OPTION1( RemainBackcolor,
                                details::console::TrueColor,
                                details::wrappers::RGBValue,
                                _remain_color );
  }

  namespace details {
    namespace aspects {
      template<typename Base, typename Derived>
      class Remain : public Base {
#define PACE__UNPAKING( OptionName, MemberName, Operation, Constexpr )                              \
  friend PACE__FORCEINLINE Constexpr void unpack( Remain& self, option::OptionName&& val ) noexcept \
  { self.MemberName = Operation( val.value() ); }
        PACE__UNPAKING( Remain, remain_, std::move, PACE__CXX20_CNSTXPR )
        PACE__UNPAKING( RemainForecolor, remain_forecolor_, , PACE__CXX20_CNSTXPR )
        PACE__UNPAKING( RemainBackcolor, remain_backcolor_, , PACE__CXX20_CNSTXPR )
#undef PACE__UNPAKING

      protected:
        charcodes::U8Raw remain_;
        console::TrueColor remain_forecolor_, remain_backcolor_;

        template<typename... Options>
        PACE__CXX20_CNSTXPR Remain( traits::TypeSet<Options...> tag ) : Base( tag )
        {
          using OptionSet = traits::TypeSet<Options...>;
          if PACE__CXX17_CNSTXPR ( !traits::TpContain<OptionSet, option::Remain>::value )
            unpack( *this, config::provide_for<Derived, option::Remain>() );
          if PACE__CXX17_CNSTXPR ( !traits::TpContain<OptionSet, option::RemainForecolor>::value )
            unpack( *this, config::provide_for<Derived, option::RemainForecolor>() );
          if PACE__CXX17_CNSTXPR ( !traits::TpContain<OptionSet, option::RemainBackcolor>::value )
            unpack( *this, config::provide_for<Derived, option::RemainForecolor>() );
        }

        PACE__CXX20_CNSTXPR Remain() = default;
        PACE__SPECIAL_MEMBERS_CX( Remain, PACE__CXX20_CNSTXPR );

      public:
#define PACE__METHOD( OptionName, ParamName, ReturnType, Operation ) \
  std::lock_guard<concurrent::SharedMutex> lock { this->rw_mtx_ };   \
  unpack( *this, option::OptionName( Operation( ParamName ) ) );     \
  return static_cast<ReturnType>( *this )

        /// @throw exception::InvalidArgument If the passed parameters are not coding in UTF-8.
        Derived& remain( types::String _remain ) &
        { PACE__METHOD( Remain, _remain, Derived&, std::move ); }
        Derived&& remain( types::String _remain ) &&
        { PACE__METHOD( Remain, _remain, Derived&&, std::move ); }
#ifdef __cpp_lib_char8_t
        Derived& remain( charcodes::U8StringView _remain ) &
        { PACE__METHOD( Remain, _remain, Derived&, ); }
        Derived&& remain( charcodes::U8StringView _remain ) &&
        { PACE__METHOD( Remain, _remain, Derived&&, ); }
#endif

        /// @throw exception::InvalidArgument If the passed parameters is not a valid RGB color string.
        Derived& remain_forecolor( wrappers::RGBValue _remain_forecolor ) &
        { PACE__METHOD( RemainForecolor, _remain_forecolor, Derived&, ); }
        Derived&& remain_forecolor( wrappers::RGBValue _remain_forecolor ) &&
        { PACE__METHOD( RemainForecolor, _remain_forecolor, Derived&&, ); }

        /// @throw exception::InvalidArgument If the passed parameters is not a valid RGB color string.
        Derived& remain_backcolor( wrappers::RGBValue _remain_backcolor ) &
        { PACE__METHOD( RemainForecolor, _remain_backcolor, Derived&, ); }
        Derived&& remain_backcolor( wrappers::RGBValue _remain_backcolor ) &&
        { PACE__METHOD( RemainForecolor, _remain_backcolor, Derived&&, ); }

#undef PACE__METHOD

        PACE__CXX20_CNSTXPR void swap( Remain& other ) noexcept
        {
          remain_.swap( other.remain_ );
          remain_forecolor_.swap( other.remain_forecolor_ );
          remain_backcolor_.swap( other.remain_backcolor_ );
          Base::swap( other );
        }
      };
    } // namespace aspects

    PACE__INHERIT_REGISTER( aspects::Remain, aspects::RenderRule );

    PACE__OPTION_REGISTER( aspects::Remain,
                           option::Remain,
                           option::RemainForecolor,
                           option::RemainBackcolor );
  } // namespace details
} // namespace pace

#endif
