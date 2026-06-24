#ifndef PACE_ALGORITHM
#define PACE_ALGORITHM

#include "Backport.hpp"
#include "Identity.hpp"
#include <tuple>

namespace pace {
  namespace details {
    namespace traits {
      template<types::Size Nth, typename... Ts>
      struct type_at
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
      using type_at_t = typename type_at<Nth, Ts...>::type;
      // After C++26, we can use `Ts...[Pos]`.

#if PACE__BUILTIN( __type_pack_element ) || defined( __GLIBCXX__ )
# define PACE__FAST_TYPEAT 1
#else
# define PACE__FAST_TYPEAT 0
#endif

      template<template<typename...> class Target, template<typename...> class... Tmps>
      struct index_in {
      private:
        template<types::Size I,
                 template<typename...> class T,
                 template<typename...> class Head,
                 template<typename...> class... Tail>
        struct helper : helper<I + 1, T, Tail...> {};
        template<types::Size I, template<typename...> class T, template<typename...> class... Tail>
        struct helper<I, T, T, Tail...> : std::integral_constant<types::Size, I> {};

      public:
        static constexpr types::Size value = helper<0, Target, Tmps...>::value;
      };

      template<typename From, template<template<typename...> class...> class To>
      struct nominal_cast_tmp;
      template<typename From, template<template<typename...> class...> class To>
      using nominal_cast_tmp_t = typename nominal_cast_tmp<From, To>::type;

      template<template<template<typename...> class...> class From,
               template<typename...> class... Tmps,
               template<template<typename...> class...> class To>
      struct nominal_cast_tmp<From<Tmps...>, To> : Identity<To<Tmps...>> {};

      template<typename Collection>
      struct split;
      template<typename Collection>
      using split_l = typename split<Collection>::left;
      template<typename Collection>
      using split_r = typename split<Collection>::right;

      template<template<typename...> class Collection, typename... Ts>
      struct split<Collection<Ts...>> {
      private:
        template<typename Front, typename Back>
        struct helper;
        template<types::Size... L, types::Size... R>
        struct helper<IndexSequence<L...>, IndexSequence<R...>> {
          using left  = Collection<type_at_t<L, Ts...>...>;
          using right = Collection<type_at_t<( sizeof...( Ts ) / 2 ) + R, Ts...>...>;
        };

        using result = helper<make_index_sequence<( sizeof...( Ts ) / 2 )>,
                              make_index_sequence<( sizeof...( Ts ) - ( sizeof...( Ts ) / 2 ) )>>;

      public:
        using left  = typename result::left;
        using right = typename result::right;
      };

      template<typename Collection, typename Element>
      struct contains_tp;

      template<typename Collection, typename T>
      struct prepend_tp;
      template<typename Collection, typename T>
      using prepend_tp_t = typename prepend_tp<Collection, T>::type;

      template<typename Collection, typename Element>
      struct append_tp;
      template<typename Collection, typename Element>
      using append_tp_t = typename append_tp<Collection, Element>::type;

      template<typename Collection, typename Element>
      struct remove_tp;
      template<typename Collection, typename Element>
      using remove_tp_t = typename remove_tp<Collection, Element>::type;

      // Checks if a List starts with the specified type sequence.
      template<typename List, typename... Elements>
      struct starts_with_tp;

      template<typename Collection, template<typename...> class Element>
      struct contains_tmp;

      template<typename Collection, template<typename...> class Element>
      struct prepend_tmp;
      template<typename Collection, template<typename...> class Element>
      using prepend_tmp_t = typename prepend_tmp<Collection, Element>::type;

      template<typename Collection, template<typename...> class Element>
      struct append_tmp;
      template<typename Collection, template<typename...> class Element>
      using append_tmp_t = typename append_tmp<Collection, Element>::type;

      // Check whether the elements in the collection are unique.
      template<typename Collection>
      struct is_unique;

      template<typename FirstCollection, typename SecondCollection>
      struct combine;
      template<typename FirstCollection, typename SecondCollection>
      using combine_t = typename combine<FirstCollection, SecondCollection>::type;

      template<typename FirstCollection, typename... TailCollections>
      struct merge {
      private:
#if PACE__FAST_TYPEAT
        template<typename Left, typename Right>
        struct helper;
        template<typename... Left, typename... Right>
        struct helper<std::tuple<Left...>, std::tuple<Right...>>
          : combine<FirstCollection,
                    combine_t<typename merge<Left...>::type, typename merge<Right...>::type>> {};

      public: // Since it is a non-evaluated context, the std::tuple here will not trigger instantiation.
        using type = typename helper<split_l<std::tuple<TailCollections...>>,
                                     split_r<std::tuple<TailCollections...>>>::type;
#else
        template<typename First, typename... Rests>
        struct helper : Identity<First> {};
        template<typename First, typename Second, typename... Tail>
        struct helper<First, Second, Tail...> : merge<combine_t<First, Second>, Tail...> {};

      public:
        using type = typename helper<FirstCollection, TailCollections...>::type;
#endif
      };
      template<typename FirstCollection, typename... TailCollections>
      using merge_t = typename merge<FirstCollection, TailCollections...>::type;

      template<typename Collection>
      struct merge<Collection> : Identity<Collection> {};
      template<typename Head, typename Tail>
      struct merge<Head, Tail> : combine<Head, Tail> {};
    } // namespace traits
  } // namespace details
} // namespace pace

#endif
