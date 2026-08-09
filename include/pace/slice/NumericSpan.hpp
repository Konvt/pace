#ifndef PACE_NUMERIC_SPAN
#define PACE_NUMERIC_SPAN

#include "../details/utils/Format.hpp"
#include "../exception/Error.hpp"
#include <cmath>
#include <limits>
#ifdef __cpp_lib_ranges
# include <ranges>
#endif
#ifdef __cpp_lib_three_way_comparison
# include <compare>
#endif

namespace pace {
  namespace slice {
    /**
     * An bidirectional range delimited by an numeric interval [start, end).
     *
     * The `end` can be less than the `start` only if the `step` is negative,
     * otherwise it throws exception `pace::exception::InvalidArgument`.
     */
    template<typename N>
    class NumericSpan
#ifdef __cpp_lib_ranges
      : public std::ranges::view_interface<NumericSpan<N>>
#endif
    {
      static_assert( std::is_arithmetic<N>::value, "only available for arithmetic types" );

      N start_ = 0, end_ = 0, step_ = 1;

    public:
      class iterator {
        friend NumericSpan;

        N itr_start_, itr_step_;
        std::uint64_t itr_cnt_;

        constexpr iterator( N startpoint, N step, std::uint64_t iterated = 0 ) noexcept
          : itr_start_ { startpoint }, itr_step_ { step }, itr_cnt_ { iterated }
        {}

      public:
        using iterator_category = std::random_access_iterator_tag;
        using value_type        = N;
        using difference_type   = typename std::make_signed<value_type>::type;
        using pointer           = value_type*;
        using reference         = value_type;

        constexpr iterator() noexcept : iterator( {}, 1, {} ) {}
        constexpr iterator( const iterator& )                        = default;
        PACE__CXX14_CNSTXPR iterator& operator=( const iterator& ) & = default;
        PACE__CXX20_CNSTXPR ~iterator()                              = default;

        PACE__FORCEINLINE PACE__CXX14_CNSTXPR iterator& operator++() & noexcept
        {
          ++itr_cnt_;
          return *this;
        }
        PACE__FORCEINLINE PACE__CXX14_CNSTXPR iterator operator++( int ) & noexcept
        {
          auto before = *this;
          operator++();
          return before;
        }
        PACE__FORCEINLINE PACE__CXX14_CNSTXPR iterator& operator--() & noexcept
        {
          --itr_cnt_;
          return *this;
        }
        PACE__FORCEINLINE PACE__CXX14_CNSTXPR iterator operator--( int ) & noexcept
        {
          auto before = *this;
          operator--();
          return before;
        }
        PACE__NODISCARD PACE__FORCEINLINE constexpr reference operator*() const noexcept
        { return static_cast<reference>( itr_start_ + itr_step_ * itr_cnt_ ); }
        PACE__NODISCARD PACE__FORCEINLINE constexpr reference operator[]( difference_type inc ) const noexcept
        { return static_cast<reference>( itr_start_ + itr_step_ * ( itr_cnt_ + inc ) ); }

        PACE__FORCEINLINE friend PACE__CXX14_CNSTXPR iterator operator+( iterator self,
                                                                         difference_type inc ) noexcept
        { return { self.itr_start_, self.itr_step_, self.itr_cnt_ + inc }; }
        PACE__FORCEINLINE friend PACE__CXX14_CNSTXPR iterator operator+( difference_type inc,
                                                                         iterator self ) noexcept
        { return self + inc; }
        PACE__FORCEINLINE friend PACE__CXX14_CNSTXPR iterator operator-( iterator self,
                                                                         difference_type inc ) noexcept
        { return { self.itr_start_, self.itr_step_, self.itr_cnt_ - inc }; }
        PACE__FORCEINLINE friend PACE__CXX14_CNSTXPR iterator operator-( difference_type inc,
                                                                         iterator self ) noexcept
        { return self - inc; }
        PACE__NODISCARD PACE__FORCEINLINE friend constexpr difference_type operator-( iterator a,
                                                                                      iterator b ) noexcept
        {
          return ( a.itr_start_ != b.itr_start_ || a.itr_step_ != b.itr_step_ )
                 ? ( std::numeric_limits<difference_type>::max )()
                 : static_cast<difference_type>( a.itr_cnt_ ) - static_cast<difference_type>( b.itr_cnt_ );
        }
        PACE__FORCEINLINE friend PACE__CXX14_CNSTXPR iterator& operator+=( iterator& self,
                                                                           difference_type inc ) noexcept
        {
          self.itr_cnt_ += inc;
          return self;
        }
        PACE__FORCEINLINE friend PACE__CXX14_CNSTXPR iterator& operator-=( iterator& self,
                                                                           difference_type inc ) noexcept
        {
          self.itr_cnt_ -= inc;
          return self;
        }
        PACE__NODISCARD PACE__FORCEINLINE friend constexpr bool operator==( iterator self,
                                                                            value_type num ) noexcept
        { return *self == num; }
        PACE__NODISCARD PACE__FORCEINLINE friend constexpr bool operator!=( iterator self,
                                                                            value_type num ) noexcept
        { return !( self == num ); }
        PACE__NODISCARD PACE__FORCEINLINE friend constexpr bool operator==( iterator a, iterator b ) noexcept
        { return a.itr_start_ == b.itr_start_ && a.itr_step_ == b.itr_step_ && a.itr_cnt_ == b.itr_cnt_; }
#ifdef __cpp_lib_three_way_comparison
        PACE__NODISCARD PACE__FORCEINLINE friend constexpr std::partial_ordering operator<=>(
          iterator a,
          iterator b ) noexcept
        {
          if ( a.itr_start_ != b.itr_start_ || a.itr_step_ != b.itr_step_ )
            return std::partial_ordering::unordered;
          return static_cast<std::partial_ordering>( a.itr_cnt_ <=> b.itr_cnt_ );
        }
#else
        PACE__NODISCARD PACE__FORCEINLINE friend constexpr bool operator!=( iterator a, iterator b ) noexcept
        { return !( a == b ); }
        PACE__NODISCARD PACE__FORCEINLINE friend constexpr bool operator<( iterator a, iterator b ) noexcept
        { return a.itr_start_ == b.itr_start_ && a.itr_step_ == b.itr_step_ && a.itr_cnt_ < b.itr_cnt_; }
        PACE__NODISCARD PACE__FORCEINLINE friend constexpr bool operator>( iterator a, iterator b ) noexcept
        { return b < a; }
        PACE__NODISCARD PACE__FORCEINLINE friend constexpr bool operator<=( iterator a, iterator b ) noexcept
        { return !( b < a ); }
        PACE__NODISCARD PACE__FORCEINLINE friend constexpr bool operator>=( iterator a, iterator b ) noexcept
        { return !( a < b ); }
#endif
      };
      using sentinel = iterator;

      /**
       * @throw exception::InvalidArgument
       *
       * If the `startpoint` is greater than `endpoint` while `step` is positive,
       * or the `startpoint` is less than `endpoint` while `step` is negative.
       */
      PACE__CXX20_CNSTXPR NumericSpan( N startpoint, N endpoint, N step )
      {
        if ( step > 0 && startpoint > endpoint )
          PACE__UNLIKELY
          {
            details::charcodes::SharedString message =
              details::charcodes::make_literal( "pace: 'endpoint (" );
            details::utils::format_to( std::back_inserter( message ), endpoint );
            message.append( ")' is less than 'startpoint (" );
            details::utils::format_to( std::back_inserter( message ), startpoint );
            message.append( ")' while 'step (" );
            details::utils::format_to( std::back_inserter( message ), step );
            message.append( ")' is positive" );
            throw exception::InvalidArgument( std::move( message ) );
          }
        else if ( step < 0 && startpoint < endpoint )
          PACE__UNLIKELY
          {
            details::charcodes::SharedString message =
              details::charcodes::make_literal( "pace: 'endpoint (" );
            details::utils::format_to( std::back_inserter( message ), endpoint );
            message.append( ")' is greater than 'startpoint (" );
            details::utils::format_to( std::back_inserter( message ), startpoint );
            message.append( ")' while 'step (" );
            details::utils::format_to( std::back_inserter( message ), step );
            message.append( ")' is negative" );
            throw exception::InvalidArgument( std::move( message ) );
          }
        else if ( step == 0 )
          PACE__UNLIKELY throw exception::InvalidArgument(
            details::charcodes::make_literal( "pace: 'step' is zero" ) );

        start_ = startpoint;
        step_  = step;
        end_   = endpoint;
      }
      /**
       * @throw exception::InvalidArgument
       *
       * If the `startpoint` is greater than `endpoint`.
       */
      PACE__CXX20_CNSTXPR NumericSpan( N startpoint, N endpoint ) : NumericSpan( startpoint, endpoint, 1 ) {}
      /**
       * @throw exception::InvalidArgument
       *
       * If the `endpoint` is less than zero.
       */
      PACE__CXX20_CNSTXPR NumericSpan( N endpoint ) : NumericSpan( {}, endpoint, 1 ) {}
      constexpr NumericSpan( const NumericSpan& )                        = default;
      PACE__CXX14_CNSTXPR NumericSpan& operator=( const NumericSpan& ) & = default;
      PACE__CXX20_CNSTXPR ~NumericSpan()                                 = default;

      PACE__NODISCARD PACE__FORCEINLINE constexpr iterator begin() const noexcept
      { return iterator( start_, step_ ); }
      PACE__NODISCARD PACE__FORCEINLINE PACE__CXX23_CNSTXPR sentinel end() const noexcept
      { return sentinel( start_, step_, size() ); }

      PACE__NODISCARD PACE__FORCEINLINE constexpr N front() const noexcept { return start_; }
      PACE__NODISCARD PACE__FORCEINLINE constexpr N back() const noexcept
      { return start_ + step_ * static_cast<N>( size() - 1 ); }
      PACE__NODISCARD PACE__FORCEINLINE constexpr N step() const noexcept { return step_; }
      PACE__NODISCARD PACE__FORCEINLINE PACE__CXX23_CNSTXPR std::uint64_t size() const noexcept
      {
        PACE__TRUST( step_ != 0 );
        if PACE__CXX17_CNSTXPR ( std::is_unsigned<N>::value )
          return ( ( end_ - start_ + step_ ) - 1 ) / step_;
        else if PACE__CXX17_CNSTXPR ( std::is_integral<N>::value ) {
          if ( step_ > 0 )
            return ( ( end_ - start_ + step_ - 1 ) / step_ );
          else
            return ( ( start_ - end_ - step_ ) - 1 ) / ( -step_ );
        } else
          return static_cast<std::uint64_t>( std::ceil( ( end_ - start_ ) / step_ ) );
      }
      PACE__NODISCARD PACE__FORCEINLINE constexpr bool empty() const noexcept { return size() == 0; }

      PACE__CXX14_CNSTXPR void swap( NumericSpan& lhs ) noexcept
      {
        PACE__TRUST( this != &lhs );
        using std::swap;
        swap( start_, lhs.start_ );
        swap( end_, lhs.end_ );
        swap( step_, lhs.step_ );
      }
      friend PACE__CXX14_CNSTXPR void swap( NumericSpan& a, NumericSpan& b ) noexcept { a.swap( b ); }

      PACE__NODISCARD PACE__FORCEINLINE constexpr typename iterator::reference operator[](
        typename iterator::difference_type inc ) const noexcept
      { return start_ + step_ * static_cast<N>( inc ); }
      explicit constexpr operator bool() const noexcept { return !empty(); }
    };
  } // namespace slice
} // namespace pace

#ifdef __cpp_lib_ranges
template<typename N>
inline constexpr bool std::ranges::enable_borrowed_range<pace::slice::NumericSpan<N>> = true;
#endif

#endif
