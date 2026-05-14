#ifndef PACE_ELAPSED
#define PACE_ELAPSED

#include "../details/aspects/Entailment.hpp"
#include "../details/aspects/Timer.hpp"
#include "../details/behaviors/Indeterminate.hpp"
#include "../details/behaviors/Plain.hpp"
#include "../details/behaviors/Temporal.hpp"
#include "../details/render/Parameter.hpp"
#include "../details/traits/C3.hpp"
#include "../details/traits/TypeSet.hpp"
#include <numeric>

namespace pace {
  namespace option {
    struct ElapsedFormat : details::wrappers::OptionPacket<details::types::String> {
    private:
      using Base = details::wrappers::OptionPacket<details::types::String>;

    public:
      PACE__CXX20_CNSTXPR ElapsedFormat() = default;

      PACE__CXX20_CNSTXPR ElapsedFormat( details::types::String _fmt ) noexcept : Base( std::move( _fmt ) ) {}
#ifdef __cpp_lib_char8_t
      ElapsedFormat( details::types::LitU8 _fmt )
        : Base( { reinterpret_cast<const details::types::Char*>( _fmt.data() ), _fmt.size() } )
      {}
#endif
    };
  } // namespace option

  namespace facade {
    template<typename Base, typename Derived>
    class Elapsed : public Base {
      friend PACE__FORCEINLINE PACE__CXX20_CNSTXPR void unpack( Elapsed& self, option::ElapsedFormat&& val )
      {
        self.fmt_ir_ = Base::parse_timer_fmt( val.value() );
        std::for_each( self.fmt_ir_.cbegin(), self.fmt_ir_.cend(), [&]( const Field& field ) noexcept {
          switch ( field.token() ) {
          case Token::Hour:   self.show_hour_ = true; break;
          case Token::Minute: self.show_minute_ = true; break;
          default:            break;
          }
        } );
      }

      using Field = typename Base::TimerField;
      using Token = typename Base::TimerToken;

      std::vector<Field> fmt_ir_;
      bool show_hour_   : 1;
      bool show_minute_ : 1;

    protected:
      details::io::CharPipeline& build( details::io::CharPipeline& pipeline,
                                        const details::render::Parameter& params ) const
      {
        if ( fmt_ir_.empty() )
          return pipeline << Base::_default_char;

        for ( const auto& field : fmt_ir_ ) {
          details::types::String formatted;

          switch ( field.token() ) {
          case Token::Literal: pipeline << field.text(); continue;

          case Token::Hour: {
            if ( field.width() == 0 ) {
              pipeline << Base::_overflow_char;
              continue;
            }
            formatted = details::utils::format(
              std::chrono::duration_cast<std::chrono::hours>( params.elapsed_time_ ).count() );
          } break;

          case Token::Minute: {
            if ( field.width() == 0 ) {
              pipeline << Base::_overflow_char;
              continue;
            }
            const auto num_minutes =
              std::chrono::duration_cast<std::chrono::minutes>( params.elapsed_time_ ).count();
            formatted = details::utils::format( show_hour_ ? num_minutes % 60 : num_minutes );
          } break;

          case Token::Second: {
            if ( field.width() == 0 ) {
              pipeline << Base::_overflow_char;
              break;
            }
            const auto num_seconds =
              std::chrono::duration_cast<std::chrono::seconds>( params.elapsed_time_ ).count();
            formatted = details::utils::format( show_hour_ || show_minute_ ? num_seconds % 60 : num_seconds );
          } break;

          default: details::utils::unreachable();
          }

          if ( formatted.size() > field.width() ) {
            pipeline.append( Base::_overflow_char, field.width() );
            continue;
          }
          auto blank_length      = field.width() - formatted.size();
          const auto num_padding = blank_length / field.padding().width();
          blank_length -= num_padding * field.padding().width();

          pipeline.append( ' ', blank_length ).append( field.padding(), num_padding ).append( formatted );
        }
        return pipeline;
      }
      PACE__NODISCARD PACE__FORCEINLINE PACE__CXX20_CNSTXPR details::types::Size fixed_length() const noexcept
      {
        if ( fmt_ir_.empty() )
          return 1;
        return std::accumulate( fmt_ir_.cbegin(),
                                fmt_ir_.cend(),
                                details::types::Size {},
                                []( details::types::Size acc, const Field& field ) noexcept {
                                  PACE__ASSERT( field.token() != Token::None );
                                  switch ( field.token() ) {
                                  case Token::Literal: return acc + field.text().width();
                                  case Token::Hour:
                                  case Token::Minute:  PACE__FALLTHROUGH;
                                  case Token::Second:  return acc + ( field.width() == 0 ? 1 : field.width() );
                                  default:             break;
                                  }
                                  details::utils::unreachable();
                                } );
      }

      template<typename... Options>
      PACE__CXX14_CNSTXPR Elapsed( details::traits::TypeSet<Options...> tag ) noexcept : Base( tag )
      {
        if PACE__CXX17_CNSTXPR ( !details::traits::TpContain<details::traits::TypeSet<Options...>,
                                                             option::ElapsedFormat>::value )
          unpack( *this, config::provide_for<Derived, option::ElapsedFormat>() );
      }

      PACE__SPECIAL_MEMBERS( Elapsed );

    public:
#define PACE__METHOD( OptionName, ParamName, Operation, ReturnType )        \
  std::lock_guard<details::concurrent::SharedMutex> lock { this->rw_mtx_ }; \
  unpack( *this, option::OptionName( Operation( ParamName ) ) );            \
  return static_cast<ReturnType>( *this )
      Derived& elapsed_format( details::types::String _fmt_str ) &
      { PACE__METHOD( ElapsedFormat, _fmt_str, std::move, Derived& ); }
      Derived&& elapsed_format( details::types::String _fmt_str ) &&
      { PACE__METHOD( ElapsedFormat, _fmt_str, std::move, Derived&& ); }
#ifdef __cpp_lib_char8_t
      Derived& elapsed_format( details::types::LitU8 _fmt_str ) &
      { PACE__METHOD( ElapsedFormat, _fmt_str, , Derived& ); }
      Derived&& elapsed_format( details::types::LitU8 _fmt_str ) &&
      { PACE__METHOD( ElapsedFormat, _fmt_str, , Derived&& ); }
#endif

#undef PACE__METHOD

      PACE__CXX20_CNSTXPR void swap( Elapsed& other ) & noexcept
      {
        fmt_ir_.swap( other.fmt_ir_ );
        auto tmp           = show_hour_;
        show_hour_         = other.show_hour_;
        other.show_hour_   = tmp;
        tmp                = show_minute_;
        show_minute_       = other.show_minute_;
        other.show_minute_ = tmp;
        Base::swap( other );
      }
    };

  } // namespace facade

  PACE__INHERIT_REGISTER( facade::Elapsed, details::aspects::Timer );

  PACE__OPTION_REGISTER( facade::Elapsed, option::ElapsedFormat );

  PACE__ENTAIL_REGISTER( facade::Elapsed,
                         details::behaviors::Indeterminate,
                         details::behaviors::Plain,
                         details::behaviors::Temporal );
} // namespace pace

#endif
