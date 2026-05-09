#ifndef PACE_DETERMINATE
#define PACE_DETERMINATE

#include "Incremental.hpp"
#include "Indeterminate.hpp"
#include "Renderable.hpp"
#include "Temporal.hpp"

namespace pace {
  namespace details {
    namespace behaviors {
      template<typename Base, typename Derived>
      class Determinate : public Base {
        template<typename, typename>
        friend class Renderable;

        void on_awaken()
        {
          this->task_end_ = this->config_.quota();
          if ( this->task_end_ == 0 ) {
            PACE__UNLIKELY throw exception::InvalidState(
              charcodes::make_literal( "pace: the number of task quota is zero" ) );
          }
          this->task_cnt_.store( 0, std::memory_order_relaxed );
          this->zero_point_ = std::chrono::steady_clock::now();
        }

        PACE__NODISCARD PACE__FORCEINLINE bool test_completion() const noexcept
        {
          return this->task_cnt_.load( std::memory_order_relaxed ) >= this->task_end_;
        }

      protected:
        constexpr Determinate() = default;
        constexpr Determinate( Determinate&& rhs ) noexcept : Base( std::move( rhs ) ) {}
        PACE__CXX14_CNSTXPR Determinate& operator=( Determinate&& rhs ) & noexcept
        {
          Base::operator=( std::move( rhs ) );
          return *this;
        }
        PACE__CXX23_CNSTXPR ~Determinate() = default;

      public:
        using Base::Base;
      };
    } // namespace behaviors

    // The Indeterminate declaration is merely to resolve the uncertain dependency sequence,
    // and it has nothing to do with the dependency relationships among them.
    PACE__INHERIT_REGISTER( behaviors::Determinate,
                            behaviors::Indeterminate,
                            behaviors::Renderable,
                            behaviors::Incremental,
                            behaviors::Temporal );
  } // namespace details
} // namespace pace

#endif
