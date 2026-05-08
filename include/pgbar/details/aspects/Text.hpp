#ifndef PGBAR_TEXT
#define PGBAR_TEXT

#include "../charcodes/U8Raw.hpp"
#include "../render/Parameter.hpp"
#include "../traits/C3.hpp"
#include "../wrappers/OptionPacket.hpp"
#include "RenderRule.hpp"

namespace pgbar {
  namespace option {
    // A wrapper that stores the prefix text.
    struct Prefix : PGBAR__DERIVING_OPTION2( Prefix, _details::charcodes::U8Raw, _prefix );

    // A wrapper that stores the postfix text.
    struct Postfix : PGBAR__DERIVING_OPTION2( Postfix, _details::charcodes::U8Raw, _postfix );

    // A wrapper that stores the prefix text color.
    struct PrefixColor
      : PGBAR__DERIVING_OPTION1( PrefixColor, _details::console::escodes::RGBColor, _prfx_color );

    // A wrapper that stores the postfix text color.
    struct PostfixColor
      : PGBAR__DERIVING_OPTION1( PostfixColor, _details::console::escodes::RGBColor, _pstfx_color );
  } // namespace option

  namespace _details {
    namespace aspects {
      template<typename Base, typename Derived>
      class Prefix : public Base {
#define PGBAR__UNPAKING( OptionName, MemberName, Constexpr )                                         \
  friend PGBAR__FORCEINLINE Constexpr void unpack( Prefix& self, option::OptionName&& val ) noexcept \
  {                                                                                                  \
    self.MemberName = std::move( val.value() );                                                      \
  }
        PGBAR__UNPAKING( Prefix, prefix_, PGBAR__CXX20_CNSTXPR )
        PGBAR__UNPAKING( PrefixColor, prfx_col_, )
#undef PGBAR__UNPAKING

      protected:
        charcodes::U8Raw prefix_;
        console::escodes::RGBColor prfx_col_;

        io::CharPipeline& build( io::CharPipeline& pipeline, const render::Parameter& params ) const
        {
          if ( prefix_.empty() )
            return pipeline;
          return pipeline << this->reset_then_style( prfx_col_, params.style_off_ ) << prefix_ << ' ';
        }

        PGBAR__NODISCARD PGBAR__FORCEINLINE PGBAR__CXX20_CNSTXPR types::Size fixed_length() const noexcept
        {
          return prefix_.width() + !prefix_.empty();
        }

        template<typename... Options>
        PGBAR__CXX20_CNSTXPR Prefix( traits::TypeSet<Options...> tag ) : Base( tag )
        {
          using OptionSet = traits::TypeSet<Options...>;
          if PGBAR__CXX17_CNSTXPR ( !traits::TpContain<OptionSet, option::Prefix>::value )
            unpack( *this, utils::provide_for<Derived, option::Prefix>() );
          if PGBAR__CXX17_CNSTXPR ( !traits::TpContain<OptionSet, option::PrefixColor>::value )
            unpack( *this, utils::provide_for<Derived, option::PrefixColor>() );
        }

        PGBAR__CXX20_CNSTXPR Prefix() = default;
        PGBAR__SPECIAL_MEMBERS_CX( Prefix, PGBAR__CXX20_CNSTXPR );

      public:
#define PGBAR__METHOD( OptionName, ParamName, ReturnType, Operation ) \
  std::lock_guard<concurrent::SharedMutex> lock { this->rw_mtx_ };    \
  unpack( *this, option::OptionName( Operation( ParamName ) ) );      \
  return static_cast<ReturnType>( *this )

        /// @throw exception::InvalidArgument If the passed parameters are not coding in UTF-8.
        Derived& prefix( types::String _prefix ) &
        {
          PGBAR__METHOD( Prefix, _prefix, Derived&, std::move );
        }
        Derived&& prefix( types::String _prefix ) &&
        {
          PGBAR__METHOD( Prefix, _prefix, Derived&&, std::move );
        }

#ifdef __cpp_lib_char8_t
        Derived& prefix( types::LitU8 _prefix ) &
        {
          PGBAR__METHOD( Prefix, _prefix, Derived&, );
        }
        Derived&& prefix( types::LitU8 _prefix ) &&
        {
          PGBAR__METHOD( Prefix, _prefix, Derived&&, );
        }
#endif

        /// @throw exception::InvalidArgument If the passed parameters is not a valid RGB color string.
        Derived& prefix_color( console::escodes::RGBColor _prfx_color ) &
        {
          PGBAR__METHOD( PrefixColor, _prfx_color, Derived&, std::move );
        }
        Derived&& prefix_color( console::escodes::RGBColor _prfx_color ) &&
        {
          PGBAR__METHOD( PrefixColor, _prfx_color, Derived&&, std::move );
        }

#undef PGBAR__METHOD

        PGBAR__CXX20_CNSTXPR void swap( Prefix& other ) noexcept
        {
          prfx_col_.swap( other.prfx_col_ );
          prefix_.swap( other.prefix_ );
          Base::swap( other );
        }
      };

      template<typename Base, typename Derived>
      class Postfix : public Base {
#define PGBAR__UNPAKING( OptionName, MemberName, Constexpr )                                          \
  friend PGBAR__FORCEINLINE Constexpr void unpack( Postfix& self, option::OptionName&& val ) noexcept \
  {                                                                                                   \
    self.MemberName = std::move( val.value() );                                                       \
  }
        PGBAR__UNPAKING( Postfix, postfix_, PGBAR__CXX20_CNSTXPR )
        PGBAR__UNPAKING( PostfixColor, pstfx_col_, )
#undef PGBAR__UNPAKING

      protected:
        charcodes::U8Raw postfix_;
        console::escodes::RGBColor pstfx_col_;

        io::CharPipeline& build( io::CharPipeline& pipeline, const render::Parameter& params ) const
        {
          if ( postfix_.empty() )
            return pipeline;
          return pipeline << this->reset_then_style( pstfx_col_, params.style_off_ ) << ' ' << postfix_;
        }

        PGBAR__NODISCARD PGBAR__FORCEINLINE PGBAR__CXX20_CNSTXPR types::Size fixed_length() const noexcept
        {
          return postfix_.width() + !postfix_.empty();
        }

        template<typename... Options>
        PGBAR__CXX14_CNSTXPR Postfix( traits::TypeSet<Options...> tag ) : Base( tag )
        {
          using OptionSet = traits::TypeSet<Options...>;
          if PGBAR__CXX17_CNSTXPR ( !traits::TpContain<OptionSet, option::Postfix>::value ) {
            unpack( *this, utils::provide_for<Derived, option::Postfix>() );
          }
        }

        PGBAR__CXX20_CNSTXPR Postfix() = default;
        PGBAR__SPECIAL_MEMBERS_CX( Postfix, PGBAR__CXX20_CNSTXPR );

      public:
#define PGBAR__METHOD( OptionName, ParamName, ReturnType, Operation ) \
  std::lock_guard<concurrent::SharedMutex> lock { this->rw_mtx_ };    \
  unpack( *this, option::OptionName( Operation( ParamName ) ) );      \
  return static_cast<ReturnType>( *this )

        /// @throw exception::InvalidArgument If the passed parameters are not coding in UTF-8.
        Derived& postfix( types::String _postfix ) &
        {
          PGBAR__METHOD( Postfix, _postfix, Derived&, std::move );
        }
        Derived&& postfix( types::String _postfix ) &&
        {
          PGBAR__METHOD( Postfix, _postfix, Derived&&, std::move );
        }
#ifdef __cpp_lib_char8_t
        Derived& postfix( types::LitU8 _postfix ) &
        {
          PGBAR__METHOD( Postfix, _postfix, Derived&, );
        }
        Derived&& postfix( types::LitU8 _postfix ) &&
        {
          PGBAR__METHOD( Postfix, _postfix, Derived&&, );
        }
#endif

        /// @throw exception::InvalidArgument If the passed parameters is not a valid RGB color string.
        Derived& postfix_color( console::escodes::RGBColor _pstfx_color ) &
        {
          PGBAR__METHOD( PostfixColor, _pstfx_color, Derived&, std::move );
        }
        Derived&& postfix_color( console::escodes::RGBColor _pstfx_color ) &&
        {
          PGBAR__METHOD( PostfixColor, _pstfx_color, Derived&&, std::move );
        }

#undef PGBAR__METHOD

        PGBAR__CXX20_CNSTXPR void swap( Postfix& other ) noexcept
        {
          pstfx_col_.swap( other.pstfx_col_ );
          postfix_.swap( other.postfix_ );
          Base::swap( other );
        }
      };
    } // namespace aspects

    PGBAR__INHERIT_REGISTER( aspects::Prefix, aspects::RenderRule );
    PGBAR__INHERIT_REGISTER( aspects::Postfix, aspects::RenderRule );

    PGBAR__OPTION_REGISTER( aspects::Prefix, option::Prefix, option::PrefixColor );
    PGBAR__OPTION_REGISTER( aspects::Postfix, option::Postfix, option::PostfixColor );
  } // namespace _details
} // namespace pgbar

#endif
