#ifndef PACE_ENTAILMENT
#define PACE_ENTAILMENT

#include "../traits/C3.hpp"

namespace pace {
  namespace details {
    namespace aspects {
      template<template<typename...> class Facade>
      struct EntailOn : traits::TypeIdentity<traits::Relation<>> {};
      template<template<typename...> class Facade>
      using EntailOn_t = typename EntailOn<Facade>::type;

#define PACE__ENTAIL_REGISTER( Facade, ... )      \
  template<>                                      \
  struct pace::details::aspects::EntailOn<Facade> \
    : pace::details::traits::TypeIdentity<pace::details::traits::Relation<__VA_ARGS__>> {}

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
          : traits::C3Merge<traits::InheritOrder_t<Behaviors>...> {};

        // template<typename /* TypeSet<...> */ MROs>
        // struct Helper;
        // template<typename... MROs>
        // struct Helper<traits::TypeSet<MROs...>> : traits::C3Merge<MROs...> {};

      public:
        // Treats entailments as unordered Behavior requirements.
        // Behavior precedence is defined solely by each Behavior's own C3 hierarchy.
        // This reuses the existing InheritOrder and avoids redundant C3 computation.
        using type = typename Helper<traits::Merge_t<traits::Relation<>, EntailOn_t<Facades>...>>::type;

        // Alternative: treat each entailment list as a C3 local precedence list.
        // This preserves declaration order, but adds an extra C3 computation per distinct entailment
        // and may impose precedence that entailment itself does not require.
        //
        // using type = typename Helper<
        //   traits::Merge_t<traits::TypeSet<>,
        //                   traits::TypeSet<typename traits::C3<EntailOn_t<Facades>>::type>...>>::type;
      };
    } // namespace aspects
  } // namespace details
} // namespace pace

#endif
