#ifndef PACE_STRING_VIEW
#define PACE_STRING_VIEW

#include "../traits/Concept.hpp"
#ifdef __cpp_lib_string_view
# include "../core/Types.hpp"
# include <string_view>
#else
# include "../traits/Identity.hpp"
# include <iterator>
# include <limits>
# include <stdexcept>
#endif

namespace pace {
  namespace details {
    namespace charcodes {
#ifdef __cpp_lib_string_view
      template<typename Char, typename Traits = std::char_traits<Char>>
      using BasicStringView = std::basic_string_view<Char, Traits>;
#else
      template<typename Char, typename Traits = std::char_traits<Char>>
      class BasicStringView {
      public:
        using traits_type     = Traits;
        using value_type      = Char;
        using pointer         = value_type*;
        using const_pointer   = const value_type*;
        using reference       = value_type&;
        using const_reference = const value_type&;
        using size_type       = types::Size;
        using difference_type = std::ptrdiff_t;

        // This is how libc++ does.
        using const_iterator         = const_pointer;
        using iterator               = const_iterator;
        using const_reverse_iterator = std::reverse_iterator<const_iterator>;
        using reverse_iterator       = const_reverse_iterator;

        static constexpr size_type npos = static_cast<size_type>( -1 );

      private:
        const_pointer data_;
        size_type length_;

      public:
        // extension
        template<typename A>
        PACE__CXX20_CNSTXPR BasicStringView( const std::basic_string<Char, Traits, A>& str )
          : BasicStringView( str.data(), str.size() )
        {}
        template<typename A>
        PACE__CXX20_CNSTXPR BasicStringView& operator=( const std::basic_string<Char, Traits, A>& str ) &
        { return operator=( BasicStringView( str ) ); }
        template<size_type N>
        constexpr BasicStringView( const Char ( &lit )[N] ) noexcept
          : data_ { static_cast<const_pointer>( lit ) }, length_ { N - 1 }
        {}
        PACE__CXX20_CNSTXPR explicit operator std::basic_string<Char, Traits>() const
        { return { data_, length_ }; }

        template<typename Alloc>
        PACE__NODISCARD friend PACE__CXX20_CNSTXPR std::basic_string<Char, Traits, Alloc> operator+(
          const std::basic_string<Char, Traits, Alloc>& a,
          BasicStringView<Char, Traits> b )
        { return std::basic_string<Char, Traits, Alloc>( a ) + b; }
        template<typename Alloc>
        PACE__NODISCARD friend PACE__CXX20_CNSTXPR std::basic_string<Char, Traits, Alloc> operator+(
          std::basic_string<Char, Traits, Alloc>&& a,
          BasicStringView<Char, Traits> b )
        {
          a.append( b.data(), b.size() );
          return a;
        }
        template<typename Alloc>
        PACE__NODISCARD friend PACE__CXX20_CNSTXPR std::basic_string<Char, Traits, Alloc> operator+(
          BasicStringView<Char, Traits> a,
          const std::basic_string<Char, Traits, Alloc>& b )
        { return static_cast<std::basic_string<Char, Traits, Alloc>>( a ) + b; }
        template<typename Alloc>
        PACE__NODISCARD friend PACE__CXX20_CNSTXPR std::basic_string<Char, Traits, Alloc>& operator+=(
          std::basic_string<Char, Traits, Alloc>& a,
          BasicStringView<Char, Traits> b )
        {
          a.append( b.data(), b.size() );
          return a;
        }
        template<typename Alloc>
        PACE__NODISCARD friend PACE__CXX20_CNSTXPR std::basic_string<Char, Traits, Alloc>&& operator+=(
          std::basic_string<Char, Traits, Alloc>&& a,
          BasicStringView<Char, Traits> b )
        {
          a.append( b.data(), b.size() );
          return std::move( a );
        }

        constexpr BasicStringView() noexcept : data_ { nullptr }, length_ { 0 } {}
        constexpr BasicStringView( const_pointer s, size_type count ) noexcept
          : data_ { s }, length_ { count }
        {}
        template<typename P,
                 typename = typename std::enable_if<traits::AllOf<
                   std::is_convertible<P, const_pointer>,
                   traits::Not<std::is_array<typename std::remove_reference<P>::type>>>::value>::type>
        PACE__CXX17_CNSTXPR BasicStringView( P&& s ) noexcept : data_ { s }, length_ { Traits::length( s ) }
        {}

# ifdef __cpp_lib_ranges
        template<std::contiguous_iterator It, typename End>
          requires( std::sized_sentinel_for<End, It> && std::is_same_v<std::iter_value_t<It>, Char>
                    && !std::is_convertible_v<End, std::size_t> )
        PACE__CXX20_CNSTXPR BasicStringView( It first, End last )
          : data_ { std::to_address( first ) }, length_ { last - first }
        {}
# endif
# ifdef __cpp_lib_containers_ranges
        template<std::ranges::contiguous_range R>
          requires( !std::is_same_v<std::remove_cvref_t<R>, BasicStringView> && std::ranges::sized_range<R>
                    && std::is_same_v<std::ranges::range_value_t<R>, Char>
                    && !std::is_convertible_v<R, const_pointer>
                    && !requires(
                      std::remove_cvref_t<R>& d ) { d.operator BasicStringView<Char, Traits>(); } )
        PACE__CXX20_CNSTXPR explicit BasicStringView( R&& r )
          : data_ { std::ranges::data( r ) }, length_ { utils::size( r ) }
        {}
# endif

        BasicStringView( std::nullptr_t ) = delete;

        constexpr BasicStringView( const BasicStringView& )                        = default;
        PACE__CXX14_CNSTXPR BasicStringView& operator=( const BasicStringView& ) & = default;

        PACE__NODISCARD constexpr const_iterator cbegin() const noexcept { return data_; }
        PACE__NODISCARD constexpr iterator begin() const noexcept { return cbegin(); }

        PACE__NODISCARD constexpr const_iterator cend() const noexcept { return data_ + length_; }
        PACE__NODISCARD constexpr iterator end() const noexcept { return cend(); }

        PACE__NODISCARD constexpr const_reverse_iterator crbegin() const noexcept { return cend(); }
        PACE__NODISCARD constexpr reverse_iterator rbegin() const noexcept { return crbegin(); }

        PACE__NODISCARD constexpr const_reverse_iterator crend() const noexcept { return cbegin(); }
        PACE__NODISCARD constexpr reverse_iterator rend() const noexcept { return crend(); }

        constexpr const_reference operator[]( size_type pos ) const noexcept { return data_[pos]; }
        PACE__CXX20_CNSTXPR const_reference at( size_type pos ) const
        {
          if ( pos >= length_ )
            PACE__UNLIKELY throw std::out_of_range( "pace: accessed position is out of range" );
          return data_[pos];
        }

        constexpr const_reference front() const noexcept { return *data_; }
        constexpr const_reference back() const noexcept { return data_[length_ - 1]; }
        constexpr const_pointer data() const noexcept { return data_; }

        PACE__NODISCARD constexpr size_type size() const noexcept { return length_; }
        PACE__NODISCARD constexpr size_type length() const noexcept { return size(); }
        // implementation referenced from libc++
        PACE__NODISCARD PACE__CNSTEVAL size_type max_size() const noexcept
        { return ( std::numeric_limits<size_type>::max )() / sizeof( value_type ); }
        PACE__NODISCARD constexpr bool empty() const noexcept { return length_ == 0; }

        PACE__CXX14_CNSTXPR void remove_prefix( size_type n ) noexcept
        {
          data_ += n;
          length_ -= n;
        }
        PACE__CXX14_CNSTXPR void remove_suffix( size_type n ) noexcept { length_ -= n; }

        PACE__CXX20_CNSTXPR size_type copy( pointer dest, size_type count, size_type pos = 0 ) const
        {
          if ( pos > length_ )
            PACE__UNLIKELY throw std::out_of_range(
              "pace: copy a sub-string at an invalid position to the destination" );
          count = (std::min)( count, length_ - pos );
          Traits::copy( dest, data() + pos, count );
          return count;
        }
        PACE__NODISCARD PACE__CXX20_CNSTXPR BasicStringView substr( size_type pos   = 0,
                                                                    size_type count = npos ) const
        {
          if ( pos > length_ )
            PACE__UNLIKELY throw std::out_of_range( "pace: assign a sub-string view with invalid subrange" );
          return { data() + pos, std::min( count, size() - pos ) };
        }

        PACE__NODISCARD PACE__CXX17_CNSTXPR int compare( BasicStringView sv ) const noexcept
        {
          auto result = Traits::compare( data(), sv.data(), std::min( size(), sv.size() ) );
          if ( result != 0 )
            return result;
          if ( size() < sv.size() )
            return -1;
          if ( size() > sv.size() )
            return 1;
          return 0;
        }
        PACE__NODISCARD PACE__CXX17_CNSTXPR int compare( size_type pos,
                                                         size_type count,
                                                         BasicStringView sv ) const
        { return substr( pos, count ).compare( sv ); }
        PACE__NODISCARD PACE__CXX17_CNSTXPR int compare( size_type pos1,
                                                         size_type count1,
                                                         BasicStringView sv,
                                                         size_type pos2,
                                                         size_type count2 ) const
        { return substr( pos1, count1 ).compare( sv.substr( pos2, count2 ) ); }
        PACE__NODISCARD PACE__CXX17_CNSTXPR int compare( const_pointer str ) const noexcept
        { return compare( BasicStringView( str ) ); }
        PACE__NODISCARD PACE__CXX17_CNSTXPR int compare( size_type pos,
                                                         size_type count,
                                                         const_pointer str ) const
        { return substr( pos, count ).compare( BasicStringView( str ) ); }
        PACE__NODISCARD PACE__CXX17_CNSTXPR int compare( size_type pos1,
                                                         size_type count1,
                                                         const_pointer str,
                                                         size_type count2 ) const
        { return substr( pos1, count1 ).compare( BasicStringView( str, count2 ) ); }

        PACE__NODISCARD PACE__CXX17_CNSTXPR size_type find( Char ch, size_type pos = 0 ) const noexcept
        {
          pos         = std::min( pos, size() );
          auto result = Traits::find( data() + pos, size() - pos, ch );
          if ( result == nullptr )
            return npos;
          return result - data();
        }

        PACE__CXX20_CNSTXPR void swap( BasicStringView& other ) noexcept
        {
          std::swap( data_, other.data_ );
          std::swap( length_, other.length_ );
        }
        friend PACE__CXX20_CNSTXPR void swap( BasicStringView& a, BasicStringView& b ) noexcept
        { a.swap( b ); }

        PACE__NODISCARD friend PACE__CXX17_CNSTXPR bool operator==(
          BasicStringView a,
          traits::Identity_t<BasicStringView> b ) noexcept
        { return a.compare( b ) == 0; }
        PACE__NODISCARD friend PACE__CXX17_CNSTXPR bool operator!=(
          BasicStringView a,
          traits::Identity_t<BasicStringView> b ) noexcept
        { return !( a == b ); }

# ifdef __cpp_lib_three_way_comparison
        PACE__NODISCARD friend PACE__CXX17_CNSTXPR auto operator<=>(
          BasicStringView a,
          traits::Identity_t<BasicStringView> b ) noexcept
        {
          return static_cast<traits::ComparisonCategory_t<Traits>>(
            a.compare( 0, a.length_, b.data(), b.size() ) <=> 0 );
        }
# else
        PACE__NODISCARD friend PACE__CXX17_CNSTXPR bool operator<(
          BasicStringView a,
          traits::Identity_t<BasicStringView> b ) noexcept
        { return a.compare( b ) < 0; }
        PACE__NODISCARD friend PACE__CXX17_CNSTXPR bool operator<=(
          BasicStringView a,
          traits::Identity_t<BasicStringView> b ) noexcept
        { return a.compare( b ) <= 0; }
        PACE__NODISCARD friend PACE__CXX17_CNSTXPR bool operator>(
          BasicStringView a,
          traits::Identity_t<BasicStringView> b ) noexcept
        { return a.compare( b ) > 0; }
        PACE__NODISCARD friend PACE__CXX17_CNSTXPR bool operator>=(
          BasicStringView a,
          traits::Identity_t<BasicStringView> b ) noexcept
        { return a.compare( b ) >= 0; }
# endif
      };
#endif

      using StringView = BasicStringView<types::Char>;
#ifdef __cpp_char8_t
      using U8StringView = BasicStringView<char8_t>;
#endif
    } // namespace charcodes
  } // namespace details
} // namespace pace

#endif
