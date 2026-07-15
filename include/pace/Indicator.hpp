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
    static constexpr bool _default_hide_completed = false;
    static constexpr bool _default_auto_style_off = true;

#ifdef __cpp_inline_variables
    static std::atomic<bool> _hide_completed;
    static std::atomic<bool> _auto_style_off;
#else
    static std::atomic<bool>& _hide_completed() noexcept
    {
      static std::atomic<bool> instance { _default_hide_completed };
      return instance;
    }
    static std::atomic<bool>& _auto_style_off() noexcept
    {
      static std::atomic<bool> instance { _default_auto_style_off };
      return instance;
    }
#endif

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
  };
#ifdef __cpp_inline_variables
  PACE__CXX17_INLINE std::atomic<bool> Indicator::_hide_completed { Indicator::_default_hide_completed };
  PACE__CXX17_INLINE std::atomic<bool> Indicator::_auto_style_off { Indicator::_default_auto_style_off };
#endif

  namespace config {
    inline void hide_completed( bool flag ) noexcept
    {
#ifdef __cpp_inline_variables
      Indicator::_hide_completed.store( flag, std::memory_order_relaxed );
#else
      Indicator::_hide_completed().store( flag, std::memory_order_relaxed );
#endif
    }
    inline bool hide_completed() noexcept
    {
#ifdef __cpp_inline_variables
      return Indicator::_hide_completed.load( std::memory_order_relaxed );
#else
      return Indicator::_hide_completed().load( std::memory_order_relaxed );
#endif
    }
    /**
     * Whether to automatically disable the style effect of the configuration object
     * when the output stream is not directed to a terminal.
     */
    inline void auto_style_off( bool flag ) noexcept
    {
#ifdef __cpp_inline_variables
      Indicator::_auto_style_off.store( flag, std::memory_order_relaxed );
#else
      Indicator::_auto_style_off().store( flag, std::memory_order_relaxed );
#endif
    }
    inline bool auto_style_off() noexcept
    {
#ifdef __cpp_inline_variables
      return Indicator::_auto_style_off;
#else
      return Indicator::_auto_style_off().load( std::memory_order_relaxed );
#endif
    }
  } // namespace config
} // namespace pace

#endif
