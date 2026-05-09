#ifndef PACE_FANCY
#define PACE_FANCY

#include "Motional.hpp"
#include "Renderable.hpp"

namespace pace {
  namespace details {
    namespace behaviors {
      template<typename Base, typename Derived>
      class Fancy;
      template<typename Base,
               template<typename, Channel, Policy, Region> class Derived,
               typename Soul,
               Channel Outlet,
               Policy Mode,
               Region Area>
      class Fancy<Base, Derived<Soul, Outlet, Mode, Area>> : public Base {
        template<typename, typename>
        friend class Renderable;

        PACE__FORCEINLINE void prologue() &
        {
          this->frame_cnt_ = 0;
          monologue();
          auto expected = Base::Phase::Awake;
          this->state_.compare_exchange_strong( expected, Base::Phase::Refresh, std::memory_order_release );
        }
        PACE__FORCEINLINE void monologue() &
        {
          PACE__ASSERT( this->task_cnt_ <= this->task_end_ );
          this->config_.build( io::OStream<Outlet>::itself(),
                               render::Parameter( this->task_end_,
                                                  this->task_cnt_.load( std::memory_order_relaxed ),
                                                  this->zero_point_,
                                                  this->frame_cnt_,
                                                  !config::intty( Outlet ) && config::auto_style_off() ) );
          ++this->frame_cnt_;
        }
        PACE__FORCEINLINE void epilogue() &
        {
          monologue();
          this->state_.store( Base::Phase::Stop, std::memory_order_release );
        }

      protected:
        constexpr Fancy() = default;
        constexpr Fancy( Fancy&& rhs ) noexcept : Base( std::move( rhs ) ) {}
        PACE__CXX14_CNSTXPR Fancy& operator=( Fancy&& rhs ) & noexcept
        {
          Base::operator=( std::move( rhs ) );
          return *this;
        }
        PACE__CXX23_CNSTXPR ~Fancy() = default;

      public:
        using Base::Base;
      };
    } // namespace behaviors

    PACE__INHERIT_REGISTER( behaviors::Fancy, behaviors::Renderable, behaviors::Motional );
  } // namespace details
} // namespace pace

#endif
