#ifndef PACE_SINGLETON
#define PACE_SINGLETON

#include "../core/Core.hpp"
#include <type_traits>

namespace pace {
  namespace details {
    namespace utils {
      template<typename Derived>
      class Singleton {
      protected:
        constexpr Singleton() = default;

      public:
        PACE__CXX20_CNSTXPR ~Singleton() = default;

        Singleton( const Singleton& )            = delete;
        Singleton& operator=( const Singleton& ) = delete;
        Singleton( Singleton&& )                 = delete;
        Singleton& operator=( Singleton&& )      = delete;

        static Derived& itself() noexcept( std::is_nothrow_default_constructible<Derived>::value )
        {
          static Derived instance;
          return instance;
        }
      };
    } // namespace utils
  } // namespace details
} // namespace pace

#endif
