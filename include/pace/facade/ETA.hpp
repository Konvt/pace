#ifndef PACE_ETA
#define PACE_ETA

#include "../details/aspects/Capacity.hpp"
#include "../details/aspects/Entailment.hpp"
#include "../details/aspects/Timer.hpp"
#include "../details/behaviors/Incremental.hpp"
#include "../details/behaviors/Temporal.hpp"
#include "../details/render/Parameter.hpp"
#include "../details/traits/C3.hpp"

namespace pace {
  namespace facade {
    template<typename Base, typename Derived>
    class ETA : public Base {
    protected:
      details::io::CharPipeline& build( details::io::CharPipeline& pipeline,
                                        const details::render::Parameter& params ) const
      {
        if ( params.tasks_completed_ == 0 || params.task_quota_ == 0 )
          return pipeline << '-' << Base::_default_timer;

        auto time_per_task = params.elapsed_time_ / params.tasks_completed_;
        if ( time_per_task.count() == 0 )
          time_per_task = details::types::Tempus( 1 );

        const auto remaining_tasks = params.task_quota_ - params.tasks_completed_;
        // overflow check
        if ( remaining_tasks > ( std::numeric_limits<std::int64_t>::max )() / time_per_task.count() )
          return pipeline << Base::_default_overflow;
        pipeline << '-';
        return this->to_hms( pipeline, time_per_task * remaining_tasks );
      }
      PACE__NODISCARD static PACE__FORCEINLINE PACE__CNSTEVAL details::types::Size fixed_length() noexcept
      { // deliberately not subtracting 1
        return sizeof( Base::_default_timer );
      }

      template<typename... Options>
      constexpr ETA( details::traits::TypeSet<Options...> tag ) noexcept : Base( tag )
      {}

      PACE__SPECIAL_MEMBERS( ETA );
    };
  } // namespace facade

  PACE__INHERIT_REGISTER( facade::ETA, details::aspects::Timer, details::aspects::Capacity );

  PACE__ENTAIL_REGISTER( facade::ETA, details::behaviors::Incremental, details::behaviors::Temporal );
} // namespace pace

#endif
