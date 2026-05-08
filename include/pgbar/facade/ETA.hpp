#ifndef PGBAR_ETA
#define PGBAR_ETA

#include "../details/aspects/Capacity.hpp"
#include "../details/aspects/Timer.hpp"
#include "../details/behaviors/Incremental.hpp"
#include "../details/behaviors/Temporal.hpp"
#include "../details/render/Parameter.hpp"
#include "../details/traits/C3.hpp"
#include "BehaviorRegistry.hpp"

namespace pgbar {
  namespace facade {
    template<typename Base, typename Derived>
    class ETA : public Base {
    protected:
      _details::io::CharPipeline& build( _details::io::CharPipeline& pipeline,
                                         const _details::render::Parameter& params ) const
      {
        if ( params.tasks_completed_ == 0 || params.task_quota_ == 0 )
          return pipeline << '-' << Base::_default_timer;

        auto time_per_task = params.elapsed_time_ / params.tasks_completed_;
        if ( time_per_task.count() == 0 )
          time_per_task = _details::types::Tempus( 1 );

        const auto remaining_tasks = params.task_quota_ - params.tasks_completed_;
        // overflow check
        if ( remaining_tasks > ( std::numeric_limits<std::int64_t>::max )() / time_per_task.count() )
          return pipeline << Base::_default_overflow;
        pipeline << '-';
        return this->to_hms( pipeline, time_per_task * remaining_tasks );
      }
      PGBAR__NODISCARD static PGBAR__FORCEINLINE PGBAR__CNSTEVAL _details::types::Size fixed_length() noexcept
      { // deliberately not subtracting 1
        return sizeof( Base::_default_timer );
      }

      template<typename... Options>
      constexpr ETA( _details::traits::TypeSet<Options...> tag ) noexcept : Base( tag )
      {}

      PGBAR__SPECIAL_MEMBERS( ETA );
    };
  } // namespace facade

  PGBAR__INHERIT_REGISTER( facade::ETA, _details::aspects::Timer, _details::aspects::Capacity );

  PGBAR__BEHAVIOR_REGISTER( facade::ETA, _details::behaviors::Incremental, _details::behaviors::Temporal );
} // namespace pgbar

#endif
