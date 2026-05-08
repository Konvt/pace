#ifndef PGBAR_BEHAVIOR_REGISTRY
#define PGBAR_BEHAVIOR_REGISTRY

#include "../details/traits/C3.hpp"
#include "../details/traits/TypeSet.hpp"

namespace pgbar {
  namespace _details {
    namespace traits {
      template<template<typename...> class Facade>
      struct BehaviorOf {
        using type = TemplateSet<>;
      };
      template<template<typename...> class Facade>
      using BehaviorOf_t = typename BehaviorOf<Facade>::type;

#define PGBAR__BEHAVIOR_REGISTER( Facade, ... )                     \
  template<>                                                        \
  struct pgbar::_details::traits::BehaviorOf<Facade> {              \
    using type = pgbar::_details::traits::TemplateSet<__VA_ARGS__>; \
  }

      // Resolves and links behaviors into a linear inheritance hierarchy.
      template<typename Config>
      struct BehaviorLinker;
      template<typename Config>
      using BehaviorLinker_t = typename BehaviorLinker<Config>::type;

      template<template<template<typename...> class...> class AnyConfig,
               template<typename...> class... Facades>
      struct BehaviorLinker<AnyConfig<Facades...>> {
      private:
        template<typename /* TypeSet<...> */ Result, typename /* TemplateSet<...> */ Behaviors>
        struct FlatMap;
        template<typename /* TypeSet<...> */ Result>
        struct FlatMap<Result, TemplateSet<>> {
          using type = Result;
        };
        template<typename /* TypeSet<...> */ Result,
                 template<typename...> class Behavior,
                 template<typename...> class... Behaviors>
        struct FlatMap<Result, TemplateSet<Behavior, Behaviors...>>
          : FlatMap<TpAppend_t<Result, InheritOrder_t<Behavior>>, TemplateSet<Behaviors...>> {};

        template<typename /* TypeSet<TemplateSet<...>, ...> */ VBLists>
        struct Helper;
        template<typename... VBLists>
        struct Helper<traits::TypeSet<VBLists...>> : traits::C3Merge<VBLists...> {};

      public:
        using type = typename Helper<typename FlatMap<
          TypeSet<>,
          traits::Merge_t<traits::TemplateSet<>, traits::BehaviorOf_t<Facades>...>>::type>::type;
      };
    } // namespace traits
  } // namespace _details
} // namespace pgbar

#endif
