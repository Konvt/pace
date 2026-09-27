#ifndef PACE_ALGORITHM
#define PACE_ALGORITHM

#include "Backport.hpp"
#include "Identity.hpp"
#include <tuple>

namespace pace {
  namespace details {
    namespace traits {
      template<std::size_t Nth, typename... Ts>
      struct TypeAt
#if PACE__BUILTIN( __type_pack_element )
      {
        using type = __type_pack_element<Nth, Ts...>;
      };
#elif PACE__BUILTIN( __builtin_type_pack_element )
      {
        using type = __builtin_type_pack_element( Nth, Ts... );
      };
#else
        // When used as a template metaprogramming tool,
        // the std::tuple here does not trigger type instantiation,
        // and thus does not generate type checks related to construction constraints.
        : std::tuple_element<Nth, std::tuple<Ts...>> {
      };
# define PACE__FAST_TYPEAT 0
#endif
      template<std::size_t Nth, typename... Ts>
      using TypeAt_t = typename TypeAt<Nth, Ts...>::type;

#ifndef PACE__FAST_TYPEAT
# define PACE__FAST_TYPEAT 1
#endif

      template<template<typename...> class Target, template<typename...> class... Tmps>
      struct TmpIndexIn {
      private:
        template<std::size_t I,
                 template<typename...> class T,
                 template<typename...> class Head,
                 template<typename...> class... Tail>
        struct Helper : Helper<I + 1, T, Tail...> {};
        template<std::size_t I, template<typename...> class T, template<typename...> class... Tail>
        struct Helper<I, T, T, Tail...> : std::integral_constant<std::size_t, I> {};

      public:
        static constexpr std::size_t value = Helper<0, Target, Tmps...>::value;
      };

      template<typename From, template<template<typename...> class...> class To>
      struct TmpNominalCast;
      template<typename From, template<template<typename...> class...> class To>
      using TmpNominalCast_t = typename TmpNominalCast<From, To>::type;

      template<template<template<typename...> class...> class From,
               template<typename...> class... Tmps,
               template<template<typename...> class...> class To>
      struct TmpNominalCast<From<Tmps...>, To> : Identity<To<Tmps...>> {};

      template<typename Collection, typename Element>
      struct TpContains;

      template<typename Collection, typename T>
      struct TpPrepend;
      template<typename Collection, typename T>
      using TpPrepend_t = typename TpPrepend<Collection, T>::type;

      template<typename Collection, typename Element>
      struct TpAppend;
      template<typename Collection, typename Element>
      using TpAppend_t = typename TpAppend<Collection, Element>::type;

      template<typename Collection, typename Element>
      struct TpRemove;
      template<typename Collection, typename Element>
      using TpRemove_t = typename TpRemove<Collection, Element>::type;

      // Checks if a List starts with the specified type sequence.
      template<typename List, typename... Elements>
      struct TpStartsWith;

      template<typename Collection, template<typename...> class Element>
      struct TmpContains;

      // Inserts an element while preserving the collection's semantics (e.g. uniqueness for sets).
      template<typename Collection, template<typename...> class Element>
      struct TmpPrepend;
      template<typename Collection, template<typename...> class Element>
      using TmpPrepend_t = typename TmpPrepend<Collection, Element>::type;

      // Inserts an element while preserving the collection's semantics (e.g. uniqueness for sets).
      template<typename Collection, template<typename...> class Element>
      struct TmpAppend;
      template<typename Collection, template<typename...> class Element>
      using TmpAppend_t = typename TmpAppend<Collection, Element>::type;

      // Unconditionally inserts an element at the front, without enforcing collection semantics.
      template<typename Collection, template<typename...> class T>
      struct TmpPushFront;
      template<typename Collection, template<typename...> class Element>
      using TmpPushFront_t = typename TmpPushFront<Collection, Element>::type;

      // Unconditionally inserts an element at the back, without enforcing collection semantics.
      template<typename Collection, template<typename...> class T>
      struct TmpPushBack;
      template<typename Collection, template<typename...> class Element>
      using TmpPushBack_t = typename TmpPushBack<Collection, Element>::type;

      // Check whether the elements in the collection are unique.
      template<typename Collection>
      struct is_unique;

      // Combines two collections while preserving the first collection's semantics.
      template<typename FirstCollection, typename SecondCollection>
      struct Combine;
      template<typename FirstCollection, typename SecondCollection>
      using Combine_t = typename Combine<FirstCollection, SecondCollection>::type;

      // Concatenates two collections unconditionally, without enforcing collection semantics.
      template<typename FirstCollection, typename SecondCollection>
      struct Concat;
      template<typename FirstCollection, typename SecondCollection>
      using Concat_t = typename Concat<FirstCollection, SecondCollection>::type;

      template<typename Collection>
      struct TpSplit;
      template<typename Collection>
      using TpSplit_l = typename TpSplit<Collection>::left_type;
      template<typename Collection>
      using TpSplit_r = typename TpSplit<Collection>::right_type;

      template<template<typename...> class Collection, typename... Ts>
      struct TpSplit<Collection<Ts...>> {
      private:
#if PACE__FAST_TYPEAT
        template<typename Front, typename Back>
        struct Helper;
        template<std::size_t... L, std::size_t... R>
        struct Helper<IndexSequence<L...>, IndexSequence<R...>> {
          using left_type  = Collection<TypeAt_t<L, Ts...>...>;
          using right_type = Collection<TypeAt_t<( sizeof...( Ts ) / 2 ) + R, Ts...>...>;
        };

        using result_type = Helper<MakeIndexSequence<( sizeof...( Ts ) / 2 )>,
                                   MakeIndexSequence<( sizeof...( Ts ) - ( sizeof...( Ts ) / 2 ) )>>;
#else
        template<std::size_t Count, typename Left, typename... Rests>
        struct Helper;
        template<typename Left, typename... Rests>
        struct Helper<( sizeof...( Ts ) / 2 ), Left, Rests...> {
          using left_type  = Left;
          using right_type = Collection<Rests...>;
        };
        template<std::size_t Count, typename Left, typename Head, typename... Rests>
        struct Helper<Count, Left, Head, Rests...> : Helper<Count + 1, TpAppend_t<Left, Head>, Rests...> {};

        using result_type = Helper<0, Collection<>, Ts...>;
#endif

      public:
        using left_type  = typename result_type::left_type;
        using right_type = typename result_type::right_type;
      };

      template<typename FirstCollection, typename... TailCollections>
      struct Merge {
      private:
#if PACE__FAST_TYPEAT
        template<typename Left, typename Right>
        struct Helper;
        template<typename... Left, typename... Right>
        struct Helper<std::tuple<Left...>, std::tuple<Right...>>
          : Combine<FirstCollection,
                    Combine_t<typename Merge<Left...>::type, typename Merge<Right...>::type>> {};

      public: // Since it is a non-evaluated context, the std::tuple here will not trigger instantiation.
        using type = typename Helper<TpSplit_l<std::tuple<TailCollections...>>,
                                     TpSplit_r<std::tuple<TailCollections...>>>::type;
#else
        template<typename First, typename... Rests>
        struct Helper : Identity<First> {};
        template<typename First, typename Second, typename... Tail>
        struct Helper<First, Second, Tail...> : Merge<Combine_t<First, Second>, Tail...> {};

      public:
        using type = typename Helper<FirstCollection, TailCollections...>::type;
#endif
      };
      template<typename FirstCollection, typename... TailCollections>
      using Merge_t = typename Merge<FirstCollection, TailCollections...>::type;

      template<typename Collection>
      struct Merge<Collection> : Identity<Collection> {};
      template<typename Head, typename Tail>
      struct Merge<Head, Tail> : Combine<Head, Tail> {};
    } // namespace traits
  } // namespace details
} // namespace pace

#endif
