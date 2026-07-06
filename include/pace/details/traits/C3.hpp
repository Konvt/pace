#ifndef PACE_C3
#define PACE_C3

#include "Backport.hpp"
#include "TemplateSet.hpp"

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
    : pace::details::traits::Identity<pace::details::traits::TmpPrepend_t<C3_t<__VA_ARGS__>, Node>> {}

      // The implementation of the "merge" function in the C3 algorithm.
      template<typename... VBLists>
      struct C3Merge {
      private:
        // Check whether Candidate is the top priority within AnotherVBs.
        template<template<typename...> class Candidate, typename /* Relation<...> */ AnotherVBs>
        struct is_preferred_within;

        template<template<typename...> class Candidate>
        struct is_preferred_within<Candidate, Relation<>> : std::true_type {};
        template<template<typename...> class Candidate, template<typename...> class... Rests>
        struct is_preferred_within<Candidate, Relation<Candidate, Rests...>> : std::true_type {};
        template<template<typename...> class Candidate,
                 template<typename...> class Head,
                 template<typename...> class... Rests>
        struct is_preferred_within<Candidate, Relation<Head, Rests...>>
          : Not<TmpContains<Relation<Rests...>, Candidate>> {};

        //////////////////////////////////////////////////

        // Check whether the next preferred candidate is from Inspected.
        template<typename /* Relation<...> */ Inspected, typename... /* Relation<...>, ... */ MergedLists>
        struct is_feasible;

        template<typename... MergedLists>
        struct is_feasible<Relation<>, MergedLists...> : std::false_type {
          // MergedLists contain the source list of Candidate.
          static_assert( sizeof...( MergedLists ) > 1,
                         "pace::details::traits::C3::FeasibleList: MergedLists is always non-empty" );
        };
        template<template<typename...> class Candidate,
                 template<typename...> class... Rests,
                 typename... MergedLists>
        struct is_feasible<Relation<Candidate, Rests...>, MergedLists...>
          : AllOf<is_preferred_within<Candidate, MergedLists>...> {};

        // Pick out the index of the candidate from the MergedList.
        template<typename... MergedLists>
        struct PickCandidate {
        private:
          template<types::Size I>
          struct Helper;

          template<bool Cond, types::Size Pos>
          struct Choice : std::integral_constant<types::Size, Pos> {};
          template<types::Size Pos>
          struct Choice<false, Pos> : Helper<Pos + 1> {};

          template<types::Size I>
          struct Helper : Choice<is_feasible<TypeAt_t<I, MergedLists...>, MergedLists...>::value, I> {};

        public:
          static constexpr types::Size value = Helper<0>::value;
        };

        //////////////////////////////////////////////////

        // Remove the Candidate from the list (if exists).
        template<template<typename...> class Candidate, typename /* Relation<...> */ List>
        struct DropCandidate;
        template<template<typename...> class Candidate, typename List>
        using DropCandidate_t = typename DropCandidate<Candidate, List>::type;

        template<template<typename...> class Candidate, template<typename...> class... Rests>
        struct DropCandidate<Candidate, Relation<Candidate, Rests...>> : Identity<Relation<Rests...>> {};
        template<template<typename...> class Candidate, template<typename...> class... Rests>
        struct DropCandidate<Candidate, Relation<Rests...>> : Identity<Relation<Rests...>> {};

        //////////////////////////////////////////////////

        template<typename /* Relation<...> */ Sorted, typename... /* Relation<...>... */ MergedLists>
        struct MakeMRO {
        private:
          template<typename Selected>
          struct Helper;
          template<template<typename...> class Candidate, template<typename...> class... Others>
          struct Helper<Relation<Candidate, Others...>>
            : MakeMRO<TmpAppend_t<Sorted, Candidate>, DropCandidate_t<Candidate, MergedLists>...> {};

        public:
          using type = typename Helper<TypeAt_t<PickCandidate<MergedLists...>::value, MergedLists...>>::type;
        };
        template<typename Sorted, typename... MergedLists>
        using MakeMro_t = typename MakeMRO<Sorted, MergedLists...>::type;

        template<typename Sorted>
        struct MakeMRO<Sorted> : Identity<Sorted> {};
        template<typename Sorted, typename... OtherLists>
        struct MakeMRO<Sorted, Relation<>, OtherLists...> : MakeMRO<Sorted, OtherLists...> {};
        template<typename Sorted, typename... OtherLists>
        struct MakeMRO<Sorted, Relation<>, Relation<>, OtherLists...> : MakeMRO<Sorted, OtherLists...> {};
        template<typename Sorted, typename... OtherLists>
        struct MakeMRO<Sorted, Relation<>, Relation<>, Relation<>, OtherLists...>
          : MakeMRO<Sorted, OtherLists...> {};
        template<typename Sorted, typename... OtherLists>
        struct MakeMRO<Sorted, Relation<>, Relation<>, Relation<>, Relation<>, Relation<>, OtherLists...>
          : MakeMRO<Sorted, OtherLists...> {};

      public:
        using type = MakeMro_t<Relation<>, VBLists...>;
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
