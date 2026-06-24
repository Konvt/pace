#ifndef PACE_FLOW_PLOT
#define PACE_FLOW_PLOT

#include "../details/aspects/Bar.hpp"
#include "../details/aspects/Entailment.hpp"
#include "../details/aspects/Filler.hpp"
#include "../details/aspects/Frame.hpp"
#include "../details/aspects/Reversible.hpp"
#include "../details/behaviors/Fancy.hpp"
#include "../details/behaviors/Indeterminate.hpp"
#include "../details/io/CharPipeline.hpp"
#include "../details/render/Parameter.hpp"

namespace pace {
  namespace facade {
    template<typename Base, typename Derived>
    class FlowPlot : public Base {
    protected:
      details::io::CharPipeline& build( details::io::CharPipeline& pipeline,
                                        const details::render::Parameter& params ) const
      {
        if ( this->bar_width_ == 0 )
          return pipeline;

        const auto frame_cnt = static_cast<std::uint64_t>( params.frame_count_ * this->shift_factor_ );

        pipeline << this->clear_then_dye(
          details::console::Dualcolor( this->start_forecolor_, this->start_backcolor_ ),
          params.style_off_ )
                 << this->starting_;

        PACE__ASSERT( this->filler_.width() > 0 );
        if ( !this->lead_.empty() ) {
          const auto& current_lead = this->lead_[frame_cnt % this->lead_.size()];
          if ( current_lead.width() <= this->bar_width_ ) {
            // virtual_point is a value between 0 and this->bar_width - 1
            const auto virtual_point = [this, frame_cnt]() noexcept {
              const auto pos = frame_cnt % this->bar_width_;
              return !this->reversed_ ? pos : ( this->bar_width_ - 1 - pos ) % this->bar_width_;
            }();
            const auto len_vacancy = this->bar_width_ - virtual_point;

            if ( current_lead.width() <= len_vacancy ) {
              const auto len_right_fill = len_vacancy - current_lead.width();

              pipeline << this->clear_then_dye(
                details::console::Dualcolor( this->filler_forecolor_, this->filler_backcolor_ ),
                params.style_off_ );
              pipeline.append( this->filler_, virtual_point / this->filler_.width() )
                .append( ' ', virtual_point % this->filler_.width() );
              pipeline << this->clear_then_dye(
                details::console::Dualcolor( this->lead_forecolor_, this->lead_backcolor_ ),
                params.style_off_ )
                       << current_lead
                       << this->clear_then_dye(
                            details::console::Dualcolor( this->filler_forecolor_, this->filler_backcolor_ ),
                            params.style_off_ );
              pipeline.append( ' ', len_right_fill % this->filler_.width() )
                .append( this->filler_, len_right_fill / this->filler_.width() );
            } else {
#ifdef __cpp_structured_bindings
              const auto& [left_part, right_part] = current_lead.split_by( len_vacancy );
#else
              const auto _division  = current_lead.split_by( len_vacancy );
              const auto &left_part = _division.first, &right_part = _division.second;
#endif
              const auto len_left_fill = virtual_point - right_part.width();

              pipeline << this->clear_then_dye(
                details::console::Dualcolor( this->lead_forecolor_, this->lead_backcolor_ ),
                params.style_off_ )
                       << right_part
                       << this->clear_then_dye(
                            details::console::Dualcolor( this->filler_forecolor_, this->filler_backcolor_ ),
                            params.style_off_ );
              pipeline.append( ' ', len_left_fill % this->filler_.width() )
                .append( this->filler_, len_left_fill / this->filler_.width() );

              pipeline << this->clear_then_dye(
                details::console::Dualcolor( this->lead_forecolor_, this->lead_backcolor_ ),
                params.style_off_ );
              pipeline.append( left_part ).append( ' ', len_vacancy - left_part.width() );
            }
          } else
            pipeline.append( ' ', this->bar_width_ );
        } else if ( this->filler_.empty() )
          pipeline.append( ' ', this->bar_width_ );
        else {
          pipeline << this->clear_then_dye(
            details::console::Dualcolor( this->filler_forecolor_, this->filler_backcolor_ ),
            params.style_off_ );
          pipeline.append( this->filler_, this->bar_width_ / this->filler_.width() )
            .append( ' ', this->bar_width_ % this->filler_.width() );
        }

        return pipeline << this->clear_then_dye(
                 details::console::Dualcolor( this->end_forecolor_, this->end_backcolor_ ),
                 params.style_off_ )
                        << this->ending_;
      }

      PACE__NODISCARD PACE__FORCEINLINE PACE__CXX20_CNSTXPR details::types::Size fixed_length() const noexcept
      { return this->details::traits::base_of_t<Base, details::aspects::Bar>::fixed_length(); }

      template<typename... Options>
      constexpr FlowPlot( details::traits::TypeSet<Options...> tag ) noexcept : Base( tag )
      {}

      PACE__SPECIAL_MEMBERS( FlowPlot );
    };
  } // namespace facade

  PACE__INHERIT_REGISTER( facade::FlowPlot,
                          details::aspects::Reversible,
                          details::aspects::Filler,
                          details::aspects::Bar,
                          details::aspects::Frame );

  PACE__ENTAIL_REGISTER( facade::FlowPlot, details::behaviors::Indeterminate, details::behaviors::Fancy );
} // namespace pace

#endif
