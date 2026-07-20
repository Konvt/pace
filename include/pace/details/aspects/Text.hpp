#ifndef PACE_TEXT
#define PACE_TEXT

#include "../charcodes/U8Raw.hpp"
#include "../console/Colorize.hpp"
#include "../io/Combinator.hpp"
#include "../render/Parameter.hpp"
#include "../traits/C3.hpp"
#include "../wrappers/OptionPacket.hpp"
#include "RenderRule.hpp"

namespace pace {
  namespace option {
    // A wrapper that stores the prefix text.
    struct Prefix : PACE__DERIVING_OPTION3( Prefix, details::charcodes::U8Raw, _prefix );

    // A wrapper that stores the postfix text.
    struct Postfix : PACE__DERIVING_OPTION3( Postfix, details::charcodes::U8Raw, _postfix );

    // A wrapper that stores the prefix text foreground color.
    struct PrefixForecolor
      : PACE__DERIVING_OPTION1( PrefixForecolor,
                                details::console::TrueColor,
                                details::wrappers::RGBValue,
                                _prfx_froecolor );

    // A wrapper that stores the prefix text background color.
    struct PrefixBackcolor
      : PACE__DERIVING_OPTION1( PrefixBackcolor,
                                details::console::TrueColor,
                                details::wrappers::RGBValue,
                                _prfx_backcolor );

    // A wrapper that stores the postfix text foreground color.
    struct PostfixForecolor
      : PACE__DERIVING_OPTION1( PostfixForecolor,
                                details::console::TrueColor,
                                details::wrappers::RGBValue,
                                _pstfx_forecolor );

    // A wrapper that stores the postfix text background color.
    struct PostfixBackcolor
      : PACE__DERIVING_OPTION1( PostfixBackcolor,
                                details::console::TrueColor,
                                details::wrappers::RGBValue,
                                _pstfx_backcolor );
  } // namespace option

  namespace details {
    namespace aspects {
      template<typename Base, typename Derived>
      class Prefix : public Base {
#define PACE__UNPAKING( OptionName, MemberName, Operation, Constexpr )                              \
  friend PACE__FORCEINLINE Constexpr void unpack( Prefix& self, option::OptionName&& val ) noexcept \
  { self.MemberName = Operation( val.value ); }
        PACE__UNPAKING( Prefix, prefix_, std::move, PACE__CXX20_CNSTXPR )
        PACE__UNPAKING( PrefixForecolor, prfx_forecolor_, , PACE__CXX20_CNSTXPR )
        PACE__UNPAKING( PrefixBackcolor, prfx_backcolor_, , PACE__CXX20_CNSTXPR )
#undef PACE__UNPAKING

      protected:
        charcodes::U8Raw prefix_;
        console::TrueColor prfx_forecolor_, prfx_backcolor_;

        io::CharPipeline& build( io::CharPipeline& pipeline, const render::Parameter& params ) const
        {
          if ( prefix_.empty() )
            return pipeline;
          pipeline << io::when( !params.style_off && this->colorful(),
                                console::resetcolor,
                                console::Dualcolor { prfx_forecolor_, prfx_backcolor_ } )
                   << prefix_ << ' ';
          return pipeline;
        }

        PACE__NODISCARD PACE__FORCEINLINE PACE__CXX20_CNSTXPR std::size_t fixed_length() const noexcept
        { return prefix_.width() + !prefix_.empty(); }

        template<typename... Options>
        PACE__CXX20_CNSTXPR Prefix( traits::TypeSet<Options...> tag ) : Base( tag )
        {
          using OptionSet = traits::TypeSet<Options...>;
          if PACE__CXX17_CNSTXPR ( !traits::TpContains<OptionSet, option::Prefix>::value )
            unpack( *this, config::provide_for<Derived, option::Prefix>() );
          if PACE__CXX17_CNSTXPR ( !traits::TpContains<OptionSet, option::PrefixForecolor>::value )
            unpack( *this, config::provide_for<Derived, option::PrefixForecolor>() );
          if PACE__CXX17_CNSTXPR ( !traits::TpContains<OptionSet, option::PrefixBackcolor>::value )
            unpack( *this, config::provide_for<Derived, option::PrefixBackcolor>() );
        }

        PACE__CXX20_CNSTXPR Prefix() = default;
        PACE__SPECIAL_MEMBERS_CX( Prefix, PACE__CXX20_CNSTXPR );

      public:
#define PACE__METHOD( OptionName, ParamName, ReturnType, Operation ) \
  std::lock_guard<concurrent::SharedMutex> lock { this->rw_mtx_ };   \
  unpack( *this, option::OptionName( Operation( ParamName ) ) );     \
  return static_cast<ReturnType>( *this )

        /// @throw exception::InvalidArgument If the passed parameters are not coding in UTF-8.
        Derived& prefix( std::string _prefix ) &
        { PACE__METHOD( Prefix, _prefix, Derived&, std::move ); }
        Derived&& prefix( std::string _prefix ) &&
        { PACE__METHOD( Prefix, _prefix, Derived&&, std::move ); }

#ifdef __cpp_lib_char8_t
        Derived& prefix( charcodes::U8StringView _prefix ) &
        { PACE__METHOD( Prefix, _prefix, Derived&, ); }
        Derived&& prefix( charcodes::U8StringView _prefix ) &&
        { PACE__METHOD( Prefix, _prefix, Derived&&, ); }
#endif

        /// @throw exception::InvalidArgument If the passed parameters is not a valid RGB color string.
        Derived& prefix_forecolor( wrappers::RGBValue _prfx_forecolor ) &
        { PACE__METHOD( PrefixForecolor, _prfx_forecolor, Derived&, ); }
        Derived&& prefix_forecolor( wrappers::RGBValue _prfx_forecolor ) &&
        { PACE__METHOD( PrefixForecolor, _prfx_forecolor, Derived&&, ); }

        /// @throw exception::InvalidArgument If the passed parameters is not a valid RGB color string.
        Derived& prefix_backcolor( wrappers::RGBValue _prfx_backcolor ) &
        { PACE__METHOD( PrefixBackcolor, _prfx_backcolor, Derived&, ); }
        Derived&& prefix_backcolor( wrappers::RGBValue _prfx_backcolor ) &&
        { PACE__METHOD( PrefixBackcolor, _prfx_backcolor, Derived&&, ); }

#undef PACE__METHOD

        PACE__CXX20_CNSTXPR void swap( Prefix& other ) noexcept
        {
          prefix_.swap( other.prefix_ );
          prfx_forecolor_.swap( other.prfx_forecolor_ );
          prfx_backcolor_.swap( other.prfx_backcolor_ );
          Base::swap( other );
        }
      };

      template<typename Base, typename Derived>
      class Postfix : public Base {
#define PACE__UNPAKING( OptionName, MemberName, Operation, Constexpr )                               \
  friend PACE__FORCEINLINE Constexpr void unpack( Postfix& self, option::OptionName&& val ) noexcept \
  { self.MemberName = Operation( val.value ); }
        PACE__UNPAKING( Postfix, postfix_, std::move, PACE__CXX20_CNSTXPR )
        PACE__UNPAKING( PostfixForecolor, pstfx_forecolor_, , PACE__CXX20_CNSTXPR )
        PACE__UNPAKING( PostfixBackcolor, pstfx_backcolor_, , PACE__CXX20_CNSTXPR )
#undef PACE__UNPAKING

      protected:
        charcodes::U8Raw postfix_;
        console::TrueColor pstfx_forecolor_, pstfx_backcolor_;

        io::CharPipeline& build( io::CharPipeline& pipeline, const render::Parameter& params ) const
        {
          if ( postfix_.empty() )
            return pipeline;
          pipeline << io::when( !params.style_off && this->colorful(),
                                console::resetcolor,
                                console::Dualcolor { pstfx_forecolor_, pstfx_backcolor_ } )
                   << ' ' << postfix_;
          return pipeline;
        }

        PACE__NODISCARD PACE__FORCEINLINE PACE__CXX20_CNSTXPR std::size_t fixed_length() const noexcept
        { return postfix_.width() + !postfix_.empty(); }

        template<typename... Options>
        PACE__CXX14_CNSTXPR Postfix( traits::TypeSet<Options...> tag ) : Base( tag )
        {
          using OptionSet = traits::TypeSet<Options...>;
          if PACE__CXX17_CNSTXPR ( !traits::TpContains<OptionSet, option::Postfix>::value )
            unpack( *this, config::provide_for<Derived, option::Postfix>() );
          if PACE__CXX17_CNSTXPR ( !traits::TpContains<OptionSet, option::PostfixForecolor>::value )
            unpack( *this, config::provide_for<Derived, option::PostfixForecolor>() );
          if PACE__CXX17_CNSTXPR ( !traits::TpContains<OptionSet, option::PostfixBackcolor>::value )
            unpack( *this, config::provide_for<Derived, option::PostfixBackcolor>() );
        }

        PACE__CXX20_CNSTXPR Postfix() = default;
        PACE__SPECIAL_MEMBERS_CX( Postfix, PACE__CXX20_CNSTXPR );

      public:
#define PACE__METHOD( OptionName, ParamName, ReturnType, Operation ) \
  std::lock_guard<concurrent::SharedMutex> lock { this->rw_mtx_ };   \
  unpack( *this, option::OptionName( Operation( ParamName ) ) );     \
  return static_cast<ReturnType>( *this )

        /// @throw exception::InvalidArgument If the passed parameters are not coding in UTF-8.
        Derived& postfix( std::string _postfix ) &
        { PACE__METHOD( Postfix, _postfix, Derived&, std::move ); }
        Derived&& postfix( std::string _postfix ) &&
        { PACE__METHOD( Postfix, _postfix, Derived&&, std::move ); }
#ifdef __cpp_lib_char8_t
        Derived& postfix( charcodes::U8StringView _postfix ) &
        { PACE__METHOD( Postfix, _postfix, Derived&, ); }
        Derived&& postfix( charcodes::U8StringView _postfix ) &&
        { PACE__METHOD( Postfix, _postfix, Derived&&, ); }
#endif

        /// @throw exception::InvalidArgument If the passed parameters is not a valid RGB color string.
        Derived& postfix_forecolor( wrappers::RGBValue _pstfx_forecolor ) &
        { PACE__METHOD( PostfixForecolor, _pstfx_forecolor, Derived&, ); }
        Derived&& postfix_forecolor( wrappers::RGBValue _pstfx_forecolor ) &&
        { PACE__METHOD( PostfixForecolor, _pstfx_forecolor, Derived&&, ); }

        /// @throw exception::InvalidArgument If the passed parameters is not a valid RGB color string.
        Derived& postfix_backcolor( wrappers::RGBValue _pstfx_backcolor ) &
        { PACE__METHOD( PostfixBackcolor, _pstfx_backcolor, Derived&, ); }
        Derived&& postfix_backcolor( wrappers::RGBValue _pstfx_backcolor ) &&
        { PACE__METHOD( PostfixBackcolor, _pstfx_backcolor, Derived&&, ); }

#undef PACE__METHOD

        PACE__CXX20_CNSTXPR void swap( Postfix& other ) noexcept
        {
          postfix_.swap( other.postfix_ );
          pstfx_forecolor_.swap( other.pstfx_forecolor_ );
          pstfx_backcolor_.swap( other.pstfx_backcolor_ );
          Base::swap( other );
        }
      };
    } // namespace aspects

    PACE__INHERIT_REGISTER( aspects::Prefix, aspects::RenderRule );
    PACE__INHERIT_REGISTER( aspects::Postfix, aspects::RenderRule );

    PACE__OPTION_REGISTER( aspects::Prefix,
                           option::Prefix,
                           option::PrefixForecolor,
                           option::PrefixBackcolor );
    PACE__OPTION_REGISTER( aspects::Postfix,
                           option::Postfix,
                           option::PostfixForecolor,
                           option::PostfixBackcolor );
  } // namespace details
} // namespace pace

#endif
