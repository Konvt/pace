#ifndef PACE_SCREEN
#define PACE_SCREEN

#include "../core/Core.hpp"
#include "../core/Platform.hpp"
#include "../core/Types.hpp"
#include "../io/OStream.hpp"
#include "../utils/Backport.hpp"
#include "../utils/Singleton.hpp"

#if PACE__UNIX
# if defined( _POSIX_VERSION ) && _POSIX_VERSION >= 202405L
#  include <termios.h>
# else
#  include <sys/ioctl.h>
# endif
#endif

namespace pace {
  namespace details {
    namespace console {
      template<Channel Sink>
      class Screen final
        : private io::OStream<Sink>
        , public utils::Singleton<Screen<Sink>> {
        template<typename>
        friend class utils::Singleton;

        // It's internal components, and is only called by the rendering thread
        // (there is only one such thread), so atomic is not used.
        std::uint16_t vertical_snapshot_;
#ifndef PACE_INTTY
        bool renderable_snapshot_;
#endif

        Screen() = default;

      public:
        io::OStream<Sink>& ostream() noexcept { return *this; }
        const io::OStream<Sink>& ostream() const noexcept { return *this; }

        // Detect whether the specified output stream is bound to a terminal.
        PACE__NODISCARD static bool interactive() noexcept
        {
#if defined( PACE_INTTY ) || PACE__UNKNOWN
          return true;
#elif PACE__WIN
          return GetFileType( sys::std_handle<Sink>() ) == FILE_TYPE_CHAR;
#else
          // The overhead of calling isatty is no more than 1ms (actually around 500 ns).
          // On Windows, this overhead is even lower, approximately 100 ns.
          // So we directly call and return the result without caching it.
          return isatty( static_cast<int>( Sink ) );
#endif
        }

        // Record whether rendering should use interactive terminal controls.
        void set_renderable( bool value ) noexcept
        {
#ifdef PACE_INTTY
          (void)value;
#else
          renderable_snapshot_ = value;
#endif
        }
        PACE__NODISCARD constexpr bool renderable() const noexcept
        {
#ifdef PACE_INTTY
          return true;
#else
          return renderable_snapshot_;
#endif
        }

        PACE__NODISCARD static std::uint16_t width() noexcept
        {
#if PACE__WIN
          CONSOLE_SCREEN_BUFFER_INFO info;
          if ( GetConsoleScreenBufferInfo( sys::std_handle<Sink>(), &info ) )
            return static_cast<std::uint16_t>( info.srWindow.Right - info.srWindow.Left + 1 );
#elif PACE__UNIX
          struct winsize ws;
# if defined( _POSIX_VERSION ) && _POSIX_VERSION >= 202405L
          if ( tcgetwinsize( static_cast<int>( Sink ), &ws ) != -1 )
# else
          if ( ioctl( static_cast<int>( Sink ), TIOCGWINSZ, &ws ) != -1 )
# endif
            return ws.ws_col;
#endif
          return 0;
        }
        PACE__NODISCARD static std::uint16_t height() noexcept
        {
#if PACE__WIN
          CONSOLE_SCREEN_BUFFER_INFO info;
          if ( GetConsoleScreenBufferInfo( sys::std_handle<Sink>(), &info ) )
            return static_cast<std::uint16_t>( info.srWindow.Bottom - info.srWindow.Top + 1 );
#elif PACE__UNIX
          struct winsize ws;
# if defined( _POSIX_VERSION ) && _POSIX_VERSION >= 202405L
          if ( tcgetwinsize( static_cast<int>( Sink ), &ws ) != -1 )
# else
          if ( ioctl( static_cast<int>( Sink ), TIOCGWINSZ, &ws ) != -1 )
# endif
            return ws.ws_row;
#endif
          return 0;
        }

        // Update the height snapshot and return the old one.
        std::uint16_t vertical_alteration( std::uint16_t new_height ) noexcept
        { return utils::exchange( vertical_snapshot_, new_height ); }
      };
    } // namespace console
  } // namespace details
} // namespace pace

#endif
