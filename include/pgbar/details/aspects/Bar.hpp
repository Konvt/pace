#ifndef PGBAR_BAR
#define PGBAR_BAR

#include "../traits/C3.hpp"
#include "RenderRule.hpp"

namespace pgbar {
  namespace option {
    // A wrapper that stores the width of the bar indicator, in the character unit.
    struct BarWidth : PGBAR__DERIVING_OPTION1( BarWidth, std::uint16_t, _num_char );

    // A wrapper that stores characters located to the left of the bar indicator.
    struct Starting : PGBAR__DERIVING_OPTION2( Starting, _details::charcodes::U8Raw, _starting );

    // A wrapper that stores characters located to the right of the bar indicator.
    struct Ending : PGBAR__DERIVING_OPTION2( Ending, _details::charcodes::U8Raw, _ending );

    // A wrapper that stores the color of component located to the left of the bar indicator.
    struct StartColor
      : PGBAR__DERIVING_OPTION1( StartColor, _details::console::escodes::RGBColor, _start_color );

    // A wrapper that stores the color of component located to the right of the bar indicator.
    struct EndColor : PGBAR__DERIVING_OPTION1( EndColor, _details::console::escodes::RGBColor, _end_color );
  } // namespace option

  namespace _details {
    namespace aspects {
      template<typename Base, typename Derived>
      class Bar : public Base {
#define PGBAR__UNPAKING( OptionName, MemberName, Constexpr )                                      \
  friend PGBAR__FORCEINLINE Constexpr void unpack( Bar& self, option::OptionName&& val ) noexcept \
  {                                                                                               \
    self.MemberName = std::move( val.value() );                                                   \
  }
        PGBAR__UNPAKING( Starting, starting_, PGBAR__CXX20_CNSTXPR )
        PGBAR__UNPAKING( Ending, ending_, PGBAR__CXX20_CNSTXPR )
        PGBAR__UNPAKING( BarWidth, bar_width_, PGBAR__CXX20_CNSTXPR )
        PGBAR__UNPAKING( StartColor, start_col_, )
        PGBAR__UNPAKING( EndColor, end_col_, )
#undef PGBAR__UNPAKING

      protected:
        charcodes::U8Raw starting_, ending_;
        console::escodes::RGBColor start_col_, end_col_;
        std::uint16_t bar_width_;

        PGBAR__NODISCARD PGBAR__FORCEINLINE PGBAR__CXX20_CNSTXPR types::Size fixed_length() const noexcept
        {
          return starting_.width() + ending_.width();
        }

        template<typename... Options>
        PGBAR__CXX20_CNSTXPR Bar( traits::TypeSet<Options...> tag ) : Base( tag )
        {
          using OptionSet = traits::TypeSet<Options...>;
          if PGBAR__CXX17_CNSTXPR ( !traits::TpContain<OptionSet, option::Starting>::value )
            unpack( *this, utils::provide_for<Derived, option::Starting>() );
          if PGBAR__CXX17_CNSTXPR ( !traits::TpContain<OptionSet, option::Ending>::value )
            unpack( *this, utils::provide_for<Derived, option::Ending>() );
          if PGBAR__CXX17_CNSTXPR ( !traits::TpContain<OptionSet, option::BarWidth>::value )
            unpack( *this, utils::provide_for<Derived, option::BarWidth>() );
          if PGBAR__CXX17_CNSTXPR ( !traits::TpContain<OptionSet, option::StartColor>::value )
            unpack( *this, utils::provide_for<Derived, option::StartColor>() );
          if PGBAR__CXX17_CNSTXPR ( !traits::TpContain<OptionSet, option::EndColor>::value )
            unpack( *this, utils::provide_for<Derived, option::EndColor>() );
        }

        PGBAR__CXX20_CNSTXPR Bar() = default;
        PGBAR__SPECIAL_MEMBERS_CX( Bar, PGBAR__CXX20_CNSTXPR );

      public:
#define PGBAR__METHOD( OptionName, ParamName, ReturnType, Operation ) \
  std::lock_guard<concurrent::SharedMutex> lock { this->rw_mtx_ };    \
  unpack( *this, option::OptionName( Operation( ParamName ) ) );      \
  return static_cast<ReturnType>( *this )

        /// @throw exception::InvalidArgument If the passed parameters are not coding in UTF-8.
        Derived& starting( types::String _starting ) &
        {
          PGBAR__METHOD( Starting, _starting, Derived&, std::move );
        }
        Derived&& starting( types::String _starting ) &&
        {
          PGBAR__METHOD( Starting, _starting, Derived&&, std::move );
        }
        /// @throw exception::InvalidArgument If the passed parameters are not coding in UTF-8.
        Derived& ending( types::String _ending ) &
        {
          PGBAR__METHOD( Ending, _ending, Derived&, std::move );
        }
        Derived&& ending( types::String _ending ) &&
        {
          PGBAR__METHOD( Ending, _ending, Derived&&, std::move );
        }
#ifdef __cpp_lib_char8_t
        Derived& starting( types::LitU8 _starting ) &
        {
          PGBAR__METHOD( Starting, _starting, Derived&, );
        }
        Derived&& starting( types::LitU8 _starting ) &&
        {
          PGBAR__METHOD( Starting, _starting, Derived&&, );
        }
        Derived& ending( types::LitU8 _ending ) &
        {
          PGBAR__METHOD( Ending, _ending, Derived&, );
        }
        Derived&& ending( types::LitU8 _ending ) &&
        {
          PGBAR__METHOD( Ending, _ending, Derived&&, );
        }
#endif

        /// @throw exception::InvalidArgument  If the passed parameters is not a valid RGB color string.
        Derived& start_color( console::escodes::RGBColor _start_color ) &
        {
          PGBAR__METHOD( StartColor, _start_color, Derived&, std::move );
        }
        Derived&& start_color( console::escodes::RGBColor _start_color ) &&
        {
          PGBAR__METHOD( StartColor, _start_color, Derived&&, std::move );
        }
        /// @throw exception::InvalidArgument If the passed parameters is not a valid RGB color string.
        Derived& end_color( console::escodes::RGBColor _end_color ) &
        {
          PGBAR__METHOD( EndColor, _end_color, Derived&, std::move );
        }
        Derived&& end_color( console::escodes::RGBColor _end_color ) &&
        {
          PGBAR__METHOD( EndColor, _end_color, Derived&&, std::move );
        }

        // Set the width of the bar indicator.
        Derived& bar_width( std::uint16_t _width ) & noexcept
        {
          PGBAR__METHOD( BarWidth, _width, Derived&, );
        }
        Derived&& bar_width( std::uint16_t _width ) && noexcept
        {
          PGBAR__METHOD( BarWidth, _width, Derived&&, );
        }

#undef PGBAR__METHOD

        PGBAR__NODISCARD std::uint16_t bar_width() const noexcept
        {
          concurrent::SharedLock<concurrent::SharedMutex> lock { this->rw_mtx_ };
          return bar_width_;
        }

        PGBAR__CXX20_CNSTXPR void swap( Bar& other ) noexcept
        {
          std::swap( bar_width_, other.bar_width_ );
          starting_.swap( other.starting_ );
          ending_.swap( other.ending_ );
          start_col_.swap( other.start_col_ );
          end_col_.swap( other.end_col_ );
          Base::swap( other );
        }
      };
    } // namespace aspects

    PGBAR__INHERIT_REGISTER( aspects::Bar, aspects::RenderRule );

    PGBAR__OPTION_REGISTER( aspects::Bar,
                            option::Starting,
                            option::Ending,
                            option::StartColor,
                            option::EndColor,
                            option::BarWidth );
  } // namespace _details
} // namespace pgbar

#endif
