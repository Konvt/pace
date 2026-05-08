#ifndef PGBAR_INDICATOR
#define PGBAR_INDICATOR

#include "details/concurrent/Util.hpp"
#include "details/console/TermContext.hpp"
#include "details/render/Renderer.hpp"

namespace pgbar {
  namespace config {
    void hide_completed( bool flag ) noexcept;
    PGBAR__NODISCARD bool hide_completed() noexcept;
    void auto_style_off( bool flag ) noexcept;
    PGBAR__NODISCARD bool auto_style_off() noexcept;
  }

  class Indicator {
    static std::atomic<bool> _hide_completed;
    static std::atomic<bool> _auto_style_off;

    friend void config::hide_completed( bool ) noexcept;
    friend bool config::hide_completed() noexcept;
    friend void config::auto_style_off( bool ) noexcept;
    friend bool config::auto_style_off() noexcept;

  public:
    Indicator()                              = default;
    Indicator( const Indicator& )            = delete;
    Indicator& operator=( const Indicator& ) = delete;
    Indicator( Indicator&& )                 = default;
    Indicator& operator=( Indicator&& ) &    = default;
    virtual ~Indicator()                     = default;

    virtual void reset()                                  = 0;
    virtual void abort() noexcept                         = 0;
    virtual void tick() &                                 = 0;
    PGBAR__NODISCARD virtual bool active() const noexcept = 0;

    // Wait until the indicator is Stop.
    void wait() const noexcept
    {
      _details::concurrent::spin_wait( [this]() noexcept { return !active(); } );
    }
    // Wait for the indicator is Stop or timed out.
    template<class Rep, class Period>
    PGBAR__NODISCARD bool wait_for( const std::chrono::duration<Rep, Period>& timeout ) const noexcept
    {
      return _details::concurrent::spin_wait_for( [this]() noexcept { return !active(); }, timeout );
    }
  };
  PGBAR__CXX17_INLINE std::atomic<bool> Indicator::_hide_completed { false };
  PGBAR__CXX17_INLINE std::atomic<bool> Indicator::_auto_style_off { true };

  namespace config {
    inline void hide_completed( bool flag ) noexcept
    {
      Indicator::_hide_completed.store( flag, std::memory_order_relaxed );
    }
    PGBAR__NODISCARD inline bool hide_completed() noexcept
    {
      return Indicator::_hide_completed.load( std::memory_order_relaxed );
    }
    /**
     * Whether to automatically disable the style effect of the configuration object
     * when the output stream is not directed to a terminal.
     */
    inline void auto_style_off( bool flag ) noexcept
    {
      Indicator::_auto_style_off.store( flag, std::memory_order_relaxed );
    }
    PGBAR__NODISCARD inline bool auto_style_off() noexcept
    {
      return Indicator::_auto_style_off.load( std::memory_order_relaxed );
    }

    /**
     * Determine if the output stream is binded to the tty based on the platform api.
     *
     * Always returns true if defined `PGBAR_INTTY`,
     * or the local platform is neither `Windows` nor `unix-like`.
     */
    PGBAR__NODISCARD inline bool intty( Channel channel ) noexcept
    {
      if ( channel == Channel::Stdout )
        return _details::console::TermContext<Channel::Stdout>::itself().detect();
      return _details::console::TermContext<Channel::Stderr>::itself().detect();
    }

    PGBAR__NODISCARD inline std::uint16_t terminal_width( Channel channel ) noexcept
    {
      if ( channel == Channel::Stdout )
        return _details::console::TermContext<Channel::Stdout>::itself().width();
      return _details::console::TermContext<Channel::Stderr>::itself().width();
    }

    // Get the current output interval.
    template<Channel Outlet>
    PGBAR__NODISCARD _details::types::Tempus refresh_interval() noexcept
    {
      return _details::render::Renderer<Outlet>::working_interval();
    }
    // Set the new output interval.
    template<Channel Outlet>
    void refresh_interval( _details::types::Tempus new_rate ) noexcept
    {
      _details::render::Renderer<Outlet>::working_interval( new_rate );
    }
    // Set every channels to the same output interval.
    inline void refresh_interval( _details::types::Tempus new_rate ) noexcept
    {
      _details::render::Renderer<Channel::Stderr>::working_interval( new_rate );
      _details::render::Renderer<Channel::Stdout>::working_interval( new_rate );
    }
  } // namespace config
} // namespace pgbar

#endif
