#ifndef PGBAR_TERM_CONTEXT
#define PGBAR_TERM_CONTEXT

#include "../core/Core.hpp"
#include "../types/Types.hpp"
#include <atomic>
#if PGBAR__WIN
# include <mutex>
# ifndef NOMINMAX
#  define NOMINMAX 1
# endif
# include <windows.h>
#else
# include <sys/ioctl.h>
# include <unistd.h>
#endif

namespace pgbar {
  namespace _details {
    namespace console {
      template<Channel Outlet>
      class TermContext {
        std::atomic<bool> cache_;

        TermContext() noexcept { detect(); }

      public:
        TermContext( const TermContext& )            = delete;
        TermContext& operator=( const TermContext& ) = delete;

        ~TermContext() = default;

        static TermContext& itself() noexcept
        {
          static TermContext self;
          return self;
        }

        // Detect whether the specified output stream is bound to a terminal.
        bool detect() noexcept
        {
          const bool value = []() noexcept {
#if defined( PGBAR_INTTY ) || PGBAR__UNKNOWN
            return true;
#elif PGBAR__WIN
            HANDLE hConsole;
            if PGBAR__CXX17_CNSTXPR ( Outlet == Channel::Stdout )
              hConsole = GetStdHandle( STD_OUTPUT_HANDLE );
            else
              hConsole = GetStdHandle( STD_ERROR_HANDLE );
            if ( hConsole == INVALID_HANDLE_VALUE )
              PGBAR__UNLIKELY return false;
            return GetFileType( hConsole ) == FILE_TYPE_CHAR;
#else
            return isatty( static_cast<int>( Outlet ) );
#endif
          }();
          cache_.store( value, std::memory_order_release );
          return value;
        }
        PGBAR__NODISCARD PGBAR__FORCEINLINE bool connected() const noexcept
        {
          return cache_.load( std::memory_order_acquire );
        }

        /**
         * Enable virtual terminal processing on the specified output channel (Windows only).
         * Guaranteed to be thread-safe and performed only once.
         */
        void virtual_term() const noexcept
        {
#if PGBAR__WIN && !defined( PGBAR_NOSTYLE ) && defined( ENABLE_VIRTUAL_TERMINAL_PROCESSING )
          static std::once_flag flag;
          std::call_once( flag, []() noexcept {
            HANDLE stream_handle =
              GetStdHandle( Outlet == Channel::Stderr ? STD_ERROR_HANDLE : STD_OUTPUT_HANDLE );
            if ( stream_handle == INVALID_HANDLE_VALUE )
              PGBAR__UNLIKELY return;

            DWORD mode {};
            if ( !GetConsoleMode( stream_handle, &mode ) )
              PGBAR__UNLIKELY return;
            mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
            SetConsoleMode( stream_handle, mode );
          } );
#endif
        }

        PGBAR__NODISCARD std::uint16_t width() noexcept
        {
          if ( !detect() )
            return 0;
#if PGBAR__WIN
          HANDLE h_con;
          if PGBAR__CXX17_CNSTXPR ( Outlet == Channel::Stdout )
            h_con = GetStdHandle( STD_OUTPUT_HANDLE );
          else
            h_con = GetStdHandle( STD_ERROR_HANDLE );
          if ( h_con != INVALID_HANDLE_VALUE ) {
            CONSOLE_SCREEN_BUFFER_INFO csbi;
            if ( GetConsoleScreenBufferInfo( h_con, &csbi ) )
              return csbi.srWindow.Right - csbi.srWindow.Left + 1;
          }
#elif PGBAR__UNIX
          struct winsize ws;
          auto fd = static_cast<int>( Outlet );
          if ( ioctl( fd, TIOCGWINSZ, &ws ) != -1 )
            return ws.ws_col;
#endif
          return 0;
        }
      };
    } // namespace console
  } // namespace _details
} // namespace pgbar

#endif
