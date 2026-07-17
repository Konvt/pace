#ifndef PACE_OSTREAM
#define PACE_OSTREAM

#include "../../exception/Error.hpp"
#include "../utils/Singleton.hpp"
#include "CharPipeline.hpp"
#include <cerrno>
#if PACE__WIN
# include "../console/TermContext.hpp"
# ifndef NOMINMAX
#  define NOMINMAX 1
# endif
# include <windows.h>
#elif PACE__UNIX
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
        stream.release();
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

#if PACE__WIN && !defined( PACE_UTF8 )
        std::vector<WCHAR> wb_buffer_;
        std::vector<types::Char> localized_;
#endif

        PACE__CXX23_CNSTXPR OStream() = default;

      public:
        static PACE__FORCEINLINE void writeout( const types::Char* first, types::Size num_to_write )
        {
#if PACE__WIN
          types::Size total_written = 0;
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
                       first + total_written,
                       static_cast<DWORD>( num_to_write - total_written ),
                       &num_written,
                       nullptr );
            total_written += static_cast<types::Size>( num_written );
          } while ( total_written < num_to_write );
#elif PACE__UNIX
          types::Size total_written = 0;
          do {
            ssize_t num_written =
              write( utils::to_underlying( Sink ), first + total_written, num_to_write - total_written );
            if ( errno == EINTR )
              num_written = (std::max<ssize_t>)( 0, num_written );
            else if ( num_written < 0 )
              PACE__UNLIKELY throw exception::SystemError(
                std::error_code( errno, std::generic_category() ),
                charcodes::make_literal( "pace: write to output stream failed" ) );
            total_written += static_cast<types::Size>( num_written );
          } while ( total_written < num_to_write );
#else
          if PACE__CXX17_CNSTXPR ( Sink == Channel::Stdout )
            std::cout.write( first, num_to_write ).flush();
          else
            std::cerr.write( first, num_to_write ).flush();
#endif
        }

        PACE__CXX20_CNSTXPR ~OStream() = default;

#if PACE__WIN && !defined( PACE_UTF8 )
        PACE__FORCEINLINE PACE__CXX23_CNSTXPR void release() noexcept
        {
          this->CharPipeline::release();
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
            writeout( this->data(), this->size() );
            this->CharPipeline::clear();
            return *this;
          }

          // The target type char is not subject to strict alias restrictions.
          const auto wlen =
            MultiByteToWideChar( CP_UTF8, 0, this->data(), static_cast<int>( this->size() ), nullptr, 0 );
          PACE__TRUST( wlen > 0 );
          wb_buffer_.resize( static_cast<types::Size>( wlen ) );
          MultiByteToWideChar( CP_UTF8,
                               0,
                               this->data(),
                               static_cast<int>( this->size() ),
                               wb_buffer_.data(),
                               wlen );

          const auto mblen =
            WideCharToMultiByte( codepage, 0, wb_buffer_.data(), wlen, nullptr, 0, nullptr, nullptr );
          PACE__TRUST( mblen > 0 );
          localized_.resize( static_cast<types::Size>( mblen ) );
          WideCharToMultiByte( codepage,
                               0,
                               wb_buffer_.data(),
                               wlen,
                               reinterpret_cast<LPSTR>( localized_.data() ),
                               mblen,
                               nullptr,
                               nullptr );
          writeout( localized_.data(), localized_.size() );
#else
          writeout( this->data(), this->size() );
#endif
          clear();
          return *this;
        }

        friend PACE__FORCEINLINE PACE__CXX23_CNSTXPR OStream& operator<<( OStream& stream,
                                                                          OStream& ( &manipulator )(OStream&))
        { return manipulator( stream ); }
      };
    } // namespace io
  } // namespace details
} // namespace pace

#endif
