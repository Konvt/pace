#ifndef PACE_TRACKED_SPAN
#define PACE_TRACKED_SPAN

#include "../details/traits/ConceptTraits.hpp"
#include "../details/traits/Util.hpp"
#include "../prefab/BasicBar.hpp"
#ifdef __cpp_lib_ranges
# include <ranges>
#endif

namespace pace {
  namespace slice {
    /**
     * A range that contains a bar object and an unidirectional abstract view of range,
     * which transforms the iterations in the abstract into a visual display of the object.
     */
    template<typename View, typename UIRef>
    class TrackedSpan
#ifdef __cpp_lib_ranges
      : public std::ranges::view_interface<TrackedSpan<View, UIRef>>
#endif
    {
      static_assert( details::traits::is_sized_range<View>::value, "only available for bounded ranges" );
      static_assert( details::traits::AllOf<std::is_copy_constructible<UIRef>,
                                            details::traits::is_pointer_like<UIRef>>::value,
                     "must be a copyable pointer-like bar reference" );
      static_assert(
        details::traits::is_iterable_bar<details::traits::PointeeOf_t<UIRef>>::value,
        "must have a method to configure the iteration count for the object's configuration type" );

      UIRef ui_;
      View view_;

      using Itr = details::traits::IteratorOf_t<View>;
      using Snt = details::traits::SentinelOf_t<View>;

      class Sentry {
        UIRef ui_;
        Snt snt_;

      public:
        constexpr Sentry( Snt endpoint, UIRef ui_ref )
          noexcept( details::traits::AllOf<std::is_nothrow_move_constructible<Snt>,
                                           std::is_nothrow_move_constructible<UIRef>>::value )
          : ui_ { std::move( ui_ref ) }, snt_ { std::move( endpoint ) }
        {}
        constexpr Snt base() const noexcept { return snt_; }
      };

    public:
      class iterator {
        UIRef ui_;
        Itr itr_;

      public:
        using iterator_category = typename std::conditional<
          std::is_same<details::traits::IterCategory_t<Itr>, std::output_iterator_tag>::value,
          details::traits::IterCategory_t<Itr>,
          std::input_iterator_tag>::type;
        using value_type      = details::traits::IterValue_t<Itr>;
        using difference_type = details::traits::IterDifference_t<Itr>;
        using reference       = details::traits::IterReference_t<Itr>;
        using pointer         = Itr;

        constexpr iterator() = default;
        PACE__CXX17_CNSTXPR iterator( Itr itr, UIRef ui_ref )
          noexcept( details::traits::AllOf<std::is_nothrow_move_constructible<Itr>,
                                           std::is_nothrow_move_constructible<UIRef>>::value )
          : ui_ { std::move( ui_ref ) }, itr_ { std::move( itr ) }
        {}
        PACE__CXX17_CNSTXPR iterator( iterator&& rhs )
          noexcept( details::traits::AllOf<std::is_nothrow_move_constructible<View>,
                                           std::is_nothrow_move_constructible<UIRef>>::value )
          : iterator( std::move( rhs.itr_ ), std::move( rhs.ui_ ) )
        {}
        PACE__CXX17_CNSTXPR iterator& operator=( iterator&& rhs ) & noexcept(
          details::traits::AllOf<std::is_nothrow_move_assignable<View>,
                                 std::is_nothrow_move_assignable<UIRef>>::value )
        {
          PACE__TRUST( this != &rhs );
          itr_ = std::move( rhs.itr_ );
          ui_  = std::move( rhs.ui_ );
          return *this;
        }
        PACE__CXX20_CNSTXPR ~iterator() = default;

        PACE__FORCEINLINE PACE__CXX14_CNSTXPR iterator& operator++() &
        {
          itr_ = std::next( itr_, 1 );
          ui_->tick();
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

        PACE__NODISCARD friend PACE__FORCEINLINE constexpr bool operator==( const iterator& a, const Itr& b )
        {
          return a.itr_ == b;
        }
        PACE__NODISCARD friend PACE__FORCEINLINE constexpr bool operator==( const Itr& a, const iterator& b )
        {
          return b == a;
        }
        PACE__NODISCARD friend PACE__FORCEINLINE constexpr bool operator!=( const iterator& a, const Itr& b )
        {
          return a.itr_ != b;
        }
        PACE__NODISCARD friend PACE__FORCEINLINE constexpr bool operator!=( const Itr& a, const iterator& b )
        {
          return b != a;
        }
        template<typename S>
        PACE__NODISCARD friend PACE__FORCEINLINE constexpr typename std::enable_if<
          details::traits::AllOf<std::is_same<S, Snt>, details::traits::Not<std::is_same<Itr, Snt>>>::value,
          bool>::type
          operator==( const iterator& a, const S& b )
        {
          return a.itr_ == b;
        }
        template<typename S>
        PACE__NODISCARD friend PACE__FORCEINLINE constexpr typename std::enable_if<
          details::traits::AllOf<std::is_same<S, Snt>, details::traits::Not<std::is_same<Itr, Snt>>>::value,
          bool>::type
          operator==( const S& ir, const iterator& itr )
        {
          return itr == ir;
        }
        template<typename S>
        PACE__NODISCARD friend PACE__FORCEINLINE constexpr typename std::enable_if<
          details::traits::AllOf<std::is_same<S, Snt>, details::traits::Not<std::is_same<Itr, Snt>>>::value,
          bool>::type
          operator!=( const iterator& itr, const S& ir )
        {
          return !( itr == ir );
        }
        template<typename S>
        PACE__NODISCARD friend PACE__FORCEINLINE constexpr typename std::enable_if<
          details::traits::AllOf<std::is_same<S, Snt>, details::traits::Not<std::is_same<Itr, Snt>>>::value,
          bool>::type
          operator!=( const S& snt, const iterator& itr )
        {
          return !( itr == snt );
        }
        PACE__NODISCARD friend PACE__FORCEINLINE constexpr bool operator==( const iterator& a,
                                                                            const iterator& b )
        {
          return a.itr_ == b.itr_;
        }
        PACE__NODISCARD friend PACE__FORCEINLINE constexpr bool operator!=( const iterator& a,
                                                                            const iterator& b )
        {
          return !( a == b );
        }
        PACE__NODISCARD friend PACE__FORCEINLINE constexpr bool operator==( const iterator& a,
                                                                            const Sentry& b )
        {
          return a.itr_ == b.base();
        }
        PACE__NODISCARD friend PACE__FORCEINLINE constexpr bool operator!=( const iterator& a,
                                                                            const Sentry& b )
        {
          return !( a == b );
        }
        PACE__NODISCARD friend PACE__FORCEINLINE constexpr bool operator==( const Sentry& a,
                                                                            const iterator& b )
        {
          return b == a;
        }
        PACE__NODISCARD friend PACE__FORCEINLINE constexpr bool operator!=( const Sentry& a,
                                                                            const iterator& b )
        {
          return !( b == a );
        }

        explicit constexpr operator bool() const noexcept { return static_cast<bool>( ui_ ); }
      };
#if PACE__CXX17
      using sentinel = std::conditional_t<std::is_same_v<Itr, Snt>, iterator, Sentry>;
#else
      using sentinel = iterator;
#endif

      constexpr TrackedSpan() = default;
      PACE__CXX17_CNSTXPR TrackedSpan( View view, UIRef ui )
        noexcept( details::traits::AllOf<std::is_nothrow_move_constructible<View>,
                                         std::is_nothrow_move_constructible<UIRef>>::value )
        : ui_ { std::move( ui ) }, view_ { std::move( view ) }
      {}
      PACE__CXX17_CNSTXPR TrackedSpan( TrackedSpan&& rhs )
        noexcept( details::traits::AllOf<std::is_nothrow_move_constructible<View>,
                                         std::is_nothrow_move_constructible<UIRef>>::value )
        : TrackedSpan( std::move( rhs.view_ ), std::move( rhs.ui_ ) )
      {}
      PACE__CXX17_CNSTXPR TrackedSpan& operator=( TrackedSpan&& rhs ) & noexcept(
        details::traits::AllOf<std::is_nothrow_move_constructible<View>,
                               std::is_nothrow_move_constructible<UIRef>>::value )
      {
        PACE__TRUST( this != &rhs );
        view_ = std::move( rhs.view_ );
        ui_   = std::move( rhs.ui_ );
        return *this;
      }
      // Intentional non-virtual destructors.
      PACE__CXX20_CNSTXPR ~TrackedSpan() = default;

      PACE__CXX14_CNSTXPR View replace( View view ) & noexcept(
        details::traits::AllOf<std::is_nothrow_move_constructible<View>,
                               std::is_nothrow_move_assignable<View>>::value )
      {
        return details::utils::exchange( view_, view );
      }
      PACE__CXX14_CNSTXPR UIRef replace( UIRef ui ) & noexcept(
        details::traits::AllOf<std::is_nothrow_move_constructible<UIRef>,
                               std::is_nothrow_move_assignable<UIRef>>::value )
      {
        return details::utils::exchange( ui_, ui );
      }

      PACE__NODISCARD PACE__FORCEINLINE PACE__CXX17_CNSTXPR bool empty() const noexcept
      {
        return view_.empty() && static_cast<bool>( ui_ );
      }
      // This function will CHANGE the state of the pace object it holds.
      PACE__NODISCARD PACE__FORCEINLINE PACE__CXX17_CNSTXPR iterator begin() &
      {
        ui_->config().quota( details::utils::size( view_ ) );
        return { details::utils::begin( view_ ), ui_ };
      }
      PACE__NODISCARD PACE__FORCEINLINE PACE__CXX17_CNSTXPR sentinel end() const
      {
        return { details::utils::end( view_ ), ui_ };
      }

      PACE__CXX14_CNSTXPR void swap( TrackedSpan& lhs ) noexcept
      {
        PACE__TRUST( this != &lhs );
        using std::swap;
        std::swap( ui_, lhs.ui_ );
        swap( view_, lhs.view_ );
      }
      friend PACE__CXX14_CNSTXPR void swap( TrackedSpan& a, TrackedSpan& b ) noexcept { a.swap( b ); }

      explicit constexpr operator bool() const noexcept { return !empty(); }
    };
  } // namespace slice
} // namespace pace

#endif
