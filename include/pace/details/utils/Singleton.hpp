#ifndef PACE_SINGLETON
#define PACE_SINGLETON

#include <type_traits>

namespace pace {
  namespace details {
    namespace utils {
      template<typename Derived>
      class Singleton {
      protected:
        Singleton() = default;

      public:
        ~Singleton() = default;

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
