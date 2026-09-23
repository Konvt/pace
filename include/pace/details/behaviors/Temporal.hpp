#ifndef PACE_TEMPORAL
#define PACE_TEMPORAL

#include <chrono>

namespace pace {
  namespace details {
    namespace behaviors {
      template<typename Base, typename Derived>
      class Temporal : public Base {
      protected:
        std::chrono::steady_clock::time_point zero_point_;

        Temporal()                          = default;
        Temporal( Temporal&& )              = default;
        Temporal& operator=( Temporal&& ) & = default;
        ~Temporal()                         = default;

      public:
        using Base::Base;
      };
    } // namespace behaviors
  } // namespace details
} // namespace pace

#endif
