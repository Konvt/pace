#ifndef PACE_ALGORITHM
#define PACE_ALGORITHM

#include "Backport.hpp"
#include "Identity.hpp"
#include <tuple>

namespace pace {
  namespace details {
    namespace traits {
      template<types::Size Nth, typename... Ts>
      struct TypeAt
#if PACE__BUILTIN( __type_pack_element )
      {
        using type = __type_pack_element<Nth, Ts...>;
      };
#else
        // When used as a template metaprogramming tool,
        // the std::tuple here does not trigger type instantiation,
        // and thus does not generate type checks related to construction constraints.
        : std::tuple_element<Nth, std::tuple<Ts...>> {
      };
#endif
      template<types::Size Nth, typename... Ts>
      using TypeAt_t = typename TypeAt<Nth, Ts...>::type;
      // After C++26, we can use `Ts...[Pos]`.

#if PACE__BUILTIN( __type_pack_element ) || defined( __GLIBCXX__ )
# define PACE__FAST_TYPEAT 1
#else
# define PACE__FAST_TYPEAT 0
#endif

      template<template<typename...> class Target, template<typename...> class... Tmps>
      struct IndexIn {
      private:
        template<types::Size I,
                 template<typename...> class T,
                 template<typename...> class Head,
                 template<typename...> class... Tail>
        struct Helper : Helper<I + 1, T, Tail...> {};
        template<types::Size I, template<typename...> class T, template<typename...> class... Tail>
        struct Helper<I, T, T, Tail...> : std::integral_constant<types::Size, I> {};

      public:
        static constexpr types::Size value = Helper<0, Target, Tmps...>::value;
      };

      template<typename From, template<template<typename...> class...> class To>
      struct TmpNominalCast;
      template<typename From, template<template<typename...> class...> class To>
      using TmpNominalCast_t = typename TmpNominalCast<From, To>::type;

      template<template<template<typename...> class...> class From,
               template<typename...> class... Tmps,
               template<template<typename...> class...> class To>
      struct TmpNominalCast<From<Tmps...>, To> : Identity<To<Tmps...>> {};

      template<typename Collection>
      struct Split;
      template<typename Collection>
      using Split_l = typename Split<Collection>::left;
      template<typename Collection>
      using Split_r = typename Split<Collection>::right;

      template<template<typename...> class Collection, typename... Ts>
      struct Split<Collection<Ts...>> {
      private:
        template<typename Front, typename Back>
        struct Helper;
        template<types::Size... L, types::Size... R>
        struct Helper<IndexSequence<L...>, IndexSequence<R...>> {
          using left  = Collection<TypeAt_t<L, Ts...>...>;
          using right = Collection<TypeAt_t<( sizeof...( Ts ) / 2 ) + R, Ts...>...>;
        };

        using result = Helper<MakeIndexSequence<( sizeof...( Ts ) / 2 )>,
                              MakeIndexSequence<( sizeof...( Ts ) - ( sizeof...( Ts ) / 2 ) )>>;

      public:
        using left  = typename result::left;
        using right = typename result::right;
      };

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

      template<typename Collection, template<typename...> class Element>
      struct TmpPrepend;
      template<typename Collection, template<typename...> class Element>
      using TmpPrepend_t = typename TmpPrepend<Collection, Element>::type;

      template<typename Collection, template<typename...> class Element>
      struct TmpAppend;
      template<typename Collection, template<typename...> class Element>
      using TmpAppend_t = typename TmpAppend<Collection, Element>::type;

      // Check whether the elements in the collection are unique.
      template<typename Collection>
      struct is_unique;

      template<typename FirstCollection, typename SecondCollection>
      struct Combine;
      template<typename FirstCollection, typename SecondCollection>
      using Combine_t = typename Combine<FirstCollection, SecondCollection>::type;

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
        using type = typename Helper<Split_l<std::tuple<TailCollections...>>,
                                     Split_r<std::tuple<TailCollections...>>>::type;
#else
        template<typename First, typename... Rests>
        struct Helper : Identity<First> {};
        template<typename First, typename Second, typename... Tail>
        struct Helper<First, Second, Tail...> : merge<Combine_t<First, Second>, Tail...> {};

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
