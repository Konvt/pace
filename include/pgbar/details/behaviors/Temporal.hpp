#ifndef PGBAR_TEMPORAL
#define PGBAR_TEMPORAL

#include "../core/Core.hpp"
#include <chrono>

namespace pgbar {
  namespace _details {
    namespace behaviors {
      template<typename Base, typename Derived>
      class Temporal : public Base {
      protected:
        std::chrono::steady_clock::time_point zero_point_;

        constexpr Temporal() = default;
        constexpr Temporal( Temporal&& rhs ) noexcept : Base( std::move( rhs ) ) {}
        PGBAR__CXX14_CNSTXPR Temporal& operator=( Temporal&& rhs ) & noexcept
        {
          Base::operator=( std::move( rhs ) );
          return *this;
        }
        PGBAR__CXX23_CNSTXPR ~Temporal() = default;

      public:
        using Base::Base;
      };
    } // namespace behaviors
  } // namespace _details
} // namespace pgbar

#endif
