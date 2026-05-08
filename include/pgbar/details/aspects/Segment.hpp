#ifndef PGBAR_SEGMENT
#define PGBAR_SEGMENT

#include "../traits/C3.hpp"
#include "../wrappers/OptionPacket.hpp"
#include "RenderRule.hpp"

namespace pgbar {
  namespace option {
    // A wrapper that stores the separator component used to separate different infomation.
    struct Divider : PGBAR__DERIVING_OPTION2( Divider, details::charcodes::U8Raw, _divider );

    // A wrapper that stores the border component located to the left of the whole indicator.
    struct LeftBorder : PGBAR__DERIVING_OPTION2( LeftBorder, details::charcodes::U8Raw, _l_border );

    // A wrapper that stores the border component located to the right of the whole indicator.
    struct RightBorder : PGBAR__DERIVING_OPTION2( RightBorder, details::charcodes::U8Raw, _r_border );

    // A wrapper that stores the color of the whole infomation indicator.
    struct InfoColor : PGBAR__DERIVING_OPTION1( InfoColor, details::console::escodes::RGBColor, _info_color );
  } // namespace option

  namespace details {
    namespace aspects {
      template<typename Base, typename Derived>
      class Segment : public Base {
#define PGBAR__UNPAKING( OptionName, MemberName, Operation, Constexpr )                               \
  friend PGBAR__FORCEINLINE Constexpr void unpack( Segment& self, option::OptionName&& val ) noexcept \
  {                                                                                                   \
    self.MemberName = Operation( val.value() );                                                       \
  }
        PGBAR__UNPAKING( Divider, divider_, std::move, PGBAR__CXX20_CNSTXPR )
        PGBAR__UNPAKING( LeftBorder, l_border_, std::move, PGBAR__CXX20_CNSTXPR )
        PGBAR__UNPAKING( RightBorder, r_border_, std::move, PGBAR__CXX20_CNSTXPR )
        PGBAR__UNPAKING( InfoColor, info_col_, std::move, )
#undef PGBAR__UNPAKING

      protected:
        charcodes::U8Raw divider_;
        charcodes::U8Raw l_border_, r_border_;
        console::escodes::RGBColor info_col_;

        PGBAR__NODISCARD PGBAR__FORCEINLINE PGBAR__CXX20_CNSTXPR types::Size fixed_length(
          types::Size num_column ) const noexcept
        {
          switch ( num_column ) {
          case 0:  return 0;
          case 1:  return l_border_.width() + r_border_.width();
          default: return ( num_column - 1 ) * divider_.width() + l_border_.width() + r_border_.width();
          }
        }

        template<typename... Options>
        PGBAR__CXX20_CNSTXPR Segment( traits::TypeSet<Options...> tag ) : Base( tag )
        {
          using OptionSet = traits::TypeSet<Options...>;
          if PGBAR__CXX17_CNSTXPR ( !traits::TpContain<OptionSet, option::Divider>::value )
            unpack( *this, config::provide_for<Derived, option::Divider>() );
          if PGBAR__CXX17_CNSTXPR ( !traits::TpContain<OptionSet, option::LeftBorder>::value )
            unpack( *this, config::provide_for<Derived, option::LeftBorder>() );
          if PGBAR__CXX17_CNSTXPR ( !traits::TpContain<OptionSet, option::RightBorder>::value )
            unpack( *this, config::provide_for<Derived, option::RightBorder>() );
          if PGBAR__CXX17_CNSTXPR ( !traits::TpContain<OptionSet, option::InfoColor>::value )
            unpack( *this, config::provide_for<Derived, option::InfoColor>() );
        }

        PGBAR__CXX20_CNSTXPR Segment() = default;
        PGBAR__SPECIAL_MEMBERS_CX( Segment, PGBAR__CXX20_CNSTXPR );

      public:
#define PGBAR__METHOD( OptionName, ParamName, ReturnType, Operation ) \
  std::lock_guard<concurrent::SharedMutex> lock { this->rw_mtx_ };    \
  unpack( *this, option::OptionName( Operation( ParamName ) ) );      \
  return static_cast<ReturnType>( *this )

        /// @throw exception::InvalidArgument If the passed parameters are not coding in UTF-8.
        Derived& divider( types::String _divider ) &
        {
          PGBAR__METHOD( Divider, _divider, Derived&, std::move );
        }
        Derived&& divider( types::String _divider ) &&
        {
          PGBAR__METHOD( Divider, _divider, Derived&&, std::move );
        }

        /// @throw exception::InvalidArgument If the passed parameters are not coding in UTF-8.
        Derived& left_border( types::String _l_border ) &
        {
          PGBAR__METHOD( LeftBorder, _l_border, Derived&, std::move );
        }
        Derived&& left_border( types::String _l_border ) &&
        {
          PGBAR__METHOD( LeftBorder, _l_border, Derived&&, std::move );
        }

        /// @throw exception::InvalidArgument If the passed parameters are not coding in UTF-8.
        Derived& right_border( types::String _r_border ) &
        {
          PGBAR__METHOD( RightBorder, _r_border, Derived&, std::move );
        }
        Derived&& right_border( types::String _r_border ) &&
        {
          PGBAR__METHOD( RightBorder, _r_border, Derived&&, std::move );
        }
#ifdef __cpp_lib_char8_t
        Derived& divider( types::LitU8 _divider ) &
        {
          PGBAR__METHOD( Divider, _divider, Derived&, );
        }
        Derived&& divider( types::LitU8 _divider ) &&
        {
          PGBAR__METHOD( Divider, _divider, Derived&&, );
        }

        Derived& left_border( types::LitU8 _l_border ) &
        {
          PGBAR__METHOD( LeftBorder, _l_border, Derived&, );
        }
        Derived&& left_border( types::LitU8 _l_border ) &&
        {
          PGBAR__METHOD( LeftBorder, _l_border, Derived&&, );
        }

        Derived& right_border( types::LitU8 _r_border ) &
        {
          PGBAR__METHOD( RightBorder, _r_border, Derived&, );
        }
        Derived&& right_border( types::LitU8 _r_border ) &&
        {
          PGBAR__METHOD( RightBorder, _r_border, Derived&&, );
        }
#endif

        /// @throw exception::InvalidArgument If the passed parameters is not a valid RGB color string.
        Derived& info_color( console::escodes::RGBColor _info_color ) &
        {
          PGBAR__METHOD( InfoColor, _info_color, Derived&, std::move );
        }
        Derived&& info_color( console::escodes::RGBColor _info_color ) &&
        {
          PGBAR__METHOD( InfoColor, _info_color, Derived&&, std::move );
        }

#undef PGBAR__METHOD

        PGBAR__CXX20_CNSTXPR void swap( Segment& other ) & noexcept
        {
          info_col_.swap( other.info_col_ );
          divider_.swap( other.divider_ );
          l_border_.swap( other.l_border_ );
          r_border_.swap( other.r_border_ );
          Base::swap( other );
        }
      };
    } // namespace aspects

    PGBAR__INHERIT_REGISTER( aspects::Segment, aspects::RenderRule );

    PGBAR__OPTION_REGISTER( aspects::Segment,
                            option::Divider,
                            option::LeftBorder,
                            option::RightBorder,
                            option::InfoColor );
  } // namespace details
} // namespace pgbar

#endif
