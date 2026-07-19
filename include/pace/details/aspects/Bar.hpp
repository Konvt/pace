#ifndef PACE_BAR
#define PACE_BAR

#include "../charcodes/U8Raw.hpp"
#include "../console/TrueColor.hpp"
#include "../traits/C3.hpp"
#include "RenderRule.hpp"

namespace pace {
  namespace option {
    // A wrapper that stores the width of the bar indicator, in the character unit.
    struct BarWidth : PACE__DERIVING_OPTION2( BarWidth, std::uint16_t, _num_char );

    // A wrapper that stores characters located to the left of the bar indicator.
    struct Starting : PACE__DERIVING_OPTION3( Starting, details::charcodes::U8Raw, _starting );

    // A wrapper that stores characters located to the right of the bar indicator.
    struct Ending : PACE__DERIVING_OPTION3( Ending, details::charcodes::U8Raw, _ending );

    // A wrapper that stores the foreground color of component located to the left of the bar indicator.
    struct StartForecolor
      : PACE__DERIVING_OPTION1( StartForecolor,
                                details::console::TrueColor,
                                details::wrappers::RGBValue,
                                _start_forecolor );

    // A wrapper that stores the foreground color of component located to the left of the bar indicator.
    struct StartBackcolor
      : PACE__DERIVING_OPTION1( StartBackcolor,
                                details::console::TrueColor,
                                details::wrappers::RGBValue,
                                _start_backcolor );

    // A wrapper that stores the foreground color of component located to the right of the bar indicator.
    struct EndForecolor
      : PACE__DERIVING_OPTION1( EndForecolor,
                                details::console::TrueColor,
                                details::wrappers::RGBValue,
                                _end_forecolor );

    // A wrapper that stores the foreground color of component located to the right of the bar indicator.
    struct EndBackcolor
      : PACE__DERIVING_OPTION1( EndBackcolor,
                                details::console::TrueColor,
                                details::wrappers::RGBValue,
                                _end_backcolor );
  } // namespace option

  namespace details {
    namespace aspects {
      template<typename Base, typename Derived>
      class Bar : public Base {
#define PACE__UNPAKING( OptionName, MemberName, Operation, Constexpr )                           \
  friend PACE__FORCEINLINE Constexpr void unpack( Bar& self, option::OptionName&& val ) noexcept \
  { self.MemberName = Operation( val.value() ); }
        PACE__UNPAKING( Starting, starting_, std::move, PACE__CXX20_CNSTXPR )
        PACE__UNPAKING( Ending, ending_, std::move, PACE__CXX20_CNSTXPR )
        PACE__UNPAKING( BarWidth, bar_width_, , PACE__CXX20_CNSTXPR )
        PACE__UNPAKING( StartForecolor, start_forecolor_, , PACE__CXX20_CNSTXPR )
        PACE__UNPAKING( StartBackcolor, start_backcolor_, , PACE__CXX20_CNSTXPR )
        PACE__UNPAKING( EndForecolor, end_forecolor_, , PACE__CXX20_CNSTXPR )
        PACE__UNPAKING( EndBackcolor, end_backcolor_, , PACE__CXX20_CNSTXPR )
#undef PACE__UNPAKING

      protected:
        charcodes::U8Raw starting_, ending_;
        console::TrueColor start_forecolor_, start_backcolor_;
        console::TrueColor end_forecolor_, end_backcolor_;
        std::uint16_t bar_width_;

        PACE__NODISCARD PACE__FORCEINLINE PACE__CXX20_CNSTXPR std::size_t fixed_length() const noexcept
        { return starting_.width() + ending_.width(); }

        template<typename... Options>
        PACE__CXX20_CNSTXPR Bar( traits::TypeSet<Options...> tag ) : Base( tag )
        {
          using OptionSet = traits::TypeSet<Options...>;
          if PACE__CXX17_CNSTXPR ( !traits::TpContains<OptionSet, option::Starting>::value )
            unpack( *this, config::provide_for<Derived, option::Starting>() );
          if PACE__CXX17_CNSTXPR ( !traits::TpContains<OptionSet, option::Ending>::value )
            unpack( *this, config::provide_for<Derived, option::Ending>() );
          if PACE__CXX17_CNSTXPR ( !traits::TpContains<OptionSet, option::BarWidth>::value )
            unpack( *this, config::provide_for<Derived, option::BarWidth>() );
          if PACE__CXX17_CNSTXPR ( !traits::TpContains<OptionSet, option::StartForecolor>::value )
            unpack( *this, config::provide_for<Derived, option::StartForecolor>() );
          if PACE__CXX17_CNSTXPR ( !traits::TpContains<OptionSet, option::StartBackcolor>::value )
            unpack( *this, config::provide_for<Derived, option::StartBackcolor>() );
          if PACE__CXX17_CNSTXPR ( !traits::TpContains<OptionSet, option::EndForecolor>::value )
            unpack( *this, config::provide_for<Derived, option::EndForecolor>() );
          if PACE__CXX17_CNSTXPR ( !traits::TpContains<OptionSet, option::EndBackcolor>::value )
            unpack( *this, config::provide_for<Derived, option::EndBackcolor>() );
        }

        PACE__CXX20_CNSTXPR Bar() = default;
        PACE__SPECIAL_MEMBERS_CX( Bar, PACE__CXX20_CNSTXPR );

      public:
#define PACE__METHOD( OptionName, ParamName, ReturnType, Operation ) \
  std::lock_guard<concurrent::SharedMutex> lock { this->rw_mtx_ };   \
  unpack( *this, option::OptionName( Operation( ParamName ) ) );     \
  return static_cast<ReturnType>( *this )

        /// @throw exception::InvalidArgument If the passed parameters are not coding in UTF-8.
        Derived& starting( std::string _starting ) &
        { PACE__METHOD( Starting, _starting, Derived&, std::move ); }
        Derived&& starting( std::string _starting ) &&
        { PACE__METHOD( Starting, _starting, Derived&&, std::move ); }
        /// @throw exception::InvalidArgument If the passed parameters are not coding in UTF-8.
        Derived& ending( std::string _ending ) &
        { PACE__METHOD( Ending, _ending, Derived&, std::move ); }
        Derived&& ending( std::string _ending ) &&
        { PACE__METHOD( Ending, _ending, Derived&&, std::move ); }
#ifdef __cpp_lib_char8_t
        Derived& starting( charcodes::U8StringView _starting ) &
        { PACE__METHOD( Starting, _starting, Derived&, ); }
        Derived&& starting( charcodes::U8StringView _starting ) &&
        { PACE__METHOD( Starting, _starting, Derived&&, ); }
        Derived& ending( charcodes::U8StringView _ending ) &
        { PACE__METHOD( Ending, _ending, Derived&, ); }
        Derived&& ending( charcodes::U8StringView _ending ) &&
        { PACE__METHOD( Ending, _ending, Derived&&, ); }
#endif

        /// @throw exception::InvalidArgument  If the passed parameters is not a valid RGB color string.
        Derived& start_forecolor( wrappers::RGBValue _start_forecolor ) &
        { PACE__METHOD( StartForecolor, _start_forecolor, Derived&, ); }
        Derived&& start_forecolor( wrappers::RGBValue _start_forecolor ) &&
        { PACE__METHOD( StartForecolor, _start_forecolor, Derived&&, ); }

        /// @throw exception::InvalidArgument  If the passed parameters is not a valid RGB color string.
        Derived& start_backcolor( wrappers::RGBValue _start_backcolor ) &
        { PACE__METHOD( StartBackcolor, _start_backcolor, Derived&, ); }
        Derived&& start_backcolor( wrappers::RGBValue _start_backcolor ) &&
        { PACE__METHOD( StartBackcolor, _start_backcolor, Derived&&, ); }

        /// @throw exception::InvalidArgument If the passed parameters is not a valid RGB color string.
        Derived& end_forecolor( wrappers::RGBValue _end_forecolor ) &
        { PACE__METHOD( EndForecolor, _end_forecolor, Derived&, ); }
        Derived&& end_forecolor( wrappers::RGBValue _end_forecolor ) &&
        { PACE__METHOD( EndForecolor, _end_forecolor, Derived&&, ); }

        /// @throw exception::InvalidArgument If the passed parameters is not a valid RGB color string.
        Derived& end_backcolor( wrappers::RGBValue _end_backcolor ) &
        { PACE__METHOD( EndBackcolor, _end_backcolor, Derived&, ); }
        Derived&& end_backcolor( wrappers::RGBValue _end_backcolor ) &&
        { PACE__METHOD( EndBackcolor, _end_backcolor, Derived&&, ); }

        // Set the width of the bar indicator.
        Derived& bar_width( std::uint16_t _width ) & noexcept
        { PACE__METHOD( BarWidth, _width, Derived&, ); }
        Derived&& bar_width( std::uint16_t _width ) && noexcept
        { PACE__METHOD( BarWidth, _width, Derived&&, ); }

#undef PACE__METHOD

        PACE__NODISCARD std::uint16_t bar_width() const noexcept
        {
          concurrent::SharedLock<concurrent::SharedMutex> lock { this->rw_mtx_ };
          return bar_width_;
        }

        PACE__CXX20_CNSTXPR void swap( Bar& other ) noexcept
        {
          starting_.swap( other.starting_ );
          ending_.swap( other.ending_ );
          start_forecolor_.swap( other.start_forecolor_ );
          start_backcolor_.swap( other.start_backcolor_ );
          end_forecolor_.swap( other.end_forecolor_ );
          end_backcolor_.swap( other.end_backcolor_ );
          std::swap( bar_width_, other.bar_width_ );
          Base::swap( other );
        }
      };
    } // namespace aspects

    PACE__INHERIT_REGISTER( aspects::Bar, aspects::RenderRule );

    PACE__OPTION_REGISTER( aspects::Bar,
                           option::BarWidth,
                           option::Starting,
                           option::Ending,
                           option::StartForecolor,
                           option::StartBackcolor,
                           option::EndForecolor,
                           option::EndBackcolor );
  } // namespace details
} // namespace pace

#endif
