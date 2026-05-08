#ifndef PGBAR_COUNTER
#define PGBAR_COUNTER

#include "../details/aspects/Capacity.hpp"
#include "../details/aspects/Entailment.hpp"
#include "../details/behaviors/Incremental.hpp"
#include "../details/io/CharPipeline.hpp"
#include "../details/render/Parameter.hpp"
#include "../details/traits/C3.hpp"

namespace pgbar {
  namespace facade {
    template<typename Base, typename Derived>
    class Counter : public Base {
    protected:
      details::io::CharPipeline& build( details::io::CharPipeline& pipeline,
                                        const details::render::Parameter& params ) const
      {
        if ( params.task_quota_ == 0 )
          pipeline << "-/-";

        return pipeline << details::utils::format<details::utils::TxtLayout::Right>(
                 details::utils::count_digits( params.task_quota_ ),
                 details::utils::format( params.tasks_completed_ ) )
                        << '/' << details::utils::format( params.task_quota_ );
      }

      PGBAR__NODISCARD PGBAR__FORCEINLINE PGBAR__CXX14_CNSTXPR std::uint32_t fixed_length() const noexcept
      {
        return details::utils::count_digits( this->task_quota_ ) * 2 + 1;
      }

      template<typename... Options>
      constexpr Counter( details::traits::TypeSet<Options...> tag ) noexcept : Base( tag )
      {}

      PGBAR__SPECIAL_MEMBERS( Counter );
    };
  } // namespace facade

  PGBAR__INHERIT_REGISTER( facade::Counter, details::aspects::Capacity );

  PGBAR__ENTAIL_REGISTER( facade::Counter, details::behaviors::Incremental );
} // namespace pgbar

#endif
