#ifndef PACE_RUNTIME
#define PACE_RUNTIME

#include "../details/io/OStream.hpp"
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
        return details::io::OStream<Channel::Out>::itself().renderable();
      return details::io::OStream<Channel::Err>::itself().renderable();
    }

    PACE__NODISCARD inline std::uint16_t terminal_width( Channel channel ) noexcept
    {
      if ( channel == Channel::Out )
        return details::io::OStream<Channel::Out>::itself().width();
      return details::io::OStream<Channel::Err>::itself().width();
    }

    /// @brief Get the current output interval.
    /// @return std::chrono::duration (a.k.a. `std::chrono::nanoseconds`)
    template<Channel Sink>
    PACE__NODISCARD std::chrono::nanoseconds refresh_interval() noexcept
    { return details::render::Renderer<Sink>::working_interval(); }
    // Set the new output interval.
    template<Channel Sink>
    void refresh_interval( std::chrono::nanoseconds new_rate ) noexcept
    { details::render::Renderer<Sink>::working_interval( new_rate ); }
    // Set every channels to the same output interval.
    inline void refresh_interval( std::chrono::nanoseconds new_rate ) noexcept
    {
      details::render::Renderer<Channel::Err>::working_interval( new_rate );
      details::render::Renderer<Channel::Out>::working_interval( new_rate );
    }

    template<Channel Sink, typename Rep, typename Period>
    void refresh_interval( std::chrono::duration<Rep, Period> new_rate ) noexcept
    {
      details::render::Renderer<Sink>::working_interval(
        std::chrono::duration_cast<std::chrono::nanoseconds>( new_rate ) );
    }
    template<typename Rep, typename Period>
    void refresh_interval( std::chrono::duration<Rep, Period> new_rate ) noexcept
    {
      details::render::Renderer<Channel::Err>::working_interval(
        std::chrono::duration_cast<std::chrono::nanoseconds>( new_rate ) );
      details::render::Renderer<Channel::Out>::working_interval(
        std::chrono::duration_cast<std::chrono::nanoseconds>( new_rate ) );
    }
  } // namespace config
} // namespace pace

#endif
