#ifndef PACE_TYPE_LIST
#define PACE_TYPE_LIST

#include "Algorithm.hpp"
#include "Backport.hpp"
#include "Identity.hpp"

namespace pace {
  namespace details {
    namespace traits {
      /**
       * A lightweight tuple type that stores multiple types.
       *
       * `std::tuple` puts some constraints on the input type that are not metaprogramming related,
       * so here is a lightweight tuple type that is used only for template type parameter passing.
       */
      template<typename... Ts>
      struct TypeList {};

      template<typename... Es, typename Element>
      struct TpPrepend<TypeList<Es...>, Element> : Identity<TypeList<Element, Es...>> {};

      template<typename... Es, typename Element>
      struct TpAppend<TypeList<Es...>, Element> : Identity<TypeList<Es..., Element>> {};

      template<typename Element>
      struct TpRemove<TypeList<>, Element> : Identity<TypeList<>> {};
      template<typename Head, typename... Tail, typename Element>
      struct TpRemove<TypeList<Head, Tail...>, Element>
#if PACE__FAST_TYPEAT
        : Combine<TpRemove_t<TpSplit_l<TypeList<Head, Tail...>>, Element>,
                  TpRemove_t<TpSplit_r<TypeList<Head, Tail...>>, Element>>
#else
        : Identity<TpPrepend_t<TpRemove_t<TypeList<Tail...>, Element>, Head>>
#endif
      {
      };

      template<typename... Es, template<typename...> class Collection, typename... Ts>
      struct Combine<TypeList<Es...>, Collection<Ts...>> : Identity<TypeList<Es..., Ts...>> {};

      template<typename Element, std::size_t N>
      struct TpFill {
      private:
        template<bool Cond, typename List>
        struct Choice : Identity<List> {};
        template<typename List>
        struct Choice<false, List> : TpAppend<List, Element> {};

        using Half = typename TpFill<Element, N / 2>::type;

      public:
        using type = typename Choice<( N % 2 == 0 ), Combine_t<Half, Half>>::type;
      };
      template<typename Element, std::size_t N>
      using TpFill_t = typename TpFill<Element, N>::type;

      template<typename Element>
      struct TpFill<Element, 0> : Identity<TypeList<>> {};
      template<typename Element>
      struct TpFill<Element, 1> : Identity<TypeList<Element>> {};

      template<typename... Ts>
      struct TpStartsWith<TypeList<>, Ts...> : std::true_type {};
      template<typename Head, typename... Tail>
      struct TpStartsWith<TypeList<Head, Tail...>> : std::false_type {};
      template<typename Head, typename... Tail, typename T, typename... Ts>
      struct TpStartsWith<TypeList<Head, Tail...>, T, Ts...>
        : AllOf<std::is_same<Head, T>, TpStartsWith<TypeList<Tail...>, Ts...>> {};
    } // namespace traits
  } // namespace details
} // namespace pace

#endif
