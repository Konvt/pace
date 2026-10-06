#ifndef PACE_C3
#define PACE_C3

#include "HAMT.hpp"
#include "TemplateSet.hpp"
#include "TypeIdentity.hpp"
#include "TypeList.hpp"

namespace pace {
  namespace details {
    namespace traits {
      // The type used in the C3 algorithm to store template types.
      template<template<typename...> class... Ts>
      using Relation = TemplateSet<Ts...>;

      /**
       * By introducing base class templates,
       * derived classes can inherit from multiple base classes arbitrarily.
       * Using the dependency relationships between these classes,
       * we can use the C3 algorithm to linearize complex inheritance structures
       * into a single inheritance chain.

       * Since no one would write multiple non-virtual inherited duplicate base classes
       * in the case of multiple inheritance
       * (if there are any, please reconsider whether your class structure is reasonable),
       * here we only need to use the C3 algorithm to treat all inheritance relationships
       * as virtual inheritance.
       * This approach retains the benefits of multiple inheritance while avoiding its drawbacks.

       * The only trade-off is a slight increase in compilation time
       * when resolving highly complex inheritance dependencies.
       */
      template<typename /* Relation<...> */ VBs>
      struct C3;
      template<template<typename...> class VB, template<typename...> class... VBs>
      using C3_t = typename C3<Relation<VB, VBs...>>::type;

      // The structure that records the inheritance order of Node includes itself,
      // just like Python's MRO.
      // The return value of this config-function will serve as both the return value
      // and the entry parameter of C3.
      template<template<typename...> class Node>
      struct InheritOrder : TypeIdentity<Relation<Node>> {};
      // Gets the inheritance order of the template class `Node`.
      template<template<typename...> class Node>
      using InheritOrder_t = typename InheritOrder<Node>::type;

// A helper macro to register the inheritance structure of a template class.
#define PACE__INHERIT_REGISTER( Node, ... )        \
  template<>                                       \
  struct pace::details::traits::InheritOrder<Node> \
    : pace::details::traits::TypeIdentity<         \
        pace::details::traits::TmpPushFront_t<pace::details::traits::C3_t<__VA_ARGS__>, Node>> {}

      // The implementation of the "merge" function in the C3 algorithm.
      template<typename /* Relation<...> */... VBLists>
      struct C3Merge {
      private:
        template<std::size_t N>
        using TailCount = std::integral_constant<std::size_t, N>;

        template<typename /* HAMT */ Counter,
                 typename /* TemplateSet<...> */ Visited,
                 typename /* Relation<...> */... VBs>
        struct CollectCounter {
          using tail_counts = Counter;
          using vb_list     = Visited;
        };
        template<typename /* HAMT */ Counter,
                 typename /* TemplateSet<...> */ Visited,
                 template<typename...> class Base,
                 template<typename...> class... Bases,
                 typename /* Relation<...> */... VBs>
        struct CollectCounter<Counter, Visited, Relation<Base, Bases...>, VBs...> {
        private:
          template<typename /* HAMT */ Tree,
                   template<typename...> class K,
                   typename /* TailCount */ Entry,
                   std::size_t Inc>
          struct FetchAdd : HAMTInsert<Tree, TemplateList<K>, TailCount<Entry::value + Inc>> {};
          template<typename /* HAMT */ Tree, template<typename...> class K, std::size_t Inc>
          struct FetchAdd<Tree, K, HAMTNil, Inc> : HAMTInsert<Tree, TemplateList<K>, TailCount<Inc>> {};

          template<typename /* HAMT */ Tree,
                   typename /* TemplateSet<...> */ Marker,
                   typename /* Relation<...> */ List>
          struct Helper {
            using tail_counts = Tree;
            using vb_list     = Marker;
          };
          template<typename /* HAMT */ Tree,
                   typename /* TemplateSet<...> */ Marker,
                   template<typename...> class V,
                   template<typename...> class... Vs>
          struct Helper<Tree, Marker, Relation<V, Vs...>>
            : Helper<typename FetchAdd<Tree, V, HAMTFind_t<Tree, TemplateList<V>>, 1>::type,
                     TmpAppend_t<Marker, V>,
                     Relation<Vs...>> {};

          using Inspected =
            Helper<typename FetchAdd<Counter, Base, HAMTFind_t<Counter, TemplateList<Base>>, 0>::type,
                   TmpAppend_t<Visited, Base>,
                   Relation<Bases...>>;
          using Result = CollectCounter<typename Inspected::tail_counts, typename Inspected::vb_list, VBs...>;

        public:
          using tail_counts = typename Result::tail_counts;
          using vb_list     = typename Result::vb_list;
        };

        //////////////////////////////////////////////////

        template<typename /* HAMT */ Index,
                 typename /* HAMT */ Counter,
                 typename /* TemplateSet<...> */ Visited>
        struct CollectIndex {
          using type = Index;
        };
        template<typename /* HAMT */ Index,
                 typename /* HAMT */ Counter,
                 template<typename...> class VB,
                 template<typename...> class... VBs>
        struct CollectIndex<Index, Counter, TemplateSet<VB, VBs...>> {
        private:
          template<typename /* TailCount */ Count, typename /* TemplateSet<...> */ Entry>
          struct FetchAdd : HAMTInsert<Index, Count, TmpAppend_t<Entry, VB>> {};
          template<typename /* TailCount */ Count>
          struct FetchAdd<Count, HAMTNil> : HAMTInsert<Index, Count, TemplateSet<VB>> {};

        public:
          using type = typename CollectIndex<
            typename FetchAdd<HAMTFind_t<Counter, TemplateList<VB>>,
                              HAMTFind_t<Index, HAMTFind_t<Counter, TemplateList<VB>>>>::type,
            Counter,
            TemplateSet<VBs...>>::type;
        };

        //////////////////////////////////////////////////

        template<typename /* HAMT */ Index, typename /* HAMTNil */ Candidate>
        struct TakeFeasible {
          static_assert( std::is_same<Candidate, HAMTNil>::value == false,
                         "invalid C3 linearization: no feasible candidate; "
                         "the inheritance order may be inconsistent or cyclic" );
        };
        template<typename /* HAMT */ Index,
                 template<typename...> class VB,
                 template<typename...> class... VBs>
        struct TakeFeasible<Index, TemplateSet<VB, VBs...>> {
          using feasible_type = TemplateList<VB>;
          using index_type    = HAMTInsert_t<Index, TailCount<0>, TemplateSet<VBs...>>;
        };

        //////////////////////////////////////////////////

        template<typename /* TemplateSet<...> */ NewCandidate,
                 typename /* HAMT */ Counter,
                 typename Feasible,
                 typename MergedLists>
        struct DropFeasible;
        template<typename /* TemplateSet<...> */ NewCandidate,
                 typename /* HAMT */ Counter,
                 template<typename...> class Feasible,
                 typename... MergedLists>
        struct DropFeasible<NewCandidate,
                            Counter,
                            TemplateList<Feasible>,
                            TypeList<Relation<Feasible>, MergedLists...>>
          : DropFeasible<NewCandidate, Counter, TemplateList<Feasible>, TypeList<MergedLists...>> {};
        template<typename /* TemplateSet<...> */ NewCandidate,
                 typename /* HAMT */ Counter,
                 template<typename...> class Feasible>
        struct DropFeasible<NewCandidate, Counter, TemplateList<Feasible>, TypeList<>> {
          using candidate_list = NewCandidate;
          using tail_counts    = Counter;
          using merged_list    = TypeList<>;
        };
        template<typename /* TemplateSet<...> */ NewCandidate,
                 typename /* HAMT */ Counter,
                 template<typename...> class Feasible,
                 template<typename...> class... Rests,
                 typename... MergedLists>
        struct DropFeasible<NewCandidate,
                            Counter,
                            TemplateList<Feasible>,
                            TypeList<Relation<Rests...>, MergedLists...>> {
        private:
          using Result =
            DropFeasible<NewCandidate, Counter, TemplateList<Feasible>, TypeList<MergedLists...>>;

        public:
          using candidate_list = typename Result::candidate_list;
          using tail_counts    = typename Result::tail_counts;
          using merged_list    = TpPrepend_t<typename Result::merged_list, Relation<Rests...>>;
        };
        template<typename /* TemplateSet<...> */ NewCandidate,
                 typename /* HAMT */ Counter,
                 template<typename...> class Feasible,
                 template<typename...> class NextHead,
                 template<typename...> class... Rests,
                 typename... MergedLists>
        struct DropFeasible<NewCandidate,
                            Counter,
                            TemplateList<Feasible>,
                            TypeList<Relation<Feasible, NextHead, Rests...>, MergedLists...>> {
        private:
          template<typename /* HAMTNil */ Entry>
          struct FetchSub {
            static_assert( sizeof( Entry ) != sizeof( Entry ), "invalid C3 linearization: target type lost" );
          };
          template<std::size_t N>
          struct FetchSub<TailCount<N>> {
            using type = TailCount<N - 1>;
          };

          using Result =
            DropFeasible<TmpAppend_t<NewCandidate, NextHead>,
                         HAMTInsert_t<Counter,
                                      TemplateList<NextHead>,
                                      typename FetchSub<HAMTFind_t<Counter, TemplateList<NextHead>>>::type>,
                         TemplateList<Feasible>,
                         TypeList<MergedLists...>>;

        public:
          using candidate_list = typename Result::candidate_list;
          using tail_counts    = typename Result::tail_counts;
          using merged_list    = TpPrepend_t<typename Result::merged_list, Relation<NextHead, Rests...>>;
        };

        //////////////////////////////////////////////////

        template<typename /* HAMT */ Index,
                 typename /* HAMT */ OldCounter,
                 typename /* HAMT */ NewCounter,
                 typename /* TemplateSet<...> */ Visited>
        struct UpdateIndex {
          using type = Index;
        };
        template<typename /* HAMT */ Index,
                 typename /* HAMT */ OldCounter,
                 typename /* HAMT */ NewCounter,
                 template<typename...> class Base,
                 template<typename...> class... Bases>
        struct UpdateIndex<Index, OldCounter, NewCounter, TemplateSet<Base, Bases...>> {
        private:
          template<typename /* HAMTNil */ Result, template<typename...> class T>
          struct FetchAppend {
            using type = TemplateSet<T>;
          };
          template<template<typename...> class... Es, template<typename...> class T>
          struct FetchAppend<TemplateSet<Es...>, T> {
            using type = TemplateSet<Es..., T>;
          };

          using OldCount = HAMTFind_t<OldCounter, TemplateList<Base>>;
          using NewCount = HAMTFind_t<NewCounter, TemplateList<Base>>;
          using Erased   = HAMTInsert_t<Index, OldCount, TmpErase_t<HAMTFind_t<Index, OldCount>, Base>>;

        public:
          using type = typename UpdateIndex<
            HAMTInsert_t<Erased, NewCount, typename FetchAppend<HAMTFind_t<Erased, NewCount>, Base>::type>,
            OldCounter,
            NewCounter,
            TemplateSet<Bases...>>::type;
        };

        template<typename /* Relation */ Sorted,
                 typename /* HAMT */ Index,
                 typename /* HAMT */ Counter,
                 typename /* TypeList<...> */ MergedLists>
        struct MakeMRO;
        template<typename Sorted, typename /* HAMT */ Index, typename /* HAMT */ Counter>
        struct MakeMRO<Sorted, Index, Counter, TypeList<>> {
          using type = Sorted;
        };
        template<typename /* Relation */ Sorted,
                 typename /* HAMT */ Index,
                 typename /* HAMT */ Counter,
                 typename... MergedLists>
        struct MakeMRO<Sorted, Index, Counter, TypeList<MergedLists...>> {
        private:
          using Feasible = TakeFeasible<Index, HAMTFind_t<Index, TailCount<0>>>;
          using Dropped =
            DropFeasible<TemplateSet<>, Counter, typename Feasible::feasible_type, TypeList<MergedLists...>>;
          using NewIndex = typename UpdateIndex<typename Feasible::index_type,
                                                Counter,
                                                typename Dropped::tail_counts,
                                                typename Dropped::candidate_list>::type;

        public:
          using type = typename MakeMRO<Concat_t<Sorted, typename Feasible::feasible_type>,
                                        NewIndex,
                                        typename Dropped::tail_counts,
                                        typename Dropped::merged_list>::type;
        };

        using Mapping = CollectCounter<HAMT, TemplateSet<>, VBLists...>;

      public:
        using type = typename MakeMRO<
          Relation<>,
          typename CollectIndex<HAMT, typename Mapping::tail_counts, typename Mapping::vb_list>::type,
          typename Mapping::tail_counts,
          TypeList<VBLists...>>::type;
      };
      template<typename /* Relation<...> */... VBLists>
      using C3Merge_t = typename C3Merge<VBLists...>::type;

      template<template<typename...> class VB, template<typename...> class... VBs>
      struct C3<Relation<VB, VBs...>>
        : C3Merge<InheritOrder_t<VB>, InheritOrder_t<VBs>..., Relation<VB, VBs...>> {};

      /**
       * Linearization of Inheritance.
       *
       * Linearize the input types using the C3 algorithm,
       * then iterate through the resulting sorted list and fill in their template types.

       * It relies on the template `InheritOrder` and `c3` classes to work.
       */
      template<typename /* Relation<...> */ VBs>
      struct LI {
      private:
        template<typename Linearized, typename RBC, typename... Args>
        struct Helper;
        template<typename Linearized, typename RBC, typename... Args>
        using Helper_t = typename Helper<Linearized, RBC, Args...>::type;

        template<typename RBC, typename... Args>
        struct Helper<Relation<>, RBC, Args...> {
          using type = RBC;
        };
        template<template<typename...> class Head,
                 template<typename...> class... Tail,
                 typename RBC,
                 typename... Args>
        struct Helper<Relation<Head, Tail...>, RBC, Args...> {
          using type = Head<Helper_t<Relation<Tail...>, RBC, Args...>, Args...>;
        };

      public:
        // RBC: Root Base Class.
        template<typename RBC, typename... Args>
        using type = Helper_t<typename C3<VBs>::type, RBC, Args...>;
      };

      template<template<typename...> class VB, template<typename...> class... VBs>
      struct LI_t {
        template<typename RBC, typename... Args>
        using type = typename LI<Relation<VB, VBs...>>::template type<RBC, Args...>;
      };

      template<typename /* class<...> */ Linearized, template<typename...> class Target>
      struct BaseOf;
      template<typename /* class<...> */ Linearized, template<typename...> class Target>
      using BaseOf_t = typename BaseOf<Linearized, Target>::type;

      template<template<typename...> class Target, typename Base, typename... Rest>
      struct BaseOf<Target<Base, Rest...>, Target> {
        using type = Target<Base, Rest...>;
      };
      template<template<typename...> class Linearized,
               typename Base,
               typename... Rest,
               template<typename...> class Target>
      struct BaseOf<Linearized<Base, Rest...>, Target> : BaseOf<Base, Target> {};
    } // namespace traits
  } // namespace details
} // namespace pace

#endif
