#ifndef PGBAR_TEMPLATE_SET
#define PGBAR_TEMPLATE_SET

#include "Algorithm.hpp"
#include "TemplateList.hpp"

namespace pgbar {
  namespace _details {
    namespace traits {
      template<template<typename...> class... Ts>
      struct TemplateSet : TemplateList<Ts>... {};

      template<template<typename...> class... Es, template<typename...> class T>
      struct TmpContain<TemplateSet<Es...>, T> : std::is_base_of<TemplateList<T>, TemplateSet<Es...>> {};

      template<template<typename...> class... Es, template<typename...> class T>
      struct TmpPrepend<TemplateSet<Es...>, T> {
      private:
        template<bool Cond, template<typename...> class NewOne>
        struct _select;
        template<template<typename...> class NewOne>
        struct _select<true, NewOne> {
          using type = TemplateSet<Es...>;
        };
        template<template<typename...> class NewOne>
        struct _select<false, NewOne> {
          using type = TemplateSet<NewOne, Es...>;
        };

      public:
        using type = typename _select<TmpContain<TemplateSet<Es...>, T>::value, T>::type;
      };

      template<template<typename...> class... Es, template<typename...> class T>
      struct TmpAppend<TemplateSet<Es...>, T> {
      private:
        template<bool Cond, template<typename...> class NewOne>
        struct _select;
        template<template<typename...> class NewOne>
        struct _select<true, NewOne> {
          using type = TemplateSet<Es...>;
        };
        template<template<typename...> class NewOne>
        struct _select<false, NewOne> {
          using type = TemplateSet<Es..., NewOne>;
        };

      public:
        using type = typename _select<TmpContain<TemplateSet<Es...>, T>::value, T>::type;
      };

      template<template<typename...> class... Es, template<template<typename...> class...> class Collection>
      struct Combine<TemplateSet<Es...>, Collection<>> {
        using type = TemplateSet<Es...>;
      };
      template<template<typename...> class... Es,
               template<template<typename...> class...> class Collection,
               template<typename...> class T,
               template<typename...> class... Ts>
      struct Combine<TemplateSet<Es...>, Collection<T, Ts...>>
        : Combine<TmpAppend_t<TemplateSet<Es...>, T>, Collection<Ts...>> {};

      template<typename Visited, typename List>
      struct _impl_distinct;
      template<typename Visited>
      struct _impl_distinct<Visited, TemplateList<>> : std::true_type {};
      template<typename Visited, template<typename...> class U, template<typename...> class... Us>
      struct _impl_distinct<Visited, TemplateList<U, Us...>>
        : AllOf<Not<TmpContain<Visited, U>>, _impl_distinct<TmpAppend_t<Visited, U>, TemplateList<Us...>>> {};
      template<template<typename...> class... Elements>
      struct Distinct<TemplateList<Elements...>>
        : _impl_distinct<TemplateSet<>, TemplateList<Elements...>> {};
    } // namespace traits
  } // namespace _details
} // namespace pgbar

template<template<typename...> class... Ts>
struct std::tuple_size<pgbar::_details::traits::TemplateSet<Ts...>>
  : std::integral_constant<pgbar::_details::types::Size, sizeof...( Ts )> {};

#endif
