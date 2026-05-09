#ifndef PACE_ENCODED_VIEW
#define PACE_ENCODED_VIEW

#include "U8Raw.hpp"

namespace pace {
  namespace details {
    namespace charcodes {
      // Basic string view, only provide basic reference semantics,
      // the function of traversing characters is not offered.
      class EncodedView {
        const types::Char *head_, *tail_;
        types::Size width_;

      public:
        using iterator = const types::Char*;

        constexpr EncodedView() noexcept : head_ { nullptr }, tail_ { nullptr }, width_ { 0 } {}
        constexpr EncodedView( const types::Char* head, const types::Char* tail, types::Size width ) noexcept
          : head_ { head }, tail_ { tail }, width_ { width }
        {
#if PACE__CXX14
          PACE__TRUST( head_ <= tail_ );
          PACE__TRUST( head_ != nullptr && tail_ != nullptr );
#endif
        }
        PACE__CXX20_CNSTXPR EncodedView( const U8Raw& u8_raw ) noexcept
          : EncodedView( u8_raw.data(), u8_raw.data() + u8_raw.size(), u8_raw.width() )
        {}

        EncodedView( std::nullptr_t, std::nullptr_t, types::Size )     = delete;
        EncodedView( std::nullptr_t, const types::Char*, types::Size ) = delete;
        EncodedView( const types::Char*, std::nullptr_t, types::Size ) = delete;

        PACE__NODISCARD PACE__FORCEINLINE constexpr types::Size size() const noexcept
        { return static_cast<types::Size>( tail_ - head_ ); }
        PACE__NODISCARD PACE__FORCEINLINE constexpr types::Size width() const noexcept { return width_; }
        PACE__NODISCARD PACE__FORCEINLINE constexpr bool empty() const noexcept { return head_ == tail_; }
        PACE__NODISCARD PACE__FORCEINLINE constexpr iterator begin() const noexcept { return head_; }
        PACE__NODISCARD PACE__FORCEINLINE constexpr iterator end() const noexcept { return tail_; }

#ifdef __cpp_lib_string_view
        PACE__NODISCARD PACE__FORCEINLINE constexpr types::ROStr as_string_view() const noexcept
        { return { head_, size() }; }
        constexpr operator types::ROStr() const noexcept { return as_string_view(); }
#endif
        explicit constexpr operator bool() const noexcept { return !empty(); }

        PACE__CXX20_CNSTXPR void swap( EncodedView& other ) noexcept
        {
          std::swap( head_, other.head_ );
          std::swap( tail_, other.tail_ );
          std::swap( width_, other.width_ );
        }
        PACE__CXX20_CNSTXPR friend void swap( EncodedView& a, EncodedView& b ) noexcept { a.swap( b ); }
      };
    } // namespace charcodes
  } // namespace details
} // namespace pace

#endif
