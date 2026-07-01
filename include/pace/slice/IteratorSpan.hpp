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
      static_assert(
        std::is_convertible<details::traits::iter_difference_t<Itr>, details::types::Size>::value,
        "the difference_type must be convertible to Size" );

      details::types::Size size_;
      Itr start_;
      Snt end_;

      class Sentry {
        friend class iterator;

        Snt endpoint_;

      public:
        constexpr Sentry() = default;
        constexpr Sentry( Snt&& endpoint ) noexcept( std::is_nothrow_move_constructible<Snt>::value )
          : endpoint_ { std::move( endpoint ) }
        {}
      };

    public:
      class iterator;
#if PACE__CXX17
      using sentinel = std::conditional_t<std::is_same_v<Itr, Snt>, iterator, Sentry>;
#else
      using sentinel = iterator;
#endif
      class iterator {
        Itr current_;

      public:
        using iterator_category = typename std::conditional<
          details::traits::any_of<
            std::is_same<details::traits::iter_category_t<Itr>, std::input_iterator_tag>,
            std::is_same<details::traits::iter_category_t<Itr>, std::output_iterator_tag>>::value,
          details::traits::iter_category_t<Itr>,
          std::forward_iterator_tag>::type;
        using value_type      = details::traits::iter_value_t<Itr>;
        using difference_type = details::traits::iter_difference_t<Itr>;
        using reference       = details::traits::iter_reference_t<Itr>;
        using pointer         = Itr;

        constexpr iterator() = default;
        constexpr iterator( Itr startpoint ) noexcept( std::is_nothrow_move_constructible<Itr>::value )
          : current_ { std::move( startpoint ) }
        {}
        constexpr iterator( const iterator& )                        = default;
        constexpr iterator( iterator&& )                             = default;
        PACE__CXX14_CNSTXPR iterator& operator=( const iterator& ) & = default;
        PACE__CXX14_CNSTXPR iterator& operator=( iterator&& ) &      = default;
        PACE__CXX20_CNSTXPR ~iterator()                              = default;

        PACE__FORCEINLINE PACE__CXX14_CNSTXPR iterator& operator++() &
        {
          current_ = std::next( current_, 1 );
          return *this;
        }
        PACE__NODISCARD PACE__FORCEINLINE PACE__CXX14_CNSTXPR iterator operator++( int ) &
        {
          auto before = *this;
          operator++();
          return before;
        }

        PACE__NODISCARD PACE__FORCEINLINE PACE__CXX14_CNSTXPR reference operator*() const
        { return *current_; }
        PACE__NODISCARD PACE__FORCEINLINE PACE__CXX17_CNSTXPR pointer operator->() const noexcept
        { return current_; }
        PACE__NODISCARD PACE__FORCEINLINE PACE__CXX17_CNSTXPR reference
          operator[]( details::types::Size inc ) const
        { return *std::next( current_, inc ); }

        PACE__NODISCARD friend PACE__FORCEINLINE constexpr bool operator==( const iterator& itr,
                                                                            const Itr& ir )
        { return itr.current_ == ir; }
        PACE__NODISCARD friend PACE__FORCEINLINE constexpr bool operator==( const Itr& ir,
                                                                            const iterator& itr )
        { return itr == ir; }
        PACE__NODISCARD friend PACE__FORCEINLINE constexpr bool operator!=( const iterator& itr,
                                                                            const Itr& ir )
        { return !( itr == ir ); }
        PACE__NODISCARD friend PACE__FORCEINLINE constexpr bool operator!=( const Itr& ir,
                                                                            const iterator& itr )
        { return !( itr == ir ); }
        template<typename S>
        PACE__NODISCARD friend PACE__FORCEINLINE constexpr typename std::enable_if<
          details::traits::all_of<std::is_same<S, Snt>, details::traits::neg<std::is_same<Itr, Snt>>>::value,
          bool>::type
          operator==( const iterator& a, const S& b )
        { return a.current_ == b; }
        template<typename S>
        PACE__NODISCARD friend PACE__FORCEINLINE constexpr typename std::enable_if<
          details::traits::all_of<std::is_same<S, Snt>, details::traits::neg<std::is_same<Itr, Snt>>>::value,
          bool>::type
          operator==( const Snt& a, const iterator& b )
        { return b == a; }
        template<typename S>
        PACE__NODISCARD friend PACE__FORCEINLINE constexpr typename std::enable_if<
          details::traits::all_of<std::is_same<S, Snt>, details::traits::neg<std::is_same<Itr, Snt>>>::value,
          bool>::type
          operator!=( const iterator& a, const Snt& b )
        { return !( a == b ); }
        template<typename S>
        PACE__NODISCARD friend PACE__FORCEINLINE constexpr typename std::enable_if<
          details::traits::all_of<std::is_same<S, Snt>, details::traits::neg<std::is_same<Itr, Snt>>>::value,
          bool>::type
          operator!=( const Snt& a, const iterator& b )
        { return !( b == a ); }
        PACE__NODISCARD friend PACE__FORCEINLINE constexpr bool operator==( const iterator& a,
                                                                            const iterator& b )
        { return a.current_ == b.current_; }
        PACE__NODISCARD friend PACE__FORCEINLINE constexpr bool operator!=( const iterator& a,
                                                                            const iterator& b )
        { return !( a == b ); }
        PACE__NODISCARD friend PACE__FORCEINLINE constexpr difference_type operator-( const iterator& a,
                                                                                      const iterator& b )
        { return details::utils::distance( a.current_, b.current_ ); }
        PACE__NODISCARD friend PACE__FORCEINLINE constexpr bool operator==( const iterator& a,
                                                                            const Sentry& b )
        { return a.current_ == b.endpoint_; }
        PACE__NODISCARD friend PACE__FORCEINLINE constexpr bool operator!=( const iterator& a,
                                                                            const Sentry& b )
        { return !( a == b ); }
        PACE__NODISCARD friend PACE__FORCEINLINE constexpr bool operator==( const Sentry& a,
                                                                            const iterator& b )
        { return b == a; }
        PACE__NODISCARD friend PACE__FORCEINLINE constexpr bool operator!=( const Sentry& a,
                                                                            const iterator& b )
        { return !( b == a ); }
        PACE__NODISCARD friend PACE__FORCEINLINE constexpr difference_type operator-( const Sentry& a,
                                                                                      const iterator& b )
        { return b - a; }

        explicit constexpr operator bool() const { return current_ != Itr(); }
      };

      constexpr IteratorSpan() = default;
      PACE__CXX17_CNSTXPR IteratorSpan( Itr startpoint, Snt endpoint )
        : start_ { std::move( startpoint ) }, end_ { std::move( endpoint ) }
      {
        const auto length = details::utils::distance( start_, end_ );
        if ( length < 0 )
          PACE__UNLIKELY throw exception::InvalidArgument(
            details::charcodes::make_literal( "pace: negative iterator range" ) );
        size_ = static_cast<details::types::Size>( length );
      }
      PACE__CXX17_CNSTXPR IteratorSpan( const IteratorSpan& )              = default;
      PACE__CXX17_CNSTXPR IteratorSpan( IteratorSpan&& )                   = default;
      PACE__CXX17_CNSTXPR IteratorSpan& operator=( const IteratorSpan& ) & = default;
      PACE__CXX17_CNSTXPR IteratorSpan& operator=( IteratorSpan&& ) &      = default;
      // Intentional non-virtual destructors.
      PACE__CXX20_CNSTXPR ~IteratorSpan()                                  = default;

      PACE__NODISCARD PACE__FORCEINLINE constexpr iterator begin() const
        noexcept( details::traits::all_of<std::is_nothrow_move_constructible<Itr>,
                                          std::is_nothrow_copy_constructible<Itr>>::value )
      { return { this->start_ }; }
      PACE__NODISCARD PACE__FORCEINLINE constexpr sentinel end() const
        noexcept( details::traits::all_of<std::is_nothrow_move_constructible<sentinel>,
                                          std::is_nothrow_copy_constructible<sentinel>>::value )
      { return { this->end_ }; }

      PACE__NODISCARD PACE__FORCEINLINE PACE__CXX14_CNSTXPR typename iterator::reference front()
        const noexcept
      { return *start_; }
      PACE__NODISCARD PACE__FORCEINLINE PACE__CXX14_CNSTXPR typename iterator::reference back() const noexcept
      { return *std::next( start_, size_ - 1 ); }
      PACE__NODISCARD PACE__FORCEINLINE constexpr details::types::Size step() const noexcept { return 1; }
      PACE__NODISCARD PACE__FORCEINLINE constexpr details::types::Size size() const noexcept { return size_; }
      PACE__NODISCARD PACE__FORCEINLINE constexpr bool empty() const noexcept { return size_ == 0; }

      PACE__CXX20_CNSTXPR void swap( IteratorSpan<Itr>& lhs ) noexcept
      {
        PACE__TRUST( this != &lhs );
        using std::swap;
        swap( start_, lhs.start_ );
        swap( end_, lhs.end_ );
        swap( size_, lhs.size_ );
      }
      friend PACE__CXX20_CNSTXPR void swap( IteratorSpan<Itr>& a, IteratorSpan<Itr>& b ) noexcept
      { a.swap( b ); }

      PACE__NODISCARD PACE__FORCEINLINE PACE__CXX17_CNSTXPR typename iterator::reference operator[](
        details::types::Size inc ) const
      { return *std::next( start_, inc ); }
      explicit PACE__CXX17_CNSTXPR operator bool() const noexcept { return !empty(); }
    };
  } // namespace slice
} // namespace pace

#endif
