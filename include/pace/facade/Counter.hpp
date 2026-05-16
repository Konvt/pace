#ifndef PACE_COUNTER
#define PACE_COUNTER

#include "../details/aspects/Capacity.hpp"
#include "../details/aspects/Entailment.hpp"
#include "../details/behaviors/Incremental.hpp"
#include "../details/behaviors/Indeterminate.hpp"
#include "../details/behaviors/Plain.hpp"
#include "../details/io/CharPipeline.hpp"
#include "../details/render/Parameter.hpp"
#include "../details/traits/C3.hpp"

namespace pace {
  namespace option {
    // Decide whether to display the total number of tasks in the counter.
    struct ShowQuota : PACE__DERIVING_OPTION2( ShowQuota, bool, _enable );
  }

  namespace facade {
    template<typename Base, typename Derived>
    class Counter : public Base {
      friend PACE__FORCEINLINE PACE__CXX14_CNSTXPR void unpack( Counter& self,
                                                                option::ShowQuota&& val ) noexcept
      { self.show_quota_ = val.value(); }

      bool show_quota_;

    protected:
      details::io::CharPipeline& build( details::io::CharPipeline& pipeline,
                                        const details::render::Parameter& params ) const
      {
        pipeline << details::utils::format_as<details::utils::TxtAlign::Right>(
          details::utils::format( params.tasks_completed_ ),
          details::utils::count_digits( params.task_quota_ ) );
        if ( show_quota_ )
          pipeline << '/' << details::utils::format( params.task_quota_ );
        return pipeline;
      }

      PACE__NODISCARD PACE__FORCEINLINE PACE__CXX14_CNSTXPR std::uint32_t fixed_length() const noexcept
      {
        const auto num_digits = details::utils::count_digits( this->task_quota_ );
        return show_quota_ ? num_digits * 2 + 1 : num_digits;
      }

      template<typename... Options>
      PACE__CXX14_CNSTXPR Counter( details::traits::TypeSet<Options...> tag ) noexcept : Base( tag )
      {
        if PACE__CXX17_CNSTXPR ( !details::traits::TpContain<details::traits::TypeSet<Options...>,
                                                             option::ShowQuota>::value )
          unpack( *this, config::provide_for<Derived, option::ShowQuota>() );
      }

      PACE__SPECIAL_MEMBERS( Counter );

    public:
#define PACE__METHOD( OptionName, ReturnType )                              \
  std::lock_guard<details::concurrent::SharedMutex> lock { this->rw_mtx_ }; \
  unpack( *this, option::OptionName( _enable ) );                           \
  return static_cast<ReturnType>( *this )

      // Decide whether to display the total number of tasks in the counter.
      Derived& show_quota( bool _enable ) & noexcept
      { PACE__METHOD( ShowQuota, Derived& ); }
      Derived&& show_quota( bool _enable ) && noexcept
      { PACE__METHOD( ShowQuota, Derived&& ); }

#undef PACE__METHOD

      PACE__NODISCARD bool show_quota() const noexcept
      {
        details::concurrent::SharedLock<details::concurrent::SharedMutex> lock { this->rw_mtx_ };
        return show_quota_;
      }
    };
  } // namespace facade

  PACE__INHERIT_REGISTER( facade::Counter, details::aspects::Capacity );

  PACE__OPTION_REGISTER( facade::Counter, option::ShowQuota );

  PACE__ENTAIL_REGISTER( facade::Counter,
                         details::behaviors::Indeterminate,
                         details::behaviors::Plain,
                         details::behaviors::Incremental );
} // namespace pace

#endif
