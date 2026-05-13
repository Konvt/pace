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
#include "../details/traits/C3.hpp"

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
      PACE__CXX20_CNSTXPR SpeedUnit( std::vector<details::types::String> _units )
      {
        data_.reserve( _units.size() );
        std::transform(
          std::make_move_iterator( _units.begin() ),
          std::make_move_iterator( _units.end() ),
          std::back_inserter( data_ ),
          []( details::types::String&& ele ) { return details::charcodes::U8Raw( std::move( ele ) ); } );
      }
#ifdef __cpp_lib_char8_t
      /**
       * @param _units
       * The given each unit will be treated as 1,000 times greater than the previous one
       * (from left to right).
       */
      PACE__CXX20_CNSTXPR SpeedUnit( const std::vector<details::types::LitU8>& _units )
      {
        data_.reserve( _units.size() );
        std::transform( _units.cbegin(),
                        _units.cend(),
                        std::back_inserter( data_ ),
                        []( const details::types::LitU8& ele ) { return details::charcodes::U8Raw( ele ); } );
      }
#endif
    };
  } // namespace option

  namespace facade {
    template<typename Base, typename Derived>
    class Speed : public Base {
      friend PACE__FORCEINLINE PACE__CXX20_CNSTXPR void unpack( Speed& self, option::Magnitude&& val )
      {
        if ( val.value() <= 1 )
          PACE__UNLIKELY throw exception::InvalidArgument( "speed magnitude must be greater than 1" );
        self.magnitude_ = val.value();
      }
      friend PACE__FORCEINLINE PACE__CXX20_CNSTXPR void unpack( Speed& self, option::SpeedUnit&& val )
      {
        if ( val.value().empty() )
          PACE__UNLIKELY throw exception::InvalidArgument( "a speed with no units is meaningless" );
        self.units_        = std::move( val.value() );
        self.widest_width_ = std::max_element( self.units_.cbegin(),
                                               self.units_.cend(),
                                               []( const details::charcodes::U8Raw& a,
                                                   const details::charcodes::U8Raw& b ) noexcept {
                                                 return a.width() < b.width();
                                               } )
                               ->width();
      }

      // The width prepared for "999.99 "
      static constexpr auto& _default_speed = "   inf ";

    protected:
      std::vector<details::charcodes::U8Raw> units_;
      details::types::Size widest_width_;
      std::uint16_t magnitude_;

      details::io::CharPipeline& build( details::io::CharPipeline& pipeline,
                                        const details::render::Parameter& params ) const
      {
        if ( params.task_quota_ == 0 )
          PACE__UNLIKELY return pipeline
            << details::utils::format<details::utils::TxtLayout::Right>( fixed_length(),
                                                                         "-- " + units_.front() );

        const auto seconds_passed =
          std::chrono::duration<details::types::Float>( params.elapsed_time_ ).count();
        const details::types::Float frequency = seconds_passed <= 0.0
                                                ? ( std::numeric_limits<details::types::Float>::max )()
                                                : params.tasks_completed_ / seconds_passed;

        bool overflow                    = false;
        details::types::Size num_powered = 0;
        /* Since the cube of the maximum value of std::uint16_t does not exceed
         * the representable range of std::uint64_t,
         * we choose to use std::uint16_t to represent the scaling magnitude. */
        std::uint64_t tier               = 1;
        PACE__ASSERT( magnitude_ > 1 );
        while ( !overflow && frequency >= static_cast<details::types::Float>( tier ) * magnitude_ ) {
          ++num_powered;
          const auto next_tier = tier * magnitude_;
          PACE__ASSERT( tier != next_tier );
          if ( tier < next_tier )
            tier = next_tier;
          else
            overflow = true;
        }

        PACE__ASSERT( units_.empty() == false );
        num_powered = std::min( num_powered, units_.size() - 1 );
        details::types::String orig;
        if ( overflow )
          orig = _default_speed;
        else
          orig = details::utils::format( frequency / tier, 2 ) + ' ';
        orig += units_[num_powered];

        return pipeline << details::utils::format<details::utils::TxtLayout::Right>( fixed_length(),
                                                                                     std::move( orig ) );
      }

      PACE__NODISCARD PACE__FORCEINLINE constexpr details::types::Size fixed_length() const noexcept
      { return sizeof( _default_speed ) - 1 + widest_width_; }

      template<typename... Options>
      PACE__CXX20_CNSTXPR Speed( details::traits::TypeSet<Options...> tag ) : Base( tag )
      {
        using OptionSet = details::traits::TypeSet<Options...>;
        if PACE__CXX17_CNSTXPR ( !details::traits::TpContain<OptionSet, option::Magnitude>::value )
          unpack( *this, config::provide_for<Derived, option::Magnitude>() );
        if PACE__CXX17_CNSTXPR ( !details::traits::TpContain<OptionSet, option::SpeedUnit>::value )
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
      Derived& speed_unit( std::vector<details::types::String> _units ) &
      { PACE__METHOD( SpeedUnit, _units, std::move, Derived& ); }
      Derived&& speed_unit( std::vector<details::types::String> _units ) &&
      { PACE__METHOD( SpeedUnit, _units, std::move, Derived&& ); }
#ifdef __cpp_lib_char8_t
      /**
       * @param _units
       * The given each unit will be treated as 1,000 times greater than the previous one
       * (from left to right).
       */
      Derived& speed_unit( const std::vector<details::types::LitU8>& _units ) &
      { PACE__METHOD( SpeedUnit, _units, , Derived& ); }
      Derived&& speed_unit( const std::vector<details::types::LitU8>& _units ) &&
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
