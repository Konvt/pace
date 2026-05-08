#ifndef PGBAR_ELAPSED
#define PGBAR_ELAPSED

#include "../details/aspects/Timer.hpp"
#include "../details/behaviors/Temporal.hpp"
#include "../details/render/Parameter.hpp"
#include "../details/traits/C3.hpp"
#include "../details/traits/TypeSet.hpp"
#include "BehaviorRegistry.hpp"

namespace pgbar {
  namespace facade {
    template<typename Base, typename Derived>
    class Elapsed : public Base {
    protected:
      _details::io::CharPipeline& build( _details::io::CharPipeline& pipeline,
                                         const _details::render::Parameter& params ) const
      {
        pipeline << '+';
        return this->to_hms( pipeline, params.elapsed_time_ );
      }
      PGBAR__NODISCARD static PGBAR__FORCEINLINE PGBAR__CNSTEVAL _details::types::Size fixed_length() noexcept
      { // deliberately not subtracting 1
        return sizeof( Base::_default_timer );
      }

      template<typename... Options>
      constexpr Elapsed( _details::traits::TypeSet<Options...> tag ) noexcept : Base( tag )
      {}

      PGBAR__SPECIAL_MEMBERS( Elapsed );
    };

  } // namespace facade

  PGBAR__INHERIT_REGISTER( facade::Elapsed, _details::aspects::Timer );

  PGBAR__BEHAVIOR_REGISTER( facade::Elapsed, _details::behaviors::Temporal );
} // namespace pgbar

#endif
