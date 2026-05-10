#ifndef PACE_TEXT
#define PACE_TEXT

#include "../charcodes/U8Raw.hpp"
#include "../render/Parameter.hpp"
#include "../traits/C3.hpp"
#include "../wrappers/OptionPacket.hpp"
#include "RenderRule.hpp"

namespace pace {
  namespace option {
    // A wrapper that stores the prefix text.
    struct Prefix : PACE__DERIVING_OPTION2( Prefix, details::charcodes::U8Raw, _prefix );

    // A wrapper that stores the postfix text.
    struct Postfix : PACE__DERIVING_OPTION2( Postfix, details::charcodes::U8Raw, _postfix );

    // A wrapper that stores the prefix text color.
    struct PrefixColor : PACE__DERIVING_OPTION1( PrefixColor, details::console::RGBColor, _prfx_color );

    // A wrapper that stores the postfix text color.
    struct PostfixColor : PACE__DERIVING_OPTION1( PostfixColor, details::console::RGBColor, _pstfx_color );
  } // namespace option

  namespace details {
    namespace aspects {
      template<typename Base, typename Derived>
      class Prefix : public Base {
#define PACE__UNPAKING( OptionName, MemberName, Constexpr )                                         \
  friend PACE__FORCEINLINE Constexpr void unpack( Prefix& self, option::OptionName&& val ) noexcept \
  { self.MemberName = std::move( val.value() ); }
        PACE__UNPAKING( Prefix, prefix_, PACE__CXX20_CNSTXPR )
        PACE__UNPAKING( PrefixColor, prfx_col_, )
#undef PACE__UNPAKING

      protected:
        charcodes::U8Raw prefix_;
        console::RGBColor prfx_col_;

        io::CharPipeline& build( io::CharPipeline& pipeline, const render::Parameter& params ) const
        {
          if ( prefix_.empty() )
            return pipeline;
          return pipeline << this->clear_then_dye( prfx_col_, params.style_off_ ) << prefix_ << ' ';
        }

        PACE__NODISCARD PACE__FORCEINLINE PACE__CXX20_CNSTXPR types::Size fixed_length() const noexcept
        { return prefix_.width() + !prefix_.empty(); }

        template<typename... Options>
        PACE__CXX20_CNSTXPR Prefix( traits::TypeSet<Options...> tag ) : Base( tag )
        {
          using OptionSet = traits::TypeSet<Options...>;
          if PACE__CXX17_CNSTXPR ( !traits::TpContain<OptionSet, option::Prefix>::value )
            unpack( *this, config::provide_for<Derived, option::Prefix>() );
          if PACE__CXX17_CNSTXPR ( !traits::TpContain<OptionSet, option::PrefixColor>::value )
            unpack( *this, config::provide_for<Derived, option::PrefixColor>() );
        }

        PACE__CXX20_CNSTXPR Prefix() = default;
        PACE__SPECIAL_MEMBERS_CX( Prefix, PACE__CXX20_CNSTXPR );

      public:
#define PACE__METHOD( OptionName, ParamName, ReturnType, Operation ) \
  std::lock_guard<concurrent::SharedMutex> lock { this->rw_mtx_ };   \
  unpack( *this, option::OptionName( Operation( ParamName ) ) );     \
  return static_cast<ReturnType>( *this )

        /// @throw exception::InvalidArgument If the passed parameters are not coding in UTF-8.
        Derived& prefix( types::String _prefix ) &
        { PACE__METHOD( Prefix, _prefix, Derived&, std::move ); }
        Derived&& prefix( types::String _prefix ) &&
        { PACE__METHOD( Prefix, _prefix, Derived&&, std::move ); }

#ifdef __cpp_lib_char8_t
        Derived& prefix( types::LitU8 _prefix ) &
        { PACE__METHOD( Prefix, _prefix, Derived&, ); }
        Derived&& prefix( types::LitU8 _prefix ) &&
        { PACE__METHOD( Prefix, _prefix, Derived&&, ); }
#endif

        /// @throw exception::InvalidArgument If the passed parameters is not a valid RGB color string.
        Derived& prefix_color( console::RGBColor _prfx_color ) &
        { PACE__METHOD( PrefixColor, _prfx_color, Derived&, std::move ); }
        Derived&& prefix_color( console::RGBColor _prfx_color ) &&
        { PACE__METHOD( PrefixColor, _prfx_color, Derived&&, std::move ); }

#undef PACE__METHOD

        PACE__CXX20_CNSTXPR void swap( Prefix& other ) noexcept
        {
          prfx_col_.swap( other.prfx_col_ );
          prefix_.swap( other.prefix_ );
          Base::swap( other );
        }
      };

      template<typename Base, typename Derived>
      class Postfix : public Base {
#define PACE__UNPAKING( OptionName, MemberName, Constexpr )                                          \
  friend PACE__FORCEINLINE Constexpr void unpack( Postfix& self, option::OptionName&& val ) noexcept \
  { self.MemberName = std::move( val.value() ); }
        PACE__UNPAKING( Postfix, postfix_, PACE__CXX20_CNSTXPR )
        PACE__UNPAKING( PostfixColor, pstfx_col_, )
#undef PACE__UNPAKING

      protected:
        charcodes::U8Raw postfix_;
        console::RGBColor pstfx_col_;

        io::CharPipeline& build( io::CharPipeline& pipeline, const render::Parameter& params ) const
        {
          if ( postfix_.empty() )
            return pipeline;
          return pipeline << this->clear_then_dye( pstfx_col_, params.style_off_ ) << ' ' << postfix_;
        }

        PACE__NODISCARD PACE__FORCEINLINE PACE__CXX20_CNSTXPR types::Size fixed_length() const noexcept
        { return postfix_.width() + !postfix_.empty(); }

        template<typename... Options>
        PACE__CXX14_CNSTXPR Postfix( traits::TypeSet<Options...> tag ) : Base( tag )
        {
          using OptionSet = traits::TypeSet<Options...>;
          if PACE__CXX17_CNSTXPR ( !traits::TpContain<OptionSet, option::Postfix>::value ) {
            unpack( *this, config::provide_for<Derived, option::Postfix>() );
          }
        }

        PACE__CXX20_CNSTXPR Postfix() = default;
        PACE__SPECIAL_MEMBERS_CX( Postfix, PACE__CXX20_CNSTXPR );

      public:
#define PACE__METHOD( OptionName, ParamName, ReturnType, Operation ) \
  std::lock_guard<concurrent::SharedMutex> lock { this->rw_mtx_ };   \
  unpack( *this, option::OptionName( Operation( ParamName ) ) );     \
  return static_cast<ReturnType>( *this )

        /// @throw exception::InvalidArgument If the passed parameters are not coding in UTF-8.
        Derived& postfix( types::String _postfix ) &
        { PACE__METHOD( Postfix, _postfix, Derived&, std::move ); }
        Derived&& postfix( types::String _postfix ) &&
        { PACE__METHOD( Postfix, _postfix, Derived&&, std::move ); }
#ifdef __cpp_lib_char8_t
        Derived& postfix( types::LitU8 _postfix ) &
        { PACE__METHOD( Postfix, _postfix, Derived&, ); }
        Derived&& postfix( types::LitU8 _postfix ) &&
        { PACE__METHOD( Postfix, _postfix, Derived&&, ); }
#endif

        /// @throw exception::InvalidArgument If the passed parameters is not a valid RGB color string.
        Derived& postfix_color( console::RGBColor _pstfx_color ) &
        { PACE__METHOD( PostfixColor, _pstfx_color, Derived&, std::move ); }
        Derived&& postfix_color( console::RGBColor _pstfx_color ) &&
        { PACE__METHOD( PostfixColor, _pstfx_color, Derived&&, std::move ); }

#undef PACE__METHOD

        PACE__CXX20_CNSTXPR void swap( Postfix& other ) noexcept
        {
          pstfx_col_.swap( other.pstfx_col_ );
          postfix_.swap( other.postfix_ );
          Base::swap( other );
        }
      };
    } // namespace aspects

    PACE__INHERIT_REGISTER( aspects::Prefix, aspects::RenderRule );
    PACE__INHERIT_REGISTER( aspects::Postfix, aspects::RenderRule );

    PACE__OPTION_REGISTER( aspects::Prefix, option::Prefix, option::PrefixColor );
    PACE__OPTION_REGISTER( aspects::Postfix, option::Postfix, option::PostfixColor );
  } // namespace details
} // namespace pace

#endif
