#ifndef PACE_EMISSION
#define PACE_EMISSION

#include "../io/CharPipeline.hpp"
#include "../utils/Backport.hpp"

namespace pace {
  namespace details {
    namespace wrappers {
      template<typename... Values>
      struct Emission {
        std::tuple<Values...> value;

        template<typename... Args>
        constexpr Emission( Args&&... args )
          noexcept( std::is_nothrow_constructible<std::tuple<Values...>, Args...>::value )
          : value { std::forward<Args>( args )... }
        {}

        friend PACE__FORCEINLINE io::CharPipeline& operator<<( io::CharPipeline& pipeline,
                                                               const Emission& self )
        {
          utils::apply(
            [&pipeline]( const typename std::remove_reference<Values>::type&... vals ) {
              (void)std::initializer_list<bool> { ( pipeline << vals, false )... };
            },
            self.value );
          return pipeline;
        }
      };

#ifdef __cpp_deduction_guides
      template<typename... Args>
      Emission( Args&&... ) -> Emission<Args...>;
#endif
    } // namespace wrappers
  } // namespace details
} // namespace pace

#endif
