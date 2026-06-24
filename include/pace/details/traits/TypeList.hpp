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
      struct prepend_tp<TypeList<Es...>, Element> : Identity<TypeList<Element, Es...>> {};

      template<typename... Es, typename Element>
      struct append_tp<TypeList<Es...>, Element> : Identity<TypeList<Es..., Element>> {};

      template<typename Element>
      struct remove_tp<TypeList<>, Element> : Identity<TypeList<>> {};
      template<typename... Tail, typename Element>
      struct remove_tp<TypeList<Element, Tail...>, Element>
        : Identity<remove_tp_t<TypeList<Tail...>, Element>> {};
#if PACE__FAST_TYPEAT
      template<typename Head, typename... Tail, typename Element>
      struct remove_tp<TypeList<Head, Tail...>, Element>
        : combine<remove_tp_t<split_l<TypeList<Head, Tail...>>, Element>,
                  remove_tp_t<split_r<TypeList<Head, Tail...>>, Element>> {};
#else
      template<typename Head, typename... Tail, typename Element>
      struct remove_tp<TypeList<Head, Tail...>, Element>
        : Identity<prepend_tp_t<remove_tp_t<TypeList<Tail...>, Element>, Head>> {};
#endif

      template<typename... Es, template<typename...> class Collection>
      struct combine<TypeList<Es...>, Collection<>> : Identity<TypeList<Es...>> {};
      template<typename... Es, template<typename...> class Collection, typename T, typename... Ts>
      struct combine<TypeList<Es...>, Collection<T, Ts...>>
        : combine<append_tp_t<TypeList<Es...>, T>, Collection<Ts...>> {};

      template<typename Element, types::Size N>
      struct fill_tp {
      private:
        template<bool Cond, typename List>
        struct select : Identity<List> {};
        template<typename List>
        struct select<false, List> : append_tp<List, Element> {};

        using half = typename fill_tp<Element, N / 2>::type;

      public:
        using type = typename select<( N % 2 == 0 ), combine_t<half, half>>::type;
      };
      template<typename Element>
      struct fill_tp<Element, 0> : Identity<TypeList<>> {};
      template<typename Element>
      struct fill_tp<Element, 1> : Identity<TypeList<Element>> {};
      template<typename Element, types::Size N>
      using fill_tp_t = typename fill_tp<Element, N>::type;

      template<typename... Ts>
      struct starts_with_tp<TypeList<>, Ts...> : std::true_type {};
      template<typename Head, typename... Tail>
      struct starts_with_tp<TypeList<Head, Tail...>> : std::false_type {};
      template<typename Head, typename... Tail, typename T, typename... Ts>
      struct starts_with_tp<TypeList<Head, Tail...>, T, Ts...>
        : all_of<std::is_same<Head, T>, starts_with_tp<TypeList<Tail...>, Ts...>> {};
    } // namespace traits
  } // namespace details
} // namespace pace

#endif
