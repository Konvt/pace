#ifndef PACE__TIMER_FIELD
#define PACE__TIMER_FIELD

#include "../charcodes/U8Char.hpp"
#include "../charcodes/U8Raw.hpp"

namespace pace {
  namespace details {
    namespace aspects {
      enum class TimerToken : std::uint8_t { None, Literal, Hour, Minute, Second };

      class TimerField {
        union Field {
          charcodes::U8Raw text_;
          struct Clock {
            types::Size width_;
            charcodes::U8Char padding_;

            PACE__CXX23_CNSTXPR Clock() = default;
            constexpr Clock( types::Size width, charcodes::U8Char padding ) noexcept
              : width_ { width }, padding_ { padding }
            {}
          } clock_;

          PACE__CXX23_CNSTXPR Field() noexcept : clock_ {} {}
          PACE__CXX20_CNSTXPR ~Field() noexcept {}
        } field_;
        TimerToken part_ { TimerToken::None };

      public:
        PACE__NODISCARD static PACE__CXX20_CNSTXPR inline std::vector<TimerField> parse(
          charcodes::StringView fmt_str );

        PACE__CXX20_CNSTXPR TimerField( types::String&& text )
        {
          utils::construct_at( &field_.text_, std::move( text ) );
          part_ = TimerToken::Literal;
        }
        PACE__CXX20_CNSTXPR TimerField( TimerToken token, types::Size width, charcodes::U8Char padding )
        {
          PACE__ASSERT( token != TimerToken::None && token != TimerToken::Literal );
          utils::construct_at( &field_.clock_, width, padding );
          part_ = token;
        }

        PACE__CXX20_CNSTXPR TimerField( const TimerField& other ) { operator=( other ); }
        PACE__CXX20_CNSTXPR TimerField& operator=( const TimerField& other ) &
        {
          if ( part_ == other.part_ ) {
            switch ( part_ ) {
            case TimerToken::None:    break;
            case TimerToken::Literal: field_.text_ = other.field_.text_; break;
            default:                  field_.clock_ = other.field_.clock_; break;
            }
          } else if ( part_ == TimerToken::Literal ) {
            utils::destroy_at( field_.text_ );
            switch ( other.part_ ) {
            case TimerToken::None:    break;
            case TimerToken::Literal: utils::unreachable();
            default:                  utils::construct_at( &field_.clock_, other.field_.clock_ ); break;
            }
          } else if ( part_ != TimerToken::None ) {
            utils::destroy_at( field_.clock_ );
            switch ( other.part_ ) {
            case TimerToken::None:    break;
            case TimerToken::Literal: utils::construct_at( &field_.text_, other.field_.text_ ); break;
            default:                  utils::unreachable();
            }
          } else {
            switch ( other.part_ ) {
            case TimerToken::None:    break;
            case TimerToken::Literal: utils::construct_at( &field_.text_, other.field_.text_ ); break;
            default:                  utils::construct_at( &field_.clock_, other.field_.clock_ ); break;
            }
          }
          part_ = other.part_;
          return *this;
        }
        PACE__CXX20_CNSTXPR TimerField( TimerField&& rhs ) noexcept { operator=( std::move( rhs ) ); }
        PACE__CXX20_CNSTXPR TimerField& operator=( TimerField&& rhs ) noexcept
        {
          if ( part_ == rhs.part_ ) {
            switch ( part_ ) {
            case TimerToken::None:    break;
            case TimerToken::Literal: field_.text_ = std::move( rhs.field_.text_ ); break;
            default:                  field_.clock_ = rhs.field_.clock_; break;
            }
          } else if ( part_ == TimerToken::Literal ) {
            utils::destroy_at( field_.text_ );
            switch ( rhs.part_ ) {
            case TimerToken::None:    break;
            case TimerToken::Literal: utils::unreachable();
            default:                  utils::construct_at( &field_.clock_, rhs.field_.clock_ ); break;
            }
          } else if ( part_ != TimerToken::None ) {
            utils::destroy_at( field_.clock_ );
            switch ( rhs.part_ ) {
            case TimerToken::None: break;
            case TimerToken::Literal:
              utils::construct_at( &field_.text_, std::move( rhs.field_.text_ ) );
              break;
            default: utils::unreachable();
            }
          } else {
            switch ( rhs.part_ ) {
            case TimerToken::None: break;
            case TimerToken::Literal:
              utils::construct_at( &field_.text_, std::move( rhs.field_.text_ ) );
              break;
            default: utils::construct_at( &field_.clock_, rhs.field_.clock_ ); break;
            }
          }
          part_ = rhs.part_;
          return *this;
        }

        PACE__CXX20_CNSTXPR ~TimerField() noexcept
        {
          switch ( part_ ) {
          case TimerToken::None:    break;
          case TimerToken::Literal: utils::destroy_at( field_.text_ ); break;
          default:                  utils::destroy_at( field_.clock_ ); break;
          }
        }

        PACE__NODISCARD PACE__CXX20_CNSTXPR TimerToken token() const noexcept { return part_; }
        PACE__NODISCARD PACE__CXX20_CNSTXPR const charcodes::U8Raw& text() const noexcept
        { return field_.text_; }
        PACE__NODISCARD PACE__CXX20_CNSTXPR types::Size width() const noexcept
        { return field_.clock_.width_; }
        PACE__NODISCARD PACE__CXX20_CNSTXPR const charcodes::U8Char& padding() const noexcept
        { return field_.clock_.padding_; }
      };

      PACE__CXX20_CNSTXPR std::vector<TimerField> TimerField::parse( charcodes::StringView fmt_str )
      {
        std::vector<TimerField> fields;
        if ( fmt_str.empty() )
          return fields;

        enum class Phase : std::uint8_t { Symbol, Marker };
        Phase state            = Phase::Symbol;
        types::Size num_seen_h = 0, num_seen_m = 0, num_seen_s = 0;
        types::Size width;
        charcodes::U8Char padding;
        types::String buffer;

        // "%H:%M:%S"
        // "%3H:%2M:%2S"
        // "%:03H:%: 2M:% 2S"
        for ( types::Size i = 0; i < fmt_str.size(); ++i ) {
          switch ( state ) {
          case Phase::Symbol: {
            const auto sentinel = fmt_str.find( '%', i );
            if ( sentinel == fmt_str.npos ) {
              buffer.append( fmt_str.data() + i, fmt_str.size() - i );
              i = fmt_str.size() - 1;
              break;
            }
            buffer.append( fmt_str.data() + i, sentinel - i );
            i       = sentinel;
            // set defaults
            width   = 2;
            padding = charcodes::U8Char( '0' );
            state   = Phase::Marker;
          } break;

          case Phase::Marker: {
            if ( fmt_str[i] == '%' ) {
              buffer.push_back( '%' );
              state = Phase::Symbol;
              break;
            }
            if ( !buffer.empty() )
              fields.emplace_back( std::move( buffer ) );
            if ( fmt_str[i] == ':' ) {
              if ( i + 1 >= fmt_str.size() )
                PACE__UNLIKELY throw exception::InvalidArgument( "pace: stray ':' at end of format" );
              padding = charcodes::U8Char::from_bytes( fmt_str.substr( i + 1 ) );
              i += padding.size();
            } else {
              switch ( fmt_str[i] ) {
              case '0':
              case '1':
              case '2':
              case '3':
              case '4':
              case '5':
              case '6':
              case '7':
              case '8': PACE__FALLTHROUGH;
              case '9': {
                auto sentinel = i + 1;
                while ( sentinel < fmt_str.size() ) {
                  if ( fmt_str[sentinel] == 'H' || fmt_str[sentinel] == 'M' || fmt_str[sentinel] == 'S' )
                    break;
                  else if ( fmt_str[sentinel] < '0' || fmt_str[sentinel] > '9' )
                    PACE__UNLIKELY throw exception::InvalidArgument(
                      "pace: non-numeric character in timer width" );
                  ++sentinel;
                }
                if ( sentinel >= fmt_str.size() )
                  PACE__UNLIKELY throw exception::InvalidArgument( "pace: missing token after width" );
                width = 0;
                while ( i < sentinel )
                  width = width * 10 + fmt_str[i++] - '0';
                --i;
              } break;

              case 'H': {
                fields.emplace_back( TimerToken::Hour, width, padding );
                ++num_seen_h;
                state = Phase::Symbol;
              } break;

              case 'M': {
                fields.emplace_back( TimerToken::Minute, width, padding );
                ++num_seen_m;
                state = Phase::Symbol;
              } break;

              case 'S': {
                fields.emplace_back( TimerToken::Second, width, padding );
                ++num_seen_s;
                state = Phase::Symbol;
              } break;

              default: throw exception::InvalidArgument( "pace: invalid clock format token" );
              }
            }
          } break;

          default: utils::unreachable();
          }
        }

        if ( state != Phase::Symbol )
          PACE__UNLIKELY throw exception::InvalidArgument( "pace: incomplete clock format sequence" );
        else if ( !buffer.empty() )
          fields.emplace_back( std::move( buffer ) );
        if ( ( num_seen_h == 0 && num_seen_m == 0 && num_seen_s == 0 ) || num_seen_h > 1 || num_seen_m > 1
             || num_seen_s > 1 )
          PACE__UNLIKELY throw exception::InvalidArgument( "pace: missing or redundant clock markers" );

        return fields;
      }
    } // namespace aspects
  } // namespace details
} // namespace pace

#endif
