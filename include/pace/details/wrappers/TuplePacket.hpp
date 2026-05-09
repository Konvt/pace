#ifndef PACE_TUPLE_PACKET
#define PACE_TUPLE_PACKET

#include "../core/Core.hpp"
#include "../types/Types.hpp"

namespace pace {
  namespace details {
    namespace wrappers {
      template<typename Base, types::Size>
      struct TuplePacket : public Base {
        using Base::Base;
        PACE__CXX23_CNSTXPR TuplePacket( TuplePacket&& )              = default;
        PACE__CXX14_CNSTXPR TuplePacket& operator=( TuplePacket&& ) & = default;
        constexpr TuplePacket( Base&& rhs ) noexcept : Base( std::move( rhs ) ) {}
        PACE__CXX14_CNSTXPR TuplePacket& operator=( Base&& rhs ) & noexcept
        {
          PACE__ASSERT( this != std::addressof( rhs ) );
          Base::operator=( std::move( rhs ) );
          return *this;
        }
      };
    } // namespace wrappers
  } // namespace details
} // namespace pace

#endif
