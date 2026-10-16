#ifndef PACE_PLATFORM
#define PACE_PLATFORM

#include "Core.hpp"
#include "Types.hpp"

#if PACE__WIN
# ifndef NOMINMAX
#  define NOMINMAX 1
# endif
# include "../../exception/Error.hpp"
# include <windows.h>
#elif PACE__UNIX
# include <unistd.h>
#endif

namespace pace {
  namespace details {
    namespace sys {
#if PACE__WIN
      template<Channel Sink>
      HANDLE std_handle()
      {
        HANDLE handle = []() noexcept {
          if PACE__CXX17_CNSTXPR ( Sink == Channel::Out )
            return GetStdHandle( STD_OUTPUT_HANDLE );
          else
            return GetStdHandle( STD_ERROR_HANDLE );
        }();
        if ( handle == INVALID_HANDLE_VALUE )
          PACE__UNLIKELY throw exception::SystemError( ERROR_INVALID_HANDLE,
                                                       std::system_category(),
                                                       "pace: cannot get the standard handle" );
        return handle;
      }
#else
      template<Channel Sink>
      void std_handle()
      { static_assert( Sink != Sink, "Windows only" ); }
#endif
    } // namespace sys
  } // namespace details
} // namespace pace

#endif
