#ifndef PACE_FANCY
#define PACE_FANCY

#include "Motional.hpp"
#include "Plain.hpp"

namespace pace {
  namespace details {
    namespace behaviors {
      template<typename Base, typename Derived>
      class Fancy : public Base {
        template<typename, typename>
        friend class Renderable;

        PACE__FORCEINLINE void prologue( io::CharPipeline& pipeline, bool style_off ) &
        {
          this->frame_cnt_ = 0;
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
                                                  this->frame_cnt_,
                                                  style_off ) );
          ++this->frame_cnt_;
        }
        PACE__FORCEINLINE void epilogue( io::CharPipeline& pipeline, bool style_off ) &
        {
          monologue( pipeline, style_off );
          this->state_.store( Base::Phase::Stop, std::memory_order_release );
        }

      protected:
        constexpr Fancy()                                 = default;
        constexpr Fancy( Fancy&& )                        = default;
        PACE__CXX14_CNSTXPR Fancy& operator=( Fancy&& ) & = default;
        PACE__CXX23_CNSTXPR ~Fancy()                      = default;

      public:
        using Base::Base;
      };
    } // namespace behaviors

    // The Plain declaration is merely to resolve the uncertain dependency sequence,
    // and it has nothing to do with the dependency relationships among them.
    PACE__INHERIT_REGISTER( behaviors::Fancy, behaviors::Plain, behaviors::Motional );
  } // namespace details
} // namespace pace

#endif
