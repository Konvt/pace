#ifndef PACE_SEGMENT
#define PACE_SEGMENT

#include "../traits/C3.hpp"
#include "../wrappers/OptionPacket.hpp"
#include "RenderRule.hpp"

namespace pace {
  namespace option {
    // A wrapper that stores the separator component used to separate different infomation.
    struct Divider : PACE__DERIVING_OPTION3( Divider, details::charcodes::U8Raw, _divider );

    // A wrapper that stores the border component located to the left of the whole indicator.
    struct LeftBorder : PACE__DERIVING_OPTION3( LeftBorder, details::charcodes::U8Raw, _l_border );

    // A wrapper that stores the border component located to the right of the whole indicator.
    struct RightBorder : PACE__DERIVING_OPTION3( RightBorder, details::charcodes::U8Raw, _r_border );

    // A wrapper that stores the foreground color of the whole infomation indicator.
    struct InfoForecolor
      : PACE__DERIVING_OPTION1( InfoForecolor,
                                details::console::TrueColor,
                                details::wrappers::RGBValue,
                                _info_forecolor );

    // A wrapper that stores the background color of the whole infomation indicator.
    struct InfoBackcolor
      : PACE__DERIVING_OPTION1( InfoBackcolor,
                                details::console::TrueColor,
                                details::wrappers::RGBValue,
                                _info_backcolor );
  } // namespace option

  namespace details {
    namespace aspects {
      template<typename Base, typename Derived>
      class Segment : public Base {
#define PACE__UNPAKING( OptionName, MemberName, Operation, Constexpr )                               \
  friend PACE__FORCEINLINE Constexpr void unpack( Segment& self, option::OptionName&& val ) noexcept \
  { self.MemberName = Operation( val.value() ); }
        PACE__UNPAKING( Divider, divider_, std::move, PACE__CXX20_CNSTXPR )
        PACE__UNPAKING( LeftBorder, l_border_, std::move, PACE__CXX20_CNSTXPR )
        PACE__UNPAKING( RightBorder, r_border_, std::move, PACE__CXX20_CNSTXPR )
        PACE__UNPAKING( InfoForecolor, info_forecolor_, , PACE__CXX20_CNSTXPR )
        PACE__UNPAKING( InfoBackcolor, info_backcolor_, , PACE__CXX20_CNSTXPR )
#undef PACE__UNPAKING

      protected:
        charcodes::U8Raw divider_;
        charcodes::U8Raw l_border_, r_border_;
        console::TrueColor info_forecolor_, info_backcolor_;

        PACE__NODISCARD PACE__FORCEINLINE PACE__CXX20_CNSTXPR types::Size fixed_length(
          types::Size num_column ) const noexcept
        {
          switch ( num_column ) {
          case 0:  return 0;
          case 1:  return l_border_.width() + r_border_.width();
          default: return ( num_column - 1 ) * divider_.width() + l_border_.width() + r_border_.width();
          }
        }

        template<typename... Options>
        PACE__CXX20_CNSTXPR Segment( traits::TypeSet<Options...> tag ) : Base( tag )
        {
          using OptionSet = traits::TypeSet<Options...>;
          if PACE__CXX17_CNSTXPR ( !traits::TpContain<OptionSet, option::Divider>::value )
            unpack( *this, config::provide_for<Derived, option::Divider>() );
          if PACE__CXX17_CNSTXPR ( !traits::TpContain<OptionSet, option::LeftBorder>::value )
            unpack( *this, config::provide_for<Derived, option::LeftBorder>() );
          if PACE__CXX17_CNSTXPR ( !traits::TpContain<OptionSet, option::RightBorder>::value )
            unpack( *this, config::provide_for<Derived, option::RightBorder>() );
          if PACE__CXX17_CNSTXPR ( !traits::TpContain<OptionSet, option::InfoForecolor>::value )
            unpack( *this, config::provide_for<Derived, option::InfoForecolor>() );
          if PACE__CXX17_CNSTXPR ( !traits::TpContain<OptionSet, option::InfoBackcolor>::value )
            unpack( *this, config::provide_for<Derived, option::InfoBackcolor>() );
        }

        PACE__CXX20_CNSTXPR Segment() = default;
        PACE__SPECIAL_MEMBERS_CX( Segment, PACE__CXX20_CNSTXPR );

      public:
#define PACE__METHOD( OptionName, ParamName, ReturnType, Operation ) \
  std::lock_guard<concurrent::SharedMutex> lock { this->rw_mtx_ };   \
  unpack( *this, option::OptionName( Operation( ParamName ) ) );     \
  return static_cast<ReturnType>( *this )

        /// @throw exception::InvalidArgument If the passed parameters are not coding in UTF-8.
        Derived& divider( types::String _divider ) &
        { PACE__METHOD( Divider, _divider, Derived&, std::move ); }
        Derived&& divider( types::String _divider ) &&
        { PACE__METHOD( Divider, _divider, Derived&&, std::move ); }

        /// @throw exception::InvalidArgument If the passed parameters are not coding in UTF-8.
        Derived& left_border( types::String _l_border ) &
        { PACE__METHOD( LeftBorder, _l_border, Derived&, std::move ); }
        Derived&& left_border( types::String _l_border ) &&
        { PACE__METHOD( LeftBorder, _l_border, Derived&&, std::move ); }

        /// @throw exception::InvalidArgument If the passed parameters are not coding in UTF-8.
        Derived& right_border( types::String _r_border ) &
        { PACE__METHOD( RightBorder, _r_border, Derived&, std::move ); }
        Derived&& right_border( types::String _r_border ) &&
        { PACE__METHOD( RightBorder, _r_border, Derived&&, std::move ); }
#ifdef __cpp_lib_char8_t
        Derived& divider( types::LitU8 _divider ) &
        { PACE__METHOD( Divider, _divider, Derived&, ); }
        Derived&& divider( types::LitU8 _divider ) &&
        { PACE__METHOD( Divider, _divider, Derived&&, ); }

        Derived& left_border( types::LitU8 _l_border ) &
        { PACE__METHOD( LeftBorder, _l_border, Derived&, ); }
        Derived&& left_border( types::LitU8 _l_border ) &&
        { PACE__METHOD( LeftBorder, _l_border, Derived&&, ); }

        Derived& right_border( types::LitU8 _r_border ) &
        { PACE__METHOD( RightBorder, _r_border, Derived&, ); }
        Derived&& right_border( types::LitU8 _r_border ) &&
        { PACE__METHOD( RightBorder, _r_border, Derived&&, ); }
#endif

        /// @throw exception::InvalidArgument If the passed parameters is not a valid RGB color string.
        Derived& info_forecolor( wrappers::RGBValue _info_forecolor ) &
        { PACE__METHOD( InfoForecolor, _info_forecolor, Derived&, ); }
        Derived&& info_forecolor( wrappers::RGBValue _info_forecolor ) &&
        { PACE__METHOD( InfoForecolor, _info_forecolor, Derived&&, ); }

        /// @throw exception::InvalidArgument If the passed parameters is not a valid RGB color string.
        Derived& info_backcolor( wrappers::RGBValue _info_backcolor ) &
        { PACE__METHOD( InfoBackcolor, _info_backcolor, Derived&, ); }
        Derived&& info_backcolor( wrappers::RGBValue _info_backcolor ) &&
        { PACE__METHOD( InfoBackcolor, _info_backcolor, Derived&&, ); }

#undef PACE__METHOD

        PACE__CXX20_CNSTXPR void swap( Segment& other ) & noexcept
        {
          divider_.swap( other.divider_ );
          l_border_.swap( other.l_border_ );
          r_border_.swap( other.r_border_ );
          info_forecolor_.swap( other.info_forecolor_ );
          info_backcolor_.swap( other.info_backcolor_ );
          Base::swap( other );
        }
      };
    } // namespace aspects

    PACE__INHERIT_REGISTER( aspects::Segment, aspects::RenderRule );

    PACE__OPTION_REGISTER( aspects::Segment,
                           option::Divider,
                           option::LeftBorder,
                           option::RightBorder,
                           option::InfoForecolor,
                           option::InfoBackcolor );
  } // namespace details
} // namespace pace

#endif
