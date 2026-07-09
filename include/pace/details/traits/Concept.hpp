// Only contains trait entities unrelated to template metaprogramming here.
#ifndef PACE_CONCEPT
#define PACE_CONCEPT

#include "Backport.hpp"
#include <iterator>
#ifdef __cpp_lib_concepts
# include <concepts>
#endif
#ifdef __cpp_lib_ranges
# include <ranges>
#endif

namespace pace {
  namespace details {
    namespace traits {
      template<typename I>
      using IterCategory_t = typename std::iterator_traits<I>::iterator_category;
      template<typename I>
      using IterPointer_t = typename std::iterator_traits<I>::pointer;
#ifdef __cpp_lib_ranges
      template<typename I>
      using IterValue_t = std::iter_value_t<I>;
      template<typename I>
      using IterDifference_t = std::iter_difference_t<I>;
      template<typename I>
      using IterReference_t = std::iter_reference_t<I>;
#else
      template<typename I>
      using IterValue_t = typename std::iterator_traits<I>::value_type;
      template<typename I>
      using IterDifference_t = typename std::iterator_traits<I>::difference_type;
      template<typename I>
      using IterReference_t = typename std::iterator_traits<I>::reference;
#endif

      template<typename T>
      struct IteratorOf {
      private:
        // Provide a default fallback to avoid the problem of the type not existing
        // in the immediate context derivation.
        template<typename U>
        static constexpr U check( ... );
#ifdef __cpp_lib_ranges
        template<typename U>
        static constexpr std::ranges::iterator_t<U> check( int );
#else
        template<typename U>
        static constexpr decltype( std::begin( std::declval<U>() ) ) check( int );
#endif
      public:
        using type = decltype( check<T>( 0 ) );
      };
      // Get the result type of `IteratorOf`.
      template<typename T>
      using IteratorOf_t = typename IteratorOf<T>::type;

      template<typename T>
      struct SentinelOf {
      private:
        template<typename U>
        static constexpr U check( ... );
#ifdef __cpp_lib_ranges
        template<typename U>
        static constexpr std::ranges::sentinel_t<T> check( int );
#else
        template<typename U>
        static constexpr decltype( std::end( std::declval<T&>() ) ) check( int );
#endif

      public:
        using type = decltype( check<T>( 0 ) );
      };
      template<typename T>
      using SentinelOf_t = typename SentinelOf<T>::type;

      template<typename T, typename = void>
      struct _impl_is_pointer_like : std::false_type {};
      template<typename P>
      struct _impl_is_pointer_like<P*, void> : std::true_type {};
      template<typename T>
      struct _impl_is_pointer_like<
        T,
        typename std::enable_if<
          Not<AnyOf<std::is_reference<T>,
                    std::is_void<decltype( *std::declval<T&>() )>,
                    std::is_void<decltype( std::declval<T&>().operator->() )>,
                    std::is_void<decltype( static_cast<bool>( std::declval<T&>() ) )>>>::value>::type>
        : std::true_type {};
      template<typename T>
      using is_pointer_like = _impl_is_pointer_like<T>;

      // Check whether the type Instance is an instantiated version of Tmp or whether it inherits from Tmp
      // itself.
      template<typename Instance, template<typename...> class Tmp>
      struct _impl_is_instance_of {
      private:
        template<typename... Args>
        static constexpr std::true_type check( const Tmp<Args...>& );
        static constexpr std::false_type check( ... );

      public:
        using result_type =
          AllOf<Not<std::is_reference<Instance>>,
                decltype( check( std::declval<typename std::remove_cv<Instance>::type>() ) )>;
      };
      template<typename Instance, template<typename...> class Tmp>
      using is_instance_of = typename _impl_is_instance_of<Instance, Tmp>::result_type;

#ifdef __cpp_lib_concepts
      template<typename Itr, typename Snt = Itr>
      using is_sized_cursor =
        BoolConstant<std::movable<Itr> && std::weakly_incrementable<Itr> && std::indirectly_readable<Itr>
                     && std::sized_sentinel_for<Snt, Itr>>;
#else
      template<typename Itr, typename Snt, typename = void>
      struct _impl_is_sized_cursor : std::false_type {};
      template<typename P>
      struct _impl_is_sized_cursor<P*, P*, void> : std::true_type {};
      template<typename Itr, typename Snt>
      struct _impl_is_sized_cursor<
        Itr,
        Snt,
        typename std::enable_if<AllOf<
          Not<std::is_reference<Itr>>,
          Not<std::is_reference<Snt>>,
          std::is_move_constructible<Itr>,
          std::is_signed<IterDifference_t<Itr>>,
          std::is_same<decltype( ++std::declval<Itr>() ), Itr&>,
          std::is_void<decltype( std::declval<Itr>()++, void() )>,
          std::is_same<decltype( *std::declval<Itr>() ), IterReference_t<Itr>>,
          std::is_same<decltype( std::distance( std::declval<Itr>(), std::declval<Snt>() ) ),
                       IterDifference_t<Itr>>,
          std::is_convertible<decltype( std::declval<Itr>() != std::declval<Snt>() ), bool>>::value>::type>
        : std::true_type {};
      template<typename Itr, typename Snt = Itr>
      using is_sized_cursor = _impl_is_sized_cursor<Itr, Snt>;
#endif

#ifdef __cpp_lib_ranges
      template<typename T>
      using is_sized_range = BoolConstant<std::ranges::sized_range<T>>;
#else
      template<typename T, typename = void>
      struct _impl_is_sized_range : std::false_type {};
      template<typename T, types::Size N>
      struct _impl_is_sized_range<T[N], void> : std::true_type {};
      template<typename T>
      struct _impl_is_sized_range<
        T,
        typename std::enable_if<
          AllOf<Not<std::is_reference<T>>,
                std::is_convertible<decltype( std::declval<T>().begin() != std::declval<T>().end() ), bool>,
                Not<std::is_void<decltype( std::declval<T>().size() )>>>::value>::type> : std::true_type {};
      template<typename T>
      using is_sized_range = _impl_is_sized_range<T>;
#endif

#ifdef __cpp_lib_three_way_comparison
      template<typename Trait>
      struct ComparisonCategory {
      private:
        template<typename T>
        static constexpr auto check( int ) -> typename T::ComparisonCategory;
        template<typename>
        static constexpr std::weak_ordering check( ... );

      public:
        using type = decltype( check<Trait>( 0 ) );
      };
      template<typename Trait>
      using ComparisonCategory_t = typename ComparisonCategory<Trait>::type;
#endif
    } // namespace traits
  } // namespace details
} // namespace pace

#endif
