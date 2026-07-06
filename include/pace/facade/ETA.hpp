#ifndef PACE_ETA
#define PACE_ETA

#include "../details/aspects/Capacity.hpp"
#include "../details/aspects/Entailment.hpp"
#include "../details/aspects/TimerField.hpp"
#include "../details/behaviors/Incremental.hpp"
#include "../details/behaviors/Indeterminate.hpp"
#include "../details/behaviors/Plain.hpp"
#include "../details/behaviors/Temporal.hpp"
#include "../details/render/Parameter.hpp"
#include "../details/traits/C3.hpp"
#include "../details/utils/Util.hpp"
#include <numeric>

namespace pace {
  namespace option {
    /**
     * A wrapper that stores format string for timer formatting.
     *
     * The format string supports three units: hour %H, minute %M and second %S.
     *
     * Characters other than format specifiers are output literally; for example:
     *
     * `%: 3H:%2M:%S`
     *  ^~~~^ ^~^ ^^ --- Time units
     *   ^~^   ^     --- Alignment and fill character
     *       ^   ^   --- Literals
     *
     * %: 3H - `3` specifies width 3; values shorter than the width are padded,
     *             while longer values are replaced with `###`.
     *         `:` means the next and only next Unicode character is used as fill character;
     *             here it is ` `.
     *         `:` is optional and does not support Unicode combining characters.
     *
     * The actual format is: %[ ':' <fill-char> ][ <width> ]<unit>; and %% escapes `%`.
     *
     * Width defaults to 2 if omitted; fill character defaults to `0`.
     *
     * If the digit count exceeds the width, equally wide `#` characters are output;
     * but width 0 always outputs a single `?`.
     *
     * Only the largest unit does not overflow; all others are normalized using clock carry rules.
     * For example:
     *
     * %H:%M:%S -> 01:01:01
     * %M:%S    -> 61:01
     * %S       -> 3661
     *
     * @throw exception::InvalidArgument
     *   Thrown if any input string fails UTF-8 validation or the array size mismatches.
     *   Thrown also if input string is not a valid format string.
     */
    struct ETAFormat : public details::wrappers::OptionPacket<std::vector<details::aspects::TimerField>> {
    private:
      using Field = details::aspects::TimerField;

    public:
      PACE__CXX20_CNSTXPR ETAFormat() = default;
      PACE__CXX20_CNSTXPR ETAFormat( details::charcodes::StringView _fmt_str ) noexcept
        : details::wrappers::OptionPacket<std::vector<Field>>( Field::parse( _fmt_str ) )
      {}
    };
  } // namespace option

  namespace facade {
    template<typename Base, typename Derived>
    class ETA : public Base {
      using Field = details::aspects::TimerField;
      using Token = details::aspects::TimerToken;

      friend PACE__FORCEINLINE PACE__CXX20_CNSTXPR void unpack( ETA& self, option::ETAFormat&& val )
      {
        self.show_hour_   = false;
        self.show_minute_ = false;
        self.fmt_ir_      = std::move( val.value() );
        std::for_each( self.fmt_ir_.cbegin(), self.fmt_ir_.cend(), [&]( const Field& field ) noexcept {
          switch ( field.token() ) {
          case Token::Hour:   self.show_hour_ = true; break;
          case Token::Minute: self.show_minute_ = true; break;
          default:            break;
          }
        } );
      }

      PACE__NODISCARD static PACE__FORCEINLINE constexpr details::types::Char overflow_char() noexcept
      { return '#'; }
      PACE__NODISCARD static PACE__FORCEINLINE constexpr details::types::Char unkown_char() noexcept
      { return '?'; }

      std::vector<Field> fmt_ir_;
      bool show_hour_   : 1;
      bool show_minute_ : 1;

    protected:
      details::io::CharPipeline& build( details::io::CharPipeline& pipeline,
                                        const details::render::Parameter& params ) const
      {
        if ( fmt_ir_.empty() )
          return pipeline << unkown_char();

        bool overflow = false;
        details::types::Tempus remaining_time;
        if ( params.task_quota_ > 0 && params.tasks_completed_ > 0 ) {
          auto time_per_task = params.elapsed_time_ / params.tasks_completed_;
          if ( time_per_task.count() == 0 )
            time_per_task = details::types::Tempus( 1 );
          const std::uint64_t remaining_tasks = params.task_quota_ - params.tasks_completed_;
          if ( remaining_tasks > ( std::numeric_limits<std::uint64_t>::max )() / time_per_task.count() )
            overflow = true;
          else
            remaining_time = time_per_task * remaining_tasks;
        }

        for ( const auto& field : fmt_ir_ ) {
          std::int64_t timer_value = 0;
          switch ( field.token() ) {
          case Token::Literal: pipeline << field.text(); continue;

          case Token::Hour: {
            if ( field.width() == 0 ) {
              pipeline << unkown_char();
              continue;
            } else if ( params.task_quota_ > 0 && !overflow )
              timer_value = std::chrono::duration_cast<std::chrono::hours>( remaining_time ).count();
          } break;

          case Token::Minute: {
            if ( field.width() == 0 ) {
              pipeline << unkown_char();
              continue;
            } else if ( params.task_quota_ > 0 && !overflow ) {
              const auto num_minutes =
                std::chrono::duration_cast<std::chrono::minutes>( remaining_time ).count();
              timer_value = show_hour_ ? num_minutes % 60 : num_minutes;
            }
          } break;

          case Token::Second: {
            if ( field.width() == 0 ) {
              pipeline << unkown_char();
              continue;
            } else if ( params.task_quota_ > 0 && !overflow ) {
              const auto num_seconds =
                std::chrono::duration_cast<std::chrono::seconds>( remaining_time ).count();
              timer_value = show_hour_ || show_minute_ ? num_seconds % 60 : num_seconds;
            }
          } break;

          default: details::utils::unreachable();
          }

          const auto num_digits = details::utils::count_digits( timer_value );
          if ( params.task_quota_ == 0 ) {
            pipeline.append( unkown_char(), field.width() );
            continue;
          } else if ( overflow || num_digits > field.width() ) {
            pipeline.append( overflow_char(), field.width() );
            continue;
          }

          auto blank_length      = field.width() - num_digits;
          const auto num_padding = blank_length / field.padding().width();
          blank_length -= num_padding * field.padding().width();

          pipeline.append( ' ', blank_length ).append( field.padding(), num_padding );
          details::utils::format_to( std::back_inserter( pipeline ), timer_value );
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
      PACE__CXX14_CNSTXPR ETA( details::traits::TypeSet<Options...> tag ) noexcept : Base( tag )
      {
        if PACE__CXX17_CNSTXPR ( !details::traits::TpContains<details::traits::TypeSet<Options...>,
                                                              option::ETAFormat>::value )
          unpack( *this, config::provide_for<Derived, option::ETAFormat>() );
      }

      PACE__SPECIAL_MEMBERS( ETA );

    public:
#define PACE__METHOD( ParamName, ReturnType )                               \
  std::lock_guard<details::concurrent::SharedMutex> lock { this->rw_mtx_ }; \
  unpack( *this, option::ETAFormat( ParamName ) );                          \
  return static_cast<ReturnType>( *this )

      /**
       * Set elapsed time format.
       *
       * Format syntax is described in `option::ETAFormat`.
       *
       * @param _fmt_str Format string.
       *
       * @throw exception::InvalidArgument Propagated from `option::ETAFormat`.
       */
      Derived& eta_format( details::charcodes::StringView _fmt_str ) &
      { PACE__METHOD( _fmt_str, Derived& ); }
      Derived&& eta_format( details::charcodes::StringView _fmt_str ) &&
      { PACE__METHOD( _fmt_str, Derived&& ); }

#undef PACE__METHOD
#ifdef __cpp_lib_char8_t
# define PACE__METHOD( ParamName, ReturnType )                                                            \
   std::lock_guard<details::concurrent::SharedMutex> lock { this->rw_mtx_ };                              \
   unpack( *this,                                                                                         \
           option::ETAFormat(                                                                             \
             { reinterpret_cast<const details::types::Char*>( ParamName.data() ), ParamName.size() } ) ); \
   return static_cast<ReturnType>( *this )

      Derived& eta_format( details::charcodes::U8StringView _fmt_str ) &
      { PACE__METHOD( _fmt_str, Derived& ); }
      Derived&& eta_format( details::charcodes::U8StringView _fmt_str ) &&
      { PACE__METHOD( _fmt_str, Derived&& ); }

# undef PACE__METHOD
#endif

      PACE__CXX20_CNSTXPR void swap( ETA& other ) & noexcept
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

  PACE__INHERIT_REGISTER( facade::ETA, details::aspects::Capacity );

  PACE__OPTION_REGISTER( facade::ETA, option::ETAFormat );

  PACE__ENTAIL_REGISTER( facade::ETA,
                         details::behaviors::Indeterminate,
                         details::behaviors::Plain,
                         details::behaviors::Incremental,
                         details::behaviors::Temporal );
} // namespace pace

#endif
