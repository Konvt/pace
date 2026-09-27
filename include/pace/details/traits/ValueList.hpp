#ifndef PACE_VALUE_LIST
#define PACE_VALUE_LIST

#include "Identity.hpp"

namespace pace {
  namespace details {
    namespace traits {
      template<typename T, T... Vals>
      struct ValueList {};
      // C++11 does not support auto NTTP, so we have to define a ValueList with fixed type.

      template<std::size_t... Nats>
      using NaturalList = ValueList<std::size_t, Nats...>;

      template<typename Nats, std::size_t N>
      struct NatPrepend;
      template<typename Nats, std::size_t N>
      using NatPrepend_t = typename NatPrepend<Nats, N>::type;

      template<std::size_t... Ns, std::size_t N>
      struct NatPrepend<NaturalList<Ns...>, N> : Identity<NaturalList<N, Ns...>> {};

      template<typename Nats, std::size_t N>
      struct NatAppend;
      template<typename Nats, std::size_t N>
      using NatAppend_t = typename NatAppend<Nats, N>::type;

      template<std::size_t... Ns, std::size_t N>
      struct NatAppend<NaturalList<Ns...>, N> : Identity<NaturalList<Ns..., N>> {};
    } // namespace traits
  } // namespace details
} // namespace pace

#endif
