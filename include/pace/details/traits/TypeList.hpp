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
      template<typename... Tail, typename Element>
      struct TpRemove<TypeList<Element, Tail...>, Element>
        : Identity<TpRemove_t<TypeList<Tail...>, Element>> {};
#if PACE__FAST_TYPEAT
      template<typename Head, typename... Tail, typename Element>
      struct TpRemove<TypeList<Head, Tail...>, Element>
        : Combine<TpRemove_t<Split_l<TypeList<Head, Tail...>>, Element>,
                  TpRemove_t<Split_r<TypeList<Head, Tail...>>, Element>> {};
#else
      template<typename Head, typename... Tail, typename Element>
      struct TpRemove<TypeList<Head, Tail...>, Element>
        : Identity<TpPrepend_t<TpRemove_t<TypeList<Tail...>, Element>, Head>> {};
#endif

      template<typename... Es, template<typename...> class Collection>
      struct Combine<TypeList<Es...>, Collection<>> : Identity<TypeList<Es...>> {};
      template<typename... Es, template<typename...> class Collection, typename T, typename... Ts>
      struct Combine<TypeList<Es...>, Collection<T, Ts...>>
        : Combine<TpAppend_t<TypeList<Es...>, T>, Collection<Ts...>> {};

      template<typename Element, types::Size N>
      struct TpFill {
      private:
        template<bool Cond, typename List>
        struct _select;
        template<typename List>
        struct _select<false, List> : TpAppend<List, Element> {};
        template<typename List>
        struct _select<true, List> : Identity<List> {};
        using Half_t = typename TpFill<Element, N / 2>::type;

      public:
        using type = typename _select<( N % 2 == 0 ), Combine_t<Half_t, Half_t>>::type;
      };
      template<typename Element>
      struct TpFill<Element, 0> : Identity<TypeList<>> {};
      template<typename Element>
      struct TpFill<Element, 1> : Identity<TypeList<Element>> {};
      template<typename Element, types::Size N>
      using TpFill_t = typename TpFill<Element, N>::type;

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
