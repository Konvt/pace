#ifndef PACE_CHAR_PIPELINE
#define PACE_CHAR_PIPELINE

#include "../charcodes/StringView.hpp"
#include "../traits/Backport.hpp"
#include "../utils/Backport.hpp"

namespace pace {
  namespace details {
    namespace io {
      class CharPipeline {
        static constexpr types::Size _bootstrap_cap = 256;

        std::unique_ptr<types::Char[]> start_;
        types::Char* end_     = nullptr;
        types::Char* current_ = nullptr;

        PACE__CXX23_CNSTXPR void grow( types::Size desired )
        {
          PACE__ASSERT( desired > 0 );
          const auto capacity = static_cast<types::Size>( end_ - start_.get() );
          auto new_capacity   = capacity == 0 ? _bootstrap_cap : capacity * 2;
          while ( new_capacity < capacity + desired )
            new_capacity *= 2;

          auto new_buffer        = utils::make_unique<types::Char[]>( new_capacity );
          const auto new_current = std::copy( start_.get(), current_, new_buffer.get() );
          end_                   = new_buffer.get() + new_capacity;
          start_.swap( new_buffer );
          current_ = new_current;
        }

      public:
        using value_type = types::Char;

        constexpr CharPipeline() = default;

        PACE__CXX23_CNSTXPR CharPipeline( CharPipeline&& rhs ) noexcept
          : start_ { std::move( rhs.start_ ) }
          , end_ { utils::exchange( rhs.end_, nullptr ) }
          , current_ { utils::exchange( rhs.current_, nullptr ) }
        {}
        PACE__CXX23_CNSTXPR CharPipeline& operator=( CharPipeline&& rhs ) & noexcept
        {
          PACE__ASSERT( this != &rhs );
          start_   = std::move( rhs.start_ );
          end_     = utils::exchange( rhs.end_, nullptr );
          current_ = utils::exchange( rhs.current_, nullptr );
          return *this;
        }

        PACE__CXX23_CNSTXPR ~CharPipeline() = default;

        PACE__FORCEINLINE PACE__CXX23_CNSTXPR const types::Char* data() const& noexcept
        { return start_.get(); }
        PACE__FORCEINLINE PACE__CXX23_CNSTXPR types::Char* data() & noexcept { return start_.get(); }
        PACE__FORCEINLINE PACE__CXX23_CNSTXPR types::Char* data() && noexcept { return start_.get(); }
        PACE__NODISCARD PACE__FORCEINLINE PACE__CXX23_CNSTXPR types::Size size() const noexcept
        { return static_cast<types::Size>( current_ - start_.get() ); }
        PACE__NODISCARD PACE__FORCEINLINE PACE__CXX23_CNSTXPR bool empty() const noexcept
        { return start_.get() == current_; }

        PACE__FORCEINLINE PACE__CXX23_CNSTXPR void clear() & noexcept { current_ = start_.get(); }
        // Releases the buffer space completely
        PACE__FORCEINLINE PACE__CXX23_CNSTXPR void release() noexcept
        {
          start_.reset();
          end_ = current_ = nullptr;
        }

        PACE__FORCEINLINE PACE__CXX23_CNSTXPR CharPipeline& reserve( types::Size capacity ) &
        {
          if ( capacity > static_cast<types::Size>( end_ - start_.get() ) ) {
            auto new_buffer        = utils::make_unique<types::Char[]>( capacity );
            const auto new_current = std::copy( start_.get(), current_, new_buffer.get() );
            end_                   = new_buffer.get() + capacity;
            start_.swap( new_buffer );
            current_ = new_current;
          }
          return *this;
        }

        PACE__FORCEINLINE PACE__CXX23_CNSTXPR CharPipeline& push_back( types::Char ch ) &
        {
          if ( end_ == current_ )
            grow( 1 );
          *( current_++ ) = ch;
          return *this;
        }

        PACE__FORCEINLINE PACE__CXX23_CNSTXPR CharPipeline& append( const types::Char* first,
                                                                    const types::Char* last ) &
        {
          PACE__TRUST( first != nullptr );
          PACE__TRUST( last != nullptr );
          PACE__TRUST( first <= last );
          const auto length   = last - first;
          const auto free_cap = end_ - current_;
          if ( length > free_cap )
            grow( length - free_cap );
          current_ = std::copy( first, last, current_ );
          return *this;
        }
        PACE__FORCEINLINE PACE__CXX23_CNSTXPR CharPipeline& append( charcodes::StringView info,
                                                                    types::Size num = 1 ) &
        {
          const auto total_length = info.size() * num;
          const auto free_cap     = static_cast<types::Size>( end_ - current_ );
          if ( total_length > free_cap )
            grow( total_length - free_cap );
          while ( num-- )
            current_ = std::copy( info.data(), info.data() + info.size(), current_ );
          return *this;
        }
        PACE__FORCEINLINE PACE__CXX23_CNSTXPR CharPipeline& append( types::Char info, types::Size num = 1 ) &
        {
          const auto free_cap = static_cast<types::Size>( end_ - current_ );
          if ( num > free_cap )
            grow( num - free_cap );
          current_ = std::fill_n( current_, num, info );
          return *this;
        }

        PACE__FORCEINLINE PACE__CXX23_CNSTXPR CharPipeline& apply(
          CharPipeline& ( &manipulator )(CharPipeline&),
          types::Size num = 1 ) &
        {
          while ( num-- )
            manipulator( *this );
          return *this;
        }

        friend PACE__FORCEINLINE PACE__CXX23_CNSTXPR CharPipeline& operator<<(
          CharPipeline& stream,
          CharPipeline& ( &manipulator )(CharPipeline&))
        { return manipulator( stream ); }

        template<typename T>
        friend PACE__FORCEINLINE PACE__CXX23_CNSTXPR typename std::enable_if<
          traits::AnyOf<std::is_convertible<T, charcodes::StringView>,
                        std::is_same<typename std::decay<T>::type, types::Char>>::value,
          CharPipeline&>::type
          operator<<( CharPipeline& stream, T&& info )
        { return stream.append( std::forward<T>( info ) ); }

        PACE__CXX20_CNSTXPR void swap( CharPipeline& other ) noexcept
        {
          PACE__TRUST( this != &other );
          start_.swap( other.start_ );
          std::swap( current_, other.current_ );
          std::swap( end_, other.end_ );
        }
        friend PACE__CXX20_CNSTXPR void swap( CharPipeline& a, CharPipeline& b ) noexcept { a.swap( b ); }
      };
    } // namespace io
  } // namespace details
} // namespace pace

#endif
