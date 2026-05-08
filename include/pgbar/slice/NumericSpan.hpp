#ifndef PGBAR_NUMERICSPAN
#define PGBAR_NUMERICSPAN

#include "../details/utils/Util.hpp"
#include "../exception/Error.hpp"
#include <cmath>
#include <limits>
#ifdef __cpp_lib_ranges
# include <ranges>
#endif
#ifdef __cpp_lib_three_way_comparison
# include <compare>
#endif

namespace pgbar {
  namespace slice {
    /**
     * An bidirectional range delimited by an numeric interval [start, end).
     *
     * The `end` can be less than the `start` only if the `step` is negative,
     * otherwise it throws exception `pgbar::exception::InvalidArgument`.
     */
    template<typename N>
    class NumericSpan
#ifdef __cpp_lib_ranges
      : public std::ranges::view_interface<NumericSpan<N>>
#endif
    {
      static_assert( std::is_arithmetic<N>::value, "only available for arithmetic types" );

      N start_, end_, step_;

    public:
      class iterator {
        N itr_start_, itr_step_;
        std::uint64_t itr_cnt_;

      public:
        using iterator_category = std::random_access_iterator_tag;
        using value_type        = N;
        using difference_type   = std::ptrdiff_t;
        using pointer           = value_type*;
        using reference         = value_type;

        constexpr iterator( N startpoint, N step, std::uint64_t iterated = 0 ) noexcept
          : itr_start_ { startpoint }, itr_step_ { step }, itr_cnt_ { iterated }
        {}
        constexpr iterator() noexcept : iterator( {}, 1, {} ) {}
        constexpr iterator( const iterator& )                         = default;
        PGBAR__CXX14_CNSTXPR iterator& operator=( const iterator& ) & = default;
        PGBAR__CXX20_CNSTXPR ~iterator()                              = default;

        PGBAR__FORCEINLINE PGBAR__CXX14_CNSTXPR iterator& operator++() & noexcept
        {
          ++itr_cnt_;
          return *this;
        }
        PGBAR__FORCEINLINE PGBAR__CXX14_CNSTXPR iterator operator++( int ) & noexcept
        {
          auto before = *this;
          operator++();
          return before;
        }
        PGBAR__FORCEINLINE PGBAR__CXX14_CNSTXPR iterator& operator--() & noexcept
        {
          --itr_cnt_;
          return *this;
        }
        PGBAR__FORCEINLINE PGBAR__CXX14_CNSTXPR iterator operator--( int ) & noexcept
        {
          auto before = *this;
          operator--();
          return before;
        }
        PGBAR__NODISCARD PGBAR__FORCEINLINE constexpr reference operator*() const noexcept
        {
          return static_cast<reference>( itr_start_ + itr_step_ * itr_cnt_ );
        }
        PGBAR__NODISCARD PGBAR__FORCEINLINE constexpr reference operator[](
          difference_type inc ) const noexcept
        {
          return static_cast<reference>( itr_start_ + itr_step_ * ( itr_cnt_ + inc ) );
        }

        friend PGBAR__FORCEINLINE PGBAR__CXX14_CNSTXPR iterator operator+( iterator itr,
                                                                           difference_type inc ) noexcept
        {
          return { itr.itr_start_, itr.itr_step_, itr.itr_cnt_ + inc };
        }
        friend PGBAR__FORCEINLINE PGBAR__CXX14_CNSTXPR iterator operator+( difference_type inc,
                                                                           iterator itr ) noexcept
        {
          return itr + inc;
        }
        friend PGBAR__FORCEINLINE PGBAR__CXX14_CNSTXPR iterator operator-( iterator itr,
                                                                           difference_type inc ) noexcept
        {
          return { itr.itr_start_, itr.itr_step_, itr.itr_cnt_ - inc };
        }
        friend PGBAR__FORCEINLINE PGBAR__CXX14_CNSTXPR iterator operator-( difference_type inc,
                                                                           iterator itr ) noexcept
        {
          return itr - inc;
        }
        PGBAR__NODISCARD friend PGBAR__FORCEINLINE constexpr difference_type operator-( iterator a,
                                                                                        iterator b ) noexcept
        {
          return ( a.itr_start_ != b.itr_start_ || a.itr_step_ != b.itr_step_ )
                 ? ( std::numeric_limits<difference_type>::max )()
                 : a.itr_cnt_ - b.itr_cnt_;
        }
        friend PGBAR__FORCEINLINE PGBAR__CXX14_CNSTXPR iterator& operator+=( iterator& itr,
                                                                             difference_type inc ) noexcept
        {
          itr.itr_cnt_ += inc;
          return itr;
        }
        friend PGBAR__FORCEINLINE PGBAR__CXX14_CNSTXPR iterator& operator-=( iterator& itr,
                                                                             difference_type inc ) noexcept
        {
          itr.itr_cnt_ -= inc;
          return itr;
        }
        PGBAR__NODISCARD friend PGBAR__FORCEINLINE constexpr bool operator==( iterator itr,
                                                                              value_type num ) noexcept
        {
          return *itr == num;
        }
        PGBAR__NODISCARD friend PGBAR__FORCEINLINE constexpr bool operator!=( iterator itr,
                                                                              value_type num ) noexcept
        {
          return !( itr == num );
        }
        PGBAR__NODISCARD friend PGBAR__FORCEINLINE constexpr bool operator==( iterator a,
                                                                              iterator b ) noexcept
        {
          return a.itr_start_ == b.itr_start_ && a.itr_step_ == b.itr_step_ && a.itr_cnt_ == b.itr_cnt_;
        }
#ifdef __cpp_lib_three_way_comparison
        PGBAR__NODISCARD friend PGBAR__FORCEINLINE constexpr std::partial_ordering operator<=>(
          iterator a,
          iterator b ) noexcept
        {
          if ( a.itr_start_ != b.itr_start_ || a.itr_step_ != b.itr_step_ )
            return std::partial_ordering::unordered;
          return static_cast<std::partial_ordering>( a.itr_cnt_ <=> b.itr_cnt_ );
        }
#else
        PGBAR__NODISCARD friend PGBAR__FORCEINLINE constexpr bool operator!=( iterator a,
                                                                              iterator b ) noexcept
        {
          return !( a == b );
        }
        PGBAR__NODISCARD friend PGBAR__FORCEINLINE constexpr bool operator<( iterator a, iterator b ) noexcept
        {
          return a.itr_start_ == b.itr_start_ && a.itr_step_ == b.itr_step_ && a.itr_cnt_ < b.itr_cnt_;
        }
        PGBAR__NODISCARD friend PGBAR__FORCEINLINE constexpr bool operator>( iterator a, iterator b ) noexcept
        {
          return b < a;
        }
        PGBAR__NODISCARD friend PGBAR__FORCEINLINE constexpr bool operator<=( iterator a,
                                                                              iterator b ) noexcept
        {
          return !( b < a );
        }
        PGBAR__NODISCARD friend PGBAR__FORCEINLINE constexpr bool operator>=( iterator a,
                                                                              iterator b ) noexcept
        {
          return !( a < b );
        }
#endif
      };
      using sentinel = iterator;

      constexpr NumericSpan() noexcept : start_ {}, end_ {}, step_ { 1 } {}
      /**
       * @throw exception::InvalidArgument
       *
       * If the `startpoint` is greater than `endpoint` while `step` is positive,
       * or the `startpoint` is less than `endpoint` while `step` is negative.
       */
      PGBAR__CXX20_CNSTXPR NumericSpan( N startpoint, N endpoint, N step ) noexcept( false ) : NumericSpan()
      {
        if ( step > 0 && startpoint > endpoint )
          PGBAR__UNLIKELY
          {
            _details::charcodes::CoWString message =
              _details::charcodes::make_literal( "pgbar: 'endpoint (" );
            message.append( _details::utils::format( endpoint ) )
              .append( ")' is less than 'startpoint (" )
              .append( _details::utils::format( startpoint ) )
              .append( ")' while 'step (" )
              .append( _details::utils::format( step ) )
              .append( ")' is positive" );
            throw exception::InvalidArgument( std::move( message ) );
          }
        else if ( step < 0 && startpoint < endpoint )
          PGBAR__UNLIKELY
          {
            _details::charcodes::CoWString message =
              _details::charcodes::make_literal( "pgbar: 'endpoint (" );
            message.append( _details::utils::format( endpoint ) )
              .append( ")' is greater than 'startpoint (" )
              .append( _details::utils::format( startpoint ) )
              .append( ")' while 'step (" )
              .append( _details::utils::format( step ) )
              .append( ")' is negative" );
            throw exception::InvalidArgument( std::move( message ) );
          }
        else if ( step == 0 )
          PGBAR__UNLIKELY throw exception::InvalidArgument(
            _details::charcodes::make_literal( "pgbar: 'step' is zero" ) );

        start_ = startpoint;
        step_  = step;
        end_   = endpoint;
      }
      /**
       * @throw exception::InvalidArgument
       *
       * If the `startpoint` is greater than `endpoint`.
       */
      PGBAR__CXX20_CNSTXPR NumericSpan( N startpoint, N endpoint ) : NumericSpan( startpoint, endpoint, 1 ) {}
      /**
       * @throw exception::InvalidArgument
       *
       * If the `endpoint` is less than zero.
       */
      PGBAR__CXX20_CNSTXPR NumericSpan( N endpoint ) : NumericSpan( {}, endpoint, 1 ) {}
      constexpr NumericSpan( const NumericSpan& )                         = default;
      PGBAR__CXX14_CNSTXPR NumericSpan& operator=( const NumericSpan& ) & = default;
      PGBAR__CXX20_CNSTXPR ~NumericSpan()                                 = default;

      PGBAR__NODISCARD PGBAR__FORCEINLINE constexpr iterator begin() const noexcept
      {
        return iterator( start_, step_ );
      }
      PGBAR__NODISCARD PGBAR__FORCEINLINE PGBAR__CXX23_CNSTXPR sentinel end() const noexcept
      {
        return sentinel( start_, step_, size() );
      }

      PGBAR__NODISCARD PGBAR__FORCEINLINE constexpr N front() const noexcept { return start_; }
      PGBAR__NODISCARD PGBAR__FORCEINLINE constexpr N back() const noexcept
      {
        return start_ + step_ * static_cast<N>( size() - 1 );
      }
      PGBAR__NODISCARD PGBAR__FORCEINLINE constexpr N step() const noexcept { return step_; }
      PGBAR__NODISCARD PGBAR__FORCEINLINE PGBAR__CXX23_CNSTXPR std::uint64_t size() const noexcept
      {
        PGBAR__TRUST( step_ != 0 );
        if PGBAR__CXX17_CNSTXPR ( std::is_unsigned<N>::value )
          return ( ( end_ - start_ + step_ ) - 1 ) / step_;
        else if PGBAR__CXX17_CNSTXPR ( std::is_integral<N>::value ) {
          if ( step_ > 0 )
            return ( ( end_ - start_ + step_ - 1 ) / step_ );
          else
            return ( ( start_ - end_ - step_ ) - 1 ) / ( -step_ );
        } else
          return static_cast<std::uint64_t>( std::ceil( ( end_ - start_ ) / step_ ) );
      }
      PGBAR__NODISCARD PGBAR__FORCEINLINE constexpr bool empty() const noexcept { return size() == 0; }

      PGBAR__CXX14_CNSTXPR void swap( NumericSpan<N>& lhs ) noexcept
      {
        PGBAR__TRUST( this != &lhs );
        using std::swap;
        swap( start_, lhs.start_ );
        swap( end_, lhs.end_ );
        swap( step_, lhs.step_ );
      }
      friend PGBAR__CXX14_CNSTXPR void swap( NumericSpan<N>& a, NumericSpan<N>& b ) noexcept { a.swap( b ); }

      PGBAR__NODISCARD PGBAR__FORCEINLINE constexpr typename iterator::reference operator[](
        typename iterator::difference_type inc ) const noexcept
      {
        return start_ + step_ * static_cast<N>( inc );
      }
      explicit constexpr operator bool() const noexcept { return !empty(); }
    };
  } // namespace slice
} // namespace pgbar

#endif
