#ifndef PACE_PROVIDER
#define PACE_PROVIDER

#include "../details/core/Core.hpp"
#include <type_traits>

namespace pace {
  namespace config {
    // Provide default values for the specified config and option.
    template<typename Config, typename Option>
    struct Provider {
      static_assert( std::is_default_constructible<Option>::value,
                     "the provided parameters cannot be constructed by default" );
      static constexpr Option provide() noexcept( std::is_nothrow_default_constructible<Option>::value )
      { return Option(); }
    };

#ifdef __cpp_variable_templates
    // Allows providing a lambda that returns a default values directly
    // instead of specializing the entire Provider.
    template<typename Config, typename Option>
    PACE__CXX17_INLINE constexpr const auto provider_v = Provider<Config, Option>::provide;
#endif

#define PACE__PROVIDE_FOR( Config, Option, Defaults )                                    \
  template<>                                                                             \
  struct pace::config::Provider<Config, Option> {                                        \
    static Option provide()                                                              \
      noexcept( std::is_nothrow_constructible<Option, decltype( ( Defaults ) )>::value ) \
    { return Option( Defaults ); }                                                       \
  }

    // This is an internal function used for extracting the arguments initializing the config type.
    // **It should not be specialized or overloaded.**
    template<typename Config, typename Option>
    constexpr Option provide_for()
#ifdef __cpp_variable_templates
      noexcept( noexcept( config::provider_v<Config, Option>() ) )
    {
      static_assert( std::is_constructible<Option, decltype( config::provider_v<Config, Option>() )>::value,
                     "the provider_v specialization must be an invocable object" );
      return config::provider_v<Config, Option>();
    }
#else
      noexcept( noexcept( config::Provider<Config, Option>::provide() ) )
    {
      static_assert(
        std::is_constructible<Option, decltype( config::Provider<Config, Option>::provide() )>::value,
        "the provider_v::provide must be an invocable object" );
      return config::Provider<Config, Option>::provide();
    }
#endif
  } // namespace config
} // namespace pace

#endif
