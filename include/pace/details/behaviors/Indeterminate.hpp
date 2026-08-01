#ifndef PACE_INDETERMINATE
#define PACE_INDETERMINATE

#include "Incremental.hpp"
#include "Reactive.hpp"
#include "Renderable.hpp"
#include "Temporal.hpp"

namespace pace {
  namespace details {
    namespace behaviors {
      template<typename Base, typename Derived>
      class Indeterminate : public Base {
        template<typename, typename>
        friend class Renderable;

        void on_awaken()
        {
          this->task_end_ = this->config_.quota();
          this->task_cnt_.store( 0, std::memory_order_relaxed );
          this->zero_point_ = std::chrono::steady_clock::now();
        }

        PACE__NODISCARD PACE__FORCEINLINE bool test_completion() const noexcept
        {
          return this->task_end_ > 0 && this->task_cnt_.load( std::memory_order_relaxed ) >= this->task_end_;
        }

      protected:
        constexpr Indeterminate()                                         = default;
        constexpr Indeterminate( Indeterminate&& )                        = default;
        PACE__CXX14_CNSTXPR Indeterminate& operator=( Indeterminate&& ) & = default;
        PACE__CXX23_CNSTXPR ~Indeterminate()                              = default;

      public:
        using Base::Base;
      };
    } // namespace behaviors

    // The type Renderable must be the top base class.
    PACE__INHERIT_REGISTER( behaviors::Indeterminate,
                            behaviors::Reactive,
                            behaviors::Incremental,
                            behaviors::Temporal,
                            behaviors::Renderable );
  } // namespace details
} // namespace pace

#endif
