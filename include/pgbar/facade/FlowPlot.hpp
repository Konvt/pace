#ifndef PGBAR_FLOW_PLOT
#define PGBAR_FLOW_PLOT

#include "../details/aspects/Bar.hpp"
#include "../details/aspects/Filler.hpp"
#include "../details/aspects/Frame.hpp"
#include "../details/aspects/Reversible.hpp"
#include "../details/behaviors/Fancy.hpp"
#include "../details/behaviors/Indeterminate.hpp"
#include "../details/io/CharPipeline.hpp"
#include "../details/render/Parameter.hpp"
#include "BehaviorRegistry.hpp"

namespace pgbar {
  namespace facade {
    template<typename Base, typename Derived>
    class FlowPlot : public Base {
    protected:
      _details::io::CharPipeline& build( _details::io::CharPipeline& pipeline,
                                         const _details::render::Parameter& params ) const
      {
        if ( this->bar_width_ == 0 )
          return pipeline;

        const auto frame_cnt = static_cast<std::uint64_t>( params.frame_count_ * this->shift_factor_ );

        pipeline << this->reset_then_dye( this->start_col_, params.style_off_ ) << this->starting_;

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

              pipeline << this->reset_then_dye( this->filler_col_, params.style_off_ );
              pipeline.append( this->filler_, virtual_point / this->filler_.width() )
                .append( ' ', virtual_point % this->filler_.width() );
              pipeline << this->reset_then_dye( this->lead_col_, params.style_off_ ) << current_lead
                       << this->reset_then_dye( this->filler_col_, params.style_off_ );
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

              pipeline << this->reset_then_dye( this->lead_col_, params.style_off_ ) << right_part
                       << this->reset_then_dye( this->filler_col_, params.style_off_ );
              pipeline.append( ' ', len_left_fill % this->filler_.width() )
                .append( this->filler_, len_left_fill / this->filler_.width() );

              pipeline << this->reset_then_dye( this->lead_col_, params.style_off_ );
              pipeline.append( left_part ).append( ' ', len_vacancy - left_part.width() );
            }
          } else
            pipeline.append( ' ', this->bar_width_ );
        } else if ( this->filler_.empty() )
          pipeline.append( ' ', this->bar_width_ );
        else {
          pipeline << this->reset_then_dye( this->filler_col_, params.style_off_ );
          pipeline.append( this->filler_, this->bar_width_ / this->filler_.width() )
            .append( ' ', this->bar_width_ % this->filler_.width() );
        }

        return pipeline << this->reset_then_dye( this->end_col_, params.style_off_ ) << this->ending_;
      }

      PGBAR__NODISCARD PGBAR__FORCEINLINE PGBAR__CXX20_CNSTXPR _details::types::Size fixed_length()
        const noexcept
      {
        return this->_details::traits::BaseOf_t<Base, _details::aspects::Bar>::fixed_length();
      }

      template<typename... Options>
      constexpr FlowPlot( _details::traits::TypeSet<Options...> tag ) noexcept : Base( tag )
      {}

      PGBAR__SPECIAL_MEMBERS( FlowPlot );
    };
  } // namespace facade

  PGBAR__INHERIT_REGISTER( facade::FlowPlot,
                           _details::aspects::Reversible,
                           _details::aspects::Filler,
                           _details::aspects::Bar,
                           _details::aspects::Frame );

  PGBAR__BEHAVIOR_REGISTER( facade::FlowPlot,
                            _details::behaviors::Indeterminate,
                            _details::behaviors::Fancy );
} // namespace pgbar

#endif
