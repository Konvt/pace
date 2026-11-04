#ifndef PACE_RUNTIME
#define PACE_RUNTIME

#include "../details/charcodes/StringView.hpp"
#include "../details/charcodes/U8Raw.hpp"
#include "../details/console/Screen.hpp"
#include "../details/render/Renderer.hpp"

namespace pace {
  namespace config {
    /**
     * Determine if the output stream is binded to the tty based on the platform api.

     * Always returns true if defined `PACE_INTTY`,
     * or the local platform is neither `Windows` nor `unix-like`.
     */
    PACE__NODISCARD inline bool intty( Channel channel ) noexcept
    {
      if ( channel == Channel::Out )
        return details::console::Screen<Channel::Out>::renderable();
      return details::console::Screen<Channel::Err>::renderable();
    }

    PACE__NODISCARD inline std::uint16_t terminal_width( Channel channel ) noexcept
    {
      if ( channel == Channel::Out )
        return details::console::Screen<Channel::Out>::width();
      return details::console::Screen<Channel::Err>::width();
    }

    // Get the current output interval.
    template<Channel Sink>
    PACE__NODISCARD std::chrono::microseconds refresh_interval() noexcept
    { return details::render::Renderer<Sink>::working_interval(); }
    // Set the new output interval.
    template<Channel Sink>
    void refresh_interval( std::chrono::microseconds new_rate ) noexcept
    { details::render::Renderer<Sink>::working_interval( new_rate ); }
    // Set every channels to the same output interval.
    inline void refresh_interval( std::chrono::microseconds new_rate ) noexcept
    {
      details::render::Renderer<Channel::Out>::working_interval( new_rate );
      details::render::Renderer<Channel::Err>::working_interval( new_rate );
    }

    template<Channel Sink, typename Rep, typename Period>
    void refresh_interval( std::chrono::duration<Rep, Period> new_rate ) noexcept
    {
      details::render::Renderer<Sink>::working_interval(
        std::chrono::duration_cast<std::chrono::microseconds>( new_rate ) );
    }
    template<typename Rep, typename Period>
    void refresh_interval( std::chrono::duration<Rep, Period> new_rate ) noexcept
    {
      details::render::Renderer<Channel::Out>::working_interval(
        std::chrono::duration_cast<std::chrono::microseconds>( new_rate ) );
      details::render::Renderer<Channel::Err>::working_interval(
        std::chrono::duration_cast<std::chrono::microseconds>( new_rate ) );
    }

    // Calculate the terminal display width of a UTF-8 encoded string.
    PACE__NODISCARD inline PACE__CXX20_CNSTXPR std::size_t text_width( details::charcodes::StringView text )
    { return details::charcodes::U8Raw::text_width( text ); }
#ifdef __cpp_lib_char8_t
    PACE__NODISCARD inline std::size_t text_width( std::u8string_view text )
    {
      return details::charcodes::U8Raw::text_width(
        { reinterpret_cast<const char*>( text.data() ), text.size() } );
    }
#endif
  } // namespace config
} // namespace pace

#endif
