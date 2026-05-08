#ifndef PGBAR_INDETERMINATE
#define PGBAR_INDETERMINATE

#include "Incremental.hpp"
#include "Renderable.hpp"
#include "Temporal.hpp"

namespace pgbar {
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

        PGBAR__NODISCARD PGBAR__FORCEINLINE bool test_completion() const noexcept
        {
          return this->task_end_ > 0 && this->task_cnt_.load( std::memory_order_relaxed ) >= this->task_end_;
        }

      protected:
        constexpr Indeterminate() = default;
        constexpr Indeterminate( Indeterminate&& rhs ) noexcept : Base( std::move( rhs ) ) {}
        PGBAR__CXX14_CNSTXPR Indeterminate& operator=( Indeterminate&& rhs ) & noexcept
        {
          Base::operator=( std::move( rhs ) );
          return *this;
        }
        PGBAR__CXX23_CNSTXPR ~Indeterminate() = default;

      public:
        using Base::Base;
      };
    } // namespace behaviors

    PGBAR__INHERIT_REGISTER( behaviors::Indeterminate,
                             behaviors::Renderable,
                             behaviors::Incremental,
                             behaviors::Temporal );
  } // namespace details
} // namespace pgbar

#endif
