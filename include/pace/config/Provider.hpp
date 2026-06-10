#ifndef PACE_PROVIDER
#define PACE_PROVIDER

#include "../details/core/Core.hpp"
#include <type_traits>

namespace pace {
  namespace config {
    // Provide default values for the specified config and option.
    template<typename Config, typename Option>
    struct ProvideFor {
      static_assert( std::is_default_constructible<Option>::value,
                     "the provided parameters cannot be constructed by default" );
      static constexpr Option provide() noexcept( std::is_nothrow_default_constructible<Option>::value )
      { return Option(); }
    };

#if PACE__CXX14
    // Allows providing a lambda that returns a default values directly
    // instead of specializing the entire ProvideFor.
    template<typename Config, typename Option>
    PACE__CXX17_INLINE constexpr const auto provide_for_v = ProvideFor<Config, Option>::provide;
#endif

#define PACE__PROVIDE_FOR( Config, Option, Defaults )                                    \
  template<>                                                                             \
  struct pace::config::ProvideFor<Config, Option> {                                      \
    static Option provide()                                                              \
      noexcept( std::is_nothrow_constructible<Option, decltype( ( Defaults ) )>::value ) \
    { return Option( Defaults ); }                                                       \
  }

    template<typename Config, typename Option>
    constexpr Option provide_for()
#if PACE__CXX14
      noexcept( noexcept( config::provide_for_v<Config, Option>() ) )
    {
      static_assert(
        std::is_constructible<Option, decltype( config::provide_for_v<Config, Option>() )>::value,
        "the provide_for_v specialization must be an invocable object" );
      return config::provide_for_v<Config, Option>();
    }
#else
      noexcept( noexcept( config::ProvideFor<Config, Option>::provide() ) )
    {
      static_assert(
        std::is_constructible<Option, decltype( config::ProvideFor<Config, Option>::provide() )>::value,
        "the provide_for_v::provide must be an invocable object" );
      return config::ProvideFor<Config, Option>::provide();
    }
#endif
  } // namespace config
} // namespace pace

#endif
