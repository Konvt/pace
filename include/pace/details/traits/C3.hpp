#ifndef PACE_C3
#define PACE_C3

#include "Identity.hpp"
#include "TemplateSet.hpp"
#include "ValueList.hpp"
#include "pace/details/traits/TypeList.hpp"

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
      struct InheritOrder : Identity<Relation<Node>> {};
      // Gets the inheritance order of the template class `Node`.
      template<template<typename...> class Node>
      using InheritOrder_t = typename InheritOrder<Node>::type;

// A helper macro to register the inheritance structure of a template class.
#define PACE__INHERIT_REGISTER( Node, ... )        \
  template<>                                       \
  struct pace::details::traits::InheritOrder<Node> \
    : pace::details::traits::Identity<             \
        pace::details::traits::TmpPushFront_t<pace::details::traits::C3_t<__VA_ARGS__>, Node>> {}

      // The implementation of the "merge" function in the C3 algorithm.
      template<typename... VBLists>
      struct C3Merge {
      private:
        template<typename /* Relation<...> */ VBs,
                 typename /* NaturalList<...> */ Nums,
                 template<typename...> class K,
                 std::size_t V>
        struct ExtendWith;
        template<template<typename...> class... Vs,
                 std::size_t... Cs,
                 template<typename...> class K,
                 std::size_t V>
        struct ExtendWith<Relation<Vs...>, NaturalList<Cs...>, K, V> {
        private:
          template<typename VBs, typename Counts>
          struct Helper;
          template<template<typename...> class... VBs, std::size_t Count, std::size_t... Counts>
          struct Helper<Relation<K, VBs...>, NaturalList<Count, Counts...>> {
            using candidate_list = Relation<K, VBs...>;
            using tail_counts    = NaturalList<Count + V, Counts...>;
          };
          template<template<typename...> class VB,
                   template<typename...> class... VBs,
                   std::size_t Count,
                   std::size_t... Counts>
          struct Helper<Relation<VB, VBs...>, NaturalList<Count, Counts...>> {
          private:
            using Result = Helper<Relation<VBs...>, NaturalList<Counts...>>;

          public:
            using candidate_list = TmpPushFront_t<typename Result::candidate_list, VB>;
            using tail_counts    = NatPrepend_t<typename Result::tail_counts, Count>;
          };

          template<bool Cond, typename Candidates, typename TailCounts>
          struct Choice : Helper<Candidates, TailCounts> {};
          template<typename Candidates, typename TailCounts>
          struct Choice<false, Candidates, TailCounts> {
            using candidate_list = TmpPushBack_t<Candidates, K>;
            using tail_counts    = NatAppend_t<TailCounts, V>;
          };

          using Result = Choice<TmpContains<Relation<Vs...>, K>::value, Relation<Vs...>, NaturalList<Cs...>>;

        public:
          using candidate_list = typename Result::candidate_list;
          using tail_counts    = typename Result::tail_counts;
        };

        //////////////////////////////////////////////////

        template<typename /* Relation<...> */ VBs,
                 typename /* NaturalList<...> */ Nums,
                 template<typename...> class K>
        struct FetchSub {
          static_assert( sizeof( VBs ) != sizeof( VBs ),
                         "invalid C3 merge state: expected node not found while updating tail counts" );
        };
        template<template<typename...> class... VBs,
                 std::size_t Count,
                 std::size_t... Counts,
                 template<typename...> class K>
        struct FetchSub<Relation<K, VBs...>, NaturalList<Count, Counts...>, K> {
          static_assert( Count > 0, "invalid C3 merge state: attempted to decrement a zero tail count" );
          using candidate_list = Relation<K, VBs...>;
          using tail_counts    = NaturalList<Count - 1, Counts...>;
        };
        template<template<typename...> class VB,
                 template<typename...> class... VBs,
                 std::size_t Count,
                 std::size_t... Counts,
                 template<typename...> class K>
        struct FetchSub<Relation<VB, VBs...>, NaturalList<Count, Counts...>, K> {
        private:
          using Result = FetchSub<Relation<VBs...>, NaturalList<Counts...>, K>;

        public:
          using candidate_list = TmpPushFront_t<typename Result::candidate_list, VB>;
          using tail_counts    = NatPrepend_t<typename Result::tail_counts, Count>;
        };

        //////////////////////////////////////////////////

        template<typename /* Relation<...> */ VBs,
                 typename /* NaturalList<...> */ Nums,
                 typename... MergedLists>
        struct CollectCounter {
          using candidate_list = VBs;
          using tail_counts    = Nums;
        };
        template<typename /* Relation<...> */ Vs,
                 typename /* NaturalList<...> */ Nums,
                 template<typename...> class First,
                 template<typename...> class... Rests,
                 typename... MergedLists>
        // The C3 algorithm requires that any initial MergedList must contain at least one element.
        struct CollectCounter<Vs, Nums, Relation<First, Rests...>, MergedLists...> {
        private:
          template<typename /* Relation<...> */ Candidates,
                   typename /* NaturalList<...> */ TailCounts,
                   template<typename...> class... VBs>
          struct Helper {
            using candidate_list = Candidates;
            using tail_counts    = TailCounts;
          };
          template<typename /* Relation<...> */ Candidates,
                   typename /* NaturalList<...> */ TailCounts,
                   template<typename...> class VB,
                   template<typename...> class... VBs>
          struct Helper<Candidates, TailCounts, VB, VBs...> {
          private:
            using Increment = ExtendWith<Candidates, TailCounts, VB, 1>;
            using Result =
              Helper<typename Increment::candidate_list, typename Increment::tail_counts, VBs...>;

          public:
            using candidate_list = typename Result::candidate_list;
            using tail_counts    = typename Result::tail_counts;
          };

          using Head = ExtendWith<Vs, Nums, First, 0>;
          using Tail = Helper<typename Head::candidate_list, typename Head::tail_counts, Rests...>;
          using Result =
            CollectCounter<typename Tail::candidate_list, typename Tail::tail_counts, MergedLists...>;

        public:
          using candidate_list = typename Result::candidate_list;
          using tail_counts    = typename Result::tail_counts;
        };

        //////////////////////////////////////////////////

        template<typename /* Relation<...> */ VBs, typename /* NaturalList<...> */ Nums>
        struct TakeFeasible {
          static_assert( sizeof( VBs ) != sizeof( VBs ),
                         "invalid C3 linearization: no feasible candidate; "
                         "the inheritance order may be inconsistent or cyclic" );
        };
        template<template<typename...> class VB, template<typename...> class... VBs, std::size_t... Counts>
        struct TakeFeasible<Relation<VB, VBs...>, NaturalList<0, Counts...>> {
          using candidate_list = Relation<VBs...>;
          using tail_counts    = NaturalList<Counts...>;
          using feasible_type  = Relation<VB>;
        };
        template<template<typename...> class VB,
                 template<typename...> class... VBs,
                 std::size_t Count,
                 std::size_t... Counts>
        struct TakeFeasible<Relation<VB, VBs...>, NaturalList<Count, Counts...>> {
        private:
          using Result = TakeFeasible<Relation<VBs...>, NaturalList<Counts...>>;

        public:
          using candidate_list = TmpPushFront_t<typename Result::candidate_list, VB>;
          using tail_counts    = NatPrepend_t<typename Result::tail_counts, Count>;
          using feasible_type  = typename Result::feasible_type;
        };

        //////////////////////////////////////////////////

        template<typename TakenCandidate, typename VBs, typename Nums, typename MergedLists>
        struct DropCandidate;
        template<template<typename...> class Candidate, typename VBs, typename Nums>
        struct DropCandidate<Relation<Candidate>, VBs, Nums, TypeList<>> {
          using candidate_list = VBs;
          using tail_counts    = Nums;
          using merged_list    = TypeList<>;
        };
        template<template<typename...> class Candidate,
                 typename VBs,
                 typename Nums,
                 template<typename...> class... Rests,
                 typename... MergedLists>
        struct DropCandidate<Relation<Candidate>, VBs, Nums, TypeList<Relation<Rests...>, MergedLists...>> {
        private:
          using Result = DropCandidate<Relation<Candidate>, VBs, Nums, TypeList<MergedLists...>>;

        public:
          using candidate_list = typename Result::candidate_list;
          using tail_counts    = typename Result::tail_counts;
          using merged_list    = TpPrepend_t<typename Result::merged_list, Relation<Rests...>>;
        };
        template<template<typename...> class Candidate,
                 typename VBs,
                 typename Nums,
                 template<typename...> class NextHead,
                 template<typename...> class... Rests,
                 typename... MergedLists>
        struct DropCandidate<Relation<Candidate>,
                             VBs,
                             Nums,
                             TypeList<Relation<Candidate, NextHead, Rests...>, MergedLists...>> {
        private:
          using Decrement = FetchSub<VBs, Nums, NextHead>;
          using Result    = DropCandidate<Relation<Candidate>,
                                          typename Decrement::candidate_list,
                                          typename Decrement::tail_counts,
                                          TypeList<MergedLists...>>;

        public:
          using candidate_list = typename Result::candidate_list;
          using tail_counts    = typename Result::tail_counts;
          using merged_list    = TpPrepend_t<typename Result::merged_list, Relation<NextHead, Rests...>>;
        };
        template<template<typename...> class Candidate, typename VBs, typename Nums, typename... MergedLists>
        struct DropCandidate<Relation<Candidate>, VBs, Nums, TypeList<Relation<Candidate>, MergedLists...>>
          : DropCandidate<Relation<Candidate>, VBs, Nums, TypeList<MergedLists...>> {};

        //////////////////////////////////////////////////

        template<typename Sorted, typename VBs, typename Nums, typename MergedLists>
        struct MakeMRO;
        template<typename Sorted, typename VBs, typename Nums>
        struct MakeMRO<Sorted, VBs, Nums, TypeList<>> : Identity<Sorted> {};
        template<typename Sorted, typename VBs, typename Nums, typename... MergedLists>
        struct MakeMRO<Sorted, VBs, Nums, TypeList<MergedLists...>> {
        private:
          using Feasible = TakeFeasible<VBs, Nums>;
          using Dropped  = DropCandidate<typename Feasible::feasible_type,
                                         typename Feasible::candidate_list,
                                         typename Feasible::tail_counts,
                                         TypeList<MergedLists...>>;

        public:
          using type = typename MakeMRO<Concat_t<Sorted, typename Feasible::feasible_type>,
                                        typename Dropped::candidate_list,
                                        typename Dropped::tail_counts,
                                        typename Dropped::merged_list>::type;
        };

        using Mapping = CollectCounter<Relation<>, NaturalList<>, VBLists...>;

      public:
        using type = typename MakeMRO<Relation<>,
                                      typename Mapping::candidate_list,
                                      typename Mapping::tail_counts,
                                      TypeList<VBLists...>>::type;
      };
      template<typename... VBLists>
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
      template<typename VBs>
      struct LI {
      private:
        template<typename Linearized, typename RBC, typename... Args>
        struct Helper;
        template<typename Linearized, typename RBC, typename... Args>
        using Helper_t = typename Helper<Linearized, RBC, Args...>::type;

        template<typename RBC, typename... Args>
        struct Helper<Relation<>, RBC, Args...> : Identity<RBC> {};
        template<template<typename...> class Head,
                 template<typename...> class... Tail,
                 typename RBC,
                 typename... Args>
        struct Helper<Relation<Head, Tail...>, RBC, Args...>
          : Identity<Head<Helper_t<Relation<Tail...>, RBC, Args...>, Args...>> {};

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

      template<typename Linearized, template<typename...> class Target>
      struct BaseOf;
      template<typename Linearized, template<typename...> class Target>
      using BaseOf_t = typename BaseOf<Linearized, Target>::type;

      template<template<typename...> class Target, typename Base, typename... Rest>
      struct BaseOf<Target<Base, Rest...>, Target> : Identity<Target<Base, Rest...>> {};
      template<template<typename...> class Linearized,
               typename Base,
               typename... Rest,
               template<typename...> class Target>
      struct BaseOf<Linearized<Base, Rest...>, Target> : Identity<BaseOf_t<Base, Target>> {};
    } // namespace traits
  } // namespace details
} // namespace pace

#endif
