#ifndef PGBAR_PARAMETER
#define PGBAR_PARAMETER

#include "../core/Core.hpp"
#include "../types/Types.hpp"

namespace pgbar {
  namespace details {
    namespace render {
      struct Parameter {
        std::uint64_t task_quota_;
        std::uint64_t tasks_completed_;
        types::Float progress_ratio_ { 0.0 };
        details::types::Tempus elapsed_time_;
        std::uint32_t frame_count_;
        bool style_off_;

        Parameter( std::uint64_t task_quota,
                   std::uint64_t tasks_completed,
                   std::chrono::steady_clock::time_point start_time,
                   std::uint32_t frame_count,
                   bool style_off ) noexcept
          : task_quota_ { task_quota }
          , tasks_completed_ { tasks_completed }
          , elapsed_time_ { std::chrono::duration_cast<details::types::Tempus>(
              std::chrono::steady_clock::now() - start_time ) }
          , frame_count_ { frame_count }
          , style_off_ { style_off }
        {
          PGBAR__TRUST( tasks_completed <= task_quota );
          if ( task_quota > 0 ) {
            progress_ratio_ = static_cast<types::Float>( tasks_completed ) / task_quota;
          }
          PGBAR__TRUST( progress_ratio_ >= 0.0 );
          PGBAR__TRUST( progress_ratio_ <= 1.0 );
        }
        Parameter( std::uint64_t task_quota,
                   std::uint64_t tasks_completed,
                   std::chrono::steady_clock::time_point start_time,
                   bool style_off ) noexcept
          : Parameter( task_quota, tasks_completed, start_time, 0, style_off )
        {}
      };
    } // namespace render
  } // namespace details
} // namespace pgbar

#endif
