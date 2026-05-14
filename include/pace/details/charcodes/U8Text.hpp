#ifndef PACE_U8_TEXT
#define PACE_U8_TEXT

#include "EncodedView.hpp"
#include "Font.hpp"
#include <numeric>
#include <vector>

namespace pace {
  namespace details {
    namespace charcodes {
      // A UTF-8 string that supports splitting strings by character width.
      class U8Text : public U8Raw {
      protected:
        std::vector<Font> chars_;

      public:
        PACE__NODISCARD static PACE__CXX23_CNSTXPR std::vector<Font> parse_glyph( StringView raw_u8_str )
        {
          std::vector<Font> characters;
          for ( types::Size i = 0; i < raw_u8_str.size(); ) {
            auto resolved = U8Char::next_codepoint( raw_u8_str.substr( i ) );
            characters.emplace_back( i, U8Char::glyph_width( resolved.first ) );
            i += resolved.second;
          }
          return characters;
        }

        PACE__CXX20_CNSTXPR U8Text() = default;
        explicit PACE__CXX23_CNSTXPR U8Text( types::String u8_bytes )
        {
          chars_ = parse_glyph( u8_bytes );
          width_ = std::accumulate( chars_.cbegin(),
                                    chars_.cend(),
                                    types::Size {},
                                    []( types::Size acc, const Font& ch ) noexcept {
                                      return acc + static_cast<types::Size>( ch.width_ );
                                    } );
          bytes_ = std::move( u8_bytes );
        }
        PACE__CXX20_CNSTXPR U8Text( const U8Text& )              = default;
        PACE__CXX20_CNSTXPR U8Text( U8Text&& )                   = default;
        PACE__CXX20_CNSTXPR U8Text& operator=( const U8Text& ) & = default;
        PACE__CXX20_CNSTXPR U8Text& operator=( U8Text&& ) &      = default;
        PACE__CXX20_CNSTXPR ~U8Text()                            = default;

        PACE__CXX23_CNSTXPR U8Text& operator=( charcodes::StringView u8_bytes ) &
        {
          auto new_chars = parse_glyph( u8_bytes );
          auto new_bytes = types::String( u8_bytes );
          chars_.swap( new_chars );
          width_ = std::accumulate( chars_.cbegin(),
                                    chars_.cend(),
                                    types::Size {},
                                    []( types::Size acc, const Font& ch ) noexcept {
                                      return acc + static_cast<types::Size>( ch.width_ );
                                    } );
          bytes_.swap( new_bytes );
          return *this;
        }
        PACE__CXX23_CNSTXPR U8Text& operator=( types::String u8_bytes ) &
        {
          auto new_chars = parse_glyph( u8_bytes );
          chars_.swap( new_chars );
          width_ = std::accumulate( chars_.cbegin(),
                                    chars_.cend(),
                                    types::Size {},
                                    []( types::Size acc, const Font& ch ) noexcept {
                                      return acc + static_cast<types::Size>( ch.width_ );
                                    } );
          bytes_.swap( u8_bytes );
          return *this;
        }

        PACE__CXX20_CNSTXPR void clear()
          noexcept( noexcept( std::declval<U8Raw&>().clear() ) && noexcept( chars_.clear() ) )
        {
          U8Raw::clear();
          chars_.clear();
        }
        PACE__CXX20_CNSTXPR void shrink_to_fit()
          noexcept( noexcept( std::declval<U8Raw&>().shrink_to_fit() ) && noexcept( chars_.shrink_to_fit() ) )
        {
          U8Raw::shrink_to_fit();
          chars_.shrink_to_fit();
        }

        /**
         * @brief Split a string into two parts based on the given width, with UTF-8 characters as the unit.
         * @param width The given width.
         * @return The split result and the width of each part.
         */
        PACE__NODISCARD PACE__FORCEINLINE PACE__CXX23_CNSTXPR std::pair<EncodedView, EncodedView> split_by(
          types::Size width ) const noexcept
        {
          if ( bytes_.empty() )
            PACE__UNLIKELY return {};

          // split_pos is the starting point of the right part
          types::Size split_pos = 0, left_width = 0;
          while ( split_pos < chars_.size() && left_width + chars_[split_pos].width_ <= width )
            left_width += chars_[split_pos++].width_;

          const auto split_loc =
            bytes_.data() + ( split_pos < chars_.size() ? chars_[split_pos].offset_ : bytes_.size() );
          return {
            { bytes_.data(), split_loc,                     left_width          },
            { split_loc,     bytes_.data() + bytes_.size(), width_ - left_width }
          };
        }

        PACE__CXX20_CNSTXPR void swap( U8Text& other ) noexcept
        {
          U8Raw::swap( other );
          chars_.swap( other.chars_ );
        }
        friend PACE__CXX20_CNSTXPR void swap( U8Text& a, U8Text& b ) noexcept { a.swap( b ); }

#ifdef __cpp_lib_char8_t
        explicit PACE__CXX23_CNSTXPR U8Text( charcodes::U8StringView u8_sv ) : U8Text()
        {
          auto new_bytes = types::String( u8_sv.size(), '\0' );
          std::copy( u8_sv.cbegin(), u8_sv.cend(), new_bytes.begin() );
          chars_ = parse_glyph( new_bytes );
          width_ = std::accumulate( chars_.cbegin(),
                                    chars_.cend(),
                                    types::Size {},
                                    []( types::Size acc, const Font& ch ) noexcept {
                                      return acc + static_cast<types::Size>( ch.width_ );
                                    } );
          bytes_ = std::move( new_bytes );
        }
#endif
      };
    } // namespace charcodes
  } // namespace details
} // namespace pace

#endif
