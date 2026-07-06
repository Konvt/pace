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
      struct TpContains<TypeSet<Es...>, T> : std::is_base_of<TypeList<T>, TypeSet<Es...>> {};

      template<typename... Es, typename T>
      struct TpAppend<TypeSet<Es...>, T> {
      private:
        template<bool Cond, typename NewOne>
        struct Choice : Identity<TypeSet<Es...>> {};
        template<typename NewOne>
        struct Choice<false, NewOne> : Identity<TypeSet<Es..., NewOne>> {};

      public:
        using type = typename Choice<TpContains<TypeSet<Es...>, T>::value, T>::type;
      };

      template<typename Element>
      struct TpRemove<TypeSet<>, Element> : Identity<TypeSet<>> {};
      template<typename... Tail, typename Element>
      struct TpRemove<TypeSet<Element, Tail...>, Element> : Identity<TypeSet<Tail...>> {};
#if PACE__FAST_TYPEAT
      template<typename... Es, typename Element>
      struct TpRemove<TypeSet<Es...>, Element> {
      private:
        template<typename Removed, typename Another>
        struct Helper;
        template<typename... Head, typename... Tail>
        struct Helper<TypeSet<Head...>, TypeSet<Tail...>> : Identity<TypeSet<Head..., Tail...>> {};

        using Left  = Split_l<TypeSet<Es...>>;
        using Right = Split_r<TypeSet<Es...>>;

      public:
        using type = typename Helper<
          TpRemove_t<typename std::conditional<TpContains<Left, Element>::value, Left, Right>::type, Element>,
          typename std::conditional<TpContains<Left, Element>::value, Right, Left>::type>::type;
      };
#else
      template<typename Head, typename... Tail, typename Element>
      struct TpRemove<TypeSet<Head, Tail...>, Element>
        : Identity<TpPrepend_t<TpRemove_t<TypeSet<Tail...>, Element>, Head>> {};
#endif

      template<typename... Es, template<typename...> class Collection>
      struct Combine<TypeSet<Es...>, Collection<>> : Identity<TypeSet<Es...>> {};
      template<typename... Es, template<typename...> class Collection, typename T, typename... Ts>
      struct Combine<TypeSet<Es...>, Collection<T, Ts...>>
        : Combine<TpAppend_t<TypeSet<Es...>, T>, Collection<Ts...>> {};

      template<bool Cond, typename Visited, typename... Elements>
      struct _impl_is_unique_tp : std::false_type {};
      template<typename Visited>
      struct _impl_is_unique_tp<false, Visited> : std::true_type {};
      template<typename Visited, typename U, typename... Us>
      struct _impl_is_unique_tp<false, Visited, U, Us...>
        : _impl_is_unique_tp<TpContains<Visited, U>::value, TpAppend_t<Visited, U>, Us...> {};
      template<typename... Elements>
      struct is_unique<TypeList<Elements...>> : _impl_is_unique_tp<false, TypeSet<>, Elements...> {};
    } // namespace traits
  } // namespace details
} // namespace pace

#endif
