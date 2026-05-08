#ifndef PGBAR_BOUNDEDSPAN
#define PGBAR_BOUNDEDSPAN

#include "../details/traits/ConceptTraits.hpp"
#include "../details/utils/Backport.hpp"
#ifdef __cpp_lib_ranges
# include <ranges>
#endif

namespace pgbar {
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

      PGBAR__CXX17_CNSTXPR BoundedSpan( R& rnge ) noexcept : rnge_ { std::addressof( rnge ) } {}

      PGBAR__CXX14_CNSTXPR BoundedSpan( const BoundedSpan& )              = default;
      PGBAR__CXX14_CNSTXPR BoundedSpan& operator=( const BoundedSpan& ) & = default;
      PGBAR__CXX20_CNSTXPR ~BoundedSpan()                                 = default;

      PGBAR__NODISCARD PGBAR__FORCEINLINE PGBAR__CXX14_CNSTXPR iterator begin() const
        noexcept( noexcept( details::utils::begin( *rnge_ ) ) )
      {
        return details::utils::begin( *rnge_ );
      }
      PGBAR__NODISCARD PGBAR__FORCEINLINE PGBAR__CXX14_CNSTXPR sentinel end() const
        noexcept( noexcept( details::utils::end( *rnge_ ) ) )
      {
        return details::utils::end( *rnge_ );
      }

      PGBAR__NODISCARD PGBAR__FORCEINLINE PGBAR__CXX14_CNSTXPR details::traits::IterReference_t<iterator>
        front() const
      {
        return *begin();
      }
      PGBAR__NODISCARD PGBAR__FORCEINLINE PGBAR__CXX17_CNSTXPR details::traits::IterReference_t<iterator>
        back() const
      {
        return *std::next( begin(), size() - 1 );
      }
      PGBAR__NODISCARD PGBAR__FORCEINLINE constexpr details::types::Size step() const noexcept { return 1; }
      PGBAR__NODISCARD PGBAR__FORCEINLINE constexpr details::types::Size size() const
      {
        return details::utils::size( *rnge_ );
      }
      PGBAR__NODISCARD PGBAR__FORCEINLINE constexpr bool empty() const noexcept { return rnge_ == nullptr; }

      PGBAR__CXX20_CNSTXPR void swap( BoundedSpan& lhs ) noexcept
      {
        PGBAR__TRUST( this != &lhs );
        std::swap( rnge_, lhs.rnge_ );
      }
      friend PGBAR__CXX20_CNSTXPR void swap( BoundedSpan& a, BoundedSpan& b ) noexcept { a.swap( b ); }

      explicit constexpr operator bool() const noexcept { return !empty(); }
    };
  } // namespace slice
} // namespace pgbar

#endif
