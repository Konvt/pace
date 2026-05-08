#ifndef PGBAR_PROVIDER
#define PGBAR_PROVIDER

#include "../details/core/Core.hpp"
#include <type_traits>

namespace pgbar {
  namespace config {
    // Provide default values for the specified config and option.
    template<typename Config, typename Option>
    struct ProvideFor {
      static_assert( std::is_default_constructible<Option>::value,
                     "the provided parameters cannot be constructed by default" );
      static constexpr Option provide() noexcept( std::is_nothrow_default_constructible<Option>::value )
      {
        return Option();
      }
    };

#if PGBAR__CXX14
    // Allows providing a lambda that returns a default values directly
    // instead of specializing the entire ProvideFor.
    template<typename Config, typename Option>
    PGBAR__CXX17_INLINE constexpr const auto ProvideFor_v = ProvideFor<Config, Option>::provide;
#endif

#define PGBAR__PROVIDE_FOR( Config, Option, Defaults )                                   \
  template<>                                                                             \
  struct pgbar::config::ProvideFor<Config, Option> {                                     \
    static Option provide()                                                              \
      noexcept( std::is_nothrow_constructible<Option, decltype( ( Defaults ) )>::value ) \
    {                                                                                    \
      return Option( Defaults );                                                         \
    }                                                                                    \
  }

    template<typename Config, typename Option>
    constexpr Option provide_for()
#if PGBAR__CXX14
      noexcept( noexcept( config::ProvideFor_v<Config, Option>() ) )
    {
      static_assert( std::is_constructible<Option, decltype( config::ProvideFor_v<Config, Option>() )>::value,
                     "the ProvideFor_v specialization must be an invocable object" );
      return config::ProvideFor_v<Config, Option>();
    }
#else
      noexcept( noexcept( config::ProvideFor<Config, Option>::provide() ) )
    {
      static_assert(
        std::is_constructible<Option, decltype( config::ProvideFor<Config, Option>::provide() )>::value,
        "the ProvideFor_v::provide must be an invocable object" );
      return config::ProvideFor<Config, Option>::provide();
    }
#endif
  } // namespace config
} // namespace pgbar

#endif
