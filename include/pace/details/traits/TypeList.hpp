#ifndef PACE_TYPE_LIST
#define PACE_TYPE_LIST

#include "Algorithm.hpp"
#include "Backport.hpp"

namespace pace {
  namespace details {
    namespace traits {
      /**
       * A lightweight tuple type that stores multiple types.
       *
       * `std::tuple` puts some constraints on the input type that are not metaprogramming related,
       * so here is a lightweight tuple type that is used only for template type parameter passing.
       */
      template<typename... Ts>
      struct TypeList {};

      template<typename... Es, typename Element>
      struct TpPrepend<TypeList<Es...>, Element> {
        using type = TypeList<Element, Es...>;
      };

      template<typename... Es, typename Element>
      struct TpAppend<TypeList<Es...>, Element> {
        using type = TypeList<Es..., Element>;
      };

      template<typename... Es, typename T>
      struct TpInsertAt<TypeList<Es...>, sizeof...( Es ), T> {
        using type = TypeList<Es..., T>;
      };
      template<typename... Es, std::size_t I, typename T>
      struct TpInsertAt<TypeList<Es...>, I, T> {
        static_assert( I < sizeof...( Es ), "index out of bounds" );
#if PACE__FAST_TYPEAT
      private:
        template<bool GoFront, typename Front, typename Back, std::size_t Pos>
        struct Helper;
        template<typename... Fs, typename Back>
        struct Helper<true, TypeList<Fs...>, Back, sizeof...( Fs )> : Combine<TypeList<Fs..., T>, Back> {};
        template<typename... Fs, typename Back, std::size_t Pos>
        struct Helper<true, TypeList<Fs...>, Back, Pos>
          : Combine<typename Helper<( Pos <= sizeof...( Fs ) / 2 ),
                                    TpSplit_l<TypeList<Fs...>>,
                                    TpSplit_r<TypeList<Fs...>>,
                                    Pos>::type,
                    Back> {};
        template<typename Front, typename... Bs>
        struct Helper<false, Front, TypeList<Bs...>, 0> : Combine<Front, TypeList<T, Bs...>> {};
        template<typename... Fs, typename... Bs, std::size_t Pos>
        struct Helper<false, TypeList<Fs...>, TypeList<Bs...>, Pos>
          : Combine<TypeList<Fs...>,
                    typename Helper<( Pos - sizeof...( Fs ) <= sizeof...( Bs ) / 2 ),
                                    TpSplit_l<TypeList<Bs...>>,
                                    TpSplit_r<TypeList<Bs...>>,
                                    Pos - sizeof...( Fs )>::type> {};

      public:
        using type = typename Helper<( I <= sizeof...( Es ) / 2 ),
                                     TpSplit_l<TypeList<Es...>>,
                                     TpSplit_r<TypeList<Es...>>,
                                     I>::type;
#else
      private:
        template<std::size_t Offset, typename List>
        struct Helper;
        template<typename U, typename... Us>
        struct Helper<I, TypeList<U, Us...>> {
          using type = TypeList<T, U, Us...>;
        };
        template<std::size_t Offset, typename U, typename... Us>
        struct Helper<Offset, TypeList<U, Us...>>
          : TpPrepend<typename Helper<Offset + 1, TypeList<Us...>>::type, U> {};

      public:
        using type = typename Helper<0, TypeList<Es...>>::type;
#endif
      };

      template<typename Element>
      struct TpErase<TypeList<>, Element> {
        using type = TypeList<>;
      };
      template<typename Head, typename... Tail, typename Element>
      struct TpErase<TypeList<Head, Tail...>, Element>
#if PACE__FAST_TYPEAT
        : Combine<TpErase_t<TpSplit_l<TypeList<Head, Tail...>>, Element>,
                  TpErase_t<TpSplit_r<TypeList<Head, Tail...>>, Element>>
#else
        : TpPrepend<TpErase_t<TypeList<Tail...>, Element>, Head>
#endif
      {
      };

      template<typename... Es, std::size_t I, typename T>
      struct TpReplace<TypeList<Es...>, I, T> {
      private:
        template<std::size_t Offset, typename List>
        struct Helper {
          static_assert( Offset != Offset, "index out of bounds" );
        };
        template<typename U, typename... Us>
        struct Helper<I, TypeList<U, Us...>> {
          using type = TypeList<T, Us...>;
        };
        template<std::size_t Offset, typename U, typename... Us>
        struct Helper<Offset, TypeList<U, Us...>>
          : TpPrepend<typename Helper<Offset + 1, TypeList<Us...>>::type, U> {};

      public:
        using type = typename Helper<0, TypeList<Es...>>::type;
      };

      template<typename... Ts, std::size_t I>
      struct Take<TypeList<Ts...>, I> {
      private:
        template<std::size_t Offset, typename List>
        struct Helper {
          static_assert( Offset != Offset, "index out of bounds" );
        };
        template<typename U, typename... Us>
        struct Helper<I, TypeList<U, Us...>> {
          using type     = TypeList<Us...>;
          using out_type = U;
        };
        template<std::size_t Offset, typename U, typename... Us>
        struct Helper<Offset, TypeList<U, Us...>> {
        private:
          using Result = Helper<Offset + 1, TypeList<Us...>>;

        public:
          using type     = TpPrepend_t<typename Result::type, U>;
          using out_type = typename Result::out_type;
        };

        using Result = Helper<0, TypeList<Ts...>>;

      public:
        using type     = typename Result::type;
        using out_type = typename Result::out_type;
      };

      template<typename... Es, template<typename...> class Collection, typename... Ts>
      struct Combine<TypeList<Es...>, Collection<Ts...>> {
        using type = TypeList<Es..., Ts...>;
      };

      template<typename Element, std::size_t N>
      struct TpFill {
      private:
        template<bool Cond, typename List>
        struct Choice {
          using type = List;
        };
        template<typename List>
        struct Choice<false, List> : TpAppend<List, Element> {};

        using Half = typename TpFill<Element, N / 2>::type;

      public:
        using type = typename Choice<( N % 2 == 0 ), Combine_t<Half, Half>>::type;
      };
      template<typename Element, std::size_t N>
      using TpFill_t = typename TpFill<Element, N>::type;

      template<typename Element>
      struct TpFill<Element, 0> {
        using type = TypeList<>;
      };
      template<typename Element>
      struct TpFill<Element, 1> {
        using type = TypeList<Element>;
      };

      template<typename... Ts>
      struct TpStartsWith<TypeList<>, Ts...> : std::true_type {};
      template<typename Head, typename... Tail>
      struct TpStartsWith<TypeList<Head, Tail...>> : std::false_type {};
      template<typename Head, typename... Tail, typename T, typename... Ts>
      struct TpStartsWith<TypeList<Head, Tail...>, T, Ts...>
        : AllOf<std::is_same<Head, T>, TpStartsWith<TypeList<Tail...>, Ts...>> {};
    } // namespace traits
  } // namespace details
} // namespace pace

#endif
