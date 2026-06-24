#ifndef PACE_TYPE_SET
#define PACE_TYPE_SET

#include "Algorithm.hpp"
#include "Identity.hpp"
#include "TypeList.hpp"

namespace pace {
  namespace details {
    namespace traits {
      /**
       * A TypeList without duplicates;
       * the presence of duplicate elements will result in a hard compile error.
       */
      template<typename... Ts>
      struct TypeSet : TypeList<Ts>... {};

      template<typename... Es, typename T>
      struct contains_tp<TypeSet<Es...>, T> : std::is_base_of<TypeList<T>, TypeSet<Es...>> {};

      template<typename... Es, typename T>
      struct append_tp<TypeSet<Es...>, T> {
      private:
        template<bool Cond, typename NewOne>
        struct select : Identity<TypeSet<Es...>> {};
        template<typename NewOne>
        struct select<false, NewOne> : Identity<TypeSet<Es..., NewOne>> {};

      public:
        using type = typename select<contains_tp<TypeSet<Es...>, T>::value, T>::type;
      };

      template<typename Element>
      struct remove_tp<TypeSet<>, Element> : Identity<TypeSet<>> {};
      template<typename... Tail, typename Element>
      struct remove_tp<TypeSet<Element, Tail...>, Element> : Identity<TypeSet<Tail...>> {};
#if PACE__FAST_TYPEAT
      template<typename... Es, typename Element>
      struct remove_tp<TypeSet<Es...>, Element> {
      private:
        template<typename Removed, typename Another>
        struct helper;
        template<typename... Head, typename... Tail>
        struct helper<TypeSet<Head...>, TypeSet<Tail...>> : Identity<TypeSet<Head..., Tail...>> {};

        using Left  = split_l<TypeSet<Es...>>;
        using Right = split_r<TypeSet<Es...>>;

      public:
        using type = typename helper<
          remove_tp_t<typename std::conditional<contains_tp<Left, Element>::value, Left, Right>::type,
                      Element>,
          typename std::conditional<contains_tp<Left, Element>::value, Right, Left>::type>::type;
      };
#else
      template<typename Head, typename... Tail, typename Element>
      struct remove_tp<TypeSet<Head, Tail...>, Element>
        : Identity<prepend_tp_t<remove_tp_t<TypeSet<Tail...>, Element>, Head>> {};
#endif

      template<typename... Es, template<typename...> class Collection>
      struct combine<TypeSet<Es...>, Collection<>> : Identity<TypeSet<Es...>> {};
      template<typename... Es, template<typename...> class Collection, typename T, typename... Ts>
      struct combine<TypeSet<Es...>, Collection<T, Ts...>>
        : combine<append_tp_t<TypeSet<Es...>, T>, Collection<Ts...>> {};

      template<bool Cond, typename Visited, typename... Elements>
      struct _impl_is_unique_tp : std::false_type {};
      template<typename Visited>
      struct _impl_is_unique_tp<false, Visited> : std::true_type {};
      template<typename Visited, typename U, typename... Us>
      struct _impl_is_unique_tp<false, Visited, U, Us...>
        : _impl_is_unique_tp<contains_tp<Visited, U>::value, append_tp_t<Visited, U>, Us...> {};
      template<typename... Elements>
      struct is_unique<TypeList<Elements...>> : _impl_is_unique_tp<false, TypeSet<>, Elements...> {};
    } // namespace traits
  } // namespace details
} // namespace pace

#endif
