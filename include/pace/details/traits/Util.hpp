#ifndef PACE_TRAITS_UTIL
#define PACE_TRAITS_UTIL

#include "Concept.hpp"
#include "Identity.hpp"
#include "TypeList.hpp"

namespace pace {
  namespace details {
    namespace traits {
      template<template<typename...> class Template, typename T, std::size_t N>
      struct FillWith {
      private:
        template<typename List>
        struct Helper;
        template<typename... Elements>
        struct Helper<TypeList<Elements...>> : Identity<Template<Elements...>> {};

      public:
        using type = typename Helper<TpFill_t<T, N>>::type;
      };
      template<template<typename...> class Template, typename T, std::size_t N>
      using FillWith_t = typename FillWith<Template, T, N>::type;

      template<typename T>
      struct PointeeOf {
        static_assert( is_pointer_like<T>::value, "invalid type" );
        using type = typename std::remove_reference<decltype( *std::declval<T&>() )>::type;
      };
      template<typename T>
      using PointeeOf_t = typename PointeeOf<T>::type;

      template<typename P>
      struct PointeeOf<P*> : Identity<P> {};

      template<typename Src, typename Dst>
      using CopyConst_t = typename std::conditional<std::is_const<Src>::value, const Dst, Dst>::type;

      template<typename T>
      using AsConst_t = const typename std::remove_reference<T>::type&;
    } // namespace traits
  } // namespace details
} // namespace pace

#endif
