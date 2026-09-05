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

      struct AnyValue {
        constexpr AnyValue() = default;
        template<typename From>
        constexpr AnyValue( From&& ) noexcept
        {}

        template<typename To,
                 typename = typename std::enable_if<std::is_default_constructible<To>::value>::type>
        constexpr operator To() const noexcept
        { return {}; }
        // only available in non-evaluation contexts
        template<typename To,
                 typename = typename std::enable_if<!std::is_default_constructible<To>::value>::type>
        constexpr operator To() const noexcept;
      };

      template<typename P>
      struct PointeeOf<P*> : Identity<P> {};

      template<typename From, typename To>
      using CopyConst_t = typename std::conditional<std::is_const<From>::value, const To, To>::type;
    } // namespace traits
  } // namespace details
} // namespace pace

#endif
