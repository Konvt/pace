#ifndef PACE_MOTIONAL
#define PACE_MOTIONAL

#include "../core/Core.hpp"

namespace pace {
  namespace details {
    namespace behaviors {
      template<typename Base, typename Derived>
      class Motional : public Base {
      protected:
        std::uint32_t frame_cnt_;

        constexpr Motional()                                    = default;
        constexpr Motional( Motional&& )                        = default;
        PACE__CXX14_CNSTXPR Motional& operator=( Motional&& ) & = default;
        PACE__CXX23_CNSTXPR ~Motional()                         = default;

      public:
        using Base::Base;
      };
    } // namespace behaviors
  } // namespace details
} // namespace pace

#endif
