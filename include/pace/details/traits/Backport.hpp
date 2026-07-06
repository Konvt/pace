#ifndef PACE_TRAITS_BACKPORT
#define PACE_TRAITS_BACKPORT

#include "../core/Core.hpp"
#include "../core/Types.hpp"
#include "Identity.hpp"

namespace pace {
  namespace details {
    namespace traits {
      // Before C++17, not all std entities had feature macros.
#if PACE__CXX14
      template<types::Size... Ns>
      using IndexSequence = std::integer_sequence<types::Size, Ns...>;

      template<types::Size N>
      using MakeIndexSequence = std::make_integer_sequence<types::Size, N>;
#else
      template<types::Size... Ns>
      struct IndexSequence {};

      // This is an internal implementation and should not be used outside of this preprocessing block.
      template<typename HeadSeq, typename TailSeq>
      struct _concat_seq;
      template<typename HeadSeq, typename TailSeq>
      using _concat_seq_t = typename _concat_seq<HeadSeq, TailSeq>::type;

      template<types::Size... HeadI, types::Size... TailI>
      struct _concat_seq<IndexSequence<HeadI...>, IndexSequence<TailI...>>
        : Identity<IndexSequence<HeadI..., ( sizeof...( HeadI ) + TailI )...>> {};

      // Internal implementation, it should not be used outside of this preprocessing block.
      template<types::Size N>
      struct _make_index_seq_helper
        : Identity<_concat_seq_t<typename _make_index_seq_helper<N / 2>::type,
                                 typename _make_index_seq_helper<N - N / 2>::type>> {};
      template<>
      struct _make_index_seq_helper<0> : Identity<IndexSequence<>> {};
      template<>
      struct _make_index_seq_helper<1> : Identity<IndexSequence<0>> {};

      template<types::Size N>
      using MakeIndexSequence = typename _make_index_seq_helper<N>::type;
#endif

#ifdef __cpp_lib_bool_constant
      template<bool B>
      using BoolConstant = std::bool_constant<B>;
#else
      template<bool B>
      using BoolConstant = std::integral_constant<bool, B>;
#endif

#if PACE__CXX14
      using std::is_final;
#elif defined( __GNUC__ ) || defined( __clang__ ) || defined( _MSC_VER )
      template<typename T>
      using is_final = BoolConstant<__is_final( T )>;
#else
      template<typename T>
      using is_final = BoolConstant<true>;
#endif

#ifdef __cpp_lib_logical_traits
      template<typename... Preds>
      using AllOf = std::conjunction<Preds...>;

      template<typename... Preds>
      using AnyOf = std::disjunction<Preds...>;

      template<typename Pred>
      using Not = std::negation<Pred>;
#else
      template<typename, typename Pred, typename... Preds>
      struct _impl_all_of : Identity<Pred> {};
      template<typename Pred1, typename Pred2, typename... Preds>
      struct _impl_all_of<typename std::enable_if<bool( Pred1::value )>::type, Pred1, Pred2, Preds...>
        : _impl_all_of<void, Pred2, Preds...> {};
      template<typename... Preds>
      struct AllOf : _impl_all_of<void, Preds...>::type {};
      template<>
      struct AllOf<> : std::true_type {};

      template<typename, typename Pred, typename... Preds>
      struct _impl_any_of : Identity<Pred> {};
      template<typename Pred1, typename Pred2, typename... Preds>
      struct _impl_any_of<typename std::enable_if<!bool( Pred1::value )>::type, Pred1, Pred2, Preds...>
        : _impl_any_of<void, Pred2, Preds...> {};
      template<typename... Preds>
      struct AnyOf : _impl_any_of<void, Preds...>::type {};
      template<>
      struct AnyOf<> : std::false_type {};

      template<typename Pred>
      using Not = BoolConstant<!bool( Pred::value )>;
#endif

#ifdef __cpp_lib_is_nothrow_convertible
      using std::is_nothrow_convertible;
#else
      template<typename From, typename To>
      struct _impl_is_nothrow_convertible {
      private:
        static constexpr void implicit_conversion( To ) noexcept;
        static constexpr void implicit_conversion( ... ) noexcept( false );

      public:
        using result = BoolConstant<noexcept( implicit_conversion( std::declval<From>() ) )>;
      };
      template<typename From>
      struct _impl_is_nothrow_convertible<From, void> {
        using result = std::false_type;
      };
      template<typename To>
      struct _impl_is_nothrow_convertible<void, To> {
        using result = std::false_type;
      };
      template<>
      struct _impl_is_nothrow_convertible<void, void> {
        using result = std::true_type;
      };
      template<typename From, typename To>
      using is_nothrow_convertible = typename _impl_is_nothrow_convertible<From, To>::result;
#endif

#ifdef __cpp_lib_is_implicit_lifetime
      using std::is_implicit_lifetime;
#else
      // We cannot guarantee that the constraints on implicit lifetime types
      // in standards prior to C++23 are as lenient.
      // Therefore, we conservatively chose std::is_trivial.
      template<typename T>
      using is_implicit_lifetime = std::is_trivial<T>;
#endif
    } // namespace traits
  } // namespace details
} // namespace pace

#endif
