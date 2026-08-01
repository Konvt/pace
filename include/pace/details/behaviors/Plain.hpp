#ifndef PACE_PLAIN
#define PACE_PLAIN

#include "Incremental.hpp"
#include "Reactive.hpp"
#include "Renderable.hpp"
#include "Temporal.hpp"

namespace pace {
  namespace details {
    namespace behaviors {
      template<typename Base, typename Derived>
      class Plain : public Base {
        template<typename, typename>
        friend class Renderable;

        PACE__FORCEINLINE void prologue( io::CharPipeline& pipeline, bool style_off ) &
        {
          monologue( pipeline, style_off );
          auto expected = Base::Phase::Awake;
          this->state_.compare_exchange_strong( expected, Base::Phase::Refresh, std::memory_order_release );
        }
        PACE__FORCEINLINE void monologue( io::CharPipeline& pipeline, bool style_off ) &
        {
          PACE__ASSERT( this->task_cnt_ <= this->task_end_ );
          this->config_.build( pipeline,
                               render::Parameter( this->task_end_,
                                                  this->task_cnt_.load( std::memory_order_relaxed ),
                                                  this->zero_point_,
                                                  style_off ) );
        }
        PACE__FORCEINLINE void epilogue( io::CharPipeline& pipeline, bool style_off ) &
        {
          monologue( pipeline, style_off );
          this->state_.store( Base::Phase::Stop, std::memory_order_release );
        }

      protected:
        constexpr Plain()                                 = default;
        constexpr Plain( Plain&& rhs )                    = default;
        PACE__CXX14_CNSTXPR Plain& operator=( Plain&& ) & = default;
        PACE__CXX23_CNSTXPR ~Plain()                      = default;

      public:
        using Base::Base;
      };
    } // namespace behaviors

    // The type Renderable must be the top base class.
    PACE__INHERIT_REGISTER( behaviors::Plain,
                            behaviors::Reactive,
                            behaviors::Incremental,
                            behaviors::Temporal,
                            behaviors::Renderable );
  } // namespace details
} // namespace pace

#endif
