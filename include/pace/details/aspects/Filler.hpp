#ifndef PACE_FILLER
#define PACE_FILLER

#include "../charcodes/U8Raw.hpp"
#include "../traits/C3.hpp"
#include "../wrappers/OptionPacket.hpp"
#include "RenderRule.hpp"

namespace pace {
  namespace option {
    // A wrapper that stores the characters of the filler in the bar indicator.
    struct Filler : PACE__DERIVING_OPTION3( Filler, details::charcodes::U8Raw, _filler );

    // A wrapper that stores the foreground color of the filler in the bar indicator.
    struct FillerForecolor
      : PACE__DERIVING_OPTION1( FillerForecolor,
                                details::console::TrueColor,
                                details::wrappers::RGBValue,
                                _filler_forecolor );

    // A wrapper that stores the background color of the filler in the bar indicator.
    struct FillerBackcolor
      : PACE__DERIVING_OPTION1( FillerBackcolor,
                                details::console::TrueColor,
                                details::wrappers::RGBValue,
                                _filler_backcolor );
  } // namespace option

  namespace details {
    namespace aspects {
      template<typename Base, typename Derived>
      class Filler : public Base {
#define PACE__UNPAKING( OptionName, MemberName, Operation, Constexpr )                              \
  friend PACE__FORCEINLINE Constexpr void unpack( Filler& self, option::OptionName&& val ) noexcept \
  { self.MemberName = Operation( val.value() ); }
        PACE__UNPAKING( Filler, filler_, std::move, PACE__CXX20_CNSTXPR )
        PACE__UNPAKING( FillerForecolor, filler_forecolor_, , PACE__CXX20_CNSTXPR )
        PACE__UNPAKING( FillerBackcolor, filler_backcolor_, , PACE__CXX20_CNSTXPR )
#undef PACE__UNPAKING

      protected:
        charcodes::U8Raw filler_;
        console::TrueColor filler_forecolor_, filler_backcolor_;

        template<typename... Options>
        PACE__CXX20_CNSTXPR Filler( traits::TypeSet<Options...> tag ) : Base( tag )
        {
          using OptionSet = traits::TypeSet<Options...>;
          if PACE__CXX17_CNSTXPR ( !traits::TpContain<OptionSet, option::Filler>::value )
            unpack( *this, config::provide_for<Derived, option::Filler>() );
          if PACE__CXX17_CNSTXPR ( !traits::TpContain<OptionSet, option::FillerForecolor>::value )
            unpack( *this, config::provide_for<Derived, option::FillerForecolor>() );
          if PACE__CXX17_CNSTXPR ( !traits::TpContain<OptionSet, option::FillerBackcolor>::value )
            unpack( *this, config::provide_for<Derived, option::FillerBackcolor>() );
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
        { PACE__METHOD( Filler, _filler, Derived&, std::move ); }
        Derived&& filler( types::String _filler ) &&
        { PACE__METHOD( Filler, _filler, Derived&&, std::move ); }
#ifdef __cpp_lib_char8_t
        Derived& filler( charcodes::U8StringView _filler ) &
        { PACE__METHOD( Filler, _filler, Derived&, ); }
        Derived&& filler( charcodes::U8StringView _filler ) &&
        { PACE__METHOD( Filler, _filler, Derived&&, ); }
#endif

        /// @throw exception::InvalidArgument If the passed parameters is not a valid RGB color string.
        Derived& filler_forecolor( wrappers::RGBValue _filler_forecolor ) &
        { PACE__METHOD( FillerForecolor, _filler_forecolor, Derived&, ); }
        Derived&& filler_forecolor( wrappers::RGBValue _filler_forecolor ) &&
        { PACE__METHOD( FillerForecolor, _filler_forecolor, Derived&&, ); }

        /// @throw exception::InvalidArgument If the passed parameters is not a valid RGB color string.
        Derived& filler_backcolor( wrappers::RGBValue _filler_backcolor ) &
        { PACE__METHOD( FillerBackcolor, _filler_backcolor, Derived&, ); }
        Derived&& filler_backcolor( wrappers::RGBValue _filler_backcolor ) &&
        { PACE__METHOD( FillerBackcolor, _filler_backcolor, Derived&&, ); }

#undef PACE__METHOD

        PACE__CXX20_CNSTXPR void swap( Filler& other ) noexcept
        {
          filler_.swap( other.filler_ );
          filler_forecolor_.swap( other.filler_forecolor_ );
          filler_backcolor_.swap( other.filler_backcolor_ );
          Base::swap( other );
        }
      };
    } // namespace aspects

    PACE__INHERIT_REGISTER( aspects::Filler, aspects::RenderRule );

    PACE__OPTION_REGISTER( aspects::Filler,
                           option::Filler,
                           option::FillerForecolor,
                           option::FillerBackcolor );
  } // namespace details
} // namespace pace

#endif
