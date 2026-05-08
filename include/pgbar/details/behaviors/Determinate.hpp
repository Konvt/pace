#ifndef PGBAR_DETERMINATE
#define PGBAR_DETERMINATE

#include "Incremental.hpp"
#include "Renderable.hpp"
#include "Temporal.hpp"

namespace pgbar {
  namespace _details {
    namespace behaviors {
      template<typename Base, typename Derived>
      class Determinate : public Base {
        template<typename, typename>
        friend class Renderable;

        void on_awaken()
        {
          this->task_end_ = this->config_.quota();
          if ( this->task_end_ == 0 ) {
            PGBAR__UNLIKELY throw exception::InvalidState(
              charcodes::make_literal( "pgbar: the number of task quota is zero" ) );
          }
          this->task_cnt_.store( 0, std::memory_order_relaxed );
          this->zero_point_ = std::chrono::steady_clock::now();
        }

        PGBAR__NODISCARD PGBAR__FORCEINLINE bool test_completion() const noexcept
        {
          return this->task_cnt_.load( std::memory_order_relaxed ) >= this->task_end_;
        }

      protected:
        constexpr Determinate() = default;
        constexpr Determinate( Determinate&& rhs ) noexcept : Base( std::move( rhs ) ) {}
        PGBAR__CXX14_CNSTXPR Determinate& operator=( Determinate&& rhs ) & noexcept
        {
          Base::operator=( std::move( rhs ) );
          return *this;
        }
        PGBAR__CXX23_CNSTXPR ~Determinate() = default;

      public:
        using Base::Base;
      };
    } // namespace behaviors

    PGBAR__INHERIT_REGISTER( behaviors::Determinate,
                             behaviors::Renderable,
                             behaviors::Incremental,
                             behaviors::Temporal );
  } // namespace _details
} // namespace pgbar

#endif
