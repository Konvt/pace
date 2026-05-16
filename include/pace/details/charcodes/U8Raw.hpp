#ifndef PACE_U8_RAW
#define PACE_U8_RAW

#include "../utils/Util.hpp"
#include "U8Char.hpp"
#include <utility>

namespace pace {
  namespace details {
    namespace charcodes {
      // A simple UTF-8 string implementation, but it does not provide specific utf-8 codec operations.
      class U8Raw {
      protected:
        types::Size width_;
        types::String bytes_;

      public:
        /**
         * @throw exception::InvalidArgument
         *
         * If the parameter `first` does not point to a valid UTF-8 string.
         *
         * @return Returns the render width of the given string.
         */
        static PACE__CXX20_CNSTXPR types::Size text_width( StringView text )
        {
          types::Size width = 0;
          for ( types::Size i = 0; i < text.size(); ) {
            auto parsed = U8Char::next_codepoint( text.substr( i ) );
            width += static_cast<types::Size>( U8Char::glyph_width( parsed.first ) );
            i += parsed.second;
          }
          return width;
        }

        PACE__CXX20_CNSTXPR U8Raw() noexcept( std::is_nothrow_default_constructible<types::String>::value )
          : width_ { 0 }
        {}
        explicit PACE__CXX20_CNSTXPR U8Raw( types::String u8_bytes ) : U8Raw()
        {
          width_ = text_width( u8_bytes );
          bytes_ = std::move( u8_bytes );
        }
        PACE__CXX20_CNSTXPR U8Raw( const U8Raw& )              = default;
        PACE__CXX20_CNSTXPR U8Raw( U8Raw&& )                   = default;
        PACE__CXX20_CNSTXPR U8Raw& operator=( const U8Raw& ) & = default;
        PACE__CXX20_CNSTXPR U8Raw& operator=( U8Raw&& ) &      = default;
        PACE__CXX20_CNSTXPR ~U8Raw()                           = default;

        PACE__CXX20_CNSTXPR U8Raw& operator=( charcodes::StringView u8_bytes ) &
        {
          const auto new_width = text_width( u8_bytes );
          auto new_bytes       = types::String( u8_bytes );
          bytes_.swap( new_bytes );
          width_ = new_width;
          return *this;
        }
        PACE__CXX20_CNSTXPR U8Raw& operator=( types::String&& u8_bytes ) &
        {
          width_ = text_width( u8_bytes );
          bytes_.swap( u8_bytes );
          return *this;
        }

        PACE__NODISCARD PACE__CXX20_CNSTXPR bool empty() const noexcept { return bytes_.empty(); }
        PACE__NODISCARD PACE__CXX20_CNSTXPR types::Size size() const noexcept { return bytes_.size(); }
        PACE__NODISCARD PACE__CXX20_CNSTXPR types::Size width() const noexcept { return width_; }

        PACE__CXX20_CNSTXPR const types::Char* data() const noexcept { return bytes_.data(); }
        PACE__CXX20_CNSTXPR charcodes::StringView str() & noexcept { return bytes_; }
        PACE__CXX20_CNSTXPR charcodes::StringView str() const& noexcept { return bytes_; }
        PACE__CXX20_CNSTXPR types::String&& str() && noexcept { return std::move( bytes_ ); }

        PACE__CXX20_CNSTXPR void clear() noexcept( noexcept( bytes_.clear() ) )
        {
          bytes_.clear();
          width_ = 0;
        }
        PACE__CXX20_CNSTXPR void shrink_to_fit() noexcept( noexcept( bytes_.shrink_to_fit() ) )
        { // The standard does not seem to specify whether the function is noexcept,
          // so let's make a judgment here.
          // At least I didn't see it on cppreference.
          bytes_.shrink_to_fit();
        }

        PACE__CXX20_CNSTXPR void swap( U8Raw& other ) noexcept
        {
          std::swap( width_, other.width_ );
          bytes_.swap( other.bytes_ );
        }
        friend PACE__CXX20_CNSTXPR void swap( U8Raw& a, U8Raw& b ) noexcept { a.swap( b ); }

        explicit PACE__CXX20_CNSTXPR operator types::String() & { return bytes_; }
        explicit PACE__CXX20_CNSTXPR operator types::String() const& { return bytes_; }
        explicit PACE__CXX20_CNSTXPR operator types::String&&() && noexcept { return std::move( bytes_ ); }
        PACE__CXX20_CNSTXPR operator charcodes::StringView() const noexcept { return str(); }

        PACE__NODISCARD friend PACE__FORCEINLINE PACE__CXX20_CNSTXPR types::String operator+( U8Raw&& a,
                                                                                              const U8Raw& b )
        { return std::move( a.bytes_ ) + b.bytes_; }
        PACE__NODISCARD friend PACE__FORCEINLINE PACE__CXX20_CNSTXPR types::String operator+(
          types::String&& a,
          const U8Raw& b )
        { return std::move( a ) + b.bytes_; }

        template<typename SV>
        PACE__NODISCARD friend PACE__FORCEINLINE PACE__CXX20_CNSTXPR
          typename std::enable_if<std::is_constructible<StringView, SV>::value, types::String>::type
          operator+( U8Raw&& a, SV&& b )
        {
          StringView sv = b;
          a.bytes_.append( b.data(), b.size() );
          return std::move( a.bytes_ );
        }
        template<typename SV>
        PACE__NODISCARD friend PACE__FORCEINLINE PACE__CXX20_CNSTXPR
          typename std::enable_if<std::is_constructible<StringView, SV>::value, types::String>::type
          operator+( SV&& a, const U8Raw& b )
        {
          StringView sv = a;
          types::String copy;
          copy.reserve( sv.size() + b.size() );
          copy.append( sv.data(), sv.size() ).append( b.bytes_ );
          return copy;
        }
        template<typename SV>
        PACE__NODISCARD friend PACE__FORCEINLINE PACE__CXX20_CNSTXPR
          typename std::enable_if<std::is_constructible<StringView, SV>::value, types::String>::type
          operator+( const U8Raw& a, SV&& b )
        {
          StringView sv = b;
          types::String copy;
          copy.reserve( a.size() + sv.size() );
          copy.append( a.bytes_ ).append( sv.data(), sv.size() );
          return copy;
        }

#ifdef __cpp_lib_char8_t
        explicit PACE__CXX20_CNSTXPR U8Raw( charcodes::U8StringView u8_sv ) : U8Raw()
        {
          auto new_bytes = types::String( u8_sv.size(), '\0' );
          std::copy( u8_sv.cbegin(), u8_sv.cend(), new_bytes.begin() );
          width_ = text_width( new_bytes );
          bytes_ = std::move( new_bytes );
        }

        explicit PACE__CXX20_CNSTXPR operator std::u8string() const
        {
          std::u8string ret;
          ret.resize( bytes_.size() );
          std::copy( bytes_.cbegin(), bytes_.cend(), ret.begin() );
          return ret;
        }

        PACE__NODISCARD friend PACE__FORCEINLINE PACE__CXX20_CNSTXPR types::String operator+(
          charcodes::U8StringView a,
          const U8Raw& b )
        {
          types::String tmp;
          tmp.reserve( a.size() );
          std::copy( a.cbegin(), a.cend(), std::back_inserter( tmp ) );
          return std::move( tmp ) + b.bytes_;
        }
        PACE__NODISCARD friend PACE__FORCEINLINE PACE__CXX20_CNSTXPR types::String operator+(
          U8Raw&& a,
          charcodes::U8StringView b )
        {
          a.bytes_.reserve( a.bytes_.size() + b.size() );
          std::copy( b.cbegin(), b.cend(), std::back_inserter( a.bytes_ ) );
          return std::move( a.bytes_ );
        }
        PACE__NODISCARD friend PACE__FORCEINLINE PACE__CXX20_CNSTXPR types::String operator+(
          const U8Raw& a,
          charcodes::U8StringView b )
        {
          auto tmp = a.bytes_;
          tmp.reserve( a.bytes_.size() + b.size() );
          std::copy( b.cbegin(), b.cend(), std::back_inserter( tmp ) );
          return tmp;
        }
#endif
      };
    } // namespace charcodes

    namespace utils {
      template<TxtAlign Alignment>
      PACE__NODISCARD PACE__FORCEINLINE PACE__CXX20_CNSTXPR types::String
        format_as( const charcodes::U8Raw& str, types::Size width, types::Char padding = ' ' )
      { return format_as<Alignment>( str.str(), width, padding ); }
      template<TxtAlign Alignment>
      PACE__NODISCARD PACE__FORCEINLINE PACE__CXX20_CNSTXPR types::String
        format_as( charcodes::U8Raw&& str, types::Size width, types::Char padding = ' ' )
      { return format_as<Alignment>( width, std::move( str ).str(), padding ); }
    } // namespace utils
  } // namespace details
} // namespace pace

#endif
