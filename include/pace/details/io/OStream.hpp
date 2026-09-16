#ifndef PACE_OSTREAM
#define PACE_OSTREAM

#include "../../exception/Error.hpp"
#include "../charcodes/StringView.hpp"
#include "../core/Types.hpp"
#include "../utils/Singleton.hpp"
#include "CharPipeline.hpp"
#include <cerrno>
#if PACE__WIN
# include <vector>
# ifndef NOMINMAX
#  define NOMINMAX 1
# endif
# include <windows.h>
#elif PACE__UNIX
# include <sys/ioctl.h>
# include <unistd.h>
#else
# include <iostream>
#endif
namespace pace {
  namespace details {
    namespace io {
      template<Channel Sink>
      class OStream;
      template<Channel Sink>
      OStream<Sink>& flush( OStream<Sink>& stream )
      { return stream.flush(); }
      template<Channel Sink>
      PACE__CXX23_CNSTXPR OStream<Sink>& release( OStream<Sink>& stream ) noexcept
      {
        stream.reset();
        return stream;
      }

      /**
       * A helper output stream that writes the data to `stdout` or `stderr` directly.
       *
       * It holds a proprietary buffer
       * so that don't have to use the common output buffers in the standard library.
       *
       * If the local platform is neither `Windows` nor `unix-like`,
       * the class still uses the method `write` of `std::ostream` in standard library.
       */
      template<Channel Sink>
      class OStream final
        : public CharPipeline
        , public utils::Singleton<OStream<Sink>> {
        friend class utils::Singleton<OStream>;

        std::atomic<bool> cache_ { true };

#if PACE__WIN && !defined( PACE_UTF8 )
        std::vector<WCHAR> wb_buffer_;
        std::vector<char> localized_;
#endif

        OStream() noexcept
        {
          // Enable virtual terminal processing on the specified output channel (Windows only).
          // `magic static` will guarantee this code to be thread-safe and performed only once.
#if PACE__WIN && !defined( PACE_NOSTYLE ) && defined( ENABLE_VIRTUAL_TERMINAL_PROCESSING )
          HANDLE h_con;
          if PACE__CXX17_CNSTXPR ( Sink == Channel::Stdout )
            h_con = GetStdHandle( STD_OUTPUT_HANDLE );
          else
            h_con = GetStdHandle( STD_ERROR_HANDLE );
          if ( h_con == INVALID_HANDLE_VALUE )
            PACE__UNLIKELY return;

          DWORD mode {};
          if ( !GetConsoleMode( h_con, &mode ) )
            PACE__UNLIKELY return;
          mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
          SetConsoleMode( h_con, mode );
#endif
        }

      public:
        // Detect whether the specified output stream is bound to a terminal.
        bool renderable() noexcept
        {
          const bool value = []() noexcept {
#if defined( PACE_INTTY ) || PACE__UNKNOWN
            return true;
#elif PACE__WIN
            HANDLE h_con;
            if PACE__CXX17_CNSTXPR ( Sink == Channel::Stdout )
              h_con = GetStdHandle( STD_OUTPUT_HANDLE );
            else
              h_con = GetStdHandle( STD_ERROR_HANDLE );
            if ( h_con == INVALID_HANDLE_VALUE )
              PACE__UNLIKELY return false;
            return GetFileType( h_con ) == FILE_TYPE_CHAR;
#else
            return isatty( static_cast<int>( Sink ) );
#endif
          }();
          cache_.store( value, std::memory_order_release );
          return value;
        }
        // Read the cached `renderable()` infomation.
        PACE__NODISCARD PACE__FORCEINLINE bool capable() const noexcept
        { return cache_.load( std::memory_order_relaxed ); }

        PACE__NODISCARD std::uint16_t width() noexcept
        {
          if ( !renderable() )
            return 0;
#if PACE__WIN
          HANDLE h_con;
          if PACE__CXX17_CNSTXPR ( Sink == Channel::Stdout )
            h_con = GetStdHandle( STD_OUTPUT_HANDLE );
          else
            h_con = GetStdHandle( STD_ERROR_HANDLE );
          if ( h_con != INVALID_HANDLE_VALUE ) {
            CONSOLE_SCREEN_BUFFER_INFO csbi;
            if ( GetConsoleScreenBufferInfo( h_con, &csbi ) )
              return csbi.srWindow.Right - csbi.srWindow.Left + 1;
          }
#elif PACE__UNIX
          struct winsize ws;
          auto fd = static_cast<int>( Sink );
          if ( ioctl( fd, TIOCGWINSZ, &ws ) != -1 )
            return ws.ws_col;
#endif
          return 0;
        }

        static PACE__FORCEINLINE void emit( charcodes::StringView content )
        {
#if PACE__WIN
          std::size_t total_written = 0;
          do {
            DWORD num_written = 0;
            auto ostream      = []() {
              if PACE__CXX17_CNSTXPR ( Sink == Channel::Stdout )
                return GetStdHandle( STD_OUTPUT_HANDLE );
              else
                return GetStdHandle( STD_ERROR_HANDLE );
            }();
            if ( ostream == INVALID_HANDLE_VALUE )
              PACE__UNLIKELY throw exception::SystemError(
                std::error_code( errno, std::generic_category() ),
                charcodes::make_literal( "pace: cannot open the standard output stream" ) );
            WriteFile( ostream,
                       content.data() + total_written,
                       static_cast<DWORD>( content.size() - total_written ),
                       &num_written,
                       nullptr );
            total_written += static_cast<std::size_t>( num_written );
          } while ( total_written < content.size() );
#elif PACE__UNIX
          std::size_t total_written = 0;
          do {
            ssize_t num_written = write( utils::to_underlying( Sink ),
                                         content.data() + total_written,
                                         content.size() - total_written );
            if ( errno == EINTR )
              num_written = (std::max<ssize_t>)( 0, num_written );
            else if ( num_written < 0 )
              PACE__UNLIKELY throw exception::SystemError(
                std::error_code( errno, std::generic_category() ),
                charcodes::make_literal( "pace: write to output stream failed" ) );
            total_written += static_cast<std::size_t>( num_written );
          } while ( total_written < content.size() );
#else
          if PACE__CXX17_CNSTXPR ( Sink == Channel::Stdout )
            std::cout.write( content.data(), content.size() ).flush();
          else
            std::cerr.write( content.data(), content.size() ).flush();
#endif
        }

        PACE__CXX20_CNSTXPR ~OStream() = default;

#if PACE__WIN && !defined( PACE_UTF8 )
        PACE__FORCEINLINE PACE__CXX23_CNSTXPR void reset() noexcept
        {
          this->CharPipeline::reset();
          wb_buffer_.clear();
          wb_buffer_.shrink_to_fit();
          localized_.clear();
          localized_.shrink_to_fit();
        }

        PACE__FORCEINLINE PACE__CXX23_CNSTXPR void clear() & noexcept
        {
          this->CharPipeline::clear();
          wb_buffer_.clear();
          localized_.clear();
        }
#endif

        OStream& flush() &
        {
          if ( this->empty() )
            return *this;

#if PACE__WIN && !defined( PACE_UTF8 )
          const auto codepage = GetConsoleOutputCP();
          if ( codepage == CP_UTF8 ) {
            emit( { this->data(), this->size() } );
            this->CharPipeline::clear();
            return *this;
          }

          const auto wlen =
            MultiByteToWideChar( CP_UTF8, 0, this->data(), static_cast<int>( this->size() ), nullptr, 0 );
          PACE__TRUST( wlen > 0 );
          wb_buffer_.resize( static_cast<std::size_t>( wlen ) );
          MultiByteToWideChar( CP_UTF8,
                               0,
                               this->data(),
                               static_cast<int>( this->size() ),
                               wb_buffer_.data(),
                               wlen );

          const auto mblen =
            WideCharToMultiByte( codepage, 0, wb_buffer_.data(), wlen, nullptr, 0, nullptr, nullptr );
          PACE__TRUST( mblen > 0 );
          localized_.resize( static_cast<std::size_t>( mblen ) );
          WideCharToMultiByte( codepage,
                               0,
                               wb_buffer_.data(),
                               wlen,
                               localized_.data(),
                               mblen,
                               nullptr,
                               nullptr );
          emit( { localized_.data(), localized_.size() } );
#else
          emit( { this->data(), this->size() } );
#endif
          clear();
          return *this;
        }

        PACE__FORCEINLINE friend PACE__CXX23_CNSTXPR OStream& operator<<( OStream& stream,
                                                                          OStream& ( &manipulator )(OStream&))
        { return manipulator( stream ); }
      };
    } // namespace io
  } // namespace details
} // namespace pace

#endif
