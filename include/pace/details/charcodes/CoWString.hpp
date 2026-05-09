#ifndef PACE_COW_STRING
#define PACE_COW_STRING

#include "../traits/Backport.hpp"
#include "../traits/Util.hpp"
#include "../utils/Backport.hpp"
#include <algorithm>
#include <atomic>
#include <iterator>
#include <limits>

namespace pace {
  namespace details {
    namespace charcodes {
      template<typename Char>
      struct Literal {
      private:
        const Char* data_;
        types::Size size_;

      public:
        constexpr Literal() noexcept : data_ { nullptr }, size_ { 0 } {}

        constexpr Literal( const Char* literal, types::Size length ) noexcept
          : data_ { literal }, size_ { length }
        {}
        template<types::Size N>
        constexpr Literal( const Char ( &literal )[N] ) noexcept : Literal( literal, N - 1 )
        {}

        PACE__NODISCARD constexpr types::Size size() const noexcept { return size_; }
        PACE__NODISCARD constexpr const Char* data() const noexcept { return data_; }
      };

      template<typename Char, types::Size N>
      constexpr Literal<Char> make_literal( const Char ( &cstr )[N] ) noexcept
      { return { cstr }; }
      template<typename Char>
      constexpr Literal<Char> make_literal( const Char* cstr, types::Size length ) noexcept
      { return { cstr, length }; }

      template<typename Alloc, typename = void>
      class CoWAllocator {
        Alloc alloc_;

      protected:
        template<typename... Args,
                 typename std::enable_if<std::is_constructible<Alloc, Args&&...>::value, bool>::type = 0>
        constexpr CoWAllocator( Args&&... args )
          noexcept( std::is_nothrow_constructible<Alloc, Args&&...>::value )
          : alloc_ { std::forward<Args>( args )... }
        {}
        template<typename... Args,
                 typename std::enable_if<!std::is_constructible<Alloc, Args&&...>::value, bool>::type = 0>
        constexpr CoWAllocator( Args&&... ) noexcept( std::is_nothrow_default_constructible<Alloc>::value )
          : alloc_ {}
        {}

        PACE__CXX14_CNSTXPR Alloc& allocator() noexcept { return alloc_; }
        constexpr const Alloc& allocator() const noexcept { return alloc_; }

      public:
        PACE__CXX20_CNSTXPR ~CoWAllocator() = default;
      };
      template<typename Alloc>
      class CoWAllocator<
        Alloc,
        typename std::enable_if<
          traits::AllOf<std::is_empty<Alloc>, traits::Not<traits::is_final<Alloc>>>::value>::type>
        : private Alloc {
      protected:
        template<typename... Args,
                 typename std::enable_if<std::is_constructible<Alloc, Args&&...>::value, bool>::type = 0>
        constexpr CoWAllocator( Args&&... args )
          noexcept( std::is_nothrow_constructible<Alloc, Args&&...>::value )
          : Alloc( std::forward<Args>( args )... )
        {}
        template<typename... Args,
                 typename std::enable_if<!std::is_constructible<Alloc, Args&&...>::value, bool>::type = 0>
        constexpr CoWAllocator( Args&&... ) noexcept( std::is_nothrow_default_constructible<Alloc>::value )
          : Alloc()
        {}

        PACE__CXX14_CNSTXPR Alloc& allocator() noexcept { return static_cast<Alloc&>( *this ); }
        constexpr const Alloc& allocator() const noexcept { return static_cast<const Alloc&>( *this ); }

      public:
        PACE__CXX20_CNSTXPR ~CoWAllocator() = default;
      };

      template<typename Alloc, typename Derived, typename = void>
      struct CoWCopyAlloc {
        constexpr CoWCopyAlloc()                                      = default;
        constexpr CoWCopyAlloc( const CoWCopyAlloc& )                 = default;
        constexpr CoWCopyAlloc( CoWCopyAlloc&& )                      = default;
        PACE__CXX14_CNSTXPR CoWCopyAlloc& operator=( CoWCopyAlloc&& ) = default;

        PACE__CXX20_CNSTXPR ~CoWCopyAlloc() = default;

        PACE__CXX14_CNSTXPR CoWCopyAlloc& operator=( const CoWCopyAlloc& ) noexcept { return *this; }
      };
      template<typename Alloc, typename Derived>
      struct CoWCopyAlloc<Alloc,
                          Derived,
                          typename std::enable_if<std::allocator_traits<
                            Alloc>::propagate_on_container_copy_assignment::value>::type> {
        constexpr CoWCopyAlloc()                                      = default;
        constexpr CoWCopyAlloc( const CoWCopyAlloc& )                 = default;
        constexpr CoWCopyAlloc( CoWCopyAlloc&& )                      = default;
        PACE__CXX14_CNSTXPR CoWCopyAlloc& operator=( CoWCopyAlloc&& ) = default;

        PACE__CXX20_CNSTXPR ~CoWCopyAlloc() = default;

        PACE__CXX14_CNSTXPR CoWCopyAlloc& operator=( const CoWCopyAlloc& other ) & noexcept
        { // if the allocator satisfy propagate_on_container_copy_assignment,
          // then its must be nothrow copy assignable
          static_cast<Derived*>( this )->allocator() = static_cast<Derived&>( other ).allocator();
          return *this;
        }
      };

      template<typename Alloc, typename Derived, typename = void>
      struct CoWMoveAlloc {
        constexpr CoWMoveAlloc()                                           = default;
        constexpr CoWMoveAlloc( const CoWMoveAlloc& )                      = default;
        constexpr CoWMoveAlloc( CoWMoveAlloc&& )                           = default;
        PACE__CXX14_CNSTXPR CoWMoveAlloc& operator=( const CoWMoveAlloc& ) = default;

        PACE__CXX20_CNSTXPR ~CoWMoveAlloc() = default;

        PACE__CXX14_CNSTXPR CoWMoveAlloc& operator=( CoWMoveAlloc&& ) noexcept { return *this; }
      };
      template<typename Alloc, typename Derived>
      struct CoWMoveAlloc<Alloc,
                          Derived,
                          typename std::enable_if<std::allocator_traits<
                            Alloc>::propagate_on_container_move_assignment::value>::type> {
        constexpr CoWMoveAlloc()                                           = default;
        constexpr CoWMoveAlloc( const CoWMoveAlloc& )                      = default;
        constexpr CoWMoveAlloc( CoWMoveAlloc&& )                           = default;
        PACE__CXX14_CNSTXPR CoWMoveAlloc& operator=( const CoWMoveAlloc& ) = default;

        PACE__CXX20_CNSTXPR ~CoWMoveAlloc() = default;

        PACE__CXX14_CNSTXPR CoWMoveAlloc& operator=( CoWMoveAlloc&& rhs ) & noexcept
        {
          static_cast<Derived*>( this )->allocator() = std::move( static_cast<Derived&>( rhs ).allocator() );
          return *this;
        }
      };

      template<typename Alloc, typename Derived, typename = void>
      struct CoWSwapAlloc {
        constexpr CoWSwapAlloc()                                           = default;
        constexpr CoWSwapAlloc( const CoWSwapAlloc& )                      = default;
        constexpr CoWSwapAlloc( CoWSwapAlloc&& )                           = default;
        PACE__CXX14_CNSTXPR CoWSwapAlloc& operator=( CoWSwapAlloc&& )      = default;
        PACE__CXX14_CNSTXPR CoWSwapAlloc& operator=( const CoWSwapAlloc& ) = default;

        PACE__CXX20_CNSTXPR ~CoWSwapAlloc() = default;

        PACE__CXX14_CNSTXPR void swap( CoWSwapAlloc& ) noexcept {}
      };
      template<typename Alloc, typename Derived>
      struct CoWSwapAlloc<
        Alloc,
        Derived,
        typename std::enable_if<std::allocator_traits<Alloc>::propagate_on_container_swap::value>::type> {
        constexpr CoWSwapAlloc()                                           = default;
        constexpr CoWSwapAlloc( const CoWSwapAlloc& )                      = default;
        constexpr CoWSwapAlloc( CoWSwapAlloc&& )                           = default;
        PACE__CXX14_CNSTXPR CoWSwapAlloc& operator=( CoWSwapAlloc&& )      = default;
        PACE__CXX14_CNSTXPR CoWSwapAlloc& operator=( const CoWSwapAlloc& ) = default;

        PACE__CXX20_CNSTXPR ~CoWSwapAlloc() = default;

        PACE__CXX14_CNSTXPR void swap( CoWSwapAlloc& other ) noexcept
        {
          using std::swap;
          swap( static_cast<Derived*>( this )->allocator(), static_cast<Derived&>( other ).allocator() );
        }
      };

      template<typename Pointee, typename Derived>
      class CoWIterator {
      protected:
        Pointee* owner_;
        typename Pointee::size_type pos_;

      public:
        using iterator_category = std::random_access_iterator_tag;
        using difference_type   = std::ptrdiff_t;
        using value_type        = typename Pointee::value_type;
        using pointer           = value_type*;
        using reference         = value_type&;

        constexpr CoWIterator() noexcept : owner_ { nullptr }, pos_ {} {}
        PACE__CXX17_CNSTXPR CoWIterator( Pointee& owner, typename Pointee::size_type pos ) noexcept
          : owner_ { std::addressof( owner ) }, pos_ { pos }
        {}
        PACE__CXX20_CNSTXPR ~CoWIterator() = default;

        constexpr Pointee* owner() const noexcept { return owner_; }
        PACE__NODISCARD constexpr typename Pointee::size_type offset() const noexcept { return pos_; }
        constexpr traits::CopyConst_t<Pointee, value_type>* base() const noexcept
        { return owner_->data() + pos_; }

        constexpr reference operator[]( difference_type offset ) const noexcept
        { return this->owner_[this->pos_ + offset]; }
        constexpr reference operator*() const noexcept { return this->owner_[this->pos_]; }
        constexpr pointer operator->() const noexcept { return std::addressof( this->owner_[this->pos_] ); }

        PACE__CXX14_CNSTXPR Derived& operator++() noexcept
        {
          ++pos_;
          return static_cast<Derived&>( *this );
        }
        PACE__CXX14_CNSTXPR Derived operator++( int ) noexcept
        {
          auto copy = static_cast<Derived&>( *this );
          operator++();
          return copy;
        }
        PACE__CXX14_CNSTXPR Derived& operator--() noexcept
        {
          --pos_;
          return static_cast<Derived&>( *this );
        }
        PACE__CXX14_CNSTXPR Derived operator--( int ) noexcept
        {
          auto copy = static_cast<Derived&>( *this );
          operator--();
          return copy;
        }
        friend constexpr bool operator==( const CoWIterator& a, const CoWIterator& b ) noexcept
        { return a.owner_ == b.owner_ && a.pos_ == b.pos_; }
        friend constexpr bool operator!=( const CoWIterator& a, const CoWIterator& b ) noexcept
        { return !( a == b ); }
        friend PACE__CXX14_CNSTXPR Derived& operator+=( CoWIterator& itr, difference_type count ) noexcept
        {
          itr.pos_ += count;
          return static_cast<Derived&>( itr );
        }
        friend PACE__CXX14_CNSTXPR Derived& operator-=( CoWIterator& itr, difference_type count ) noexcept
        {
          itr.pos_ -= count;
          return static_cast<Derived&>( itr );
        }

        friend constexpr Derived operator+( const CoWIterator& itr, difference_type count ) noexcept
        { return Derived( itr.owner_, itr.pos_ + count ); }
        friend constexpr Derived operator+( difference_type count, const CoWIterator& itr ) noexcept
        { return itr + count; }
        friend constexpr Derived operator-( const CoWIterator& itr, difference_type count ) noexcept
        { return Derived( itr.owner_, itr.pos_ + ( -count ) ); }
        friend constexpr Derived operator-( difference_type count, const CoWIterator& itr ) noexcept
        { return itr - count; }
        friend PACE__CXX14_CNSTXPR difference_type operator-( const CoWIterator& a,
                                                              const CoWIterator& b ) noexcept
        {
          if ( a.owner_ == b.owner_ )
            return static_cast<difference_type>( a.pos_ ) - static_cast<difference_type>( b.pos_ );
          return ( std::numeric_limits<difference_type>::max )();
        }

        explicit constexpr operator bool() const noexcept { return owner_ != nullptr; }
      };

      template<typename Char, typename Traits = std::char_traits<Char>, typename Alloc = std::allocator<Char>>
      class BasicCoWString
        : public CoWAllocator<Alloc>
        , private CoWCopyAlloc<Alloc, BasicCoWString<Char, Traits, Alloc>>
        , private CoWMoveAlloc<Alloc, BasicCoWString<Char, Traits, Alloc>>
        , private CoWSwapAlloc<Alloc, BasicCoWString<Char, Traits, Alloc>> {
        static_assert( traits::is_implicit_lifetime<Char>::value, "Char must be an implicit-lifetime type" );

        using CoWRef = std::atomic<std::uint64_t>;
#ifdef __cpp_lib_string_view
        template<typename StringViewLike>
        using is_string_view_like =
          traits::AllOf<std::is_convertible<const StringViewLike&, std::basic_string_view<Char, Traits>>,
                        traits::Not<std::is_convertible<const StringViewLike&, const Char*>>>;
#endif
#ifndef __cpp_lib_concepts
        template<typename InputIt>
        using is_legacy_input_iterator = traits::AllOf<
          std::is_base_of<std::input_iterator_tag, typename std::iterator_traits<InputIt>::iterator_category>,
          std::is_convertible<decltype( std::declval<const InputIt&>() == std::declval<const InputIt&>() ),
                              bool>,
          std::is_convertible<decltype( std::declval<const InputIt&>() != std::declval<const InputIt&>() ),
                              bool>>;
#endif
#ifdef __cpp_lib_three_way_comparison
        template<typename Trait>
        struct ComparisonCategory {
        private:
          template<typename T>
          static constexpr auto check( int ) -> typename T::comparison_category;
          template<typename>
          static constexpr std::weak_ordering check( ... );

        public:
          using type = decltype( check<Trait>( 0 ) );
        };
#endif

      public:
        using value_type      = Char;
        using traits_type     = Traits;
        using allocator_type  = Alloc;
        using size_type       = typename std::allocator_traits<Alloc>::size_type;
        using difference_type = typename std::allocator_traits<Alloc>::difference_type;
        using reference       = value_type&;
        using const_reference = const value_type&;
        using pointer         = typename std::allocator_traits<Alloc>::pointer;
        using const_pointer   = typename std::allocator_traits<Alloc>::const_pointer;

        static constexpr size_type npos = static_cast<size_type>( -1 );

        class iterator : public CoWIterator<BasicCoWString, iterator> {
        public:
          using CoWIterator<BasicCoWString, iterator>::CoWIterator;
        };
        class const_iterator : public CoWIterator<const BasicCoWString, const_iterator> {
        public:
          using CoWIterator<const BasicCoWString, const_iterator>::CoWIterator;

          constexpr const_iterator( iterator itr ) noexcept
            : CoWIterator<const BasicCoWString, const_iterator>( *itr.owner(), itr.offset() )
          {}
        };
        using reverse_iterator       = std::reverse_iterator<iterator>;
        using const_reverse_iterator = std::reverse_iterator<const_iterator>;

        class unsafe_iterator {
          const Char* cursor_;

        public:
#if PACE__CXX20
          using iterator_category = std::contiguous_iterator_tag;
#else
          using iterator_category = std::random_access_iterator_tag;
#endif
          using difference_type = std::ptrdiff_t;
          using value_type      = Char;
          using pointer         = const value_type*;
          using reference       = const value_type&;

          constexpr unsafe_iterator() noexcept : cursor_ { nullptr } {}
          constexpr unsafe_iterator( const Char* cursor ) noexcept : cursor_ { cursor } {}
          constexpr unsafe_iterator( const_iterator itr ) noexcept
            : cursor_ { itr.owner()->data() + itr.offset() }
          {}
          PACE__CXX20_CNSTXPR ~unsafe_iterator() = default;

          constexpr pointer base() const noexcept { return cursor_; }

          constexpr reference operator[]( difference_type offset ) const noexcept { return cursor_[offset]; }
          constexpr reference operator*() const noexcept { return *cursor_; }
          PACE__CXX17_CNSTXPR pointer operator->() const noexcept { return std::addressof( *cursor_ ); }

          PACE__CXX14_CNSTXPR unsafe_iterator& operator++() noexcept
          {
            ++cursor_;
            return *this;
          }
          PACE__CXX14_CNSTXPR unsafe_iterator operator++( int ) noexcept
          {
            const auto cursor = cursor_;
            ++cursor_;
            return { cursor };
          }
          PACE__CXX14_CNSTXPR unsafe_iterator& operator--() noexcept
          {
            --cursor_;
            return *this;
          }
          PACE__CXX14_CNSTXPR unsafe_iterator operator--( int ) noexcept
          {
            const auto cursor = cursor_;
            --cursor_;
            return { cursor };
          }
          friend constexpr bool operator==( const unsafe_iterator& a, const unsafe_iterator& b ) noexcept
          { return a.cursor_ == b.cursor_; }
          friend constexpr bool operator!=( const unsafe_iterator& a, const unsafe_iterator& b ) noexcept
          { return !( a == b ); }
          friend PACE__CXX14_CNSTXPR unsafe_iterator& operator+=( unsafe_iterator& itr,
                                                                  difference_type count ) noexcept
          {
            itr.cursor_ += count;
            return itr;
          }
          friend PACE__CXX14_CNSTXPR unsafe_iterator& operator-=( unsafe_iterator& itr,
                                                                  difference_type count ) noexcept
          {
            itr.cursor_ -= count;
            return itr;
          }

          friend constexpr unsafe_iterator operator+( const unsafe_iterator& itr,
                                                      difference_type count ) noexcept
          { return { itr.cursor_ + count }; }
          friend constexpr unsafe_iterator operator+( difference_type count,
                                                      const unsafe_iterator& itr ) noexcept
          { return itr + count; }
          friend constexpr unsafe_iterator operator-( const unsafe_iterator& itr,
                                                      difference_type count ) noexcept
          { return { itr.cursor_ + ( -count ) }; }
          friend constexpr unsafe_iterator operator-( difference_type count,
                                                      const unsafe_iterator& itr ) noexcept
          { return itr - count; }
          friend constexpr difference_type operator-( const unsafe_iterator& a,
                                                      const unsafe_iterator& b ) noexcept
          { return a.cursor_ - b.cursor_; }

          explicit constexpr operator bool() const noexcept { return cursor_ != nullptr; }
        };
        using reverse_unsafe_iterator = std::reverse_iterator<unsafe_iterator>;

      private:
        struct CoWBlock final {
          CoWRef* refs_;
          pointer ptr_;
          size_type capacity_;

          constexpr CoWBlock( CoWRef* refs, pointer ptr, size_type capacity ) noexcept
            : refs_ { refs }, ptr_ { std::move( ptr ) }, capacity_ { capacity }
          {}
          PACE__CXX20_CNSTXPR ~CoWBlock() = default;

          // it's extremely surprising that the type alias `pointer` of the allocator can be a fancy pointer
          PACE__FORCEINLINE constexpr Char* str() const noexcept { return utils::to_address( ptr_ ); }
        };
        union Payload {
          Char _; // this memeber is only used to control alignment
          const Char* literal_;
          CoWBlock remote_;

          constexpr Payload() noexcept : literal_ { nullptr } {}
          constexpr Payload( const Char* lit ) noexcept : literal_ { lit } {}
        };
        enum class Kind : std::uint8_t { Literal, Inline, Dynamic };

        Payload as_;
        size_type length_;
        Kind tag_;

        static PACE__CNSTEVAL size_type small_capacity() noexcept
        { return ( sizeof( Payload ) / sizeof( Char ) ) - 1; }
        static constexpr size_type dynamic_capacity( size_type old_cap ) noexcept
        { return static_cast<size_type>( old_cap * 1.5 ); }

        /// @brief Performs an in-place insertion of a character sequence into the buffer
        /// @param suffix Pointer to the start position where insertion happens
        /// @param num_wiped Number of characters to-be-overwritten at suffix
        /// @param suffix_len Length of the remaining suffix before insertion
        /// @param cstr Pointer to the characters to insert
        /// @param count Number of characters to insert
        static PACE__FORCEINLINE PACE__CXX14_CNSTXPR void embed( Char* suffix,
                                                                 size_type num_wiped,
                                                                 size_type suffix_len,
                                                                 const Char* cstr,
                                                                 size_type count ) noexcept
        {
          PACE__TRUST( suffix != cstr );
          PACE__TRUST( num_wiped <= suffix_len );
          const auto sublen = suffix_len - num_wiped;
          Traits::move( suffix + count, suffix + num_wiped, sublen );
          Traits::copy( suffix, cstr, count );
          Traits::assign( suffix[count + sublen], Char() );
        }
        /// @brief Performs an in-place insertion of a character sequence into the buffer
        /// @param suffix Pointer to the start position where insertion happens
        /// @param num_wiped Number of characters to-be-overwritten at `suffix`
        /// @param suffix_len Length of the remaining suffix before insertion
        /// @param count Number of characters to insert
        /// @param ch Character to be inserted repeatedly
        static PACE__FORCEINLINE PACE__CXX14_CNSTXPR void embed( Char* suffix,
                                                                 size_type num_wiped,
                                                                 size_type suffix_len,
                                                                 size_type count,
                                                                 Char ch ) noexcept
        {
          PACE__TRUST( num_wiped <= suffix_len );
          const auto sublen = suffix_len - num_wiped;
          Traits::move( suffix + count, suffix + num_wiped, sublen );
          Traits::assign( suffix, count, ch );
          Traits::assign( suffix[count + sublen], Char() );
        }

        // Concatenate the data into a different destination buffer composed of `prefix + inserted + suffix`
        // This function **does not** add a terminator at the end
        static PACE__FORCEINLINE PACE__CXX14_CNSTXPR void assemble( Char* dest,
                                                                    const Char* prefix,
                                                                    size_type prefix_len,
                                                                    const Char* suffix,
                                                                    size_type suffix_len,
                                                                    const Char* inserted,
                                                                    size_type count ) noexcept
        {
          PACE__TRUST( dest != prefix );
          PACE__TRUST( dest != suffix );
          PACE__TRUST( prefix < suffix );
          PACE__TRUST( dest != inserted );
          Traits::copy( dest, prefix, prefix_len );
          Traits::copy( dest + prefix_len, inserted, count );
          Traits::copy( dest + prefix_len + count, suffix, suffix_len );
        }

        PACE__CXX14_CNSTXPR void deallocate( pointer ptr, size_type cap ) noexcept
        { std::allocator_traits<Alloc>::deallocate( this->allocator(), ptr, cap ); }
        PACE__CXX14_CNSTXPR pointer allocate( size_type cap )
        { return std::allocator_traits<Alloc>::allocate( this->allocator(), cap ); }

        PACE__FORCEINLINE PACE__CXX20_CNSTXPR void throw_if_exceed( size_type expected_size ) const
          noexcept( false )
        {
          if ( expected_size > max_size() )
            PACE__UNLIKELY throw std::length_error( "pace: CoW string size would exceed max_size()" );
        }

        PACE__NODISCARD PACE__FORCEINLINE PACE__CXX20_CNSTXPR CoWBlock make_cow( size_type cap )
        {
          throw_if_exceed( cap - 1 );
          auto refs = ::new CoWRef( 1 );
          // Although we have a ScopeFail, we can't use it here
          // because ScopeFail doesn't meet the constexpr requirements under C++20.
          try {
            auto ptr = allocate( cap );
            (void)utils::start_lifetime_as_array<Char>( utils::to_address( ptr ), cap );
            return { refs, std::move( ptr ), cap };
          } catch ( ... ) {
            ::delete refs;
            throw;
          }
        }
        PACE__NODISCARD PACE__CXX20_CNSTXPR CoWBlock make_cow( const Char* first,
                                                               size_type count,
                                                               size_type capacity )
        {
          PACE__TRUST( capacity > count );
          const auto cow = make_cow( capacity );
          Traits::copy( cow.str(), first, count );
          Traits::assign( cow.str()[count], Char() );
          return cow;
        }
        PACE__NODISCARD PACE__FORCEINLINE PACE__CXX20_CNSTXPR CoWBlock
          make_cow( const CoWBlock& other ) noexcept
        {
          other.refs_->fetch_add( 1, std::memory_order_relaxed );
          return other;
        }
        PACE__FORCEINLINE PACE__CXX20_CNSTXPR void destroy_cow( CoWBlock& cow ) noexcept
        {
          if ( cow.refs_->fetch_sub( 1, std::memory_order_acq_rel ) == 1 ) {
            ::delete cow.refs_;
            deallocate( utils::exchange( cow.ptr_, pointer() ), utils::exchange( cow.capacity_, 0 ) );
          }
        }

        template<Kind Next, typename Operation>
        PACE__FORCEINLINE PACE__CXX20_CNSTXPR void transfer_to( size_type cap, Operation&& op )
        {
          static_assert( Next != Kind::Literal, "invalid next state" );
          if PACE__CXX17_CNSTXPR ( Next == Kind::Inline ) {
            const auto dest = utils::launder_as<Char>( &as_ );
            const size_type num_written =
              std::forward<Operation>( op )( utils::launder_as<Char>( &as_ ), small_capacity() );
            Traits::assign( dest[num_written], Char() );
            tag_ = Kind::Inline;
          } else if PACE__CXX17_CNSTXPR ( Next == Kind::Dynamic ) {
            PACE__TRUST( tag_ != Kind::Dynamic );
            PACE__TRUST( cap > 1 );
            throw_if_exceed( cap - 1 );
            auto cow = make_cow( cap );
            try {
              // cap should contain an additional terminator
              const size_type num_written = std::forward<Operation>( op )( cow.str(), cap - 1 );
              Traits::assign( cow.str()[num_written], Char() );
              utils::construct_at( &as_.remote_, cow );
              tag_ = Kind::Dynamic;
            } catch ( ... ) {
              destroy_cow( cow );
              throw;
            }
          }
        }
        // Transfers the string storage state to a specified state using a character range
        template<Kind Next>
        PACE__CXX20_CNSTXPR void transfer_to( const Char* first, size_type count, size_type cap )
          noexcept( Next == Kind::Inline )
        {
          PACE__TRUST( cap > count );
          transfer_to<Next>( cap, [&]( Char* dest, size_type new_cap ) noexcept {
            PACE__TRUST( count <= new_cap );
            (void)new_cap;
            Traits::copy( dest, first, count );
            return count;
          } );
        }
        // Transfers the string storage state to a specified state using a repeated character
        template<Kind Next>
        PACE__CXX20_CNSTXPR void transfer_to( size_type count, Char ch, size_type cap )
          noexcept( Next == Kind::Inline )
        {
          PACE__TRUST( cap > count );
          transfer_to<Next>( cap, [&]( Char* dest, size_type new_cap ) noexcept {
            PACE__TRUST( count <= new_cap );
            (void)new_cap;
            Traits::assign( dest, count, ch );
            return count;
          } );
        }
        // Transfers the string storage state to a specified state
        // while inserting data between prefix and suffix
        template<Kind Next>
        PACE__CXX20_CNSTXPR void transfer_to( const Char* prefix,
                                              size_type prefix_len,
                                              const Char* suffix,
                                              size_type suffix_len,
                                              const Char* inserted,
                                              size_type count ) noexcept( Next == Kind::Inline )
        {
          if PACE__CXX17_CNSTXPR ( Next == Kind::Inline )
            PACE__TRUST( tag_ != Kind::Inline );
          const auto total_length = prefix_len + suffix_len + count;
          transfer_to<Next>( (std::max)( total_length, dynamic_capacity( small_capacity() ) ) + 1,
                             [&]( Char* dest, size_type new_cap ) noexcept {
                               (void)new_cap;
                               assemble( dest, prefix, prefix_len, suffix, suffix_len, inserted, count );
                               return total_length;
                             } );
        }
        // Transfers the string storage state to a specified state
        // while inserting repeated characters between prefix and suffix
        template<Kind Next>
        PACE__CXX20_CNSTXPR void transfer_to( const Char* prefix,
                                              size_type prefix_len,
                                              const Char* suffix,
                                              size_type suffix_len,
                                              size_type count,
                                              Char inserted ) noexcept( Next == Kind::Inline )
        {
          if PACE__CXX17_CNSTXPR ( Next == Kind::Inline )
            PACE__TRUST( tag_ != Kind::Inline );
          const auto total_length = prefix_len + suffix_len + count;
          transfer_to<Next>( (std::max)( total_length, dynamic_capacity( small_capacity() ) ) + 1,
                             [&]( Char* dest, size_type new_cap ) noexcept {
                               PACE__TRUST( total_length <= new_cap );
                               (void)new_cap;
                               Traits::copy( dest, prefix, prefix_len );
                               Traits::assign( dest + prefix_len, count, inserted );
                               Traits::copy( dest + prefix_len + count, suffix, suffix_len );
                               return total_length;
                             } );
        }

        template<typename Operation>
        PACE__FORCEINLINE PACE__CXX20_CNSTXPR void duplicate_from( size_type cap, Operation&& op )
        {
          PACE__TRUST( tag_ == Kind::Dynamic );
          PACE__ASSERT( as_.remote_.refs_ > 1 );
          auto cow = make_cow( cap );
          try {
            const size_type num_written = std::forward<Operation>( op )( cow.str(), cap - 1 );
            PACE__TRUST( cap > num_written );
            Traits::assign( cow.str()[num_written], Char() );
            destroy_cow( as_.remote_ );
            as_.remote_ = cow;
          } catch ( ... ) {
            destroy_cow( cow );
            throw;
          }
        }
        // Duplicates the shared CoW buffer into a unique one containing the given data
        PACE__CXX20_CNSTXPR void duplicate_from( const Char* first, size_type count, size_type cap )
        {
          PACE__TRUST( cap > count );
          duplicate_from( cap, [&]( Char* dest, size_type new_cap ) noexcept {
            PACE__TRUST( count <= new_cap );
            (void)new_cap;
            Traits::copy( dest, first, count );
            return count;
          } );
        }
        // Duplicates the shared CoW buffer into a unique one while inserting data
        PACE__CXX20_CNSTXPR void duplicate_from( const Char* prefix,
                                                 size_type prefix_len,
                                                 const Char* suffix,
                                                 size_type suffix_len,
                                                 const Char* inserted,
                                                 size_type count )
        {
          const auto total_length = prefix_len + suffix_len + count;
          duplicate_from( (std::max)( total_length, dynamic_capacity( as_.remote_.capacity_ - 1 ) ) + 1,
                          [&]( Char* dest, size_type new_cap ) noexcept {
                            PACE__TRUST( count <= new_cap );
                            (void)new_cap;
                            assemble( dest, prefix, prefix_len, suffix, suffix_len, inserted, count );
                            return total_length;
                          } );
        }
        // Duplicates the shared CoW buffer into a unique one while inserting repeated characters
        PACE__CXX20_CNSTXPR void duplicate_from( const Char* prefix,
                                                 size_type prefix_len,
                                                 const Char* suffix,
                                                 size_type suffix_len,
                                                 size_type count,
                                                 Char inserted )
        {
          const auto total_length = prefix_len + suffix_len + count;
          duplicate_from( (std::max)( total_length, dynamic_capacity( as_.remote_.capacity_ - 1 ) ) + 1,
                          [&]( Char* dest, size_type new_cap ) noexcept {
                            PACE__TRUST( count <= new_cap );
                            (void)new_cap;
                            Traits::copy( dest, prefix, prefix_len );
                            Traits::assign( dest + prefix_len, count, inserted );
                            Traits::copy( dest + prefix_len + count, suffix, suffix_len );
                            return total_length;
                          } );
        }

        template<typename Operation>
        PACE__FORCEINLINE PACE__CXX20_CNSTXPR void expand_with( size_type cap, Operation&& op )
        {
          PACE__TRUST( tag_ == Kind::Dynamic );
          PACE__ASSERT( as_.remote_.refs_ == 1 );
          throw_if_exceed( cap - 1 );
          auto str = allocate( cap );
          try {
            const auto dest = utils::start_lifetime_as_array<Char>( utils::to_address( str ), cap );
            const size_type num_written = std::forward<Operation>( op )( dest, cap - 1 );
            Traits::assign( dest[num_written], Char() );
          } catch ( ... ) {
            deallocate( str, cap );
            throw;
          }
          deallocate( as_.remote_.ptr_, as_.remote_.capacity_ );
          std::tie( as_.remote_.ptr_, as_.remote_.capacity_ ) = std::make_pair( std::move( str ), cap );
        }
        // Reallocates the unique CoW buffer to a larger capacity and copies data into it
        PACE__CXX20_CNSTXPR void expand_with( const Char* first, size_type count, size_type cap )
        {
          PACE__ASSERT( cap > count );
          expand_with( cap, [&]( Char* dest, size_type new_cap ) noexcept {
            PACE__TRUST( count <= new_cap );
            (void)new_cap;
            Traits::copy( dest, first, count );
            return count;
          } );
        }
        // Reallocates and expands the unique CoW buffer while inserting data between prefix and suffix
        PACE__CXX20_CNSTXPR void expand_with( const Char* prefix,
                                              size_type prefix_len,
                                              const Char* suffix,
                                              size_type suffix_len,
                                              const Char* inserted,
                                              size_type count )
        {
          expand_with( dynamic_capacity( as_.remote_.capacity_ - 1 ) + 1,
                       [&]( Char* dest, size_type new_cap ) noexcept {
                         const auto total_length = prefix_len + suffix_len + count;
                         PACE__TRUST( total_length <= new_cap );
                         (void)new_cap;
                         assemble( dest, prefix, prefix_len, suffix, suffix_len, inserted, count );
                         return total_length;
                       } );
        }
        // Reallocates and expands the unique CoW buffer while inserting repeated characters
        PACE__CXX20_CNSTXPR void expand_with( const Char* prefix,
                                              size_type prefix_len,
                                              const Char* suffix,
                                              size_type suffix_len,
                                              size_type count,
                                              Char inserted )
        {
          expand_with( dynamic_capacity( as_.remote_.capacity_ - 1 ) + 1,
                       [&]( Char* dest, size_type new_cap ) noexcept {
                         const auto total_length = prefix_len + suffix_len + count;
                         PACE__TRUST( total_length <= new_cap );
                         (void)new_cap;
                         Traits::copy( dest, count, inserted );
                         Traits::assign( dest + prefix_len, prefix, prefix_len );
                         Traits::copy( dest + prefix_len + count, suffix, suffix_len );
                         return total_length;
                       } );
        }

        PACE__FORCEINLINE PACE__CXX20_CNSTXPR void destroy_self() noexcept
        {
          if ( tag_ == Kind::Dynamic )
            destroy_cow( as_.remote_ );
        }
        template<bool PropagativeAlloc>
        PACE__FORCEINLINE PACE__CXX20_CNSTXPR void propagate_self( const BasicCoWString& other )
          noexcept( std::allocator_traits<Alloc>::is_always_equal::value || PropagativeAlloc )
        {
          switch ( other.tag_ ) {
          case Kind::Literal: {
            destroy_self();
            utils::construct_at( &as_.literal_, other.as_.literal_ );
          } break;
          case Kind::Inline: {
            destroy_self();
            Traits::copy( utils::launder_as<Char>( &as_ ),
                          utils::launder_as<const Char>( &other.as_ ),
                          other.length_ + 1 );
          } break;
          case Kind::Dynamic: {
            if ( std::allocator_traits<Alloc>::is_always_equal::value || PropagativeAlloc
                 || this->allocator() == other.allocator() ) {
              destroy_self();
              utils::construct_at( &as_.remote_, make_cow( other.as_.remote_ ) );
            } else {
              const auto cow = make_cow( other.as_.remote_.str(), other.length_, other.length_ + 1 );
              destroy_self();
              utils::construct_at( &as_.remote_, cow );
            }
          } break;
          default: utils::unreachable();
          }
        }

      public:
        template<typename A>
        PACE__CXX20_CNSTXPR BasicCoWString( const std::basic_string<Char, Traits, A>& str )
          : BasicCoWString( str.data(), str.size() )
        {}
        template<typename A>
        PACE__CXX20_CNSTXPR BasicCoWString& operator=( const std::basic_string<Char, Traits, A>& str ) &
        { return assign( str.data(), str.size() ); }
        template<typename A>
        PACE__CXX20_CNSTXPR BasicCoWString& assign( const std::basic_string<Char, Traits, A>& str ) &
        { return assign( str.data(), str.size() ); }
        constexpr BasicCoWString( Literal<Char> literal_str ) noexcept
          : as_ { literal_str.data() }, length_ { literal_str.size() }, tag_ { Kind::Literal }
        {}
        PACE__CXX14_CNSTXPR BasicCoWString& operator=( Literal<Char> literal_str ) noexcept
        { return assign( std::move( literal_str ) ); }
        PACE__CXX14_CNSTXPR BasicCoWString& assign( Literal<Char> literal_str ) noexcept
        {
          switch ( tag_ ) {
          case Kind::Literal: as_.literal_ = literal_str.data(); break;
          case Kind::Dynamic: destroy_cow( as_.remote_ ); PACE__FALLTHROUGH;
          case Kind::Inline:  utils::construct_at( &as_.literal_, literal_str.data() ); break;
          default:            utils::unreachable();
          }
          length_ = literal_str.size();
          tag_    = Kind::Literal;
          return *this;
        }

        template<typename A>
        PACE__CXX20_CNSTXPR BasicCoWString& insert( size_type index,
                                                    const std::basic_string<Char, Traits, A>& str,
                                                    size_type str_index,
                                                    size_type count = npos )
        {
          count = (std::min)( count, str.size() - str_index );
          if ( index == length_ )
            return append( str, str_index, count );
          else if ( str_index > str.size() )
            PACE__UNLIKELY throw std::out_of_range( "pace: insert a std::string with an invalid subrange" );

          return insert( index, str.data() + str_index, count );
        }
        template<typename A>
        PACE__CXX20_CNSTXPR BasicCoWString& insert( size_type index,
                                                    const std::basic_string<Char, Traits, A>& str )
        { return insert( index, str, 0 ); }
        template<typename A>
        PACE__CXX20_CNSTXPR BasicCoWString& append( const std::basic_string<Char, Traits, A>& str,
                                                    size_type pos,
                                                    size_type count = npos )
        {
          if ( pos > str.size() )
            PACE__UNLIKELY throw std::out_of_range( "pace: append a std::string with an invalid subrange" );
          return append( str.data() + pos, (std::min)( count, str.size() - pos ) );
        }
        template<typename A>
        PACE__CXX20_CNSTXPR BasicCoWString& append( const std::basic_string<Char, Traits, A>& str )
        { return append( str, 0 ); }
        template<typename A>
        PACE__CXX20_CNSTXPR BasicCoWString& replace( size_type pos,
                                                     size_type count,
                                                     const std::basic_string<Char, Traits, A>& str,
                                                     size_type str_pos,
                                                     size_type str_count = npos )
        {
          if ( pos == length_ )
            return append( str, str_pos, str_count );
          str_count = (std::min)( str_count, str.size() - str_pos );
          if ( str_pos > str.size() )
            PACE__UNLIKELY throw std::out_of_range(
              "pace: replace the string content with a std::string that has an invalid subrange" );

          return replace( pos, count, str.data() + str_pos, str_count );
        }
        template<typename A>
        PACE__CXX20_CNSTXPR BasicCoWString& replace( size_type pos,
                                                     size_type count,
                                                     const std::basic_string<Char, Traits, A>& str )
        { return replace( pos, count, str, 0, str.size() ); }

        PACE__NODISCARD PACE__CXX14_CNSTXPR bool unique() const noexcept
        {
          switch ( tag_ ) {
          case Kind::Dynamic:
            if ( as_.remote_.refs_->load( std::memory_order_acquire ) == 1 )
              return true;
            PACE__FALLTHROUGH;
          case Kind::Literal: return false;
          default:            return true;
          }
        }
        PACE__CXX20_CNSTXPR BasicCoWString& isolate() &
        {
          reserve( 0 );
          return *this;
        }

        PACE__NODISCARD constexpr unsafe_iterator unsafe_begin() const& noexcept { return { data() }; }
        PACE__NODISCARD constexpr unsafe_iterator unsafe_begin() const&& noexcept { return begin(); }

        PACE__NODISCARD constexpr unsafe_iterator unsafe_end() const& noexcept
        { return { data() + length_ }; }
        PACE__NODISCARD constexpr const_iterator unsafe_end() const&& noexcept { return end(); }

        PACE__NODISCARD constexpr reverse_unsafe_iterator unsafe_rbegin() const& noexcept
        { return unsafe_iterator( data() + length_ - 1 ); }
        PACE__NODISCARD constexpr reverse_unsafe_iterator unsafe_rbegin() const&& noexcept
        { return rbegin(); }

        PACE__NODISCARD constexpr reverse_unsafe_iterator unsafe_rend() const& noexcept
        { return unsafe_iterator( data() - 1 ); }
        PACE__NODISCARD constexpr const_reverse_iterator unsafe_rend() const&& noexcept { return rend(); }

        template<typename A>
        PACE__CXX20_CNSTXPR BasicCoWString& operator+=( const std::basic_string<Char, Traits, A>& str )
        { return append( str.data(), str.size() ); }
        template<typename A>
        PACE__NODISCARD friend PACE__CXX20_CNSTXPR BasicCoWString
          operator+( const BasicCoWString& a, const std::basic_string<Char, Traits, A>& b )
        {
          auto ret = a;
          ret += b;
          return ret;
        }
        template<typename A>
        PACE__NODISCARD friend PACE__CXX20_CNSTXPR BasicCoWString
          operator+( const std::basic_string<Char, Traits, A>& a, const BasicCoWString& b )
        {
          BasicCoWString ret = a;
          ret.append( b );
          return ret;
        }
        template<typename A>
        PACE__NODISCARD friend PACE__CXX20_CNSTXPR BasicCoWString
          operator+( BasicCoWString&& a, const std::basic_string<Char, Traits, A>& b )
        {
          a += b;
          return std::move( a );
        }
        template<typename A>
        PACE__NODISCARD friend PACE__CXX20_CNSTXPR BasicCoWString
          operator+( const std::basic_string<Char, Traits, A>& a, BasicCoWString&& b )
        {
          b.insert( 0, a.data(), a.size() );
          return std::move( b );
        }

        template<typename A>
        PACE__NODISCARD friend constexpr bool operator==(
          const BasicCoWString& a,
          const std::basic_string<Char, Traits, A>& b ) noexcept
        { return a.compare( 0, a.length_, b.data(), b.size() ) == 0; }
        template<typename A>
        PACE__NODISCARD friend constexpr bool operator==( const std::basic_string<Char, Traits, A>& a,
                                                          const BasicCoWString& b ) noexcept
        { return b == a; }
        template<typename A>
        PACE__NODISCARD friend constexpr bool operator!=(
          const BasicCoWString& a,
          const std::basic_string<Char, Traits, A>& b ) noexcept
        { return !( a == b ); }
        template<typename A>
        PACE__NODISCARD friend constexpr bool operator!=( const std::basic_string<Char, Traits, A>& a,
                                                          const BasicCoWString& b ) noexcept
        { return b != a; }
#ifdef __cpp_lib_three_way_comparison
        template<typename A>
        PACE__NODISCARD friend constexpr auto operator<=>(
          const BasicCoWString& a,
          const std::basic_string<Char, Traits, A>& b ) noexcept
        {
          return static_cast<typename ComparisonCategory<Traits>::type>(
            a.compare( 0, a.length_, b.data(), b.size() ) <=> 0 );
        }
#else
        template<typename A>
        PACE__NODISCARD friend constexpr bool operator<( const BasicCoWString& a,
                                                         const std::basic_string<Char, Traits, A>& b )
        { return a.compare( 0, a.length_, b.data(), b.size() ) < 0; }
        template<typename A>
        PACE__NODISCARD friend constexpr bool operator<=( const BasicCoWString& a,
                                                          const std::basic_string<Char, Traits, A>& b )
        { return a.compare( 0, a.length_, b.data(), b.size() ) <= 0; }
        template<typename A>
        PACE__NODISCARD friend constexpr bool operator>( const BasicCoWString& a,
                                                         const std::basic_string<Char, Traits, A>& b )
        { return a.compare( 0, a.length_, b.data(), b.size() ) > 0; }
        template<typename A>
        PACE__NODISCARD friend constexpr bool operator>=( const BasicCoWString& a,
                                                          const std::basic_string<Char, Traits, A>& b )
        { return a.compare( 0, a.length_, b.data(), b.size() ) >= 0; }
        template<typename A>
        PACE__NODISCARD friend constexpr bool operator<( const std::basic_string<Char, Traits, A>& a,
                                                         const BasicCoWString& b )
        { return !( b >= a ); }
        template<typename A>
        PACE__NODISCARD friend constexpr bool operator<=( const std::basic_string<Char, Traits, A>& a,
                                                          const BasicCoWString& b )
        { return !( b > a ); }
        template<typename A>
        PACE__NODISCARD friend constexpr bool operator>( const std::basic_string<Char, Traits, A>& a,
                                                         const BasicCoWString& b )
        { return !( b <= a ); }
        template<typename A>
        PACE__NODISCARD friend constexpr bool operator>=( const std::basic_string<Char, Traits, A>& a,
                                                          const BasicCoWString& b )
        { return !( b < a ); }
#endif

        PACE__NODISCARD explicit constexpr operator std::basic_string<Char, Traits, Alloc>() const
        { return { data(), length_ }; }
#ifdef __cpp_lib_string_view
        PACE__NODISCARD constexpr operator std::basic_string_view<Char, Traits>() const noexcept
        { return { data(), length_ }; }
#endif

        PACE__CXX14_CNSTXPR BasicCoWString() noexcept( noexcept( Alloc() ) ) : BasicCoWString( Alloc() ) {}
        explicit PACE__CXX14_CNSTXPR BasicCoWString( const Alloc& alloc )
          noexcept( std::is_nothrow_copy_constructible<Alloc>::value )
          : CoWAllocator<Alloc>( alloc ), as_ {}, length_ { 0 }, tag_ { Kind::Inline }
        { Traits::assign( utils::launder_as<Char>( &as_ )[0], Char() ); }
        PACE__CXX20_CNSTXPR BasicCoWString( size_type count, Char ch, const Alloc& alloc = Alloc() )
          : BasicCoWString( alloc )
        { assign( count, ch ); }
        template<typename InputIt
#ifdef __cpp_lib_concepts
                 >
          requires( std::input_iterator<InputIt> && std::equality_comparable<InputIt> )
#else
                 ,
                 typename std::enable_if<is_legacy_input_iterator<InputIt>::value, bool>::type = 0>
#endif
        PACE__CXX20_CNSTXPR BasicCoWString( InputIt first, InputIt last, const Alloc& alloc = Alloc() )
          : BasicCoWString( alloc )
        {
          bool transfered = false;
          size_type cap = small_capacity() + 1, length = 0;

          pointer str;
          auto dest = utils::launder_as<Char>( &as_ );
          try {
            while ( first != last ) {
              Traits::assign( dest[length++], *( first++ ) );
              if ( length >= cap ) {
                if ( transfered ) {
                  const auto new_cap = dynamic_capacity( cap - 1 ) + 1;
                  throw_if_exceed( new_cap );
                  auto new_str = allocate( new_cap );
                  Traits::copy( utils::start_lifetime_as_array<Char>( utils::to_address( new_str ) ),
                                dest,
                                length );
                  deallocate( utils::exchange( str, std::move( new_str ) ), cap );
                  cap  = new_cap;
                  dest = utils::to_address( str );
                } else {
                  const auto new_cap = dynamic_capacity( cap ) + 1;
                  throw_if_exceed( new_cap );
                  str = allocate( new_cap );
                  Traits::copy( utils::to_address( str ), dest, length );
                  cap        = new_cap;
                  dest       = utils::to_address( str );
                  transfered = true;
                }
              }
            }
            if ( transfered ) {
              Traits::assign( dest[length], Char() );
              utils::construct_at( &as_.remote_, ::new CoWRef( 1 ), str, cap );
              tag_ = Kind::Dynamic;
            } else {
              Traits::assign( dest[length], Char() );
              tag_ = Kind::Inline;
            }
          } catch ( ... ) {
            if ( transfered )
              deallocate( std::move( str ), cap );
            throw;
          }
          length_ = length;
        }
        template<typename ForwardIt
#ifdef __cpp_lib_concepts
                 >
          requires( std::forward_iterator<ForwardIt> && std::equality_comparable<ForwardIt> )
#else
                 ,
                 typename std::enable_if<
                   traits::AllOf<std::is_base_of<std::forward_iterator_tag,
                                                 typename std::iterator_traits<ForwardIt>::iterator_category>,
                                 std::is_convertible<decltype( std::declval<const ForwardIt&>()
                                                               == std::declval<const ForwardIt&>() ),
                                                     bool>,
                                 std::is_convertible<decltype( std::declval<const ForwardIt&>()
                                                               != std::declval<const ForwardIt&>() ),
                                                     bool>>::value,
                   bool>::type = 0>
#endif
        PACE__CXX20_CNSTXPR BasicCoWString( ForwardIt first, ForwardIt last, const Alloc& alloc = Alloc() )
          : BasicCoWString( alloc )
        {
          const auto length = static_cast<size_type>( utils::distance( first, last ) );
          if ( length > small_capacity() ) {
            transfer_to<Kind::Dynamic>( (std::max)( length, dynamic_capacity( small_capacity() ) ) + 1,
                                        [&]( Char* dest, size_type new_cap ) {
                                          PACE__TRUST( length <= new_cap );
                                          (void)new_cap;
                                          std::copy( first, last, dest );
                                          return length;
                                        } );
          } else {
            transfer_to<Kind::Inline>( 0, [&]( Char* dest, size_type ) {
              std::copy( first, last, dest );
              return length;
            } );
          }
          length_ = length;
        }
#ifdef __cpp_lib_containers_ranges
        template<std::ranges::input_range R>
          requires( std::convertible_to<std::ranges::range_reference_t<R>, value_type> )
        PACE__CXX20_CNSTXPR BasicCoWString( std::from_range_t, R&& rg, const Alloc& alloc )
          : BasicCoWString( std::ranges::begin( rg ), std::ranges::end( rg ), alloc )
        {}
        template<std::ranges::forward_range R>
          requires( std::ranges::sized_range<R>
                    && std::convertible_to<std::ranges::range_reference_t<R>, value_type> )
        PACE__CXX20_CNSTXPR BasicCoWString( std::from_range_t, R&& rg, const Alloc& alloc )
          : BasicCoWString( alloc )
        {
          const auto length = static_cast<size_type>( utils::size( rg ) );
          if ( length > small_capacity() ) {
            transfer_to<Kind::Dynamic>( (std::max)( length, dynamic_capacity( small_capacity() ) ) + 1,
                                        [&]( Char* dest, size_type new_cap ) {
                                          PACE__TRUST( length <= new_cap );
                                          (void)new_cap;
                                          std::ranges::copy( std::forward<R>( rg ), dest );
                                          return length;
                                        } );
          } else {
            transfer_to<Kind::Inline>( 0, [&]( Char* dest, size_type ) {
              std::ranges::copy( std::forward<R>( rg ), dest );
              return length;
            } );
          }
          length_ = length;
        }
#endif
        PACE__CXX20_CNSTXPR BasicCoWString( const Char* cstr, size_type count, const Alloc& alloc = Alloc() )
          : BasicCoWString( alloc )
        { assign( cstr, count ); }
        constexpr BasicCoWString( const Char* cstr, const Alloc& alloc = Alloc() )
          : BasicCoWString( cstr, Traits::length( cstr ), alloc )
        {}
        BasicCoWString( std::nullptr_t ) = delete;
#ifdef __cpp_lib_string_view
        template<typename StringViewLike,
                 typename = typename std::enable_if<is_string_view_like<StringViewLike>::value>::type>
        explicit PACE__CXX20_CNSTXPR BasicCoWString( const StringViewLike& str_v,
                                                     const Alloc& alloc = Alloc() )
          : BasicCoWString( alloc )
        {
          std::basic_string_view<Char, Traits> sv = str_v;
          assign( sv.data(), sv.size() );
        }
        template<typename StringViewLike,
                 typename = typename std::enable_if<is_string_view_like<StringViewLike>::value>::type>
        PACE__CXX20_CNSTXPR BasicCoWString( const StringViewLike& str_v,
                                            size_type pos,
                                            size_type count,
                                            const Alloc& alloc = Alloc() )
          : BasicCoWString( alloc )
        { assign( str_v, pos, count ); }
#endif
        PACE__CXX20_CNSTXPR BasicCoWString( const BasicCoWString& other )
          noexcept( std::is_nothrow_copy_constructible<Alloc>::value
                    && noexcept( std::allocator_traits<Alloc>::select_on_container_copy_construction(
                      other.allocator() ) ) )
          : BasicCoWString(
              std::allocator_traits<Alloc>::select_on_container_copy_construction( other.allocator() ) )
        { assign( other ); }
        PACE__CXX20_CNSTXPR BasicCoWString( BasicCoWString&& rhs ) noexcept
          : CoWAllocator<Alloc>( std::move( rhs.allocator() ) ), as_ {}, length_ { 0 }, tag_ { Kind::Inline }
        {
          PACE__TRUST( this != &rhs );
          switch ( rhs.tag_ ) {
          case Kind::Literal: utils::construct_at( &as_.literal_, rhs.as_.literal_ ); break;
          case Kind::Inline:
            Traits::copy( utils::launder_as<Char>( &as_ ),
                          utils::launder_as<Char>( &rhs.as_ ),
                          rhs.length_ + 1 );
            break;
          case Kind::Dynamic: utils::construct_at( &as_.remote_, rhs.as_.remote_ ); break;
          default:            utils::unreachable();
          }
          length_ = utils::exchange( rhs.length_, 0 );
          tag_    = utils::exchange( rhs.tag_, Kind::Inline );
          Traits::assign( utils::launder_as<Char>( &rhs.as_ )[0], Char() );
        }
        PACE__CXX20_CNSTXPR BasicCoWString( const BasicCoWString& other, const Alloc& alloc )
          noexcept( traits::AllOf<std::is_nothrow_copy_constructible<Alloc>,
                                  typename std::allocator_traits<Alloc>::is_always_equal>::value )
          : BasicCoWString( alloc )
        {
          switch ( other.tag_ ) {
          case Kind::Literal: utils::construct_at( &as_.literal_, other.as_.literal_ ); break;
          case Kind::Inline:  {
            PACE__ASSERT( rhs.length_ <= small_capacity() );
            Traits::copy( utils::launder_as<Char>( &as_ ),
                          utils::launder_as<const Char>( &other.as_ ),
                          other.length_ + 1 );
          } break;
          case Kind::Dynamic: {
            if ( std::allocator_traits<Alloc>::is_always_equal::value
                 || this->allocator() == other.allocator() )
              utils::construct_at( &as_.remote_, make_cow( other.as_.remote_ ) );
            else
              utils::construct_at( &as_.remote_,
                                   make_cow( other.as_.remote_.str(), other.length_, other.length_ + 1 ) );
          } break;
          default: utils::unreachable();
          }
          tag_    = other.tag_;
          length_ = other.length_;
        }
        PACE__CXX20_CNSTXPR BasicCoWString( BasicCoWString&& rhs, const Alloc& alloc )
          noexcept( traits::AllOf<std::is_nothrow_copy_constructible<Alloc>,
                                  typename std::allocator_traits<Alloc>::is_always_equal>::value )
          : BasicCoWString( alloc )
        {
          switch ( rhs.tag_ ) {
          case Kind::Literal: utils::construct_at( &as_.literal_, rhs.as_.literal_ ); break;
          case Kind::Inline:  {
            PACE__ASSERT( rhs.length_ <= small_capacity() );
            Traits::copy( utils::launder_as<Char>( &as_ ),
                          utils::launder_as<Char>( &rhs.as_ ),
                          rhs.length_ + 1 );
          } break;
          case Kind::Dynamic: {
            if ( std::allocator_traits<Alloc>::is_always_equal::value
                 || this->allocator() == rhs.allocator() )
              utils::construct_at( &as_.remote_, rhs.as_.remote_ );
            else {
              utils::construct_at( &as_.remote_,
                                   make_cow( rhs.as_.remote_.str(), rhs.length_, rhs.length_ + 1 ) );
              destroy_cow( rhs.as_.remote_ );
            }
          } break;
          default: utils::unreachable();
          }
          tag_    = rhs.tag_;
          length_ = rhs.length_;
          Traits::assign( utils::launder_as<Char>( &rhs.as_ )[0], Char() );
          rhs.length_ = 0;
          rhs.tag_    = Kind::Inline;
        }
        constexpr BasicCoWString( const BasicCoWString& other, size_type pos, const Alloc& alloc = Alloc() )
          : BasicCoWString( other, pos, npos, alloc )
        {}
        constexpr BasicCoWString( BasicCoWString&& rhs, size_type pos, const Alloc& alloc = Alloc() )
          : BasicCoWString( std::move( rhs ), pos, npos, alloc )
        {}
        PACE__CXX20_CNSTXPR BasicCoWString( const BasicCoWString& other,
                                            size_type pos,
                                            size_type count,
                                            const Alloc& alloc = Alloc() )
          : BasicCoWString( alloc )
        {
          if ( pos > other.length_ )
            PACE__UNLIKELY throw std::out_of_range(
              "pace: construct a CoW string using a const-lvalue string with invalid subrange" );
          count = (std::min)( pos, other.length_ - pos );
          switch ( other.tag_ ) {
          case Kind::Literal: {
            if ( pos == 0 && count == other.length_ ) {
              utils::construct_at( &as_.literal_, other.as_.literal_ );
              tag_ = Kind::Literal;
            } else if ( count <= small_capacity() )
              transfer_to<Kind::Inline>( other.as_.literal_ + pos, count, 0 );
            else {
              utils::construct_at( &as_.remote_, make_cow( other.as_.literal_ + pos, count, count + 1 ) );
              tag_ = Kind::Dynamic;
            }
          } break;
          case Kind::Inline: {
            PACE__ASSERT( rhs.length_ <= small_capacity() );
            transfer_to<Kind::Inline>( utils::launder_as<const Char>( &other.as_ ) + pos, count, 0 );
          } break;
          case Kind::Dynamic: {
            if ( ( std::allocator_traits<Alloc>::is_always_equal::value
                   || this->allocator() == other.allocator() )
                 && pos == 0 && count == other.length_ )
              utils::construct_at( &as_.remote_, make_cow( other.as_.remote_ ) );
            else
              utils::construct_at( &as_.remote_,
                                   make_cow( other.as_.remote_.str() + pos, count, count + 1 ) );
            tag_ = Kind::Dynamic;
          } break;
          default: utils::unreachable();
          }
          length_ = count;
        }
        PACE__CXX20_CNSTXPR BasicCoWString( BasicCoWString&& rhs,
                                            size_type pos,
                                            size_type count,
                                            const Alloc& alloc = Alloc() )
          : BasicCoWString( alloc )
        {
          if ( pos > rhs.length_ )
            PACE__UNLIKELY throw std::out_of_range(
              "pace: construct a CoW string using a rvalue string with invalid subrange" );
          count = (std::min)( pos, rhs.length_ - pos );
          switch ( rhs.tag_ ) {
          case Kind::Literal: {
            if ( pos == 0 && count == rhs.length_ )
              utils::construct_at( &as_.literal_, rhs.as_.literal_ );
            else if ( count > small_capacity() )
              transfer_to<Kind::Dynamic>( rhs.as_.literal_ + pos,
                                          count,
                                          (std::max)( count, dynamic_capacity( small_capacity() ) ) + 1 );
            else
              transfer_to<Kind::Inline>( rhs.as_.literal_ + pos, count, 0 );
          } break;
          case Kind::Inline: {
            PACE__ASSERT( rhs.length_ <= small_capacity() );
            transfer_to<Kind::Inline>( utils::launder_as<Char>( &rhs.as_ ) + pos, count, 0 );
          } break;
          case Kind::Dynamic: {
            if ( ( std::allocator_traits<Alloc>::is_always_equal::value
                   || this->allocator() == rhs.allocator() )
                 && ( ( pos == 0 && count == rhs.length_ )
                      || rhs.as_.remote_.refs_->load( std::memory_order_acquire ) == 1 ) ) {
              utils::construct_at( &as_.remote_, rhs.as_.remote_ );
              if ( pos != 0 || count < rhs.length_ )
                Traits::move( as_.remote_.str(), as_.remote_.str() + pos, count );
            } else {
              utils::construct_at( &as_.remote_, make_cow( rhs.as_.remote_.str() + pos, count, count + 1 ) );
              destroy_cow( rhs.as_.remote_ );
            }
            tag_ = Kind::Dynamic;
          } break;
          default: utils::unreachable();
          }
          length_ = count;
          Traits::assign( utils::launder_as<Char>( &rhs.as_ )[0], Char() );
          rhs.length_ = 0;
          rhs.tag_    = Kind::Inline;
        }
        // BasicCoWString( std::initializer_list<Char>, const Alloc&  = Alloc() ) = delete;

        PACE__CXX20_CNSTXPR ~BasicCoWString() noexcept { destroy_self(); }

        PACE__CXX20_CNSTXPR BasicCoWString& operator=( const BasicCoWString& other ) & noexcept(
          traits::AnyOf<typename std::allocator_traits<Alloc>::propagate_on_container_copy_assignment,
                        typename std::allocator_traits<Alloc>::is_always_equal>::value )
        { return assign( other ); }
        PACE__CXX20_CNSTXPR BasicCoWString& operator=( BasicCoWString&& rhs ) & noexcept(
          traits::AnyOf<typename std::allocator_traits<Alloc>::propagate_on_container_move_assignment,
                        typename std::allocator_traits<Alloc>::is_always_equal>::value )
        { return assign( std::move( rhs ) ); }
        PACE__CXX20_CNSTXPR BasicCoWString& operator=( const Char* cstr ) & { return assign( cstr ); }
        PACE__CXX20_CNSTXPR BasicCoWString& operator=( Char ch ) & noexcept( small_capacity() >= 1 )
        { return assign( 1, ch ); }
        // BasicCoWString& operator=( std::initializer_list<Char> ) = delete;
#ifdef __cpp_lib_string_view
        template<typename StringViewLike>
        PACE__CXX20_CNSTXPR
          typename std::enable_if<is_string_view_like<StringViewLike>::value, BasicCoWString&>::type
          operator=( const StringViewLike& str_v ) &
        { return assign( str_v ); }
#endif
        BasicCoWString& operator=( std::nullptr_t ) = delete;

        PACE__CXX20_CNSTXPR BasicCoWString& assign( const BasicCoWString& other ) & noexcept(
          traits::AnyOf<typename std::allocator_traits<Alloc>::propagate_on_container_copy_assignment,
                        typename std::allocator_traits<Alloc>::is_always_equal>::value )
        { // self-assignment without extra parameters is invalid
          PACE__TRUST( this != &other );
          this->CoWCopyAlloc<Alloc, BasicCoWString>::operator=( other );
          propagate_self<
            traits::AnyOf<typename std::allocator_traits<Alloc>::propagate_on_container_copy_assignment,
                          typename std::allocator_traits<Alloc>::is_always_equal>::value>( other );
          length_ = other.length_;
          tag_    = other.tag_;
          return *this;
        }
        PACE__CXX20_CNSTXPR BasicCoWString& assign( BasicCoWString&& rhs ) & noexcept(
          traits::AnyOf<typename std::allocator_traits<Alloc>::propagate_on_container_move_assignment,
                        typename std::allocator_traits<Alloc>::is_always_equal>::value )
        {
          PACE__TRUST( this != &rhs );
          this->CoWMoveAlloc<Alloc, BasicCoWString>::operator=( std::move( rhs ) );
          propagate_self<std::allocator_traits<Alloc>::is_always_equal::value>( rhs );
          length_ = utils::exchange( rhs.length_, 0 );
          tag_    = utils::exchange( rhs.tag_, Kind::Inline );
          Traits::assign( utils::launder_as<Char>( &rhs.as_ )[0], Char() );
          return *this;
        }
        PACE__CXX20_CNSTXPR BasicCoWString& assign( size_type count, Char ch ) &
        {
          switch ( tag_ ) {
          case Kind::Literal: PACE__FALLTHROUGH;
          case Kind::Inline:  {
            if ( count > small_capacity() )
              transfer_to<Kind::Dynamic>( count,
                                          ch,
                                          (std::max)( count, dynamic_capacity( small_capacity() ) ) + 1 );
            else
              transfer_to<Kind::Inline>( count, ch, 0 );
          } break;
          case Kind::Dynamic: {
            if ( as_.remote_.refs_->load( std::memory_order_acquire ) > 1 )
              expand_with( nullptr, 0, nullptr, 0, count, ch );
            else if ( as_.remote_.capacity_ <= count )
              duplicate_from( nullptr, 0, nullptr, 0, count, ch );
            else {
              Traits::assign( as_.remote_.str(), count, ch );
              Traits::assign( as_.remote_.str()[count], Char() );
            }
          } break;
          default: utils::unreachable();
          }
          length_ = count;
          return *this;
        }
        PACE__CXX20_CNSTXPR BasicCoWString& assign( const Char* cstr, size_type count ) &
        {
          PACE__TRUST( cstr != nullptr );
          switch ( tag_ ) {
          case Kind::Literal: PACE__FALLTHROUGH;
          case Kind::Inline:  {
            if ( count > small_capacity() )
              transfer_to<Kind::Dynamic>( cstr,
                                          count,
                                          (std::max)( count, dynamic_capacity( small_capacity() ) ) + 1 );
            else
              transfer_to<Kind::Inline>( cstr, count, 0 );
          } break;
          case Kind::Dynamic: {
            if ( as_.remote_.refs_->load( std::memory_order_acquire ) > 1 )
              duplicate_from( cstr, count, count + 1 );
            else if ( as_.remote_.capacity_ <= count )
              expand_with( cstr, count, dynamic_capacity( as_.remote_.capacity_ - 1 ) + 1 );
            else {
              Traits::copy( as_.remote_.str(), cstr, count );
              Traits::assign( as_.remote_.str()[count], Char() );
            }
          } break;
          default: utils::unreachable();
          }
          length_ = count;
          return *this;
        }
        PACE__CXX20_CNSTXPR BasicCoWString& assign( const Char* cstr ) &
        {
          PACE__TRUST( cstr != nullptr );
          return assign( cstr, Traits::length( cstr ) );
        }
#ifdef __cpp_lib_string_view
        template<typename StringViewLike>
        PACE__CXX20_CNSTXPR
          typename std::enable_if<is_string_view_like<StringViewLike>::value, BasicCoWString&>::type
          assign( const StringViewLike& str_v, size_type pos, size_type count = npos ) &
        {
          std::basic_string_view<Char, Traits> sv = str_v;
          if ( pos > sv.size() )
            PACE__UNLIKELY throw std::out_of_range( "pace: assign a sub-string view with invalid subrange" );
          return assign( sv.substr( pos, count ) );
        }
        template<typename StringViewLike>
        PACE__CXX20_CNSTXPR
          typename std::enable_if<is_string_view_like<StringViewLike>::value, BasicCoWString&>::type
          assign( const StringViewLike& str_v ) &
        {
          std::basic_string_view<Char, Traits> sv = str_v;
          if ( sv.empty() ) {
            clear();
            return *this;
          }
          return assign( sv.data(), sv.size() );
        }
#endif
        PACE__CXX20_CNSTXPR BasicCoWString& assign( const BasicCoWString& other,
                                                    size_type pos,
                                                    size_type count = npos ) &
        {
          if ( pos > other.length_ )
            PACE__UNLIKELY throw std::out_of_range(
              "pace: the CoW sub-string of the const-lvalue is out of range in assign()" );

          count = (std::min)( count, other.length_ - pos );
          if ( this == &other ) {
            if ( pos == 0 && count == length_ )
              return *this;
            const auto remaining_length = length_ - ( pos + count );
            switch ( tag_ ) {
            case Kind::Literal: {
              if ( remaining_length > small_capacity() )
                transfer_to<Kind::Dynamic>(
                  as_.literal_ + pos,
                  count,
                  (std::max)( remaining_length, dynamic_capacity( small_capacity() ) ) + 1 );
              else
                transfer_to<Kind::Inline>( as_.literal_ + pos, count, 0 );
            } break;
            case Kind::Inline:  embed( utils::launder_as<Char>( &as_ ), pos, pos + count, nullptr, 0 ); break;
            case Kind::Dynamic: {
              if ( as_.remote_.refs_->load( std::memory_order_acquire ) > 1 )
                duplicate_from( as_.remote_.str() + pos, count, remaining_length + 1 );
              else
                embed( as_.remote_.str(), pos, pos + count, nullptr, 0 );
            } break;
            default: utils::unreachable();
            }
            length_ = remaining_length;
          } else if ( pos == 0 && count == other.length_
                      && ( std::allocator_traits<Alloc>::is_always_equal::value
                           || this->allocator() == other.allocator() ) ) {
            // The post-C++20 standards explicitly said that this method does not propagate allocator.
            // Therefore we cannot call `assign( other )` alternatively.
            destroy_self();
            switch ( other.tag_ ) {
            case Kind::Literal: utils::construct_at( &as_.literal_, other.as_.literal_ ); break;
            case Kind::Inline:
              Traits::copy( utils::launder_as<Char>( &as_ ),
                            utils::launder_as<const Char>( &other.as_ ),
                            other.length_ + 1 );
              break;
            case Kind::Dynamic: utils::construct_at( &as_.remote_, make_cow( other.as_.remote_ ) ); break;
            default:            utils::unreachable();
            }
            length_ = other.length_;
            tag_    = other.tag_;
          } else
            return assign( other.data() + pos, count );
          return *this;
        }
        template<typename InputIt>
#ifdef __cpp_lib_concepts
          requires( std::input_iterator<InputIt> && std::equality_comparable<InputIt> )
        PACE__CXX20_CNSTXPR BasicCoWString&
#else
        PACE__CXX20_CNSTXPR
          typename std::enable_if<is_legacy_input_iterator<InputIt>::value, BasicCoWString&>::type
#endif
          assign( InputIt first, InputIt last ) &
        { return assign( BasicCoWString( std::move( first ), std::move( last ), this->allocator() ) ); }
        // BasicCoWString& assign( std::initializer_list<Char> ) = delete;

#ifdef __cpp_lib_containers_ranges
        template<typename R>
          requires( std::ranges::input_range<R>
                    && std::convertible_to<std::ranges::range_reference_t<R>, Char> )
        PACE__CXX20_CNSTXPR BasicCoWString& assign_range( R&& rg )
        { return assign( BasicCoWString( std::from_range, std::forward<R>( rg ), this->allocator() ) ); }
#endif

        constexpr allocator_type get_allocator() const { return this->allocator(); }

        PACE__NODISCARD PACE__CXX20_CNSTXPR Char* data() &
        {
          switch ( tag_ ) {
          case Kind::Literal: {
            if ( length_ > small_capacity() )
              transfer_to<Kind::Dynamic>( as_.literal_, length_, length_ + 1 );
            else
              transfer_to<Kind::Inline>( as_.literal_, length_, 0 );
          }
            PACE__FALLTHROUGH;
          case Kind::Inline:  return utils::launder_as<Char>( &as_ );
          case Kind::Dynamic: {
            if ( as_.remote_.refs_->load( std::memory_order_acquire ) > 1 )
              duplicate_from( as_.remote_.str(), length_, length_ + 1 );
            return as_.remote_.str();
          }
          default: utils::unreachable();
          }
        }
        PACE__CXX14_CNSTXPR const Char* data() const& noexcept
        {
          switch ( tag_ ) {
          case Kind::Literal: return as_.literal_;
          case Kind::Inline:  return utils::launder_as<const Char>( &as_ );
          case Kind::Dynamic: return as_.remote_.str();
          default:            utils::unreachable();
          }
        }
        constexpr const Char* data() const&& noexcept { return data(); }
        constexpr const Char* c_str() const noexcept { return data(); }

        PACE__NODISCARD PACE__CXX14_CNSTXPR Char& operator[]( size_type pos ) &
        {
          PACE__TRUST( pos <= length_ );
          return data()[pos];
        }
        PACE__CXX14_CNSTXPR const Char& operator[]( size_type pos ) const& noexcept
        {
          PACE__TRUST( pos <= length_ );
          return data()[pos];
        }
        constexpr const Char& operator[]( size_type pos ) const&& noexcept { return operator[]( pos ); }
        PACE__NODISCARD PACE__CXX20_CNSTXPR Char& at( size_type pos ) &
        {
          if ( pos > length_ )
            PACE__UNLIKELY throw std::out_of_range( "pace: accessed position is out of range" );
          return ( *this )[pos];
        }
        PACE__CXX20_CNSTXPR const Char& at( size_type pos ) const&
        {
          if ( pos > length_ )
            PACE__UNLIKELY throw std::out_of_range( "pace: accessed position is out of range" );
          return ( *this )[pos];
        }
        PACE__CXX20_CNSTXPR const Char& at( size_type pos ) const&& { return at( pos ); }

        PACE__NODISCARD PACE__CXX14_CNSTXPR Char& front() & { return ( *this )[0]; }
        constexpr const Char& front() const& noexcept { return ( *this )[0]; }
        constexpr const Char& front() const&& noexcept { return front(); }

        PACE__NODISCARD PACE__CXX14_CNSTXPR Char& back() & { return ( *this )[length_ - 1]; }
        constexpr const Char& back() const& noexcept { return ( *this )[length_ - 1]; }
        constexpr const Char& back() const&& noexcept { return back(); }

        PACE__NODISCARD PACE__CXX14_CNSTXPR iterator begin() & noexcept { return iterator( *this, 0 ); }
        PACE__NODISCARD constexpr const_iterator begin() const& noexcept
        { return const_iterator( *this, 0 ); }
        PACE__NODISCARD constexpr const_iterator begin() const&& noexcept { return begin(); }
        PACE__NODISCARD constexpr const_iterator cbegin() const noexcept { return begin(); }

        PACE__NODISCARD PACE__CXX14_CNSTXPR iterator end() & noexcept { return iterator( *this, length_ ); }
        PACE__NODISCARD constexpr const_iterator end() const& noexcept
        { return const_iterator( *this, length_ ); }
        PACE__NODISCARD constexpr const_iterator end() const&& noexcept { return end(); }
        PACE__NODISCARD constexpr const_iterator cend() const noexcept { return end(); }

        PACE__NODISCARD PACE__CXX14_CNSTXPR reverse_iterator rbegin() & noexcept
        { return iterator( *this, length_ - 1 ); }
        PACE__NODISCARD constexpr const_reverse_iterator rbegin() const& noexcept
        { return const_iterator( *this, length_ - 1 ); }
        PACE__NODISCARD constexpr const_reverse_iterator rbegin() const&& noexcept { return rbegin(); }
        PACE__NODISCARD constexpr const_reverse_iterator crbegin() const noexcept { return rbegin(); }

        PACE__NODISCARD PACE__CXX14_CNSTXPR reverse_iterator rend() & noexcept
        { // The previous index of 0 in size_type is npos.
          return iterator( *this, npos );
        }
        PACE__NODISCARD constexpr const_reverse_iterator rend() const& noexcept
        { return const_iterator( *this, npos ); }
        PACE__NODISCARD constexpr const_reverse_iterator rend() const&& noexcept { return rend(); }
        PACE__NODISCARD constexpr const_reverse_iterator crend() const noexcept { return end(); }

        PACE__NODISCARD constexpr bool empty() const noexcept { return length_ == 0; }
        PACE__NODISCARD constexpr size_type size() const noexcept { return length_; }
        PACE__NODISCARD constexpr size_type length() const noexcept { return size(); }
        PACE__NODISCARD constexpr size_type max_size() const noexcept
        { return std::allocator_traits<Alloc>::max_size( this->allocator() ) - 1; }
        PACE__NODISCARD PACE__CXX14_CNSTXPR size_type capacity() const noexcept
        {
          switch ( tag_ ) {
          case Kind::Inline: return small_capacity();
          case Kind::Dynamic:
            return as_.remote_.refs_->load( std::memory_order_acquire ) > 1 ? 0 : as_.remote_.capacity_ - 1;
          default: return 0;
          }
        }

        PACE__CXX20_CNSTXPR void reserve( size_type new_cap ) &
        {
          new_cap = (std::max)( length_, new_cap );
          switch ( tag_ ) {
          case Kind::Literal: {
            if ( new_cap > small_capacity() )
              transfer_to<Kind::Dynamic>( as_.literal_, length_, new_cap + 1 );
            else
              transfer_to<Kind::Inline>( as_.literal_, length_, 0 );
          } break;
          case Kind::Inline: {
            if ( new_cap > small_capacity() )
              transfer_to<Kind::Dynamic>( utils::launder_as<Char>( &as_ ), length_, new_cap + 1 );
          } break;
          case Kind::Dynamic: {
            PACE__ASSERT( length_ <= max_size() );
            if ( as_.remote_.refs_->load( std::memory_order_acquire ) > 1 ) {
              new_cap = (std::max)( as_.remote_.capacity_, new_cap + 1 );
              duplicate_from( as_.remote_.str(), length_, new_cap );
            } else if ( as_.remote_.capacity_ <= new_cap )
              expand_with( as_.remote_.str(), length_, new_cap + 1 );
          } break;
          default: utils::unreachable();
          }
        }

        PACE__CXX20_CNSTXPR void shrink_to_fit() noexcept
        {
          if ( tag_ == Kind::Dynamic ) {
            if ( length_ <= small_capacity() ) {
              auto cow = as_.remote_;
              transfer_to<Kind::Inline>( cow.str(), length_, 0 );
              destroy_cow( cow );
            } else if ( as_.remote_.refs_->load( std::memory_order_acquire ) == 1
                        && length_ + 1 < as_.remote_.capacity_ )
              expand_with( as_.remote_.str(), length_, length_ + 1 );
          }
        }
        PACE__CXX20_CNSTXPR void clear() noexcept
        {
          switch ( tag_ ) {
          case Kind::Literal: tag_ = Kind::Inline; PACE__FALLTHROUGH;
          case Kind::Inline:  Traits::assign( utils::launder_as<Char>( &as_ )[0], Char() ); break;
          case Kind::Dynamic: {
            if ( as_.remote_.refs_->load( std::memory_order_acquire ) > 1 ) {
              destroy_cow( as_.remote_ );
              Traits::assign( utils::launder_as<Char>( &as_ )[0], Char() );
              tag_ = Kind::Inline;
            } else
              Traits::assign( as_.remote_.str()[0], Char() );
          } break;
          default: utils::unreachable();
          }
          length_ = 0;
        }

        PACE__CXX20_CNSTXPR BasicCoWString& insert( size_type index, const Char* cstr, size_type count )
        {
          if ( index > length_ )
            PACE__UNLIKELY throw std::out_of_range( "pace: insert c-style string at an invalid position" );
          else if ( index == length_ )
            return append( cstr, count );

          const auto total_length = length_ + count;
          switch ( tag_ ) {
          case Kind::Literal: {
            if ( total_length > small_capacity() )
              transfer_to<Kind::Dynamic>( as_.literal_,
                                          index,
                                          as_.literal_ + index,
                                          length_ - index,
                                          cstr,
                                          count );
            else
              transfer_to<Kind::Inline>( as_.literal_,
                                         index,
                                         as_.literal_ + index,
                                         length_ - index,
                                         cstr,
                                         count );
          } break;
          case Kind::Inline: {
            const auto src = utils::launder_as<Char>( &as_ );
            if ( total_length > small_capacity() )
              transfer_to<Kind::Dynamic>( src, index, src + index, length_ - index, cstr, count );
            else if ( cstr >= src && cstr < src + length_ ) { // self insert
              Char substring[small_capacity()];
              Traits::copy( substring, cstr, count );
              embed( src + index, 0, length_ - index, substring, count );
            } else
              embed( src + index, 0, length_ - index, cstr, count );
          } break;
          case Kind::Dynamic: {
            if ( as_.remote_.refs_->load( std::memory_order_acquire ) > 1 )
              duplicate_from( as_.remote_.str(),
                              index,
                              as_.remote_.str() + index,
                              length_ - index,
                              cstr,
                              count );
            else if ( as_.remote_.capacity_ <= total_length )
              expand_with( as_.remote_.str(),
                           index,
                           as_.remote_.str() + index,
                           length_ - index,
                           cstr,
                           count );
            else if ( cstr >= as_.remote_.str() && cstr < as_.remote_.str() + length_ ) {
              // self insert
              const auto offset = static_cast<size_type>( cstr - as_.remote_.str() );
              PACE__TRUST( offset <= length_ );
              Traits::move( as_.remote_.str() + index + count, as_.remote_.str() + index, count );
              Traits::assign( as_.remote_.str()[total_length], Char() );
              if ( offset + count < index )
                Traits::copy( as_.remote_.str() + index, cstr, count );
              else if ( offset < index ) {
                Traits::copy( as_.remote_.str() + index, cstr, index - offset );
                Traits::copy( as_.remote_.str() + 2 * index - offset,
                              as_.remote_.str() + index + count,
                              count - index + offset );
              } else
                Traits::copy( as_.remote_.str() + index, as_.remote_.str() + offset + count, count );
            } else
              embed( as_.remote_.str() + index, 0, length_ - index, cstr, count );
          } break;
          default: utils::unreachable();
          }
          length_ = total_length;
          return *this;
        }
        PACE__CXX20_CNSTXPR BasicCoWString& insert( size_type index, const Char* cstr )
        { return insert( index, cstr, Traits::length( cstr ) ); }
        PACE__CXX20_CNSTXPR BasicCoWString& insert( size_type index,
                                                    const BasicCoWString& other,
                                                    size_type other_index,
                                                    size_type count = npos )
        {
          count = (std::min)( count, other.length_ - other_index );
          if ( index == length_ )
            return append( other, other_index, count );
          else if ( other_index > other.length_ )
            PACE__UNLIKELY throw std::out_of_range(
              "pace: insert a CoW string itself with an invalid subrange" );

          return insert( index, other.data() + other_index, count );
        }
        PACE__CXX20_CNSTXPR BasicCoWString& insert( size_type index, const BasicCoWString& other )
        { return insert( index, other, 0 ); }
        PACE__CXX20_CNSTXPR BasicCoWString& insert( size_type index, size_type count, Char ch )
        {
          if ( index > length_ )
            PACE__UNLIKELY throw std::out_of_range( "pace: insert characters at an invalid position" );
          else if ( index == length_ )
            return append( count, ch );

          const auto total_length = length_ + count;
          switch ( tag_ ) {
          case Kind::Literal: {
            if ( total_length > small_capacity() )
              transfer_to<Kind::Dynamic>( as_.literal_,
                                          index,
                                          as_.literal_ + index,
                                          length_ - index,
                                          count,
                                          ch );
            else
              transfer_to<Kind::Inline>( as_.literal_,
                                         index,
                                         as_.literal_ + index,
                                         length_ - index,
                                         count,
                                         ch );
          } break;
          case Kind::Inline: {
            const auto src = utils::launder_as<Char>( &as_ );
            if ( total_length > small_capacity() )
              transfer_to<Kind::Dynamic>( src, index, src + index, length_ - index, count, ch );
            else
              embed( src + index, 0, length_ - index, count, ch );
          } break;
          case Kind::Dynamic: {
            if ( as_.remote_.refs_->load( std::memory_order_acquire ) > 1 )
              duplicate_from( as_.remote_.str(),
                              index,
                              as_.remote_.str() + index,
                              length_ - index,
                              count,
                              ch );
            else if ( as_.remote_.capacity_ <= total_length )
              expand_with( as_.remote_.str(), index, as_.remote_.str() + index, length_ - index, count, ch );
            else
              embed( as_.remote_.str() + index, 0, length_ - index, count, ch );
          } break;
          default: utils::unreachable();
          }
          length_ = total_length;
          return *this;
        }
        PACE__CXX20_CNSTXPR iterator insert( const_iterator pos, size_type count, Char ch )
        {
          PACE__ASSERT( this == pos.owner() );
          insert( pos.offset(), count, ch );
          return iterator( *this, pos.offset() + 1 );
        }
        PACE__CXX20_CNSTXPR iterator insert( const_iterator pos, Char ch ) { return insert( pos, 1, ch ); }
        template<typename InputIt>
#ifdef __cpp_lib_concepts
          requires( std::input_iterator<InputIt> && std::equality_comparable<InputIt> )
        PACE__CXX20_CNSTXPR BasicCoWString&
#else
        PACE__CXX20_CNSTXPR
          typename std::enable_if<is_legacy_input_iterator<InputIt>::value, BasicCoWString&>::type
#endif
          insert( const_iterator pos, InputIt first, InputIt last )
        {
          insert( pos - cbegin(),
                  BasicCoWString( std::move( first ), std::move( last ), this->allocator() ) );
        }
        // iterator insert(const_iterator, std::initializer_list<Char> ) = delete;
#ifdef __cpp_lib_string_view
        template<typename StringViewLike>
        PACE__CXX20_CNSTXPR
          typename std::enable_if<is_string_view_like<StringViewLike>::value, BasicCoWString&>::type
          insert( size_type index, const StringViewLike str_v, size_type sv_index, size_type count = npos )
        {
          std::basic_string_view<Char, Traits> sv = str_v;
          if ( sv_index > sv.size() )
            PACE__UNLIKELY throw std::out_of_range( "pace: insert a string view with an invalid subrange" );
          sv.remove_prefix( sv_index );
          count = (std::min)( count, sv.size() );
          return insert( index, sv.data(), count );
        }
        template<typename StringViewLike>
        PACE__CXX20_CNSTXPR
          typename std::enable_if<is_string_view_like<StringViewLike>::value, BasicCoWString&>::type
          insert( size_type index, const StringViewLike str_v )
        {
          std::basic_string_view<Char, Traits> sv = str_v;
          return insert( index, sv.data(), sv.size() );
        }
#endif

#ifdef __cpp_lib_containers_ranges
        template<typename R>
          requires( std::ranges::input_range<R>
                    && std::convertible_to<std::ranges::range_reference_t<R>, Char> )
        PACE__CXX20_CNSTXPR BasicCoWString& insert_range( const_iterator pos, R&& rg )
        {
          return insert( pos - cbegin(),
                         BasicCoWString( std::from_range, std::forward<R>( rg ), this->allocator() ) );
        }
#endif

        PACE__CXX20_CNSTXPR BasicCoWString& erase( size_type index = 0, size_type count = npos )
        {
          if ( index > length_ )
            PACE__UNLIKELY throw std::out_of_range( "pace: the erased range is invalid" );
          else if ( index == 0 && count >= length_ ) {
            clear();
            return *this;
          }

          count                       = (std::min)( count, length_ - index );
          const auto remaining_length = length_ - count;
          switch ( tag_ ) {
          case Kind::Literal: {
            if ( remaining_length > small_capacity() )
              transfer_to<Kind::Dynamic>( as_.literal_,
                                          index,
                                          as_.literal_ + index + count,
                                          length_ - ( index + count ),
                                          nullptr,
                                          0 );
            else
              transfer_to<Kind::Inline>( as_.literal_,
                                         index,
                                         as_.literal_ + index + count,
                                         length_ - ( index + count ),
                                         nullptr,
                                         0 );
          } break;
          case Kind::Inline: {
            const auto dest = utils::launder_as<Char>( &as_ );
            Traits::move( dest, dest + index + count, length_ - ( index + count ) );
            Traits::assign( dest[remaining_length], Char() );
          } break;
          case Kind::Dynamic: {
            if ( as_.remote_.refs_->load( std::memory_order_acquire ) > 1 )
              duplicate_from( as_.remote_.str(),
                              index,
                              as_.remote_.str() + index + count,
                              length_ - ( index + count ),
                              nullptr,
                              0 );
            else {
              Traits::move( as_.remote_.str() + index,
                            as_.remote_.str() + index + count,
                            length_ - ( index + count ) );
              Traits::assign( as_.remote_.str()[remaining_length], Char() );
            }
          } break;
          default: utils::unreachable();
          }
          length_ = remaining_length;
          return *this;
        }
        PACE__CXX20_CNSTXPR iterator erase( const_iterator first, const_iterator last ) noexcept
        {
          PACE__ASSERT( first.owner() == last.owner() );
          PACE__ASSERT( this == first.owner() );
          if ( first.offset() < length_ ) {
            PACE__ASSERT( last - first <= length_ );
            erase( first.offset(), last - first );
          }
          return { *this, first.offset() };
        }
        PACE__CXX20_CNSTXPR iterator erase( const_iterator position ) noexcept
        { return erase( position, cend() ); }

        PACE__CXX20_CNSTXPR void push_back( Char ch ) & { append( 1, ch ); }
        PACE__CXX20_CNSTXPR void pop_back() noexcept { erase( cend() - 1 ); }

        PACE__CXX20_CNSTXPR BasicCoWString& append( const Char* cstr, size_type count )
        {
          const auto total_length = count + length_;
          switch ( tag_ ) {
          case Kind::Literal: PACE__FALLTHROUGH;
          case Kind::Inline:  {
            const auto src = tag_ == Kind::Literal ? as_.literal_ : utils::launder_as<Char>( &as_ );
            if ( total_length > small_capacity() )
              transfer_to<Kind::Dynamic>( src, length_, cstr, count, nullptr, 0 );
            else if ( tag_ == Kind::Literal )
              transfer_to<Kind::Inline>( src, length_, cstr, count, nullptr, 0 );
            else {
              const auto writable_src = utils::launder_as<Char>( &as_ );
              Traits::copy( writable_src + length_, cstr, count );
              Traits::assign( writable_src[total_length], Char() );
            }
          } break;
          case Kind::Dynamic: {
            if ( as_.remote_.refs_->load( std::memory_order_acquire ) > 1 )
              duplicate_from( as_.remote_.str(), length_, cstr, count, nullptr, 0 );
            else if ( as_.remote_.capacity_ <= total_length )
              expand_with( as_.remote_.str(), length_, cstr, count, nullptr, 0 );
            else {
              Traits::copy( as_.remote_.str() + length_, cstr, count );
              Traits::assign( as_.remote_.str()[total_length], Char() );
            }
          } break;
          default: utils::unreachable();
          }
          length_ = total_length;
          return *this;
        }
        PACE__CXX20_CNSTXPR BasicCoWString& append( const Char* cstr )
        { return append( cstr, Traits::length( cstr ) ); }
        PACE__CXX20_CNSTXPR BasicCoWString& append( size_type count, Char ch )
        {
          const auto total_length = count + length_;
          switch ( tag_ ) {
          case Kind::Literal: PACE__FALLTHROUGH;
          case Kind::Inline:  {
            const Char* src = tag_ == Kind::Literal ? as_.literal_ : utils::launder_as<Char>( &as_ );
            Char* dest      = nullptr;
            if ( total_length > small_capacity() ) {
              transfer_to<Kind::Dynamic>( src,
                                          length_,
                                          (std::max)( total_length, dynamic_capacity( small_capacity() ) )
                                            + 1 );
              dest = as_.remote_.str();
            } else {
              if ( tag_ == Kind::Literal )
                transfer_to<Kind::Inline>( src, length_, 0 );
              dest = utils::launder_as<Char>( &as_ );
            }
            Traits::assign( dest + length_, count, ch );
            Traits::assign( dest[total_length], Char() );
          } break;
          case Kind::Dynamic: {
            auto append_to = [&]( Char* dest, size_type new_cap ) noexcept {
              PACE__TRUST( total_length <= new_cap );
              (void)new_cap;
              Traits::copy( dest, as_.remote_.str(), length_ );
              Traits::assign( dest + length_, count, ch );
              return total_length;
            };
            if ( as_.remote_.refs_->load( std::memory_order_acquire ) > 1 )
              duplicate_from( total_length + 1, std::move( append_to ) );
            else if ( as_.remote_.capacity_ <= total_length )
              expand_with( dynamic_capacity( as_.remote_.capacity_ - 1 ) + 1, std::move( append_to ) );
            else {
              Traits::assign( as_.remote_.str() + length_, count, ch );
              Traits::assign( as_.remote_.str()[total_length], Char() );
            }
          } break;
          default: utils::unreachable();
          }
          length_ = total_length;
          return *this;
        }
#ifdef __cpp_lib_string_view
        template<typename StringViewLike>
        PACE__CXX20_CNSTXPR
          typename std::enable_if<is_string_view_like<StringViewLike>::value, BasicCoWString&>::type
          append( const StringViewLike& str_v, size_type pos, size_type count = npos )
        {
          std::basic_string_view<Char, Traits> sv = str_v;
          if ( pos > length_ )
            PACE__UNLIKELY throw std::out_of_range( "pace: append a string view with an invalid subrange" );
          return append( sv.substr( pos, count ) );
        }
        template<typename StringViewLike>
        PACE__CXX20_CNSTXPR
          typename std::enable_if<is_string_view_like<StringViewLike>::value, BasicCoWString&>::type
          append( const StringViewLike& str_v )
        {
          std::basic_string_view<Char, Traits> sv = str_v;
          return append( sv.data(), sv.size() );
        }
#endif
        PACE__CXX20_CNSTXPR BasicCoWString& append( const BasicCoWString& other,
                                                    size_type pos,
                                                    size_type count = npos )
        {
          if ( pos > other.length_ )
            PACE__UNLIKELY throw std::out_of_range(
              "pace: append a CoW string itself with an invalid subrange" );

          return append( other.data() + pos, count );
        }
        PACE__CXX20_CNSTXPR BasicCoWString& append( const BasicCoWString& other )
        { return append( other, 0 ); }
        template<typename InputIt>
#ifdef __cpp_lib_concepts
          requires( std::input_iterator<InputIt> && std::equality_comparable<InputIt> )
        PACE__CXX20_CNSTXPR BasicCoWString&
#else
        PACE__CXX20_CNSTXPR
          typename std::enable_if<is_legacy_input_iterator<InputIt>::value, BasicCoWString&>::type
#endif
          append( InputIt first, InputIt last )
        { return append( BasicCoWString( std::move( first ), std::move( last ), this->allocator() ) ); }
        // BasicCoWString& append( std::initializer_list<Char> ) = delete;

#ifdef __cpp_lib_containers_ranges
        template<typename R>
          requires( std::ranges::input_range<R>
                    && std::convertible_to<std::ranges::range_reference_t<R>, Char> )
        PACE__CXX20_CNSTXPR BasicCoWString& append_range( R&& rg )
        { return append( BasicCoWString( std::from_range, std::forward<R>( rg ), this->allocator() ) ); }
#endif

        PACE__CXX20_CNSTXPR BasicCoWString& replace( size_type pos,
                                                     size_type count,
                                                     const Char* cstr,
                                                     size_type cstr_count )
        {
          if ( pos == length_ )
            return append( cstr, cstr_count );
          else if ( pos > length_ )
            PACE__UNLIKELY throw std::out_of_range(
              "pace: replace the string content with c-style string at an invalid position" );

          count                   = (std::min)( count, length_ - pos );
          const auto total_length = length_ - count + cstr_count;
          switch ( tag_ ) {
          case Kind::Literal: {
            if ( total_length > small_capacity() )
              transfer_to<Kind::Dynamic>( as_.literal_,
                                          pos,
                                          as_.literal_ + pos + count,
                                          length_ - ( pos + count ),
                                          cstr,
                                          cstr_count );
            else
              transfer_to<Kind::Inline>( as_.literal_,
                                         pos,
                                         as_.literal_ + pos + count,
                                         length_ - ( pos + count ),
                                         cstr,
                                         cstr_count );
          } break;
          case Kind::Inline: {
            const auto src = utils::launder_as<Char>( &as_ );
            if ( total_length > small_capacity() )
              transfer_to<Kind::Dynamic>( src,
                                          pos,
                                          src + pos + count,
                                          length_ - ( pos + count ),
                                          cstr,
                                          cstr_count );
            else if ( cstr >= src && cstr < src + length_ ) { // self replace
              Char substring[small_capacity()];
              Traits::copy( substring, cstr, cstr_count );
              embed( src + pos, count, length_ - pos, substring, cstr_count );
            } else
              embed( src + pos, count, length_ - pos, cstr, cstr_count );
          } break;
          case Kind::Dynamic: {
            if ( as_.remote_.refs_->load( std::memory_order_acquire ) > 1 )
              duplicate_from( as_.remote_.str(),
                              pos,
                              as_.remote_.str() + pos + count,
                              length_ - ( pos + count ),
                              cstr,
                              cstr_count );
            else if ( as_.remote_.capacity_ <= total_length )
              expand_with( as_.remote_.str(),
                           pos,
                           as_.remote_.str() + pos + count,
                           length_ - ( pos + count ),
                           cstr,
                           cstr_count );
            else if ( cstr >= as_.remote_.str() && cstr < as_.remote_.str() + length_ ) {
              // self replace
              const auto offset = static_cast<size_type>( cstr - as_.remote_.str() );
              PACE__TRUST( offset <= length_ );
              if ( offset + cstr_count < pos ) {
                Traits::move( as_.remote_.str() + pos + cstr_count,
                              as_.remote_.str() + pos + count,
                              length_ - ( pos + count ) );
                Traits::copy( as_.remote_.str() + pos, as_.remote_.str() + offset, cstr_count );
              } else if ( offset <= pos && offset + cstr_count < pos + count ) {
                Traits::move( as_.remote_.str() + pos + cstr_count,
                              as_.remote_.str() + pos + count,
                              length_ - pos - count );
                Traits::move( as_.remote_.str() + 2 * pos - offset,
                              as_.remote_.str() + pos,
                              offset + cstr_count - pos );
                Traits::copy( as_.remote_.str() + pos, as_.remote_.str() + offset, pos - offset );
              } else if ( offset <= pos && offset + cstr_count >= pos + count ) {
                const auto pos_suffix = 2 * pos - offset + cstr_count - count;
                Traits::move( as_.remote_.str() + pos_suffix,
                              as_.remote_.str() + pos + count,
                              length_ - ( pos + count ) );
                Traits::copy( as_.remote_.str() + 2 * pos - offset + count,
                              as_.remote_.str() + pos_suffix,
                              cstr_count - ( pos - offset + count ) );
                Traits::move( as_.remote_.str() + 2 * pos - offset, as_.remote_.str() + pos, count );
                Traits::copy( as_.remote_.str() + pos, as_.remote_.str() + offset, pos - offset );
              } else if ( offset >= pos && offset < pos + count && offset + cstr_count <= pos + count ) {
                Traits::move( as_.remote_.str() + pos, as_.remote_.str() + offset, cstr_count );
                Traits::move( as_.remote_.str() + pos + cstr_count,
                              as_.remote_.str() + pos + count,
                              length_ - ( pos + count ) );
              } else if ( offset >= pos && offset < pos + count && offset + cstr_count >= pos + count ) {
                const auto sublen = pos + count - offset;
                Traits::move( as_.remote_.str() + pos, as_.remote_.str() + offset, sublen );
                Traits::copy( as_.remote_.str() + sublen,
                              as_.remote_.str() + pos + count,
                              cstr_count - sublen );
                Traits::move( as_.remote_.str() + pos + cstr_count,
                              as_.remote_.str() + pos + count,
                              length_ - ( pos + count ) );
              } else {
                Traits::move( as_.remote_.str() + pos + cstr_count,
                              as_.remote_.str() + pos + count,
                              length_ - ( pos + count ) );
                Traits::copy( as_.remote_.str() + pos,
                              as_.remote_.str() + offset + cstr_count - count,
                              cstr_count );
              }
              Traits::assign( as_.remote_.str()[total_length], Char() );
            } else
              embed( as_.remote_.str() + pos, count, length_ - pos, cstr, cstr_count );
          } break;
          default: utils::unreachable();
          }
          length_ = total_length;
          return *this;
        }
        PACE__CXX20_CNSTXPR BasicCoWString& replace( size_type pos, size_type count, const Char* cstr )
        { return replace( pos, count, cstr, Traits::length( cstr ) ); }
        PACE__CXX20_CNSTXPR BasicCoWString& replace( const_iterator first,
                                                     const_iterator last,
                                                     const Char* cstr,
                                                     size_type cstr_count )
        {
          PACE__ASSERT( this == first.owner() );
          PACE__ASSERT( this == last.owner() );
          PACE__ASSERT( first.offset() < length_ );
          return replace( first.offset(), last - first, cstr, cstr_count );
        }
        PACE__CXX20_CNSTXPR BasicCoWString& replace( const_iterator first,
                                                     const_iterator last,
                                                     const Char* cstr )
        {
          PACE__ASSERT( this == first.owner() );
          PACE__ASSERT( this == last.owner() );
          PACE__ASSERT( first.offset() < length_ );
          return replace( first.offset(), last - first, cstr, Traits::length( cstr ) );
        }
        PACE__CXX20_CNSTXPR BasicCoWString& replace( size_type pos,
                                                     size_type count,
                                                     const BasicCoWString& other,
                                                     size_type other_pos,
                                                     size_type other_count = npos )
        {
          if ( pos == length_ )
            return append( other, other_pos, other_count );
          other_count = (std::min)( other_count, other.length_ - other_pos );
          if ( other_pos > other.length_ )
            PACE__UNLIKELY throw std::out_of_range(
              "pace: replace the string content with an another CoW string that has an invalid subrange" );

          return replace( pos, count, other.data() + other_pos, other_count );
        }
        PACE__CXX20_CNSTXPR BasicCoWString& replace( size_type pos,
                                                     size_type count,
                                                     const BasicCoWString& other )
        { return replace( pos, count, other, 0, other.length_ ); }
        PACE__CXX20_CNSTXPR BasicCoWString& replace( const_iterator first,
                                                     const_iterator last,
                                                     const BasicCoWString& other )
        {
          PACE__ASSERT( this == first.owner() );
          PACE__ASSERT( this == last.owner() );
          PACE__ASSERT( first.offset() < length_ );
          return replace( first.offset(), last - first, other, 0, other.length_ );
        }
        PACE__CXX20_CNSTXPR BasicCoWString& replace( size_type pos,
                                                     size_type count,
                                                     size_type ch_count,
                                                     Char ch )
        {
          if ( pos == length_ )
            return append( ch_count, ch );
          else if ( pos > length_ )
            PACE__UNLIKELY throw std::out_of_range(
              "pace: replace the string content with characters at an invalid position" );

          count                   = (std::min)( count, length_ - pos );
          const auto total_length = length_ - count + ch_count;
          switch ( tag_ ) {
          case Kind::Literal: {
            if ( total_length > small_capacity() )
              transfer_to<Kind::Dynamic>( as_.literal_,
                                          pos,
                                          as_.literal_ + pos + count,
                                          length_ - ( pos + count ),
                                          ch_count,
                                          ch );
            else
              transfer_to<Kind::Inline>( as_.literal_,
                                         pos,
                                         as_.literal_ + pos + count,
                                         length_ - ( pos + count ),
                                         ch_count,
                                         ch );
          } break;
          case Kind::Inline: {
            const auto src = utils::launder_as<Char>( &as_ );
            if ( total_length > small_capacity() )
              transfer_to<Kind::Dynamic>( src,
                                          pos,
                                          src + pos + count,
                                          length_ - ( pos + count ),
                                          ch_count,
                                          ch );
            else
              embed( src + pos, count, length_ - pos, count, ch );
          } break;
          case Kind::Dynamic: {
            if ( as_.remote_.refs_->load( std::memory_order_acquire ) > 1 )
              duplicate_from( as_.remote_.str(),
                              pos,
                              as_.remote_.str() + pos + count,
                              length_ - ( pos + count ),
                              ch_count,
                              ch );
            else if ( as_.remote_.capacity_ <= total_length )
              expand_with( as_.remote_.str(),
                           pos,
                           as_.remote_.str() + pos + count,
                           length_ - ( pos + count ),
                           ch_count,
                           ch );
            else
              embed( as_.remote_.str() + pos, count, length_ - pos, count, ch );
          } break;
          default: utils::unreachable();
          }
          return *this;
        }
        PACE__CXX20_CNSTXPR BasicCoWString& replace( const_iterator first,
                                                     const_iterator last,
                                                     size_type ch_count,
                                                     Char ch )
        {
          PACE__ASSERT( this == first.owner() );
          PACE__ASSERT( this == last.owner() );
          PACE__ASSERT( first.offset() < length_ );
          return replace( first.offset(), last - first, ch_count, ch );
        }
        template<typename InputIt>
#ifdef __cpp_lib_concepts
          requires( std::input_iterator<InputIt> && std::equality_comparable<InputIt> )
        PACE__CXX20_CNSTXPR BasicCoWString&
#else
        PACE__CXX20_CNSTXPR
          typename std::enable_if<is_legacy_input_iterator<InputIt>::value, BasicCoWString&>::type
#endif
          replace( const_iterator first, const_iterator last, InputIt in_first, InputIt in_last )
        {
          return replace( first.offset(),
                          last - first,
                          BasicCoWString( std::move( in_first ), std::move( in_last ) ) );
        }
        // BasicCoWString& replace( const_iterator, const_iterator, std::initializer_list<Char> ) = delete;
#ifdef __cpp_lib_string_view
        template<typename StringViewLike>
        PACE__CXX20_CNSTXPR
          typename std::enable_if<is_string_view_like<StringViewLike>::value, BasicCoWString&>::type replace(
            size_type pos,
            size_type count,
            const StringViewLike& str_v,
            size_type sv_pos,
            size_type sv_count = npos )
        {
          std::basic_string_view<Char, Traits> sv = str_v;
          if ( sv_pos > sv.size() )
            PACE__UNLIKELY throw std::out_of_range(
              "pace: replace the string content with a string view that has an invalid subrange" );
          sv.remove_prefix( sv_pos );
          count = (std::min)( count, sv.size() );
          return replace( pos, count, sv.data(), sv_count );
        }
        template<typename StringViewLike>
        PACE__CXX20_CNSTXPR
          typename std::enable_if<is_string_view_like<StringViewLike>::value, BasicCoWString&>::type
          replace( size_type pos, size_type count, const StringViewLike& str_v )
        {
          std::basic_string_view<Char, Traits> sv = str_v;
          return replace( pos, count, sv.data(), sv.size() );
        }
        template<typename StringViewLike>
        PACE__CXX20_CNSTXPR
          typename std::enable_if<is_string_view_like<StringViewLike>::value, BasicCoWString&>::type
          replace( const_iterator first, const_iterator last, const StringViewLike& str_v )
        {
          PACE__ASSERT( this == first.owner() );
          PACE__ASSERT( this == last.owner() );
          PACE__ASSERT( first.offset() < length_ );
          return replace( first.offset(), last - first, str_v );
        }
#endif

#ifdef __cpp_lib_containers_ranges
        template<typename R>
          requires( std::ranges::input_range<R>
                    && std::convertible_to<std::ranges::range_reference_t<R>, Char> )
        PACE__CXX20_CNSTXPR BasicCoWString& replace_with_range( const_iterator first,
                                                                const_iterator last,
                                                                R&& rg )
        {
          return replace( first,
                          last,
                          BasicCoWString( std::from_range, std::forward<R>( rg ), this->allocator() ) );
        }
#endif

        PACE__CXX20_CNSTXPR size_type copy( Char* dest, size_type count, size_type pos = 0 ) const
        {
          if ( pos > length_ )
            PACE__UNLIKELY throw std::out_of_range(
              "pace: copy a sub-string at an invalid position to the destination" );

          PACE__ASSERT( dest != data() );
          count = (std::min)( count, length_ - pos );
          Traits::copy( dest, data() + pos, count );
          return count;
        }

        template<typename Operation>
        PACE__CXX20_CNSTXPR void resize_and_overwrite( size_type count, Operation op ) &
        {
          Char* dest = nullptr;
          switch ( tag_ ) {
          case Kind::Literal: PACE__FALLTHROUGH;
          case Kind::Inline:  {
            const auto src = tag_ == Kind::Literal ? as_.literal_ : utils::launder_as<Char>( &as_ );
            if ( count > small_capacity() ) {
              transfer_to<Kind::Dynamic>( (std::max)( count, dynamic_capacity( small_capacity() ) ) + 1,
                                          [&]( Char* dest, size_type new_cap ) {
                                            Traits::copy( dest, src, (std::min)( length_, count ) );
                                            length_ = std::move( op )( dest, new_cap );
                                            return length_;
                                          } );
              dest = as_.remote_.str();
            } else {
              transfer_to<Kind::Inline>( src, (std::min)( length_, count ), 0 );
              dest    = utils::launder_as<Char>( &as_ );
              length_ = std::move( op )( dest, count );
            }
          } break;
          case Kind::Dynamic: {
            if ( as_.remote_.refs_->load( std::memory_order_acquire ) > 1 )
              duplicate_from( count + 1, [&]( Char* dest, size_type new_cap ) {
                Traits::copy( dest, as_.remote_.str(), (std::min)( length_, count ) );
                length_ = std::move( op )( dest, new_cap );
                return length_;
              } );
            else if ( count <= as_.remote_.capacity_ )
              expand_with( count + 1, [&]( Char* dest, size_type new_cap ) {
                Traits::copy( dest, as_.remote_.str(), (std::min)( length_, count ) );
                length_ = std::move( op )( dest, new_cap );
                return length_;
              } );
            dest = as_.remote_.str();
          } break;
          default: utils::unreachable();
          }
          Traits::assign( dest[length_], Char() );
        }

        PACE__CXX20_CNSTXPR void resize( size_type count, Char ch ) &
        {
          resize_and_overwrite( count, [&]( Char* buffer, size_type buffer_len ) noexcept {
            if ( buffer_len > length_ )
              Traits::assign( buffer + length_, buffer_len - length_, ch );
            return buffer_len;
          } );
        }
        PACE__CXX20_CNSTXPR void resize( size_type count ) & { resize( count, Char() ); }

        PACE__NODISCARD PACE__CXX20_CNSTXPR int compare( size_type pos,
                                                         size_type count,
                                                         const Char* cstr,
                                                         size_type cstr_count ) const
        {
          if ( pos > length_ )
            PACE__UNLIKELY throw std::out_of_range(
              "pace: compare a c-style string with an invalid subrange" );
          count             = (std::min)( count, length_ - pos );
          const auto result = Traits::compare( data() + pos, cstr, (std::min)( count, cstr_count ) );
          return result != 0 ? result : ( count < cstr_count ? -1 : ( count > cstr_count ? 1 : 0 ) );
        }
        PACE__NODISCARD PACE__CXX20_CNSTXPR int compare( const Char* cstr,
                                                         size_type cstr_count ) const noexcept
        { return compare( 0, npos, cstr, cstr_count ); }
        PACE__NODISCARD PACE__CXX20_CNSTXPR int compare( const Char* cstr ) const noexcept
        { return compare( cstr, Traits::length( cstr ) ); }
        PACE__NODISCARD PACE__CXX20_CNSTXPR int compare( size_type pos,
                                                         size_type count,
                                                         const BasicCoWString& other,
                                                         size_type other_pos,
                                                         size_type other_count = npos ) const
        {
          if ( other_pos > other.length_ )
            PACE__UNLIKELY throw std::out_of_range(
              "pace: compare with another CoW string that has an invalid subrange" );
          other_count           = (std::min)( other_count, other.length_ - other_pos );
          const auto other_data = other.data();
          return compare( pos, count, other_data + other_pos, other_count );
        }
        PACE__NODISCARD PACE__CXX20_CNSTXPR int compare( size_type pos,
                                                         size_type count,
                                                         const BasicCoWString& other ) const
        { return compare( pos, count, other, 0 ); }
        PACE__NODISCARD PACE__CXX20_CNSTXPR int compare( const BasicCoWString& other ) const noexcept
        { return compare( 0, npos, other, 0 ); }
#ifdef __cpp_lib_string_view
        template<typename StringViewLike>
        PACE__NODISCARD PACE__CXX20_CNSTXPR
          typename std::enable_if<is_string_view_like<StringViewLike>::value, int>::type compare(
            size_type pos,
            size_type count,
            const StringViewLike& str_v,
            size_type pos_sv,
            size_type count_sv ) const
        {
          std::basic_string_view<Char, Traits> sv = str_v;
          if ( pos_sv > sv.size() )
            PACE__UNLIKELY throw std::out_of_range(
              "pace: compare a string view that has an invalid subrange" );
          sv = sv.substr( pos_sv, count_sv );
          return compare( pos, count, sv.data(), sv.size() );
        }
        template<typename StringViewLike>
        PACE__NODISCARD PACE__CXX20_CNSTXPR
          typename std::enable_if<is_string_view_like<StringViewLike>::value, int>::type
          compare( size_type pos, size_type count, const StringViewLike& str_v ) const
        { return compare( pos, count, str_v, 0 ); }
        template<typename StringViewLike>
        PACE__NODISCARD PACE__CXX20_CNSTXPR
          typename std::enable_if<is_string_view_like<StringViewLike>::value, int>::type
          compare( const StringViewLike& str_v ) const
# ifdef __cpp_lib_is_nothrow_convertible
          noexcept(
            std::is_nothrow_convertible<const StringViewLike&, std::basic_string_view<Char, Traits>>::value )
# endif
        { return compare( 0, npos, str_v, 0 ); }
#endif

        PACE__NODISCARD constexpr BasicCoWString substr( size_type pos = 0, size_type count = npos ) const&
        { return { *this, pos, count }; }
        PACE__NODISCARD PACE__CXX14_CNSTXPR BasicCoWString substr( size_type pos   = 0,
                                                                   size_type count = npos ) &&
        { return { std::move( *this ), pos, count }; }

        PACE__CXX20_CNSTXPR void swap( BasicCoWString& other )
          noexcept( traits::AnyOf<typename std::allocator_traits<Alloc>::is_always_equal,
                                  typename std::allocator_traits<Alloc>::propagate_on_container_swap>::value )
        {
          PACE__TRUST( this != &other );
          this->CoWSwapAlloc<Alloc, BasicCoWString>::swap( other );
          switch ( tag_ ) {
          case Kind::Literal: {
            const auto lit = as_.literal_;
            switch ( other.tag_ ) {
            case Kind::Literal: as_.literal_ = other.as_.literal_; break;
            case Kind::Inline:
              Traits::copy( utils::launder_as<Char>( &as_ ),
                            utils::launder_as<Char>( &other.as_ ),
                            other.length_ + 1 );
              break;
            case Kind::Dynamic: utils::construct_at( &as_.remote_, other.as_.remote_ ); break;
            default:            utils::unreachable();
            }
            utils::construct_at( &other.as_.literal_, lit );
          } break;
          case Kind::Inline: {
            Char buffer[small_capacity() + 1];
            const auto dest_self = utils::launder_as<Char>( &as_ );
            Traits::copy( buffer, dest_self, length_ + 1 );
            const auto dest_other = utils::launder_as<Char>( &other.as_ );
            switch ( other.tag_ ) {
            case Kind::Literal: utils::construct_at( &as_.literal_, other.as_.literal_ ); break;
            case Kind::Inline:  Traits::copy( dest_self, dest_other, other.length_ + 1 ); break;
            case Kind::Dynamic: utils::construct_at( &as_.remote_, other.as_.remote_ ); break;
            default:            utils::unreachable();
            }
            Traits::copy( dest_other, buffer, length_ + 1 );
          } break;
          case Kind::Dynamic: {
            const auto cow = as_.remote_;
            switch ( other.tag_ ) {
            case Kind::Literal: utils::construct_at( &as_.literal_, other.as_.literal_ ); break;
            case Kind::Inline:
              Traits::copy( utils::launder_as<Char>( &as_ ),
                            utils::launder_as<Char>( &other.as_ ),
                            other.length_ + 1 );
              break;
            case Kind::Dynamic: as_.remote_ = other.as_.remote_; break;
            default:            utils::unreachable();
            }
            utils::construct_at( &other.as_.remote_, cow );
          } break;
          default: utils::unreachable();
          }
          std::swap( length_, other.length_ );
          std::swap( tag_, other.tag_ );
        }
        friend PACE__CXX14_CNSTXPR void swap( BasicCoWString& a, BasicCoWString& b ) noexcept { a.swap( b ); }

        PACE__CXX20_CNSTXPR BasicCoWString& operator+=( const BasicCoWString& str ) { return append( str ); }
        PACE__CXX20_CNSTXPR BasicCoWString& operator+=( Char ch ) { return append( 1, ch ); }
        PACE__CXX20_CNSTXPR BasicCoWString& operator+=( const Char* cstr ) { return append( cstr ); }
        // BasicCoWString& operator+=( std::initializer_list<Char> ) = delete;
#ifdef __cpp_lib_string_view
        template<typename StringViewLike>
        PACE__CXX20_CNSTXPR
          typename std::enable_if<is_string_view_like<StringViewLike>::value, BasicCoWString&>::type
          operator+=( const StringViewLike& str_v )
        { return append( str_v ); }
#endif

        PACE__NODISCARD friend PACE__CXX20_CNSTXPR BasicCoWString operator+( const BasicCoWString& a,
                                                                             const BasicCoWString& b )
        {
          auto ret = a;
          ret.append( b );
          return ret;
        }
        PACE__NODISCARD friend PACE__CXX20_CNSTXPR BasicCoWString operator+( BasicCoWString&& a,
                                                                             BasicCoWString&& b )
        {
          a.append( std::move( b ) );
          // Strictly speaking, function parameters are not entities that NRVO would consider,
          // so it is necessary to use std::move on the return value here.
          return std::move( a );
        }
        PACE__NODISCARD friend PACE__CXX20_CNSTXPR BasicCoWString operator+( BasicCoWString&& a,
                                                                             const BasicCoWString& b )
        {
          a.append( b );
          return std::move( a );
        }
        PACE__NODISCARD friend PACE__CXX20_CNSTXPR BasicCoWString operator+( const BasicCoWString& a,
                                                                             BasicCoWString&& b )
        {
          b.insert( 0, a );
          return std::move( b );
        }
        PACE__NODISCARD friend PACE__CXX20_CNSTXPR BasicCoWString operator+( const BasicCoWString& a,
                                                                             const Char* b )
        {
          auto ret = a;
          ret.append( b );
          return ret;
        }
        PACE__NODISCARD friend PACE__CXX20_CNSTXPR BasicCoWString operator+( BasicCoWString&& a,
                                                                             const Char* b )
        {
          a.append( b );
          return std::move( a );
        }
        PACE__NODISCARD friend PACE__CXX20_CNSTXPR BasicCoWString operator+( const Char* a,
                                                                             const BasicCoWString& b )
        {
          auto ret = BasicCoWString(
            a,
            std::allocator_traits<Alloc>::select_on_container_copy_construction( b.get_allocator() ) );
          ret.append( b );
          return ret;
        }
        PACE__NODISCARD friend PACE__CXX20_CNSTXPR BasicCoWString operator+( const Char* a,
                                                                             BasicCoWString&& b )
        {
          b.insert( 0, a );
          return std::move( b );
        }
        PACE__NODISCARD friend PACE__CXX20_CNSTXPR BasicCoWString operator+( const BasicCoWString& a, Char b )
        {
          auto ret = a;
          a.push_back( b );
          return ret;
        }
        PACE__NODISCARD friend PACE__CXX20_CNSTXPR BasicCoWString operator+( BasicCoWString&& a, Char b )
        {
          a.push_back( b );
          return std::move( a );
        }
        PACE__NODISCARD friend PACE__CXX20_CNSTXPR BasicCoWString operator+( Char a, const BasicCoWString& b )
        {
          auto ret = BasicCoWString(
            1,
            a,
            std::allocator_traits<Alloc>::select_on_container_copy_construction( b.get_allocator() ) );
          ret.append( b );
          return ret;
        }
        PACE__NODISCARD friend PACE__CXX20_CNSTXPR BasicCoWString operator+( Char a, BasicCoWString&& b )
        {
          b.insert( 0, 1, a );
          return std::move( b );
        }
#if defined( __cpp_lib_string_view ) && defined( __cpp_lib_type_identity )
        PACE__NODISCARD friend PACE__CXX20_CNSTXPR BasicCoWString
          operator+( const BasicCoWString& a, std::type_identity_t<std::basic_string_view<Char, Traits>> b )
        {
          auto ret = a;
          a.append( b );
        }
        PACE__NODISCARD friend PACE__CXX20_CNSTXPR BasicCoWString
          operator+( std::type_identity_t<std::basic_string_view<Char, Traits>> a, const BasicCoWString& b )
        {
          auto ret = BasicCoWString(
            a,
            std::allocator_traits<Alloc>::select_on_container_copy_construction( b.get_allocator() ) );
          ret.append( b );
          return ret;
        }
        PACE__NODISCARD friend PACE__CXX20_CNSTXPR BasicCoWString
          operator+( BasicCoWString&& a, std::type_identity_t<std::basic_string_view<Char, Traits>> b )
        {
          a.append( b );
          return std::move( a );
        }
        PACE__NODISCARD friend PACE__CXX20_CNSTXPR BasicCoWString
          operator+( std::type_identity_t<std::basic_string_view<Char, Traits>> a, BasicCoWString&& b )
        {
          b.insert( 0, a );
          return std::move( b );
        }
#endif

        PACE__NODISCARD friend constexpr bool operator==( const BasicCoWString& a,
                                                          const BasicCoWString& b ) noexcept
        { return a.compare( b ) == 0; }
        PACE__NODISCARD friend constexpr bool operator==( const BasicCoWString& a, const Char* b )
        { return a.compare( b ) == 0; }
        PACE__NODISCARD friend constexpr bool operator==( const Char* a, const BasicCoWString& b )
        { return b == a; }
        PACE__NODISCARD friend constexpr bool operator!=( const BasicCoWString& a,
                                                          const BasicCoWString& b ) noexcept
        { return a.compare( b ) != 0; }
        PACE__NODISCARD friend constexpr bool operator!=( const BasicCoWString& a, const Char* b )
        { return a.compare( b ) != 0; }
        PACE__NODISCARD friend constexpr bool operator!=( const Char* a, const BasicCoWString& b )
        { return b != a; }

#ifdef __cpp_lib_three_way_comparison
        PACE__NODISCARD friend constexpr auto operator<=>( const BasicCoWString& a,
                                                           const BasicCoWString& b ) noexcept
        { return static_cast<typename ComparisonCategory<Traits>::type>( a.compare( b ) <=> 0 ); }
        PACE__NODISCARD friend constexpr auto operator<=>( const BasicCoWString& a, const Char* b )
        { return static_cast<typename ComparisonCategory<Traits>::type>( a.compare( b ) <=> 0 ); }
#else
        PACE__NODISCARD friend constexpr bool operator<( const BasicCoWString& a,
                                                         const BasicCoWString& b ) noexcept
        { return a.compare( b ) < 0; }
        PACE__NODISCARD friend constexpr bool operator<=( const BasicCoWString& a,
                                                          const BasicCoWString& b ) noexcept
        { return a.compare( b ) <= 0; }
        PACE__NODISCARD friend constexpr bool operator>( const BasicCoWString& a,
                                                         const BasicCoWString& b ) noexcept
        { return a.compare( b ) > 0; }
        PACE__NODISCARD friend constexpr bool operator>=( const BasicCoWString& a,
                                                          const BasicCoWString& b ) noexcept
        { return a.compare( b ) >= 0; }
        PACE__NODISCARD friend constexpr bool operator<( const BasicCoWString& a, const Char* b )
        { return a.compare( b ) < 0; }
        PACE__NODISCARD friend constexpr bool operator<=( const BasicCoWString& a, const Char* b )
        { return a.compare( b ) <= 0; }
        PACE__NODISCARD friend constexpr bool operator>( const BasicCoWString& a, const Char* b )
        { return a.compare( b ) > 0; }
        PACE__NODISCARD friend constexpr bool operator>=( const BasicCoWString& a, const Char* b )
        { return a.compare( b ) >= 0; }
        PACE__NODISCARD friend constexpr bool operator<( const Char* a, const BasicCoWString& b )
        { return !( b >= a ); }
        PACE__NODISCARD friend constexpr bool operator<=( const Char* a, const BasicCoWString& b )
        { return !( b > a ); }
        PACE__NODISCARD friend constexpr bool operator>( const Char* a, const BasicCoWString& b )
        { return !( b <= a ); }
        PACE__NODISCARD friend constexpr bool operator>=( const Char* a, const BasicCoWString& b )
        { return !( b < a ); }
#endif
      };

#if defined( __cpp_deduction_guides ) && defined( __cpp_lib_containers_ranges )
      template<std::ranges::input_range R, typename Alloc = std::allocator<std::ranges::range_value_t<R>>>
      BasicCoWString( std::from_range_t, R&&, Alloc = Alloc() )
        -> BasicCoWString<std::ranges::range_value_t<R>,
                          std::char_traits<std::ranges::range_value_t<R>>,
                          Alloc>;
#endif

      // For now, we have only used the char type.
      using CoWString = BasicCoWString<types::Char>;

      PACE__CXX20_CNSTXPR CoWString operator""_cow( const types::Char* str, types::Size len ) noexcept
      { return { make_literal( str, len ) }; }
    } // namespace charcodes
  } // namespace details
} // namespace pace

#endif
