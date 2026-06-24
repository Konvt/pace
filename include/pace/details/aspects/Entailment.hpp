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
      struct link_entailments;
      template<typename Config>
      using link_entailments_t = typename link_entailments<Config>::type;

      template<template<template<typename...> class...> class AnyConfig,
               template<typename...> class... Facades>
      struct link_entailments<AnyConfig<Facades...>> {
      private:
        template<typename /* TypeSet<...> */ Result, typename /* Relation<...> */ Behaviors>
        struct flat_map;
        template<typename /* TypeSet<...> */ Result>
        struct flat_map<Result, traits::Relation<>> : traits::Identity<Result> {};
        template<typename /* TypeSet<...> */ Result,
                 template<typename...> class Behavior,
                 template<typename...> class... Behaviors>
        struct flat_map<Result, traits::Relation<Behavior, Behaviors...>>
          : flat_map<traits::append_tp_t<Result, traits::InheritOrder_t<Behavior>>,
                     traits::Relation<Behaviors...>> {};

        template<typename /* TypeSet<Relation<...>, ...> */ VBLists>
        struct helper;
        template<typename... VBLists>
        struct helper<traits::TypeSet<VBLists...>> : traits::c3_merge<VBLists...> {};

      public:
        using type = typename helper<
          typename flat_map<traits::TypeSet<>,
                            traits::merge_t<traits::Relation<>, EntailOn_t<Facades>...>>::type>::type;
      };
    } // namespace aspects
  } // namespace details
} // namespace pace

#endif
