#ifndef PACE_ELAPSED
#define PACE_ELAPSED

#include "../details/aspects/Entailment.hpp"
#include "../details/aspects/Timer.hpp"
#include "../details/behaviors/Temporal.hpp"
#include "../details/render/Parameter.hpp"
#include "../details/traits/C3.hpp"
#include "../details/traits/TypeSet.hpp"

namespace pace {
  namespace facade {
    template<typename Base, typename Derived>
    class Elapsed : public Base {
    protected:
      details::io::CharPipeline& build( details::io::CharPipeline& pipeline,
                                        const details::render::Parameter& params ) const
      {
        pipeline << '+';
        return this->to_hms( pipeline, params.elapsed_time_ );
      }
      PACE__NODISCARD static PACE__FORCEINLINE PACE__CNSTEVAL details::types::Size fixed_length() noexcept
      { // deliberately not subtracting 1
        return sizeof( Base::_default_timer );
      }

      template<typename... Options>
      constexpr Elapsed( details::traits::TypeSet<Options...> tag ) noexcept : Base( tag )
      {}

      PACE__SPECIAL_MEMBERS( Elapsed );
    };

  } // namespace facade

  PACE__INHERIT_REGISTER( facade::Elapsed, details::aspects::Timer );

  PACE__ENTAIL_REGISTER( facade::Elapsed, details::behaviors::Temporal );
} // namespace pace

#endif
