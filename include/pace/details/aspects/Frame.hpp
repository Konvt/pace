#ifndef PACE_FRAME
#define PACE_FRAME

#include "../charcodes/U8Text.hpp"
#include "../concurrent/SharedMutex.hpp"
#include "../traits/C3.hpp"
#include "../wrappers/OptionPacket.hpp"
#include "Animation.hpp"
#include "RenderRule.hpp"

namespace pace {
  namespace option {
    // A wrapper that stores the `lead` animated element.
    struct Lead : details::wrappers::OptionPacket<std::vector<details::charcodes::U8Text>> {
    private:
      using Base = details::wrappers::OptionPacket<std::vector<details::charcodes::U8Text>>;

    public:
      PACE__CXX20_CNSTXPR Lead() = default;
      /**
       * @throw exception::InvalidArgument
       *
       * If the passed parameters are not coding in UTF-8.
       */
      Lead( std::vector<details::types::String> _leads )
      {
        std::transform(
          std::make_move_iterator( _leads.begin() ),
          std::make_move_iterator( _leads.end() ),
          std::back_inserter( data_ ),
          []( details::types::String&& ele ) { return details::charcodes::U8Text( std::move( ele ) ); } );
      }
      /**
       * @throw exception::InvalidArgument
       *
       * If the passed parameters are not coding in UTF-8.
       */
      Lead( details::types::String _lead ) : Base( { details::charcodes::U8Text( std::move( _lead ) ) } ) {}
#ifdef __cpp_lib_char8_t
      Lead( const std::vector<details::types::LitU8>& _leads )
      {
        std::transform(
          _leads.cbegin(),
          _leads.cend(),
          std::back_inserter( data_ ),
          []( const details::types::LitU8& ele ) { return details::charcodes::U8Text( ele ); } );
      }
      Lead( const details::types::LitU8& _lead ) : Base( { details::charcodes::U8Text( _lead ) } ) {}
#endif
    };

    // A wrapper that stores the color of the lead in the bar indicator.
    struct LeadColor : PACE__DERIVING_OPTION1( LeadColor, details::console::escodes::RGBColor, _lead_color );
  } // namespace option

  namespace details {
    namespace aspects {
      template<typename Base, typename Derived>
      class Frame : public Base {
        friend PACE__FORCEINLINE void unpack( Frame& self, option::LeadColor&& val ) noexcept
        {
          self.lead_col_ = val.value();
        }
        friend PACE__FORCEINLINE PACE__CXX20_CNSTXPR void unpack( Frame& self, option::Lead&& val ) noexcept
        {
          if ( std::all_of( val.value().cbegin(),
                            val.value().cend(),
                            []( const charcodes::U8Raw& ele ) noexcept { return ele.empty(); } ) ) {
            self.lead_.clear();
            self.len_longest_lead_ = 0;
          } else {
            self.lead_ = std::move( val.value() );
            self.len_longest_lead_ =
              std::max_element( self.lead_.cbegin(),
                                self.lead_.cend(),
                                []( const charcodes::U8Raw& a, const charcodes::U8Raw& b ) noexcept {
                                  return a.width() < b.width();
                                } )
                ->width();
          }
        }

      protected:
        types::Size len_longest_lead_;
        std::vector<charcodes::U8Text> lead_;
        console::escodes::RGBColor lead_col_;

        PACE__NODISCARD PACE__FORCEINLINE PACE__CXX20_CNSTXPR types::Size fixed_length() const noexcept
        {
          return len_longest_lead_;
        }

        template<typename... Options>
        PACE__CXX20_CNSTXPR Frame( traits::TypeSet<Options...> tag ) : Base( tag )
        {
          using OptionSet = traits::TypeSet<Options...>;
          if PACE__CXX17_CNSTXPR ( !traits::TpContain<OptionSet, option::Lead>::value )
            unpack( *this, config::provide_for<Derived, option::Lead>() );
          if PACE__CXX17_CNSTXPR ( !traits::TpContain<OptionSet, option::LeadColor>::value )
            unpack( *this, config::provide_for<Derived, option::LeadColor>() );
        }

        PACE__CXX20_CNSTXPR Frame() = default;
        PACE__SPECIAL_MEMBERS_CX( Frame, PACE__CXX20_CNSTXPR );

      public:
#define PACE__METHOD( OptionName, ParamName, ReturnType, Operation ) \
  std::lock_guard<concurrent::SharedMutex> lock { this->rw_mtx_ };   \
  unpack( *this, option::OptionName( Operation( ParamName ) ) );     \
  return static_cast<ReturnType>( *this )

        /// @throw exception::InvalidArgument If the passed parameters are not coding in UTF-8.
        Derived& lead( std::vector<types::String> _leads ) &
        {
          PACE__METHOD( Lead, _leads, Derived&, std::move );
        }
        Derived&& lead( std::vector<types::String> _leads ) &&
        {
          PACE__METHOD( Lead, _leads, Derived&&, std::move );
        }
        /// @throw exception::InvalidArgument If the passed parameters are not coding in UTF-8.
        Derived& lead( types::String _lead ) &
        {
          PACE__METHOD( Lead, _lead, Derived&, std::move );
        }
        Derived&& lead( types::String _lead ) &&
        {
          PACE__METHOD( Lead, _lead, Derived&&, std::move );
        }
#ifdef __cpp_lib_char8_t
        Derived& lead( const std::vector<types::LitU8>& _leads ) &
        {
          PACE__METHOD( Lead, _leads, Derived&, );
        }
        Derived&& lead( const std::vector<types::LitU8>& _leads ) &&
        {
          PACE__METHOD( Lead, _leads, Derived&&, );
        }
        Derived& lead( types::LitU8 _lead ) &
        {
          PACE__METHOD( Lead, _lead, Derived&, );
        }
        Derived&& lead( types::LitU8 _lead ) &&
        {
          PACE__METHOD( Lead, _lead, Derived&&, );
        }
#endif

        /// @brief Set the color of the component `lead`.
        /// @throw exception::InvalidArgument If the passed parameters is not a valid RGB color string.
        Derived& lead_color( console::escodes::RGBColor _lead_color ) &
        {
          PACE__METHOD( LeadColor, _lead_color, Derived&, std::move );
        }
        Derived&& lead_color( console::escodes::RGBColor _lead_color ) &&
        {
          PACE__METHOD( LeadColor, _lead_color, Derived&&, std::move );
        }

#undef PACE__METHOD

        PACE__CXX20_CNSTXPR void swap( Frame& other ) noexcept
        {
          using std::swap;
          lead_col_.swap( other.lead_col_ );
          lead_.swap( other.lead_ );
          swap( len_longest_lead_, other.len_longest_lead_ );
          Base::swap( other );
        }
      };
    } // namespace aspects

    PACE__INHERIT_REGISTER( aspects::Frame, aspects::Animation, aspects::RenderRule );

    PACE__OPTION_REGISTER( aspects::Frame, option::Lead, option::LeadColor );
  } // namespace details
} // namespace pace

#endif
