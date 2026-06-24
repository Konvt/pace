#ifndef PACE_TEMPLATE_SET
#define PACE_TEMPLATE_SET

#include "Algorithm.hpp"
#include "Identity.hpp"
#include "TemplateList.hpp"

namespace pace {
  namespace details {
    namespace traits {
      template<template<typename...> class... Ts>
      struct TemplateSet : TemplateList<Ts>... {};

      template<template<typename...> class... Es, template<typename...> class T>
      struct contains_tmp<TemplateSet<Es...>, T> : std::is_base_of<TemplateList<T>, TemplateSet<Es...>> {};

      template<template<typename...> class... Es, template<typename...> class T>
      struct prepend_tmp<TemplateSet<Es...>, T> {
      private:
        template<bool Cond, template<typename...> class NewOne>
        struct select : Identity<TemplateSet<Es...>> {};
        template<template<typename...> class NewOne>
        struct select<false, NewOne> : Identity<TemplateSet<NewOne, Es...>> {};

      public:
        using type = typename select<contains_tmp<TemplateSet<Es...>, T>::value, T>::type;
      };

      template<template<typename...> class... Es, template<typename...> class T>
      struct append_tmp<TemplateSet<Es...>, T> {
      private:
        template<bool Cond, template<typename...> class NewOne>
        struct select : Identity<TemplateSet<Es...>> {};
        template<template<typename...> class NewOne>
        struct select<false, NewOne> : Identity<TemplateSet<Es..., NewOne>> {};

      public:
        using type = typename select<contains_tmp<TemplateSet<Es...>, T>::value, T>::type;
      };

      template<template<typename...> class... Es, template<template<typename...> class...> class Collection>
      struct combine<TemplateSet<Es...>, Collection<>> : Identity<TemplateSet<Es...>> {};
      template<template<typename...> class... Es,
               template<template<typename...> class...> class Collection,
               template<typename...> class T,
               template<typename...> class... Ts>
      struct combine<TemplateSet<Es...>, Collection<T, Ts...>>
        : combine<append_tmp_t<TemplateSet<Es...>, T>, Collection<Ts...>> {};

      template<bool Cond, typename Visited, template<typename...> class... Elements>
      struct _impl_is_unique_tmp : std::false_type {};
      template<typename Visited>
      struct _impl_is_unique_tmp<false, Visited> : std::true_type {};
      template<typename Visited, template<typename...> class U, template<typename...> class... Us>
      struct _impl_is_unique_tmp<false, Visited, U, Us...>
        : _impl_is_unique_tmp<contains_tmp<Visited, U>::value, append_tmp_t<Visited, U>, Us...> {};
      template<template<typename...> class... Elements>
      struct is_unique<TemplateList<Elements...>> : _impl_is_unique_tmp<false, TemplateSet<>, Elements...> {};
    } // namespace traits
  } // namespace details
} // namespace pace

template<template<typename...> class... Ts>
struct std::tuple_size<pace::details::traits::TemplateSet<Ts...>>
  : std::integral_constant<pace::details::types::Size, sizeof...( Ts )> {};

#endif
