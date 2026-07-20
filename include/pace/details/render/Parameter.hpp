#ifndef PACE_PARAMETER
#define PACE_PARAMETER

#include "../core/Core.hpp"
#include "../core/Types.hpp"

namespace pace {
  namespace details {
    namespace render {
      struct Parameter {
        std::uint64_t task_quota;
        std::uint64_t tasks_completed;
        types::Float progress_ratio { 0.0 };
        types::Tempus elapsed_time;
        std::uint32_t frame_count;
        bool style_off;

        Parameter( std::uint64_t quota,
                   std::uint64_t completed,
                   std::chrono::steady_clock::time_point start_time,
                   std::uint32_t frames,
                   bool sty_off ) noexcept
          : task_quota { quota }
          , tasks_completed { completed }
          , elapsed_time { std::chrono::duration_cast<types::Tempus>( std::chrono::steady_clock::now()
                                                                      - start_time ) }
          , frame_count { frames }
          , style_off { sty_off }
        {
          PACE__TRUST( tasks_completed <= task_quota );
          if ( task_quota > 0 ) {
            progress_ratio = static_cast<types::Float>( tasks_completed ) / task_quota;
          }
          PACE__TRUST( progress_ratio >= 0.0 );
          PACE__TRUST( progress_ratio <= 1.0 );
        }
        Parameter( std::uint64_t quota,
                   std::uint64_t completed,
                   std::chrono::steady_clock::time_point start_time,
                   bool sty_off ) noexcept
          : Parameter( quota, completed, start_time, 0, sty_off )
        {}
      };
    } // namespace render
  } // namespace details
} // namespace pace

#endif
