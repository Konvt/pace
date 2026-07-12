#ifndef PACE_SIZED_SPAN
#define PACE_SIZED_SPAN

#include "../details/traits/Concept.hpp"
#include "../details/utils/Backport.hpp"
#ifdef __cpp_lib_ranges
# include <ranges>
#endif

namespace pace {
  namespace slice {
    template<typename R>
    class SizedSpan
#ifdef __cpp_lib_ranges
      : public std::ranges::view_interface<SizedSpan<R>>
#endif
    {
#ifdef __cpp_lib_ranges
      static_assert( details::traits::is_sized_range<R>::value && !std::ranges::view<R>,
                     "only available for bounded ranges, excluding view types" );
#else
      static_assert( details::traits::is_sized_range<R>::value, "only available for bounded ranges" );
#endif

      R* rnge_ = nullptr;

    public:
      using iterator = details::traits::IteratorOf_t<R>;
      using sentinel = details::traits::SentinelOf_t<R>;

      PACE__CXX17_CNSTXPR SizedSpan( R& rnge ) noexcept : rnge_ { std::addressof( rnge ) } {}
      PACE__CXX20_CNSTXPR ~SizedSpan() = default;

      PACE__FORCEINLINE PACE__CXX14_CNSTXPR R& base() const noexcept { return *rnge_; }

      PACE__NODISCARD PACE__FORCEINLINE PACE__CXX14_CNSTXPR iterator begin() const
        noexcept( noexcept( details::utils::begin( *rnge_ ) ) )
      { return details::utils::begin( *rnge_ ); }
      PACE__NODISCARD PACE__FORCEINLINE PACE__CXX14_CNSTXPR sentinel end() const
        noexcept( noexcept( details::utils::end( *rnge_ ) ) )
      { return details::utils::end( *rnge_ ); }

      PACE__NODISCARD PACE__FORCEINLINE constexpr details::types::Size size() const
      { return details::utils::size( *rnge_ ); }
      PACE__NODISCARD PACE__FORCEINLINE constexpr bool empty() const noexcept
      { return details::utils::empty( *rnge_ ); }

      PACE__CXX20_CNSTXPR void swap( SizedSpan& lhs ) noexcept
      {
        PACE__TRUST( this != &lhs );
        std::swap( rnge_, lhs.rnge_ );
      }
      friend PACE__CXX20_CNSTXPR void swap( SizedSpan& a, SizedSpan& b ) noexcept { a.swap( b ); }

      explicit constexpr operator bool() const noexcept { return !empty(); }
    };
  } // namespace slice
} // namespace pace

#endif
