#ifndef PACE_CORE
#define PACE_CORE

#include "Version.hpp"
#include <cstdint>

#if defined( _WIN32 ) || defined( _WIN64 )
# define PACE__WIN     1
# define PACE__UNIX    0
# define PACE__UNKNOWN 0
#elif defined( __unix__ )
# define PACE__WIN     0
# define PACE__UNIX    1
# define PACE__UNKNOWN 0
#else
# define PACE__WIN     0
# define PACE__UNIX    0
# define PACE__UNKNOWN 1
#endif

#if defined( __GNUC__ ) || defined( __clang__ )
// pace does not detect the differences in compiler versions
// which released before the publication of the C++11 standard.
# define PACE__FORCEINLINE __attribute__( ( always_inline ) ) inline
# define PACE__NOINLINE    __attribute__( ( noinline ) )
#elif defined( _MSC_VER )
// For msvc, it is a proprietary compiler implementation,
// and I believe it supports this builtin.
# define PACE__FORCEINLINE __forceinline
# define PACE__NOINLINE    __declspec( noinline )
#else
# define PACE__FORCEINLINE inline
# define PACE__NOINLINE
#endif

#ifdef __has_builtin
# define PACE__BUILTIN( builtin ) __has_builtin( builtin )
#else
# define PACE__BUILTIN( _ ) 0
#endif

#if defined( _MSC_VER ) && defined( _MSVC_LANG ) // for msvc
# define PACE__CC_STD _MSVC_LANG
#else
# define PACE__CC_STD __cplusplus
#endif

#if PACE__CC_STD >= 202302L
# define PACE__CXX23          1
# define PACE__CXX23_CNSTXPR  constexpr
# define PACE__ASSUME( expr ) [[assume( expr )]]
#else
# define PACE__CXX23 0
# define PACE__CXX23_CNSTXPR

# ifdef _MSC_VER
#  define PACE__ASSUME( expr ) __assume( expr )
# elif __clang_major__ > 3 || ( __clang_major__ == 3 && __clang_minor__ >= 6 )
#  define PACE__ASSUME( expr ) __builtin_assume( expr )
# elif defined( __GNUC__ )
#  define PACE__ASSUME( expr )   \
    do {                         \
      if ( expr ) {              \
      } else {                   \
        __builtin_unreachable(); \
      }                          \
    } while ( false )
# endif

# ifndef PACE__ASSUME
#  define PACE__ASSUME( _ ) PACE__ASSERT( 0 )
# endif

#endif
#if PACE__CC_STD >= 202002L
# define PACE__CXX20         1
# define PACE__CXX20_CNSTXPR constexpr
# define PACE__CNSTEVAL      consteval
# define PACE__UNLIKELY      [[unlikely]]
#else
# define PACE__CXX20 0
# define PACE__CXX20_CNSTXPR
# define PACE__CNSTEVAL constexpr
# define PACE__UNLIKELY
#endif
#if PACE__CC_STD >= 201703L
# define PACE__CXX17         1
# define PACE__CXX17_CNSTXPR constexpr
# define PACE__CXX17_INLINE  inline
# define PACE__FALLTHROUGH   [[fallthrough]]
# define PACE__NODISCARD     [[nodiscard]]
#else
# define PACE__CXX17 0
# define PACE__CXX17_CNSTXPR
# define PACE__CXX17_INLINE

# ifdef _MSC_VER
#  define PACE__NODISCARD _Check_return_
# elif __clang_major__ > 3 || ( __clang_major__ == 3 && __clang_minor__ >= 9 )
#  define PACE__FALLTHROUGH [[clang::fallthrough]]
# elif ( __clang_major__ == 3 && __clang_minor__ >= 5 ) || defined( __GNUC__ )
#  define PACE__NODISCARD __attribute__( ( warn_unused_result ) )
# endif
# if __GNUC__ >= 7
#  define PACE__FALLTHROUGH __attribute__( ( fallthrough ) )
# endif

# ifndef PACE__FALLTHROUGH
#  define PACE__FALLTHROUGH ( (void)0 )
# endif
# ifndef PACE__NODISCARD
#  define PACE__NODISCARD
# endif

#endif
#if PACE__CC_STD >= 201402L
# define PACE__CXX14         1
# define PACE__CXX14_CNSTXPR constexpr
#else
# define PACE__CXX14 0
# define PACE__CXX14_CNSTXPR
#endif
#if PACE__CC_STD < 201103L
# error "The library 'pace' requires C++11"
#endif

#ifdef PACE_DEBUG
# include <cassert>
# define PACE__ASSERT( expr ) assert( expr )
# undef PACE__FORCEINLINE
# define PACE__FORCEINLINE
#else
# define PACE__ASSERT( _ ) ( (void)0 )
#endif

// For assertion conditions without side effects.
#define PACE__TRUST( expr ) \
  do {                      \
    PACE__ASSERT( expr );   \
    PACE__ASSUME( expr );   \
  } while ( false )

// Pack multiple macro parameters into a single one.
#define PACE__WRAP( ... ) __VA_ARGS__

#define PACE__SPECIAL_MEMBERS_CX( ClassName, Constexpr )          \
  Constexpr ClassName( const ClassName& )              = default; \
  Constexpr ClassName( ClassName&& )                   = default; \
  Constexpr ClassName& operator=( const ClassName& ) & = default; \
  Constexpr ClassName& operator=( ClassName&& ) &      = default; \
  PACE__CXX20_CNSTXPR ~ClassName()                     = default

#define PACE__SPECIAL_MEMBERS( ClassName )                                  \
  constexpr ClassName()                                          = default; \
  constexpr ClassName( const ClassName& )                        = default; \
  constexpr ClassName( ClassName&& )                             = default; \
  PACE__CXX14_CNSTXPR ClassName& operator=( const ClassName& ) & = default; \
  PACE__CXX14_CNSTXPR ClassName& operator=( ClassName&& ) &      = default; \
  PACE__CXX20_CNSTXPR ~ClassName()                               = default

#endif
