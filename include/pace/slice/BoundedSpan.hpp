#ifndef PACE_BOUNDEDSPAN
#define PACE_BOUNDEDSPAN

#include "../details/traits/ConceptTraits.hpp"
#include "../details/utils/Backport.hpp"
#ifdef __cpp_lib_ranges
# include <ranges>
#endif

namespace pace {
  namespace slice {
    template<typename R>
    class BoundedSpan
#ifdef __cpp_lib_ranges
      : public std::ranges::view_interface<BoundedSpan<R>>
#endif
    {
#ifdef __cpp_lib_ranges
      static_assert( details::traits::is_bounded_range<R>::value && !std::ranges::view<R>,
                     "only available for bounded ranges, excluding view types" );
#else
      static_assert( details::traits::is_bounded_range<R>::value, "only available for bounded ranges" );
#endif

      R* rnge_;

    public:
      using iterator = details::traits::IteratorOf_t<R>;
      using sentinel = details::traits::SentinelOf_t<R>;

      PACE__CXX17_CNSTXPR BoundedSpan( R& rnge ) noexcept : rnge_ { std::addressof( rnge ) } {}

      PACE__CXX14_CNSTXPR BoundedSpan( const BoundedSpan& )              = default;
      PACE__CXX14_CNSTXPR BoundedSpan& operator=( const BoundedSpan& ) & = default;
      PACE__CXX20_CNSTXPR ~BoundedSpan()                                 = default;

      PACE__NODISCARD PACE__FORCEINLINE PACE__CXX14_CNSTXPR iterator begin() const
        noexcept( noexcept( details::utils::begin( *rnge_ ) ) )
      {
        return details::utils::begin( *rnge_ );
      }
      PACE__NODISCARD PACE__FORCEINLINE PACE__CXX14_CNSTXPR sentinel end() const
        noexcept( noexcept( details::utils::end( *rnge_ ) ) )
      {
        return details::utils::end( *rnge_ );
      }

      PACE__NODISCARD PACE__FORCEINLINE PACE__CXX14_CNSTXPR details::traits::IterReference_t<iterator> front()
        const
      {
        return *begin();
      }
      PACE__NODISCARD PACE__FORCEINLINE PACE__CXX17_CNSTXPR details::traits::IterReference_t<iterator> back()
        const
      {
        return *std::next( begin(), size() - 1 );
      }
      PACE__NODISCARD PACE__FORCEINLINE constexpr details::types::Size step() const noexcept { return 1; }
      PACE__NODISCARD PACE__FORCEINLINE constexpr details::types::Size size() const
      {
        return details::utils::size( *rnge_ );
      }
      PACE__NODISCARD PACE__FORCEINLINE constexpr bool empty() const noexcept { return rnge_ == nullptr; }

      PACE__CXX20_CNSTXPR void swap( BoundedSpan& lhs ) noexcept
      {
        PACE__TRUST( this != &lhs );
        std::swap( rnge_, lhs.rnge_ );
      }
      friend PACE__CXX20_CNSTXPR void swap( BoundedSpan& a, BoundedSpan& b ) noexcept { a.swap( b ); }

      explicit constexpr operator bool() const noexcept { return !empty(); }
    };
  } // namespace slice
} // namespace pace

#endif
