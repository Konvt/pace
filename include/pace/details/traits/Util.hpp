#ifndef PACE_TRAITS_UTIL
#define PACE_TRAITS_UTIL

#include "Concept.hpp"
#include "Identity.hpp"
#include "TypeList.hpp"

namespace pace {
  namespace details {
    namespace traits {
      template<template<typename...> class Template, typename T, types::Size N>
      struct fill_with {
      private:
        template<typename List>
        struct helper;
        template<typename... Elements>
        struct helper<TypeList<Elements...>> : Identity<Template<Elements...>> {};

      public:
        using type = typename helper<fill_tp_t<T, N>>::type;
      };
      template<template<typename...> class Template, typename T, types::Size N>
      using fill_with_t = typename fill_with<Template, T, N>::type;

      template<typename T>
      struct pointee_of {
        static_assert( is_pointer_like<T>::value, "invalid type" );
        using type = typename std::remove_reference<decltype( *std::declval<T&>() )>::type;
      };
      template<typename T>
      using pointee_of_t = typename pointee_of<T>::type;

      template<typename P>
      struct pointee_of<P*> : Identity<P> {};

      template<typename Src, typename Dst>
      using copy_const_t = typename std::conditional<std::is_const<Src>::value, const Dst, Dst>::type;
    } // namespace traits
  } // namespace details
} // namespace pace

#endif
