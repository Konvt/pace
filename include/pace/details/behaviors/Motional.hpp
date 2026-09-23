#ifndef PACE_MOTIONAL
#define PACE_MOTIONAL

#include <cstdint>

namespace pace {
  namespace details {
    namespace behaviors {
      template<typename Base, typename Derived>
      class Motional : public Base {
      protected:
        std::uint32_t frame_cnt_;

        Motional()                          = default;
        Motional( Motional&& )              = default;
        Motional& operator=( Motional&& ) & = default;
        ~Motional()                         = default;

      public:
        using Base::Base;
      };
    } // namespace behaviors
  } // namespace details
} // namespace pace

#endif
