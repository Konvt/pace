#ifndef PGBAR_PLAIN
#define PGBAR_PLAIN

#include "Renderable.hpp"

namespace pgbar {
  namespace details {
    namespace behaviors {
      template<typename Base, typename Derived>
      class Plain;
      template<typename Base,
               template<typename, Channel, Policy, Region> class Derived,
               typename Soul,
               Channel Outlet,
               Policy Mode,
               Region Area>
      class Plain<Base, Derived<Soul, Outlet, Mode, Area>> : public Base {
        template<typename, typename>
        friend class Renderable;

        PGBAR__FORCEINLINE void prologue() &
        {
          monologue();
          auto expected = Base::Phase::Awake;
          this->state_.compare_exchange_strong( expected, Base::Phase::Refresh, std::memory_order_release );
        }
        PGBAR__FORCEINLINE void monologue() &
        {
          PGBAR__ASSERT( this->task_cnt_ <= this->task_end_ );
          this->config_.build( io::OStream<Outlet>::itself(),
                               render::Parameter( this->task_end_,
                                                  this->task_cnt_.load( std::memory_order_relaxed ),
                                                  this->zero_point_,
                                                  !config::intty( Outlet ) && config::auto_style_off() ) );
        }
        PGBAR__FORCEINLINE void epilogue() &
        {
          monologue();
          this->state_.store( Base::Phase::Stop, std::memory_order_release );
        }

      protected:
        constexpr Plain() = default;
        constexpr Plain( Plain&& rhs ) noexcept : Base( std::move( rhs ) ) {}
        PGBAR__CXX14_CNSTXPR Plain& operator=( Plain&& rhs ) & noexcept
        {
          Base::operator=( std::move( rhs ) );
          return *this;
        }
        PGBAR__CXX23_CNSTXPR ~Plain() = default;

      public:
        using Base::Base;
      };
    } // namespace behaviors

    PGBAR__INHERIT_REGISTER( behaviors::Plain, behaviors::Renderable );
  } // namespace details
} // namespace pgbar

#endif
