#ifndef PACE_IDENTITY
#define PACE_IDENTITY

#include <type_traits>

namespace pace {
  namespace details {
    namespace traits {
#ifdef __cpp_lib_type_identity
      template<typename T>
      using Identity = std::type_identity<T>;
      template<typename T>
      using Identity_t = std::type_identity_t<T>;
#else
      template<typename T>
      struct Identity {
        using type = T;
      };
      template<typename T>
      using Identity_t = typename Identity<T>::type;
#endif
    }
  } // namespace details
} // namespace pace

#endif
