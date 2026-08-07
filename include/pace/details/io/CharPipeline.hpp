#ifndef PACE_CHAR_PIPELINE
#define PACE_CHAR_PIPELINE

#include "../charcodes/StringView.hpp"
#include "../traits/Backport.hpp"
#include "../utils/Backport.hpp"

namespace pace {
  namespace details {
    namespace io {
      class CharPipeline {
        static constexpr std::size_t _bootstrap_cap = 256;

        char* start_   = nullptr;
        char* end_     = nullptr;
        char* current_ = nullptr;

        PACE__CXX20_CNSTXPR void grow( std::size_t desired )
        {
          PACE__TRUST( desired > 0 );
          const auto capacity = static_cast<std::size_t>( end_ - start_ );
          auto new_capacity   = capacity == 0 ? _bootstrap_cap : capacity * 2;
          while ( new_capacity < capacity + desired )
            new_capacity *= 2;

          auto new_buffer        = new char[new_capacity];
          const auto new_current = std::copy( start_, current_, new_buffer );
          end_                   = new_buffer + new_capacity;
          delete[] start_;
          start_   = new_buffer;
          current_ = new_current;
        }

      public:
        using value_type = char;

        constexpr CharPipeline() = default;
        PACE__CXX20_CNSTXPR ~CharPipeline() noexcept { delete[] start_; }

        PACE__CXX20_CNSTXPR CharPipeline( CharPipeline&& rhs ) noexcept
          : start_ { utils::exchange( rhs.start_, nullptr ) }
          , end_ { utils::exchange( rhs.end_, nullptr ) }
          , current_ { utils::exchange( rhs.current_, nullptr ) }
        {}
        PACE__CXX20_CNSTXPR CharPipeline& operator=( CharPipeline&& rhs ) & noexcept
        {
          PACE__ASSERT( this != &rhs );
          start_   = utils::exchange( rhs.start_, nullptr );
          end_     = utils::exchange( rhs.end_, nullptr );
          current_ = utils::exchange( rhs.current_, nullptr );
          return *this;
        }

        PACE__FORCEINLINE PACE__CXX20_CNSTXPR const char* data() const& noexcept { return start_; }
        PACE__FORCEINLINE PACE__CXX20_CNSTXPR char* data() & noexcept { return start_; }

        PACE__NODISCARD PACE__FORCEINLINE PACE__CXX20_CNSTXPR std::size_t capacity() const noexcept
        { return static_cast<std::size_t>( end_ - start_ ); }
        PACE__NODISCARD PACE__FORCEINLINE PACE__CXX20_CNSTXPR std::size_t size() const noexcept
        { return static_cast<std::size_t>( current_ - start_ ); }
        PACE__NODISCARD PACE__FORCEINLINE PACE__CXX20_CNSTXPR bool empty() const noexcept
        { return start_ == current_; }

        PACE__FORCEINLINE PACE__CXX20_CNSTXPR void clear() & noexcept { current_ = start_; }
        // Releases the buffer space completely
        PACE__FORCEINLINE PACE__CXX20_CNSTXPR void reset() noexcept
        {
          delete[] start_;
          start_ = end_ = current_ = nullptr;
        }

        PACE__FORCEINLINE PACE__CXX20_CNSTXPR CharPipeline& reserve( std::size_t capacity ) &
        {
          if ( capacity > static_cast<std::size_t>( end_ - start_ ) ) {
            auto new_buffer        = new char[capacity];
            const auto new_current = std::copy( start_, current_, new_buffer );
            end_                   = new_buffer + capacity;
            delete[] start_;
            start_   = new_buffer;
            current_ = new_current;
          }
          return *this;
        }

        PACE__FORCEINLINE PACE__CXX20_CNSTXPR CharPipeline& push_back( char ch ) &
        {
          if ( end_ == current_ )
            grow( 1 );
          *( current_++ ) = ch;
          return *this;
        }

        PACE__FORCEINLINE PACE__CXX20_CNSTXPR CharPipeline& append( charcodes::StringView info,
                                                                    std::size_t num = 1 ) &
        {
          const auto total_length = info.size() * num;
          const auto free_cap     = static_cast<std::size_t>( end_ - current_ );
          if ( total_length > free_cap )
            grow( total_length - free_cap );
          while ( num-- )
            current_ = std::copy( info.cbegin(), info.cend(), current_ );
          return *this;
        }
        PACE__FORCEINLINE PACE__CXX20_CNSTXPR CharPipeline& append( char info, std::size_t num = 1 ) &
        {
          const auto free_cap = static_cast<std::size_t>( end_ - current_ );
          if ( num > free_cap )
            grow( num - free_cap );
          current_ = std::fill_n( current_, num, info );
          return *this;
        }

        PACE__FORCEINLINE friend PACE__CXX20_CNSTXPR CharPipeline& operator<<(
          CharPipeline& self,
          CharPipeline& ( &manipulator )(CharPipeline&))
        { return manipulator( self ); }

        template<typename T>
        PACE__FORCEINLINE friend PACE__CXX20_CNSTXPR
          typename std::enable_if<traits::AnyOf<std::is_convertible<T, charcodes::StringView>,
                                                std::is_same<typename std::decay<T>::type, char>>::value,
                                  CharPipeline&>::type
          operator<<( CharPipeline& self, T&& info )
        { return self.append( std::forward<T>( info ) ); }

        PACE__CXX20_CNSTXPR void swap( CharPipeline& other ) noexcept
        {
          PACE__TRUST( this != &other );
          std::swap( start_, other.start_ );
          std::swap( current_, other.current_ );
          std::swap( end_, other.end_ );
        }
        friend PACE__CXX20_CNSTXPR void swap( CharPipeline& a, CharPipeline& b ) noexcept { a.swap( b ); }
      };
    } // namespace io
  } // namespace details
} // namespace pace

#endif
