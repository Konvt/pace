#ifndef PACE_PLATFORM
#define PACE_PLATFORM

#include "Core.hpp"

#if PACE__WIN
# ifndef NOMINMAX
#  define NOMINMAX 1
# endif
# include <windows.h>
#elif PACE__UNIX
# include <unistd.h>
#endif

#endif
