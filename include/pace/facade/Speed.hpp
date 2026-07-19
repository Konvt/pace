#ifndef PACE_SPEED
#define PACE_SPEED

#include "../details/aspects/Capacity.hpp"
#include "../details/aspects/Entailment.hpp"
#include "../details/behaviors/Incremental.hpp"
#include "../details/behaviors/Indeterminate.hpp"
#include "../details/behaviors/Plain.hpp"
#include "../details/behaviors/Temporal.hpp"
#include "../details/io/CharPipeline.hpp"
#include "../details/render/Parameter.hpp"
#include "../details/render/TextAlign.hpp"
#include "../details/traits/C3.hpp"
#include "../details/utils/Util.hpp"

namespace pace {
  namespace option {
    /**
     * A wrapper that stores the base magnitude for unit scaling in formatted output.
     *
     * Defines the threshold at which values are converted to higher-order units
     * (e.g. 1000 -> "1k", 1000000 -> "1M").
     *
     * The effective range is between 1 and 65535.
     *
     * - A zero value implies no scaling (raw numeric display).
     *
     * - Typical usage: 1000 (decimal) or 1024 (binary) scaling.
     */
    struct Magnitude : PACE__DERIVING_OPTION2( Magnitude, std::uint16_t, _magnitude );

    /**
     * A wrapper that stores ordered units for information rate formatting (e.g. B/s, kB/s).
     *
     * Encapsulates various consecutive scaling units where each unit is scaled
     * by the configured magnitude factor.
     *
     * Unit order MUST be ascending: [base_unit, scaled_unit_1, scaled_unit_2, scaled_unit_3].
     *
     * Example:
     *
     * - magnitude=1000: ["B/s", "kB/s", "MB/s", "GB/s"]
     *
     * - magnitude=1024: ["B/s", "KiB/s", "MiB/s", "GiB/s"]
     *
     * Scaling logic: value >= magnitude -> upgrade to next unit tier.
     *
     * @throw exception::InvalidArgument
     *   Thrown if any input string fails UTF-8 validation or the array size mismatches.
     */
    struct SpeedUnit : details::wrappers::OptionPacket<std::vector<details::charcodes::U8Raw>> {
      PACE__CXX20_CNSTXPR SpeedUnit() = default;

      /**
       * @throw exception::InvalidArgument
       *
       * If the passed parameters are not coding in UTF-8.
       *
       * @param _units
       * The given each unit will be treated as 1,000 times greater than the previous one
       * (from left to right).
       */
      PACE__CXX20_CNSTXPR SpeedUnit( std::vector<std::string> _units )
      {
        data_.reserve( _units.size() );
        std::transform( std::make_move_iterator( _units.begin() ),
                        std::make_move_iterator( _units.end() ),
                        std::back_inserter( data_ ),
                        []( std::string&& ele ) { return details::charcodes::U8Raw( std::move( ele ) ); } );
      }
#ifdef __cpp_lib_char8_t
      /**
       * @param _units
       * The given each unit will be treated as 1,000 times greater than the previous one
       * (from left to right).
       */
      PACE__CXX20_CNSTXPR SpeedUnit( const std::vector<details::charcodes::U8StringView>& _units )
      {
        data_.reserve( _units.size() );
        std::transform(
          _units.cbegin(),
          _units.cend(),
          std::back_inserter( data_ ),
          []( details::charcodes::U8StringView ele ) { return details::charcodes::U8Raw( ele ); } );
      }
#endif
    };
  } // namespace option

  namespace facade {
    template<typename Base, typename Derived>
    class Speed : public Base {
      friend PACE__FORCEINLINE PACE__CXX20_CNSTXPR void unpack( Speed& self,
                                                                option::Magnitude&& val ) noexcept
      {
        self.magnitude_ = val.value();
        if ( self.magnitude_ > 1 )
          // 3 is the length of ".00"
          self.numeric_width_ = 3 + details::utils::count_digits( val.value() );
        else
          self.numeric_width_ = undefined_text().size() - 1;
      }
      friend PACE__FORCEINLINE PACE__CXX20_CNSTXPR void unpack( Speed& self,
                                                                option::SpeedUnit&& val ) noexcept
      {
        self.units_ = std::move( val.value() );
        auto itr    = std::max_element(
          self.units_.cbegin(),
          self.units_.cend(),
          []( const details::charcodes::U8Raw& a, const details::charcodes::U8Raw& b ) noexcept {
            return a.width() < b.width();
          } );
        if ( itr != self.units_.cend() )
          self.widest_width_ = itr->width();
        else
          self.widest_width_ = 0;
      }

      // The overflow char is used to replace values that exceed the display width.
      PACE__NODISCARD static PACE__FORCEINLINE constexpr char overflow_char() noexcept { return '#'; }
      PACE__NODISCARD static PACE__FORCEINLINE constexpr details::charcodes::StringView
        overflow_text() noexcept
      { return "inf. "; }
      PACE__NODISCARD static PACE__FORCEINLINE constexpr details::charcodes::StringView
        invalid_text() noexcept
      { return "nan. "; }
      PACE__NODISCARD static PACE__FORCEINLINE constexpr details::charcodes::StringView
        undefined_text() noexcept
      { return "und. "; }

      std::vector<details::charcodes::U8Raw> units_;
      std::size_t widest_width_;
      std::size_t numeric_width_;
      std::uint16_t magnitude_;

    protected:
      details::io::CharPipeline& build( details::io::CharPipeline& pipeline,
                                        const details::render::Parameter& params ) const
      {
        if ( params.task_quota_ == 0 || magnitude_ <= 1 ) {
          const auto prompt = params.task_quota_ == 0 ? invalid_text() : undefined_text();
          if ( units_.empty() )
            details::render::align_to<details::render::TextAlign::Right>( std::back_inserter( pipeline ),
                                                                          prompt,
                                                                          fixed_length() );
          else
            details::render::align_to<details::render::TextAlign::Right>( std::back_inserter( pipeline ),
                                                                          prompt + units_.front(),
                                                                          fixed_length() );
          return pipeline;
        }

        const auto seconds_passed =
          std::chrono::duration<details::types::Float>( params.elapsed_time_ ).count();
        const details::types::Float frequency = seconds_passed <= 0.0
                                                ? ( std::numeric_limits<details::types::Float>::max )()
                                                : params.tasks_completed_ / seconds_passed;

        bool overflow           = false;
        std::size_t num_powered = 0;
        /* Since the cube of the maximum value of std::uint16_t does not exceed
         * the representable range of std::uint64_t,
         * we choose to use std::uint16_t to represent the scaling magnitude. */
        std::uint64_t tier      = 1;
        while ( !overflow && frequency >= static_cast<details::types::Float>( tier ) * magnitude_ ) {
          ++num_powered;
          const auto next_tier = tier * magnitude_;
          PACE__ASSERT( tier != next_tier );
          if ( tier < next_tier )
            tier = next_tier;
          else
            overflow = true;
        }

        num_powered = std::min( num_powered, units_.size() - 1 );
        std::string orig;
        if ( overflow )
          orig = static_cast<std::string>( overflow_text() );
        else {
          details::utils::format_to( std::back_inserter( orig ), frequency / tier, 2 );
          if ( orig.size() > numeric_width_ )
            orig = std::string( numeric_width_, overflow_char() );
        }
        orig += ' ';
        if ( !units_.empty() )
          orig.append( units_[num_powered].data(), units_[num_powered].size() );

        details::render::align_to<details::render::TextAlign::Right>( std::back_inserter( pipeline ),
                                                                      orig,
                                                                      fixed_length() );
        return pipeline;
      }

      PACE__NODISCARD PACE__FORCEINLINE constexpr std::size_t fixed_length() const noexcept
      { return numeric_width_ + 1 + widest_width_; }

      template<typename... Options>
      PACE__CXX20_CNSTXPR Speed( details::traits::TypeSet<Options...> tag ) : Base( tag )
      {
        using OptionSet = details::traits::TypeSet<Options...>;
        if PACE__CXX17_CNSTXPR ( !details::traits::TpContains<OptionSet, option::Magnitude>::value )
          unpack( *this, config::provide_for<Derived, option::Magnitude>() );
        if PACE__CXX17_CNSTXPR ( !details::traits::TpContains<OptionSet, option::SpeedUnit>::value )
          unpack( *this, config::provide_for<Derived, option::SpeedUnit>() );
      }

      PACE__CXX20_CNSTXPR Speed() = default;
      PACE__SPECIAL_MEMBERS_CX( Speed, PACE__CXX20_CNSTXPR );

    public:
#define PACE__METHOD( OptionName, ParamName, Operation, ReturnType )        \
  std::lock_guard<details::concurrent::SharedMutex> lock { this->rw_mtx_ }; \
  unpack( *this, option::OptionName( Operation( ParamName ) ) );            \
  return static_cast<ReturnType>( *this )

      /**
       * @throw exception::InvalidArgument
       *
       * If the passed parameters are not coding in UTF-8.
       *
       * @param _units
       * The given each unit will be treated as 1,000 times greater than the previous one
       * (from left to right).
       */
      Derived& speed_unit( std::vector<std::string> _units ) &
      { PACE__METHOD( SpeedUnit, _units, std::move, Derived& ); }
      Derived&& speed_unit( std::vector<std::string> _units ) &&
      { PACE__METHOD( SpeedUnit, _units, std::move, Derived&& ); }
#ifdef __cpp_lib_char8_t
      /**
       * @param _units
       * The given each unit will be treated as 1,000 times greater than the previous one
       * (from left to right).
       */
      Derived& speed_unit( const std::vector<details::charcodes::U8StringView>& _units ) &
      { PACE__METHOD( SpeedUnit, _units, , Derived& ); }
      Derived&& speed_unit( const std::vector<details::charcodes::U8StringView>& _units ) &&
      { PACE__METHOD( SpeedUnit, _units, , Derived&& ); }
#endif

      /**
       * @param _magnitude
       * The base magnitude for unit scaling in formatted output.
       *
       * Defines the threshold at which values are converted to higher-order units
       * (e.g. 1000 -> "1k", 1000000 -> "1M").
       */
      Derived& magnitude( std::uint16_t _magnitude ) & noexcept
      { PACE__METHOD( Magnitude, _magnitude, , Derived& ); }
      Derived&& magnitude( std::uint16_t _magnitude ) && noexcept
      { PACE__METHOD( Magnitude, _magnitude, , Derived&& ); }

#undef PACE__METHOD

      PACE__CXX20_CNSTXPR void swap( Speed& other ) & noexcept
      {
        units_.swap( other.units_ );
        std::swap( widest_width_, other.widest_width_ );
        std::swap( magnitude_, other.magnitude_ );
        Base::swap( other );
      }
    };
  } // namespace facade

  PACE__INHERIT_REGISTER( facade::Speed, details::aspects::Capacity );

  PACE__OPTION_REGISTER( facade::Speed, option::Magnitude, option::SpeedUnit );

  PACE__ENTAIL_REGISTER( facade::Speed,
                         details::behaviors::Indeterminate,
                         details::behaviors::Plain,
                         details::behaviors::Incremental,
                         details::behaviors::Temporal );
} // namespace pace

#endif
