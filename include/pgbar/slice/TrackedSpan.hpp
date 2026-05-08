#ifndef PGBAR_TRACKEDSPAN
#define PGBAR_TRACKEDSPAN

#include "../details/prefabs/BasicBar.hpp"
#include "../details/traits/ConceptTraits.hpp"
#include "../details/traits/Util.hpp"
#ifdef __cpp_lib_ranges
# include <ranges>
#endif

namespace pgbar {
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
      static_assert( _details::traits::is_bounded_range<View>::value, "only available for bounded ranges" );
      static_assert( _details::traits::AllOf<std::is_copy_constructible<UIRef>,
                                             _details::traits::is_pointer_like<UIRef>>::value,
                     "must be a copyable pointer-like bar reference" );
      static_assert(
        _details::traits::is_iterable_bar<_details::traits::PointeeOf_t<UIRef>>::value,
        "must have a method to configure the iteration count for the object's configuration type" );

      UIRef ui_;
      View view_;

      using Itr = _details::traits::IteratorOf_t<View>;
      using Snt = _details::traits::SentinelOf_t<View>;

      class Sentry {
        UIRef ui_;
        Snt snt_;

      public:
        constexpr Sentry( Snt endpoint, UIRef ui_ref )
          noexcept( _details::traits::AllOf<std::is_nothrow_move_constructible<Snt>,
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
          std::is_same<_details::traits::IterCategory_t<Itr>, std::output_iterator_tag>::value,
          _details::traits::IterCategory_t<Itr>,
          std::input_iterator_tag>::type;
        using value_type      = _details::traits::IterValue_t<Itr>;
        using difference_type = _details::traits::IterDifference_t<Itr>;
        using reference       = _details::traits::IterReference_t<Itr>;
        using pointer         = Itr;

        constexpr iterator() = default;
        PGBAR__CXX17_CNSTXPR iterator( Itr itr, UIRef ui_ref )
          noexcept( _details::traits::AllOf<std::is_nothrow_move_constructible<Itr>,
                                            std::is_nothrow_move_constructible<UIRef>>::value )
          : ui_ { std::move( ui_ref ) }, itr_ { std::move( itr ) }
        {}
        PGBAR__CXX17_CNSTXPR iterator( iterator&& rhs )
          noexcept( _details::traits::AllOf<std::is_nothrow_move_constructible<View>,
                                            std::is_nothrow_move_constructible<UIRef>>::value )
          : iterator( std::move( rhs.itr_ ), std::move( rhs.ui_ ) )
        {}
        PGBAR__CXX17_CNSTXPR iterator& operator=( iterator&& rhs ) & noexcept(
          _details::traits::AllOf<std::is_nothrow_move_assignable<View>,
                                  std::is_nothrow_move_assignable<UIRef>>::value )
        {
          PGBAR__TRUST( this != &rhs );
          itr_ = std::move( rhs.itr_ );
          ui_  = std::move( rhs.ui_ );
          return *this;
        }
        PGBAR__CXX20_CNSTXPR ~iterator() = default;

        PGBAR__FORCEINLINE PGBAR__CXX14_CNSTXPR iterator& operator++() &
        {
          itr_ = std::next( itr_, 1 );
          ui_->tick();
          return *this;
        }
        PGBAR__NODISCARD PGBAR__FORCEINLINE PGBAR__CXX14_CNSTXPR iterator operator++( int ) &
        {
          auto before = *this;
          operator++();
          return before;
        }

        PGBAR__NODISCARD PGBAR__FORCEINLINE PGBAR__CXX14_CNSTXPR reference operator*() const { return *itr_; }
        PGBAR__FORCEINLINE PGBAR__CXX17_CNSTXPR pointer operator->() const { return itr_; }

        PGBAR__NODISCARD friend PGBAR__FORCEINLINE constexpr bool operator==( const iterator& a,
                                                                              const Itr& b )
        {
          return a.itr_ == b;
        }
        PGBAR__NODISCARD friend PGBAR__FORCEINLINE constexpr bool operator==( const Itr& a,
                                                                              const iterator& b )
        {
          return b == a;
        }
        PGBAR__NODISCARD friend PGBAR__FORCEINLINE constexpr bool operator!=( const iterator& a,
                                                                              const Itr& b )
        {
          return a.itr_ != b;
        }
        PGBAR__NODISCARD friend PGBAR__FORCEINLINE constexpr bool operator!=( const Itr& a,
                                                                              const iterator& b )
        {
          return b != a;
        }
        template<typename S>
        PGBAR__NODISCARD friend PGBAR__FORCEINLINE constexpr typename std::enable_if<
          _details::traits::AllOf<std::is_same<S, Snt>, _details::traits::Not<std::is_same<Itr, Snt>>>::value,
          bool>::type
          operator==( const iterator& a, const S& b )
        {
          return a.itr_ == b;
        }
        template<typename S>
        PGBAR__NODISCARD friend PGBAR__FORCEINLINE constexpr typename std::enable_if<
          _details::traits::AllOf<std::is_same<S, Snt>, _details::traits::Not<std::is_same<Itr, Snt>>>::value,
          bool>::type
          operator==( const S& ir, const iterator& itr )
        {
          return itr == ir;
        }
        template<typename S>
        PGBAR__NODISCARD friend PGBAR__FORCEINLINE constexpr typename std::enable_if<
          _details::traits::AllOf<std::is_same<S, Snt>, _details::traits::Not<std::is_same<Itr, Snt>>>::value,
          bool>::type
          operator!=( const iterator& itr, const S& ir )
        {
          return !( itr == ir );
        }
        template<typename S>
        PGBAR__NODISCARD friend PGBAR__FORCEINLINE constexpr typename std::enable_if<
          _details::traits::AllOf<std::is_same<S, Snt>, _details::traits::Not<std::is_same<Itr, Snt>>>::value,
          bool>::type
          operator!=( const S& snt, const iterator& itr )
        {
          return !( itr == snt );
        }
        PGBAR__NODISCARD friend PGBAR__FORCEINLINE constexpr bool operator==( const iterator& a,
                                                                              const iterator& b )
        {
          return a.itr_ == b.itr_;
        }
        PGBAR__NODISCARD friend PGBAR__FORCEINLINE constexpr bool operator!=( const iterator& a,
                                                                              const iterator& b )
        {
          return !( a == b );
        }
        PGBAR__NODISCARD friend PGBAR__FORCEINLINE constexpr bool operator==( const iterator& a,
                                                                              const Sentry& b )
        {
          return a.itr_ == b.base();
        }
        PGBAR__NODISCARD friend PGBAR__FORCEINLINE constexpr bool operator!=( const iterator& a,
                                                                              const Sentry& b )
        {
          return !( a == b );
        }
        PGBAR__NODISCARD friend PGBAR__FORCEINLINE constexpr bool operator==( const Sentry& a,
                                                                              const iterator& b )
        {
          return b == a;
        }
        PGBAR__NODISCARD friend PGBAR__FORCEINLINE constexpr bool operator!=( const Sentry& a,
                                                                              const iterator& b )
        {
          return !( b == a );
        }

        explicit constexpr operator bool() const noexcept { return static_cast<bool>( ui_ ); }
      };
#if PGBAR__CXX17
      using sentinel = std::conditional_t<std::is_same_v<Itr, Snt>, iterator, Sentry>;
#else
      using sentinel = iterator;
#endif

      constexpr TrackedSpan() = default;
      PGBAR__CXX17_CNSTXPR TrackedSpan( View view, UIRef ui )
        noexcept( _details::traits::AllOf<std::is_nothrow_move_constructible<View>,
                                          std::is_nothrow_move_constructible<UIRef>>::value )
        : ui_ { std::move( ui ) }, view_ { std::move( view ) }
      {}
      PGBAR__CXX17_CNSTXPR TrackedSpan( TrackedSpan&& rhs )
        noexcept( _details::traits::AllOf<std::is_nothrow_move_constructible<View>,
                                          std::is_nothrow_move_constructible<UIRef>>::value )
        : TrackedSpan( std::move( rhs.view_ ), std::move( rhs.ui_ ) )
      {}
      PGBAR__CXX17_CNSTXPR TrackedSpan& operator=( TrackedSpan&& rhs ) & noexcept(
        _details::traits::AllOf<std::is_nothrow_move_constructible<View>,
                                std::is_nothrow_move_constructible<UIRef>>::value )
      {
        PGBAR__TRUST( this != &rhs );
        view_ = std::move( rhs.view_ );
        ui_   = std::move( rhs.ui_ );
        return *this;
      }
      // Intentional non-virtual destructors.
      PGBAR__CXX20_CNSTXPR ~TrackedSpan() = default;

      PGBAR__CXX14_CNSTXPR View replace( View view ) & noexcept(
        _details::traits::AllOf<std::is_nothrow_move_constructible<View>,
                                std::is_nothrow_move_assignable<View>>::value )
      {
        return _details::utils::exchange( view_, view );
      }
      PGBAR__CXX14_CNSTXPR UIRef replace( UIRef ui ) & noexcept(
        _details::traits::AllOf<std::is_nothrow_move_constructible<UIRef>,
                                std::is_nothrow_move_assignable<UIRef>>::value )
      {
        return _details::utils::exchange( ui_, ui );
      }

      PGBAR__NODISCARD PGBAR__FORCEINLINE PGBAR__CXX17_CNSTXPR bool empty() const noexcept
      {
        return view_.empty() && static_cast<bool>( ui_ );
      }
      // This function will CHANGE the state of the pgbar object it holds.
      PGBAR__NODISCARD PGBAR__FORCEINLINE PGBAR__CXX17_CNSTXPR iterator begin() &
      {
        ui_->config().quota( _details::utils::size( view_ ) );
        return { _details::utils::begin( view_ ), ui_ };
      }
      PGBAR__NODISCARD PGBAR__FORCEINLINE PGBAR__CXX17_CNSTXPR sentinel end() const
      {
        return { _details::utils::end( view_ ), ui_ };
      }

      PGBAR__CXX14_CNSTXPR void swap( TrackedSpan& lhs ) noexcept
      {
        PGBAR__TRUST( this != &lhs );
        using std::swap;
        std::swap( ui_, lhs.ui_ );
        swap( view_, lhs.view_ );
      }
      friend PGBAR__CXX14_CNSTXPR void swap( TrackedSpan& a, TrackedSpan& b ) noexcept { a.swap( b ); }

      explicit constexpr operator bool() const noexcept { return !empty(); }
    };
  } // namespace slice
} // namespace pgbar

#endif
