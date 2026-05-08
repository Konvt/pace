#ifndef PGBAR_MOTIONAL
#define PGBAR_MOTIONAL

#include "../core/Core.hpp"
#include <utility>

namespace pgbar {
  namespace details {
    namespace behaviors {
      template<typename Base, typename Derived>
      class Motional : public Base {
      protected:
        std::uint32_t frame_cnt_;

        constexpr Motional() = default;
        constexpr Motional( Motional&& rhs ) noexcept : Base( std::move( rhs ) ) {}
        PGBAR__CXX14_CNSTXPR Motional& operator=( Motional&& rhs ) & noexcept
        {
          Base::operator=( std::move( rhs ) );
          return *this;
        }
        PGBAR__CXX23_CNSTXPR ~Motional() = default;

      public:
        using Base::Base;
      };
    } // namespace behaviors
  } // namespace details
} // namespace pgbar

#endif
