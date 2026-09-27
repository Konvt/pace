#ifndef PACE_ENTAILMENT
#define PACE_ENTAILMENT

#include "../traits/C3.hpp"

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
        template<typename /* Relation<...> */ Behaviors>
        struct Helper;
        template<template<typename...> class... Behaviors>
        struct Helper<traits::Relation<Behaviors...>>
          // The Behaviors is unique, we can directly "concat" them together.
          : traits::C3Merge<traits::InheritOrder_t<Behaviors>...> {};
        // Since the `Behaviors` parameter is auto-generated,
        // it is not possible to follow the C3 algorithm steps by
        // appending at the end a list of direct base classes composed of `Behaviors`,
        // as auto-generation does not guarantee correct dependency ordering.

      public:
        using type = typename Helper<traits::Merge_t<traits::Relation<>, EntailOn_t<Facades>...>>::type;
      };
    } // namespace aspects
  } // namespace details
} // namespace pace

#endif
