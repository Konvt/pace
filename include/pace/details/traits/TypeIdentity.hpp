#ifndef PACE_TYPE_IDENTITY
#define PACE_TYPE_IDENTITY

#include <type_traits>

namespace pace {
  namespace details {
    namespace traits {
#ifdef __cpp_lib_type_identity
      template<typename T>
      using TypeIdentity = std::type_identity<T>;
      template<typename T>
      using TypeIdentity_t = std::type_identity_t<T>;
#else
      template<typename T>
      struct TypeIdentity {
        using type = T;
      };
      template<typename T>
      using TypeIdentity_t = typename TypeIdentity<T>::type;
#endif
    }
  } // namespace details
} // namespace pace

#endif
