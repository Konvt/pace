#ifndef PACE_CHAR_PIPELINE
#define PACE_CHAR_PIPELINE

#include "../charcodes/StringView.hpp"
#include "../traits/Backport.hpp"
#include <vector>

namespace pace {
  namespace details {
    namespace io {
      class CharPipeline {
      protected:
        std::vector<types::Char> buffer_;

      public:
        using value_type = types::Char;

        PACE__CXX20_CNSTXPR CharPipeline() = default;

        PACE__CXX20_CNSTXPR CharPipeline( const CharPipeline& )              = default;
        PACE__CXX20_CNSTXPR CharPipeline( CharPipeline&& ) noexcept          = default;
        PACE__CXX20_CNSTXPR CharPipeline& operator=( const CharPipeline& ) & = default;
        PACE__CXX20_CNSTXPR CharPipeline& operator=( CharPipeline&& ) &      = default;
        // Intentional non-virtual destructors.
        PACE__CXX20_CNSTXPR ~CharPipeline()                                  = default;

        PACE__NODISCARD PACE__FORCEINLINE PACE__CXX20_CNSTXPR bool empty() const noexcept
        { return buffer_.empty(); }
        PACE__FORCEINLINE PACE__CXX20_CNSTXPR void clear() & noexcept { buffer_.clear(); }

        // Releases the buffer space completely
        PACE__FORCEINLINE PACE__CXX20_CNSTXPR void release() noexcept
        {
          clear();
          buffer_.shrink_to_fit();
        }

        PACE__FORCEINLINE PACE__CXX20_CNSTXPR CharPipeline& reserve( types::Size capacity ) &
        {
          buffer_.reserve( capacity );
          return *this;
        }

        PACE__FORCEINLINE PACE__CXX20_CNSTXPR CharPipeline& push_back( types::Char ch ) &
        {
          buffer_.push_back( ch );
          return *this;
        }

        PACE__FORCEINLINE PACE__CXX20_CNSTXPR CharPipeline& append( const types::Char* first,
                                                                    const types::Char* last ) &
        {
          PACE__TRUST( first != nullptr );
          PACE__TRUST( last != nullptr );
          PACE__TRUST( first <= last );
          buffer_.insert( buffer_.end(), first, last );
          return *this;
        }
        PACE__FORCEINLINE PACE__CXX20_CNSTXPR CharPipeline& append( charcodes::StringView info,
                                                                    types::Size num = 1 ) &
        {
          while ( num-- )
            append( info.data(), info.data() + info.size() );
          return *this;
        }
        PACE__FORCEINLINE PACE__CXX20_CNSTXPR CharPipeline& append( types::Char info, types::Size num = 1 ) &
        {
          buffer_.insert( buffer_.end(), num, info );
          return *this;
        }

        PACE__FORCEINLINE PACE__CXX20_CNSTXPR CharPipeline& apply( CharPipeline& ( *fnptr )(CharPipeline&),
                                                                   types::Size num = 1 ) &
        {
          while ( num-- )
            fnptr( *this );
          return *this;
        }

        friend PACE__FORCEINLINE PACE__CXX20_CNSTXPR CharPipeline& operator<<(
          CharPipeline& stream,
          CharPipeline& ( *fnptr )(CharPipeline&))
        {
          PACE__TRUST( fnptr != nullptr );
          return fnptr( stream );
        }

        template<typename T>
        friend PACE__FORCEINLINE PACE__CXX20_CNSTXPR typename std::enable_if<
          traits::any_of<std::is_convertible<T, charcodes::StringView>,
                         std::is_same<typename std::decay<T>::type, types::Char>>::value,
          CharPipeline&>::type
          operator<<( CharPipeline& stream, T&& info )
        { return stream.append( std::forward<T>( info ) ); }

        PACE__CXX20_CNSTXPR void swap( CharPipeline& other ) noexcept
        {
          PACE__TRUST( this != &other );
          buffer_.swap( other.buffer_ );
        }
        friend PACE__CXX20_CNSTXPR void swap( CharPipeline& a, CharPipeline& b ) noexcept { a.swap( b ); }

#ifdef __cpp_lib_char8_t
        PACE__FORCEINLINE CharPipeline& append( charcodes::U8StringView info, types::Size num = 1 ) &
        {
          while ( num-- )
            append( reinterpret_cast<const types::Char*>( info.data() ),
                    reinterpret_cast<const types::Char*>( info.data() ) + info.size() );
          return *this;
        }
        friend PACE__FORCEINLINE CharPipeline& operator<<( CharPipeline& stream,
                                                           charcodes::U8StringView info )
        { return stream.append( info ); }
#endif
      };
    } // namespace io
  } // namespace details
} // namespace pace

#endif
