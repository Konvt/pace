#ifndef PACE_UTILS_UTIL
#define PACE_UTILS_UTIL

#include "../core/Core.hpp"
#include "../traits/Backport.hpp"
#include <cmath>
#include <tuple>

namespace pace {
  namespace details {
    namespace utils {
      // Perfectly forward the I-th element of a tuple, constructing one by default if it's out of bound.
      template<types::Size I, typename T, typename Tuple>
      PACE__FORCEINLINE constexpr auto pick_or( Tuple&& tup ) noexcept ->
        typename std::enable_if<std::is_default_constructible<T>::value,
                                decltype( std::get<I>( std::forward<Tuple>( tup ) ) )>::type
      {
        static_assert( std::is_convertible<typename std::tuple_element<I, Tuple>::type, T>::value,
                       "incompatible type" );
        return std::get<I>( std::forward<Tuple>( tup ) );
      }
      template<types::Size I, typename T, typename Tuple>
      PACE__FORCEINLINE constexpr auto pick_or( Tuple&& )
        noexcept( std::is_nothrow_default_constructible<T>::value ) -> typename std::enable_if<
          traits::AllOf<
            traits::BoolConstant<( I >= std::tuple_size<typename std::decay<Tuple>::type>::value )>,
            std::is_default_constructible<T>>::value,
          T>::type
      { return T(); }

      template<typename Numeric>
      PACE__NODISCARD constexpr typename std::enable_if<std::is_unsigned<Numeric>::value, types::Size>::type
        count_digits( Numeric val ) noexcept
      {
#ifdef __cpp_lib_is_constant_evaluated
        if ( std::is_constant_evaluated() ) {
#endif
#if PACE__CXX14
          types::Size digits = val == 0;
          for ( ; val > 0; val /= 10 )
            ++digits;
          return digits;
#else
        return val < 10 ? 1 : 1 + count_digits( static_cast<Numeric>( val / 10 ) );
#endif
#ifdef __cpp_lib_is_constant_evaluated
        } else if ( val == 0 )
          return 1;
        else
          return std::log10( val ) + 1;
#endif
      }
      template<typename Numeric>
      PACE__NODISCARD PACE__FORCEINLINE constexpr
        typename std::enable_if<std::is_signed<Numeric>::value, types::Size>::type
        count_digits( Numeric val ) noexcept
      { return count_digits( static_cast<std::uint64_t>( val < 0 ? -val : val ) ); }
    } // namespace utils
  } // namespace details
} // namespace pace

#endif
