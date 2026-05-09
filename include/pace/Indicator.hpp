#ifndef PACE_INDICATOR
#define PACE_INDICATOR

#include "config/Runtime.hpp"
#include "details/concurrent/Util.hpp"

namespace pace {
  namespace config {
    void hide_completed( bool flag ) noexcept;
    PACE__NODISCARD bool hide_completed() noexcept;
    void auto_style_off( bool flag ) noexcept;
    PACE__NODISCARD bool auto_style_off() noexcept;
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

    virtual void reset()                                 = 0;
    virtual void abort() noexcept                        = 0;
    virtual void tick() &                                = 0;
    PACE__NODISCARD virtual bool active() const noexcept = 0;

    // Wait until the indicator is Stop.
    void wait() const noexcept
    {
      details::concurrent::spin_wait( [this]() noexcept { return !active(); } );
    }
    // Wait for the indicator is Stop or timed out.
    template<class Rep, class Period>
    PACE__NODISCARD bool wait_for( const std::chrono::duration<Rep, Period>& timeout ) const noexcept
    {
      return details::concurrent::spin_wait_for( [this]() noexcept { return !active(); }, timeout );
    }
  };
  PACE__CXX17_INLINE std::atomic<bool> Indicator::_hide_completed { false };
  PACE__CXX17_INLINE std::atomic<bool> Indicator::_auto_style_off { true };

  namespace config {
    inline void hide_completed( bool flag ) noexcept
    { Indicator::_hide_completed.store( flag, std::memory_order_relaxed ); }
    inline bool hide_completed() noexcept
    { return Indicator::_hide_completed.load( std::memory_order_relaxed ); }
    /**
     * Whether to automatically disable the style effect of the configuration object
     * when the output stream is not directed to a terminal.
     */
    inline void auto_style_off( bool flag ) noexcept
    { Indicator::_auto_style_off.store( flag, std::memory_order_relaxed ); }
    inline bool auto_style_off() noexcept
    { return Indicator::_auto_style_off.load( std::memory_order_relaxed ); }
  } // namespace config
} // namespace pace

#endif
