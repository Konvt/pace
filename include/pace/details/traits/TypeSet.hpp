#ifndef PACE_TYPE_SET
#define PACE_TYPE_SET

#include "Algorithm.hpp"
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
      struct TpPrepend<TypeSet<Es...>, T> {
      private:
        template<bool Cond, typename NewOne>
        struct Choice {
          using type = TypeSet<Es...>;
        };
        template<typename NewOne>
        struct Choice<false, NewOne> {
          using type = TypeSet<NewOne, Es...>;
        };

      public:
        using type = typename Choice<TpContains<TypeSet<Es...>, T>::value, T>::type;
      };

      template<typename... Es, typename T>
      struct TpAppend<TypeSet<Es...>, T> {
      private:
        template<bool Cond, typename NewOne>
        struct Choice {
          using type = TypeSet<Es...>;
        };
        template<typename NewOne>
        struct Choice<false, NewOne> {
          using type = TypeSet<Es..., NewOne>;
        };

      public:
        using type = typename Choice<TpContains<TypeSet<Es...>, T>::value, T>::type;
      };

      template<typename Element>
      struct TpErase<TypeSet<>, Element> {
        using type = TypeSet<>;
      };
      template<typename... Tail, typename Element>
      struct TpErase<TypeSet<Element, Tail...>, Element> {
        using type = TypeSet<Tail...>;
      };
      template<typename... Es, typename Element>
      struct TpErase<TypeSet<Es...>, Element> {
      private:
        template<bool Cond, typename List>
        struct Choice : TpErase<List, Element> {};
        template<typename List>
        struct Choice<false, List> {
          using type = List;
        };

        template<typename Front, typename Back>
        struct Helper;
        template<typename... Ts, typename... Us>
        struct Helper<TypeSet<Ts...>, TypeSet<Us...>> {
          using type = TypeSet<Ts..., Us...>;
        };

        using Left  = TpSplit_l<TypeSet<Es...>>;
        using Right = TpSplit_r<TypeSet<Es...>>;

      public:
        using type = typename Helper<typename Choice<TpContains<Left, Element>::value, Left>::type,
                                     typename Choice<!TpContains<Left, Element>::value, Right>::type>::type;
      };

      template<typename... Es, template<typename...> class Collection>
      struct Combine<TypeSet<Es...>, Collection<>> {
        using type = TypeSet<Es...>;
      };
#if PACE__FAST_TYPEAT
      template<typename... Es, template<typename...> class Collection, typename T>
      struct Combine<TypeSet<Es...>, Collection<T>> : TpAppend<TypeSet<Es...>, T> {};
      template<typename... Es, template<typename...> class Collection, typename... Ts>
      struct Combine<TypeSet<Es...>, Collection<Ts...>> {
      private:
        using Left  = TpSplit_l<TypeList<Ts...>>;
        using Right = TpSplit_r<TypeList<Ts...>>;
        using LL    = TpSplit_l<Left>;
        using LR    = TpSplit_r<Left>;
        using RL    = TpSplit_l<Right>;
        using RR    = TpSplit_r<Right>;

      public:
        using type = Combine_t<Combine_t<Combine_t<Combine_t<TypeSet<Es...>, LL>, LR>, RL>, RR>;
      };
#else
      template<typename... Es, template<typename...> class Collection, typename T, typename... Ts>
      struct Combine<TypeSet<Es...>, Collection<T, Ts...>>
        : Combine<TpAppend_t<TypeSet<Es...>, T>, Collection<Ts...>> {};
#endif

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
