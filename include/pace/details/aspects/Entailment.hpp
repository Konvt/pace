#ifndef PACE_ENTAILMENT
#define PACE_ENTAILMENT

#include "../traits/C3.hpp"
#include "../traits/TypeSet.hpp"

namespace pace {
  namespace details {
    namespace aspects {
      template<template<typename...> class Facade>
      struct EntailOf {
        using type = traits::C3Container<>;
      };
      template<template<typename...> class Facade>
      using EntailOf_t = typename EntailOf<Facade>::type;

#define PACE__ENTAIL_REGISTER( Facade, ... )                      \
  template<>                                                      \
  struct pace::details::aspects::EntailOf<Facade> {               \
    using type = pace::details::traits::C3Container<__VA_ARGS__>; \
  }

      // Resolves and links behaviors into a linear inheritance hierarchy.
      template<typename Config>
      struct EntailLinker;
      template<typename Config>
      using EntailLinker_t = typename EntailLinker<Config>::type;

      template<template<template<typename...> class...> class AnyConfig,
               template<typename...> class... Facades>
      struct EntailLinker<AnyConfig<Facades...>> {
      private:
        template<typename /* TypeSet<...> */ Result, typename /* C3Container<...> */ Behaviors>
        struct FlatMap;
        template<typename /* TypeSet<...> */ Result>
        struct FlatMap<Result, traits::C3Container<>> {
          using type = Result;
        };
        template<typename /* TypeSet<...> */ Result,
                 template<typename...> class Behavior,
                 template<typename...> class... Behaviors>
        struct FlatMap<Result, traits::C3Container<Behavior, Behaviors...>>
          : FlatMap<traits::TpAppend_t<Result, traits::InheritOrder_t<Behavior>>,
                    traits::C3Container<Behaviors...>> {};

        template<typename /* TypeSet<C3Container<...>, ...> */ VBLists>
        struct Helper;
        template<typename... VBLists>
        struct Helper<traits::TypeSet<VBLists...>> : traits::C3Merge<VBLists...> {};

      public:
        using type = typename Helper<
          typename FlatMap<traits::TypeSet<>,
                           traits::Merge_t<traits::C3Container<>, EntailOf_t<Facades>...>>::type>::type;
      };
    } // namespace aspects
  } // namespace details
} // namespace pace

#endif
