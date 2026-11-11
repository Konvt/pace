#ifndef PACE_TRACKED_SPAN
#define PACE_TRACKED_SPAN

#include "../Indicator.hpp"
#include "../details/traits/Concept.hpp"
#include "../details/traits/Util.hpp"
#include "../details/utils/Backport.hpp"
#ifdef __cpp_lib_ranges
# include <ranges>
#endif

namespace pace {
  namespace details {
    namespace wrappers {
      template<typename View, typename Storage, typename Derived>
      class Visualizer
#ifdef __cpp_lib_ranges
        : public std::ranges::view_interface<Visualizer<View, Storage, Derived>>
#endif
      {
        static_assert( traits::is_sized_range<View>::value, "only available for sized ranges" );

        using UnderlyingIterator = traits::IteratorOf_t<View>;
        using UnderlyingSentinel = traits::SentinelOf_t<View>;

        class Sentinel {
          friend Visualizer;

          UnderlyingSentinel endpoint_;

          constexpr Sentinel( UnderlyingSentinel endpoint, const Storage& )
            noexcept( std::is_nothrow_move_constructible<UnderlyingSentinel>::value )
            : endpoint_ { std::move( endpoint ) }
          {}

        public:
          constexpr UnderlyingSentinel base() const
            noexcept( std::is_nothrow_copy_constructible<UnderlyingSentinel>::value )
          { return endpoint_; }
        };

      protected:
        View view_;
        Storage automat_;

      public:
        class iterator {
          friend Visualizer;
          using Monitor = typename Derived::Monitor;

          UnderlyingIterator itr_;
          Monitor ui_;

          template<typename U>
          constexpr iterator( UnderlyingIterator itr, U& ui )
            noexcept( std::is_nothrow_move_constructible<UnderlyingIterator>::value )
            : itr_ { std::move( itr ) }, ui_ { Derived::materialize( ui ) }
          {}

        public:
          using iterator_category = typename std::conditional<
            std::is_same<traits::IterCategory_t<UnderlyingIterator>, std::output_iterator_tag>::value,
            traits::IterCategory_t<UnderlyingIterator>,
            std::input_iterator_tag>::type;
          using value_type      = traits::IterValue_t<UnderlyingIterator>;
          using difference_type = traits::IterDifference_t<UnderlyingIterator>;
          using reference       = traits::IterReference_t<UnderlyingIterator>;
          using pointer         = traits::IterPointer_t<UnderlyingIterator>;

          ~iterator() = default;

          PACE__FORCEINLINE PACE__CXX14_CNSTXPR iterator& operator++() &
          {
            itr_ = std::next( itr_ );
            Derived::on_increment( ui_ );
            return *this;
          }
          PACE__NODISCARD PACE__FORCEINLINE PACE__CXX14_CNSTXPR iterator operator++( int ) &
          {
            auto before = *this;
            operator++();
            return before;
          }

          PACE__NODISCARD PACE__FORCEINLINE PACE__CXX14_CNSTXPR reference operator*() const { return *itr_; }
          PACE__FORCEINLINE PACE__CXX17_CNSTXPR pointer operator->() const { return itr_; }

          PACE__NODISCARD PACE__FORCEINLINE friend constexpr bool operator==( const iterator& a,
                                                                              const UnderlyingIterator& b )
          { return a.itr_ == b; }
          PACE__NODISCARD PACE__FORCEINLINE friend constexpr bool operator==( const UnderlyingIterator& a,
                                                                              const iterator& b )
          { return b == a; }
          PACE__NODISCARD PACE__FORCEINLINE friend constexpr bool operator!=( const iterator& a,
                                                                              const UnderlyingIterator& b )
          { return a.itr_ != b; }
          PACE__NODISCARD PACE__FORCEINLINE friend constexpr bool operator!=( const UnderlyingIterator& a,
                                                                              const iterator& b )
          { return b != a; }
          template<typename S>
          PACE__NODISCARD PACE__FORCEINLINE friend constexpr typename std::enable_if<
            traits::AllOf<std::is_same<S, UnderlyingSentinel>,
                          traits::Not<std::is_same<UnderlyingIterator, UnderlyingSentinel>>>::value,
            bool>::type
            operator==( const iterator& a, const S& b )
          { return a.itr_ == b; }
          template<typename S>
          PACE__NODISCARD PACE__FORCEINLINE friend constexpr typename std::enable_if<
            traits::AllOf<std::is_same<S, UnderlyingSentinel>,
                          traits::Not<std::is_same<UnderlyingIterator, UnderlyingSentinel>>>::value,
            bool>::type
            operator==( const S& ir, const iterator& itr )
          { return itr == ir; }
          template<typename S>
          PACE__NODISCARD PACE__FORCEINLINE friend constexpr typename std::enable_if<
            traits::AllOf<std::is_same<S, UnderlyingSentinel>,
                          traits::Not<std::is_same<UnderlyingIterator, UnderlyingSentinel>>>::value,
            bool>::type
            operator!=( const iterator& itr, const S& ir )
          { return !( itr == ir ); }
          template<typename S>
          PACE__NODISCARD PACE__FORCEINLINE friend constexpr typename std::enable_if<
            traits::AllOf<std::is_same<S, UnderlyingSentinel>,
                          traits::Not<std::is_same<UnderlyingIterator, UnderlyingSentinel>>>::value,
            bool>::type
            operator!=( const S& snt, const iterator& itr )
          { return !( itr == snt ); }
          PACE__NODISCARD PACE__FORCEINLINE friend constexpr bool operator==( const iterator& a,
                                                                              const iterator& b )
          { return a.itr_ == b.itr_; }
          PACE__NODISCARD PACE__FORCEINLINE friend constexpr bool operator!=( const iterator& a,
                                                                              const iterator& b )
          { return !( a == b ); }
#if __cpp_range_based_for >= 201603L
          PACE__NODISCARD PACE__FORCEINLINE friend constexpr bool operator==( const iterator& a,
                                                                              const Sentinel& b )
          { return a.itr_ == b.base(); }
          PACE__NODISCARD PACE__FORCEINLINE friend constexpr bool operator!=( const iterator& a,
                                                                              const Sentinel& b )
          { return !( a == b ); }
          PACE__NODISCARD PACE__FORCEINLINE friend constexpr bool operator==( const Sentinel& a,
                                                                              const iterator& b )
          { return b == a; }
          PACE__NODISCARD PACE__FORCEINLINE friend constexpr bool operator!=( const Sentinel& a,
                                                                              const iterator& b )
          { return !( b == a ); }
#endif
        };

        using sentinel =
          typename std::conditional<( __cpp_range_based_for < 201603L )
                                      || std::is_same<UnderlyingIterator, UnderlyingSentinel>::value,
                                    iterator,
                                    Sentinel>::type;

        template<typename... Args>
        constexpr Visualizer( View view, Args&&... args )
          noexcept( traits::AllOf<std::is_nothrow_move_constructible<View>,
                                  std::is_nothrow_constructible<Storage, Args...>>::value )
          : view_ { std::move( view ) }, automat_ { std::forward<Args>( args )... }
        { static_assert( std::is_constructible<Storage, Args...>::value, "invalid arguments" ); }

        ~Visualizer() = default;

        // This function will CHANGE the state of the object it holds.
        PACE__NODISCARD PACE__FORCEINLINE PACE__CXX17_CNSTXPR iterator begin() &
        {
          Derived::on_ignition( automat_, utils::size( view_ ) );
          return { utils::begin( view_ ), automat_ };
        }
        PACE__NODISCARD PACE__FORCEINLINE PACE__CXX17_CNSTXPR sentinel end() const
        { return { utils::end( view_ ), automat_ }; }
      };
    } // namespace wrappers
  } // namespace details

  namespace views {
    /**
     * A range that contains a bar object and an unidirectional abstract view of range,
     * which transforms the iterations in the abstract into a visual display of the object.
     */
    template<typename View, typename Automation>
    class TrackedSpan
      : public details::wrappers::Visualizer<View, Automation, TrackedSpan<View, Automation>> {
      static_assert( details::traits::is_dereferenceable<Automation>::value,
                     "must be a dereferenceable reference" );
      static_assert( std::is_base_of<Indicator, details::traits::PointeeOf_t<Automation>>::value,
                     "the pointee must be an Indicator" );
      static_assert( !std::is_pointer<Automation>::value, "should not be a trivial type" );
      using Base = details::wrappers::Visualizer<View, Automation, TrackedSpan>;
      friend Base;

      using Monitor = Automation*;
      PACE__FORCEINLINE PACE__CXX17_CNSTXPR static Monitor materialize( Automation& automat )
      { return std::addressof( automat ); }
      PACE__FORCEINLINE constexpr static Monitor materialize( const Automation& ) { return nullptr; }
      PACE__FORCEINLINE static void on_ignition( Automation& automat, std::size_t size )
      { automat->config().quota( size ); }
      PACE__FORCEINLINE static void on_increment( Monitor automat ) { ( *automat ).tick(); }

    public:
      using Base::Base;
    };
    // This specialized version is a reference to an existing object,
    // so it should maintain the affine property.
    template<typename View, typename Automation>
    class TrackedSpan<View, Automation*>
      : public details::wrappers::Visualizer<View, Automation*, TrackedSpan<View, Automation*>> {
      static_assert( details::traits::is_dereferenceable<Automation>::value,
                     "must be a dereferenceable reference" );
      static_assert( std::is_base_of<Indicator, details::traits::PointeeOf_t<Automation>>::value,
                     "the pointee must be an Indicator" );
      static_assert( !std::is_pointer<Automation>::value, "should not be a trivial type" );
      using Base = details::wrappers::Visualizer<View, Automation*, TrackedSpan>;
      friend Base;

      using Monitor = Automation*;
      PACE__FORCEINLINE constexpr static Monitor materialize( Monitor automat ) { return automat; }
      PACE__FORCEINLINE static void on_ignition( Monitor automat, std::size_t size )
      { // If a null pointer dereference occurs here,
        // it is possible that the TrackedSpan was moved before C++17,
        // which is not supposed to happen.
        PACE__TRUST( automat != nullptr );
        ( *automat )->config().quota( size );
      }
      PACE__FORCEINLINE static void on_increment( Monitor automat ) { ( *automat ).tick(); }

    public:
      using Base::Base;

      // In principle, we should not have supported move semantics,
      // but in order to address the issue of copy-elision that does not yet exist,
      // we still need to provide move support in specific C++ standards.
#if !PACE__CXX17
      PACE__CXX14_CNSTXPR TrackedSpan( TrackedSpan&& rhs ) noexcept : Base( std::move( rhs ) )
      { rhs.automat_ = nullptr; }
#endif

      TrackedSpan( const TrackedSpan& )            = delete;
      TrackedSpan& operator=( const TrackedSpan& ) = delete;
    };
  } // namespace views
} // namespace pace

#ifdef __cpp_lib_ranges
template<typename View, typename Automation>
inline constexpr bool std::ranges::enable_borrowed_range<pace::views::TrackedSpan<View, Automation*>> =
  std::ranges::borrowed_range<View>;
#endif

#endif
