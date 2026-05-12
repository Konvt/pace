#ifndef PACE_SPIN_PLOT
#define PACE_SPIN_PLOT

#include "../details/aspects/Entailment.hpp"
#include "../details/aspects/Frame.hpp"
#include "../details/behaviors/Fancy.hpp"
#include "../details/behaviors/Indeterminate.hpp"
#include "../details/io/CharPipeline.hpp"
#include "../details/render/Parameter.hpp"

namespace pace {
  namespace facade {
    template<typename Base, typename Derived>
    class SpinPlot : public Base {
    protected:
      details::io::CharPipeline& build( details::io::CharPipeline& pipeline,
                                        const details::render::Parameter& params ) const
      {
        if ( this->lead_.empty() )
          return pipeline;
        auto frame_cnt = static_cast<std::uint64_t>( params.frame_count_ * this->shift_factor_ );
        frame_cnt %= this->lead_.size();
        PACE__ASSERT( this->len_longest_lead_ >= this->lead_[frame_cnt].width() );

        return pipeline << this->clear_then_dye(
                 details::console::Dualcolor( this->lead_forecolor_, this->lead_backcolor_ ),
                 params.style_off_ )
                        << details::utils::format<details::utils::TxtLayout::Left>( this->len_longest_lead_,
                                                                                    this->lead_[frame_cnt] );
      }

      PACE__NODISCARD PACE__FORCEINLINE PACE__CXX20_CNSTXPR details::types::Size fixed_length() const noexcept
      { return this->details::traits::BaseOf_t<Base, details::aspects::Frame>::fixed_length(); }

      template<typename... Options>
      constexpr SpinPlot( details::traits::TypeSet<Options...> tag ) noexcept : Base( tag )
      {}

      PACE__SPECIAL_MEMBERS( SpinPlot );
    };
  } // namespace facade

  PACE__INHERIT_REGISTER( facade::SpinPlot, details::aspects::Frame );

  PACE__ENTAIL_REGISTER( facade::SpinPlot, details::behaviors::Indeterminate, details::behaviors::Fancy );
} // namespace pace

#endif
