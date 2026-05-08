#ifndef PGBAR_TUPLE_PACKET
#define PGBAR_TUPLE_PACKET

#include "../core/Core.hpp"
#include "../types/Types.hpp"

namespace pgbar {
  namespace details {
    namespace wrappers {
      template<typename Base, types::Size>
      struct TuplePacket : public Base {
        using Base::Base;
        PGBAR__CXX23_CNSTXPR TuplePacket( TuplePacket&& )              = default;
        PGBAR__CXX14_CNSTXPR TuplePacket& operator=( TuplePacket&& ) & = default;
        constexpr TuplePacket( Base&& rhs ) noexcept : Base( std::move( rhs ) ) {}
        PGBAR__CXX14_CNSTXPR TuplePacket& operator=( Base&& rhs ) & noexcept
        {
          PGBAR__ASSERT( this != std::addressof( rhs ) );
          Base::operator=( std::move( rhs ) );
          return *this;
        }
      };
    } // namespace wrappers
  } // namespace details
} // namespace pgbar

#endif
