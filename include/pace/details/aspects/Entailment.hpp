#ifndef PACE_ENTAILMENT
#define PACE_ENTAILMENT

#include "../traits/C3.hpp"
#include "../traits/TypeSet.hpp"

namespace pace {
  namespace details {
    namespace aspects {
      template<template<typename...> class Facade>
      struct EntailOn : traits::Identity<traits::Relation<>> {};
      template<template<typename...> class Facade>
      using EntailOn_t = typename EntailOn<Facade>::type;

#define PACE__ENTAIL_REGISTER( Facade, ... )      \
  template<>                                      \
  struct pace::details::aspects::EntailOn<Facade> \
    : pace::details::traits::Identity<pace::details::traits::Relation<__VA_ARGS__>> {}

      // Resolves and links behaviors into a linear inheritance hierarchy.
      template<typename Config>
      struct EntailmentLinker;
      template<typename Config>
      using EntailmentLinker_t = typename EntailmentLinker<Config>::type;

      template<template<template<typename...> class...> class AnyConfig,
               template<typename...> class... Facades>
      struct EntailmentLinker<AnyConfig<Facades...>> {
      private:
        template<typename /* TypeSet<...> */ Result, typename /* Relation<...> */ Behaviors>
        struct FlatMap;
        template<typename /* TypeSet<...> */ Result>
        struct FlatMap<Result, traits::Relation<>> : traits::Identity<Result> {};
        template<typename /* TypeSet<...> */ Result,
                 template<typename...> class Behavior,
                 template<typename...> class... Behaviors>
        struct FlatMap<Result, traits::Relation<Behavior, Behaviors...>>
          : FlatMap<traits::TpAppend_t<Result, traits::InheritOrder_t<Behavior>>,
                    traits::Relation<Behaviors...>> {};

        template<typename /* TypeSet<Relation<...>, ...> */ VBLists>
        struct Helper;
        template<typename... VBLists>
        struct Helper<traits::TypeSet<VBLists...>> : traits::C3Merge<VBLists...> {};

      public:
        using type = typename Helper<
          typename FlatMap<traits::TypeSet<>,
                           traits::Merge_t<traits::Relation<>, EntailOn_t<Facades>...>>::type>::type;
      };
    } // namespace aspects
  } // namespace details
} // namespace pace

#endif
