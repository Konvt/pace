#ifndef PACE_TRUE_COLOR
#define PACE_TRUE_COLOR

#include "../io/CharPipeline.hpp"
#include "../wrappers/RGBValue.hpp"

namespace pace {
  namespace details {
    namespace console {
      class TrueColor {
#ifndef PACE_NOSTYLE
        using Tonality = wrappers::RGBValue::Tonality;
        static constexpr auto& _digits =
          "0  1  2  3  4  5  6  7  8  9  10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31 "
          "32 33 34 35 36 37 38 39 40 41 42 43 44 45 46 47 48 49 50 51 52 53 54 55 56 57 58 59 60 61 62 63 "
          "64 65 66 67 68 69 70 71 72 73 74 75 76 77 78 79 80 81 82 83 84 85 86 87 88 89 90 91 92 93 94 95 "
          "96 97 98 99 "
          "10010110210310410510610710810911011111211311411511611711811912012112212312412512612712812913013113"
          "21331341351361371381391401411421431441451461471481491501511521531541551561571581591601611621631641"
          "65166167168169170171172173174175176177178179180181182183184185186187188189190191192193194195196197"
          "19819920020120220320420520620720820921021121221321421521621721821922022122222322422522622722822923"
          "0231232233234235236237238239240241242243244245246247248249250251252253254255";

        std::array<std::uint8_t, 3> sgr_;
        Tonality ton_;
#endif

      public:
#ifdef PACE_NOSTYLE
        constexpr TrueColor() = default;
#else
        PACE__CXX20_CNSTXPR TrueColor() noexcept : ton_ { Tonality::None } {}
#endif

        PACE__CXX20_CNSTXPR TrueColor( wrappers::RGBValue rgb ) noexcept
#ifdef PACE_NOSTYLE
        { (void)rgb; }
#else
          : ton_ { rgb.tonality() }
        {
          switch ( rgb.tonality() ) {
          case Tonality::Fixed: sgr_.front() = utils::to_underlying( rgb.color() ); break;
          case Tonality::Hex:   sgr_ = { rgb.r(), rgb.g(), rgb.b() }; break;
          default:              break;
          }
        }
#endif

        PACE__CXX20_CNSTXPR TrueColor( const TrueColor& )              = default;
        PACE__CXX20_CNSTXPR TrueColor& operator=( const TrueColor& ) & = default;

        PACE__CXX20_CNSTXPR TrueColor& operator=( wrappers::RGBValue rgb ) & noexcept
        {
#ifdef PACE_NOSTYLE
          (void)rgb;
#else
          sgr_ = { rgb.r(), rgb.g(), rgb.b() };
          ton_ = Tonality::Hex;
#endif
          return *this;
        }

        PACE__FORCEINLINE PACE__CXX20_CNSTXPR void clear() noexcept
        {
#ifndef PACE_NOSTYLE
          ton_ = Tonality::None;
#endif
        }

        PACE__CXX20_CNSTXPR void swap( TrueColor& other ) noexcept
        {
#ifdef PACE_NOSTYLE
          (void)other;
#else
          std::swap( sgr_, other.sgr_ );
#endif
        }
        friend PACE__CXX20_CNSTXPR void swap( TrueColor& a, TrueColor& b ) noexcept { a.swap( b ); }

        friend PACE__FORCEINLINE io::CharPipeline& operator<<( io::CharPipeline& pipeline,
                                                               const TrueColor& col )
        {
#ifdef PACE_NOSTYLE
          (void)col;
#else
          switch ( col.ton_ ) {
          case Tonality::Fixed: {
            pipeline << '\x1B' << '[';
            const auto str = _digits + ( col.sgr_.front() * 3 );
            PACE__ASSERT( col.sgr_.front() >= 10 && col.sgr_.front() < 100 );
            pipeline << str[0] << str[1] << 'm';
          } break;
          case Tonality::Hex: {
            pipeline << '\x1B' << '[' << '3' << '8' << ';' << '2';
            for ( auto offset : col.sgr_ ) {
              pipeline << ';';
              const auto str = _digits + ( offset * 3 );
              pipeline << str[0];
              if ( offset >= 10 )
                pipeline << str[1];
              if ( offset >= 100 )
                pipeline << str[2];
            }
            pipeline << 'm';
          } break;
          default: break;
          }
#endif
          return pipeline;
        }
      };
    } // namespace console
  } // namespace details
} // namespace pace

#endif
