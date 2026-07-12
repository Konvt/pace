#ifndef PACE_ITERATOR_SPAN
#define PACE_ITERATOR_SPAN

#include "../details/traits/Concept.hpp"
#include "../details/utils/Backport.hpp"
#include "../exception/Error.hpp"
#ifdef __cpp_lib_ranges
# include <ranges>
#endif

namespace pace {
  namespace slice {
    /**
     * An undirectional range delimited by a pair of iterators, including pointer types.
     *
     * Accepted iterator types must satisfy subtractable.
     */
    template<typename Itr, typename Snt = Itr>
    class IteratorSpan
#ifdef __cpp_lib_ranges
      : public std::ranges::view_interface<IteratorSpan<Itr, Snt>>
#endif
    {
      static_assert( details::traits::is_sized_cursor<Itr, Snt>::value,
                     "only available for sized iterator and sentinel pair" );
      static_assert( std::is_convertible<details::traits::IterDifference_t<Itr>, details::types::Size>::value,
                     "the difference_type must be convertible to Size" );

      details::types::Size size_ = 0;
      Itr start_;
      Snt end_;

    public:
      using iterator = Itr;
      using sentinel = Snt;

      PACE__CXX20_CNSTXPR IteratorSpan( Itr startpoint, Snt endpoint )
        : start_ { std::move( startpoint ) }, end_ { std::move( endpoint ) }
      {
        const auto length = details::utils::distance( start_, end_ );
        if ( length < 0 )
          PACE__UNLIKELY throw exception::InvalidArgument(
            details::charcodes::make_literal( "pace: negative iterator range" ) );
        size_ = static_cast<details::types::Size>( length );
      }
      PACE__CXX20_CNSTXPR ~IteratorSpan() = default;

      PACE__NODISCARD PACE__FORCEINLINE constexpr iterator begin() const
        noexcept( std::is_nothrow_copy_constructible<iterator>::value )
      { return start_; }
      PACE__NODISCARD PACE__FORCEINLINE constexpr sentinel end() const
        noexcept( std::is_nothrow_copy_constructible<sentinel>::value )
      { return end_; }

      PACE__NODISCARD PACE__FORCEINLINE constexpr details::types::Size size() const noexcept { return size_; }
      PACE__NODISCARD PACE__FORCEINLINE constexpr bool empty() const noexcept { return size_ == 0; }

      PACE__CXX20_CNSTXPR void swap( IteratorSpan& lhs ) noexcept
      {
        PACE__TRUST( this != &lhs );
        using std::swap;
        swap( start_, lhs.start_ );
        swap( end_, lhs.end_ );
        swap( size_, lhs.size_ );
      }
      friend PACE__CXX20_CNSTXPR void swap( IteratorSpan& a, IteratorSpan& b ) noexcept { a.swap( b ); }

      explicit constexpr operator bool() const noexcept { return !empty(); }
    };
  } // namespace slice
} // namespace pace

#endif
