#ifndef PGBAR_C3
#define PGBAR_C3

#include "Backport.hpp"
#include "TemplateSet.hpp"

namespace pgbar {
  namespace details {
    namespace traits {
      // The type used in the C3 algorithm to store template types.
      template<template<typename...> class... Ts>
      using C3Container = TemplateSet<Ts...>;

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
      template<typename /* C3Container<...> */ VBs>
      struct C3;
      template<template<typename...> class VB, template<typename...> class... VBs>
      using C3_t = typename C3<C3Container<VB, VBs...>>::type;

      // The structure that records the inheritance order of Node includes itself,
      // just like Python's MRO.
      // The return value of this config-function will serve as both the return value
      // and the entry parameter of C3.
      template<template<typename...> class Node>
      struct InheritOrder {
        using type = C3Container<Node>;
      };
      // Gets the inheritance order of the template class `Node`.
      template<template<typename...> class Node>
      using InheritOrder_t = typename InheritOrder<Node>::type;

// A helper macro to register the inheritance structure of a template class.
#define PGBAR__INHERIT_REGISTER( Node, ... )                                    \
  template<>                                                                    \
  struct pgbar::details::traits::InheritOrder<Node> {                           \
    using type = pgbar::details::traits::TmpPrepend_t<C3_t<__VA_ARGS__>, Node>; \
  }

      // The implementation of the "merge" function in the C3 algorithm.
      template<typename... VBLists>
      struct C3Merge {
      private:
        // Check whether Candidate is the top priority within AnotherVBs.
        template<template<typename...> class Candidate, typename /* C3Container<...> */ AnotherVBs>
        struct PreferredWithin;

        template<template<typename...> class Candidate>
        struct PreferredWithin<Candidate, C3Container<>> : std::true_type {};
        template<template<typename...> class Candidate, template<typename...> class... Rests>
        struct PreferredWithin<Candidate, C3Container<Candidate, Rests...>> : std::true_type {};
        template<template<typename...> class Candidate,
                 template<typename...> class Head,
                 template<typename...> class... Rests>
        struct PreferredWithin<Candidate, C3Container<Head, Rests...>>
          : Not<TmpContain<C3Container<Rests...>, Candidate>> {};

        //////////////////////////////////////////////////

        // Check whether the next preferred candidate is from Inspected.
        template<typename /* C3Container<...> */ Inspected,
                 typename... /* C3Container<...>, ... */ MergedLists>
        struct FeasibleList;

        template<typename... MergedLists>
        struct FeasibleList<C3Container<>, MergedLists...> : std::false_type {
          // MergedLists contain the source list of Candidate.
          static_assert( sizeof...( MergedLists ) > 1,
                         "pgbar::details::traits::C3::FeasibleList: MergedLists is always non-empty" );
        };
        template<template<typename...> class Candidate,
                 template<typename...> class... Rests,
                 typename... MergedLists>
        struct FeasibleList<C3Container<Candidate, Rests...>, MergedLists...>
          : AllOf<PreferredWithin<Candidate, MergedLists>...> {};

        // Pick out the index of the candidate from the MergedList.
        template<typename... MergedLists>
        struct PickCandidate {
        private:
          template<types::Size I>
          struct Helper;

          template<bool Cond, types::Size Pos>
          struct _select;
          template<types::Size Pos>
          struct _select<true, Pos> : std::integral_constant<types::Size, Pos> {};
          template<types::Size Pos>
          struct _select<false, Pos> : Helper<Pos + 1> {};

          template<types::Size I>
          struct Helper : _select<FeasibleList<TypeAt_t<I, MergedLists...>, MergedLists...>::value, I> {};

        public:
          static constexpr types::Size value = Helper<0>::value;
        };

        //////////////////////////////////////////////////

        // Remove the Candidate from the list (if exists).
        template<template<typename...> class Candidate, typename /* C3Container<...> */ List>
        struct DropCandidate;
        template<template<typename...> class Candidate, typename List>
        using DropCandidate_t = typename DropCandidate<Candidate, List>::type;

        template<template<typename...> class Candidate, template<typename...> class... Rests>
        struct DropCandidate<Candidate, C3Container<Candidate, Rests...>> {
          using type = C3Container<Rests...>;
        };
        template<template<typename...> class Candidate, template<typename...> class... Rests>
        struct DropCandidate<Candidate, C3Container<Rests...>> {
          using type = C3Container<Rests...>;
        };

        //////////////////////////////////////////////////

        template<typename /* C3Container<...> */ Sorted, typename... /* C3Container<...>... */ MergedLists>
        struct Linearize {
        private:
          template<typename Selected>
          struct Helper;
          template<template<typename...> class Candidate, template<typename...> class... Others>
          struct Helper<C3Container<Candidate, Others...>>
            : Linearize<TmpAppend_t<Sorted, Candidate>, DropCandidate_t<Candidate, MergedLists>...> {};

        public:
          using type = typename Helper<TypeAt_t<PickCandidate<MergedLists...>::value, MergedLists...>>::type;
        };
        template<typename Sorted, typename... MergedLists>
        using Linearize_t = typename Linearize<Sorted, MergedLists...>::type;

        template<typename Sorted>
        struct Linearize<Sorted> {
          using type = Sorted;
        };
        template<typename Sorted, typename... OtherLists>
        struct Linearize<Sorted, C3Container<>, OtherLists...> : Linearize<Sorted, OtherLists...> {};
        template<typename Sorted, typename... OtherLists>
        struct Linearize<Sorted, C3Container<>, C3Container<>, OtherLists...>
          : Linearize<Sorted, OtherLists...> {};
        template<typename Sorted, typename... OtherLists>
        struct Linearize<Sorted, C3Container<>, C3Container<>, C3Container<>, OtherLists...>
          : Linearize<Sorted, OtherLists...> {};
        template<typename Sorted, typename... OtherLists>
        struct Linearize<Sorted,
                         C3Container<>,
                         C3Container<>,
                         C3Container<>,
                         C3Container<>,
                         C3Container<>,
                         OtherLists...> : Linearize<Sorted, OtherLists...> {};

      public:
        using type = Linearize_t<C3Container<>, VBLists...>;
      };
      template<typename... VBLists>
      using C3Merge_t = typename C3Merge<VBLists...>::type;

      template<template<typename...> class VB, template<typename...> class... VBs>
      struct C3<C3Container<VB, VBs...>>
        : C3Merge<InheritOrder_t<VB>, InheritOrder_t<VBs>..., C3Container<VB, VBs...>> {};

      /**
       * Linearization of Inheritance.

       * Linearize the input types using the C3 algorithm,
       * then iterate through the resulting sorted list and fill in their template types.

       * It relies on the template `InheritOrder` and `C3` classes to work.
       */
      template<typename VBs>
      struct LI {
      private:
        template<typename Linearized, typename RBC, typename... Args>
        struct Helper;
        template<typename Linearized, typename RBC, typename... Args>
        using Helper_t = typename Helper<Linearized, RBC, Args...>::type;

        template<typename RBC, typename... Args>
        struct Helper<C3Container<>, RBC, Args...> {
          using type = RBC;
        };
        template<template<typename...> class Head,
                 template<typename...> class... Tail,
                 typename RBC,
                 typename... Args>
        struct Helper<C3Container<Head, Tail...>, RBC, Args...> {
          using type = Head<Helper_t<C3Container<Tail...>, RBC, Args...>, Args...>;
        };

      public:
        // RBC: Root Base Class.
        template<typename RBC, typename... Args>
        using type = Helper_t<typename C3<VBs>::type, RBC, Args...>;
      };

      template<template<typename...> class VB, template<typename...> class... VBs>
      struct LI_t {
        template<typename RBC, typename... Args>
        using type = typename LI<C3Container<VB, VBs...>>::template type<RBC, Args...>;
      };

      template<typename Linearized, template<typename...> class Target>
      struct BaseOf;
      template<typename Linearized, template<typename...> class Target>
      using BaseOf_t = typename BaseOf<Linearized, Target>::type;

      template<template<typename...> class Target, typename Base, typename... Rest>
      struct BaseOf<Target<Base, Rest...>, Target> {
        using type = Target<Base, Rest...>;
      };
      template<template<typename...> class Linearized,
               typename Base,
               typename... Rest,
               template<typename...> class Target>
      struct BaseOf<Linearized<Base, Rest...>, Target> {
        using type = BaseOf_t<Base, Target>;
      };
    } // namespace traits
  } // namespace details
} // namespace pgbar

#endif
