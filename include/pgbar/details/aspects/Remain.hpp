#ifndef PGBAR_REMAIN
#define PGBAR_REMAIN

#include "../traits/C3.hpp"
#include "../wrappers/OptionPacket.hpp"
#include "RenderRule.hpp"

namespace pgbar {
  namespace option { // A wrapper that stores the characters of the remain in the bar indicator.
    struct Remain : PGBAR__DERIVING_OPTION2( Remain, _details::charcodes::U8Raw, _remain );

    // A wrapper that stores the color of the remain in the bar indicator.
    struct RemainColor
      : PGBAR__DERIVING_OPTION1( RemainColor, _details::console::escodes::RGBColor, _remain_color );
  }

  namespace _details {
    namespace aspects {
      template<typename Base, typename Derived>
      class Remain : public Base {
#define PGBAR__UNPAKING( OptionName, MemberName, Constexpr )                                         \
  friend PGBAR__FORCEINLINE Constexpr void unpack( Remain& self, option::OptionName&& val ) noexcept \
  {                                                                                                  \
    self.MemberName = std::move( val.value() );                                                      \
  }
        PGBAR__UNPAKING( Remain, remain_, PGBAR__CXX20_CNSTXPR )
        PGBAR__UNPAKING( RemainColor, remain_col_, )
#undef PGBAR__UNPAKING

      protected:
        charcodes::U8Raw remain_;
        console::escodes::RGBColor remain_col_;

        template<typename... Options>
        PGBAR__CXX20_CNSTXPR Remain( traits::TypeSet<Options...> tag ) : Base( tag )
        {
          using OptionSet = traits::TypeSet<Options...>;
          if PGBAR__CXX17_CNSTXPR ( !traits::TpContain<OptionSet, option::Remain>::value )
            unpack( *this, utils::provide_for<Derived, option::Remain>() );
          if PGBAR__CXX17_CNSTXPR ( !traits::TpContain<OptionSet, option::RemainColor>::value )
            unpack( *this, utils::provide_for<Derived, option::RemainColor>() );
        }

        PGBAR__CXX20_CNSTXPR Remain() = default;
        PGBAR__SPECIAL_MEMBERS_CX( Remain, PGBAR__CXX20_CNSTXPR );

      public:
#define PGBAR__METHOD( OptionName, ParamName, ReturnType, Operation ) \
  std::lock_guard<concurrent::SharedMutex> lock { this->rw_mtx_ };    \
  unpack( *this, option::OptionName( Operation( ParamName ) ) );      \
  return static_cast<ReturnType>( *this )

        /// @throw exception::InvalidArgument If the passed parameters is not a valid RGB color string.
        Derived& remain_color( console::escodes::RGBColor _remain_color ) &
        {
          PGBAR__METHOD( RemainColor, _remain_color, Derived&, std::move );
        }
        Derived&& remain_color( console::escodes::RGBColor _remain_color ) &&
        {
          PGBAR__METHOD( RemainColor, _remain_color, Derived&&, std::move );
        }

        /// @throw exception::InvalidArgument If the passed parameters are not coding in UTF-8.
        Derived& remain( types::String _remain ) &
        {
          PGBAR__METHOD( Remain, _remain, Derived&, std::move );
        }
        Derived&& remain( types::String _remain ) &&
        {
          PGBAR__METHOD( Remain, _remain, Derived&&, std::move );
        }
#ifdef __cpp_lib_char8_t
        Derived& remain( types::LitU8 _remain ) &
        {
          PGBAR__METHOD( Remain, _remain, Derived&, );
        }
        Derived&& remain( types::LitU8 _remain ) &&
        {
          PGBAR__METHOD( Remain, _remain, Derived&&, );
        }
#endif

#undef PGBAR__METHOD

        PGBAR__CXX20_CNSTXPR void swap( Remain& other ) noexcept
        {
          remain_col_.swap( other.remain_col_ );
          remain_.swap( other.remain_ );
          Base::swap( other );
        }
      };
    } // namespace aspects

    PGBAR__INHERIT_REGISTER( aspects::Remain, aspects::RenderRule );

    PGBAR__OPTION_REGISTER( aspects::Remain, option::Remain, option::RemainColor );
  } // namespace _details
} // namespace pgbar

#endif
