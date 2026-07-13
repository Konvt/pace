#ifndef PACE_PACKAGED_BAR
#define PACE_PACKAGED_BAR

#include "../../prefab/BasicBar.hpp"

namespace pace {
  namespace details {
    namespace assets {
      template<typename C, Channel S, Policy M, Region Z, types::Size>
      class PackagedBar : public prefab::BasicBar<C, S, M, Z> {
        using Base = prefab::BasicBar<C, S, M, Z>;

      public:
        using Base::Base;

        constexpr PackagedBar()                                       = default;
        constexpr PackagedBar( PackagedBar&& )                        = default;
        PACE__CXX14_CNSTXPR PackagedBar& operator=( PackagedBar&& ) & = default;

        constexpr PackagedBar( Base&& rhs ) noexcept : Base( std::move( rhs ) ) {}
        PACE__CXX14_CNSTXPR PackagedBar& operator=( Base&& rhs ) & noexcept
        {
          PACE__ASSERT( this != std::addressof( rhs ) );
          Base::operator=( std::move( rhs ) );
          return *this;
        }

        using Base::draw;
      };
    } // namespace assets
  } // namespace details
} // namespace pace

#endif
