#ifndef PACE_TEMPORAL
#define PACE_TEMPORAL

#include "../core/Core.hpp"
#include <chrono>

namespace pace {
  namespace details {
    namespace behaviors {
      template<typename Base, typename Derived>
      class Temporal : public Base {
      protected:
        std::chrono::steady_clock::time_point zero_point_;

        constexpr Temporal()                                    = default;
        constexpr Temporal( Temporal&& )                        = default;
        PACE__CXX14_CNSTXPR Temporal& operator=( Temporal&& ) & = default;
        PACE__CXX23_CNSTXPR ~Temporal()                         = default;

      public:
        using Base::Base;
      };
    } // namespace behaviors
  } // namespace details
} // namespace pace

#endif
