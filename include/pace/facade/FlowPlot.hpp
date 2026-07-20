#ifndef PACE_FLOW_PLOT
#define PACE_FLOW_PLOT

#include "../details/aspects/Bar.hpp"
#include "../details/aspects/Entailment.hpp"
#include "../details/aspects/Filler.hpp"
#include "../details/aspects/Frame.hpp"
#include "../details/aspects/Reversible.hpp"
#include "../details/behaviors/Fancy.hpp"
#include "../details/behaviors/Indeterminate.hpp"
#include "../details/io/Combinator.hpp"
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

        const auto brush     = details::io::when( !params.style_off && this->colorful() );
        const auto frame_cnt = static_cast<std::uint64_t>( params.frame_count * this->shift_factor_ );

        pipeline << brush( details::console::resetcolor,
                           details::console::Dualcolor { this->start_forecolor_, this->start_backcolor_ } )
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

              pipeline
                << brush( details::console::resetcolor,
                          details::console::Dualcolor { this->filler_forecolor_, this->filler_backcolor_ } )
                << details::io::repeat( virtual_point / this->filler_.width(), this->filler_ )
                << details::io::repeat( virtual_point % this->filler_.width(), ' ' )
                << brush( details::console::resetcolor,
                          details::console::Dualcolor { this->lead_forecolor_, this->lead_backcolor_ } )
                << current_lead
                << brush( details::console::resetcolor,
                          details::console::Dualcolor { this->filler_forecolor_, this->filler_backcolor_ } )
                << details::io::repeat( len_right_fill % this->filler_.width(), ' ' )
                << details::io::repeat( len_right_fill / this->filler_.width(), this->filler_ );
            } else {
#ifdef __cpp_structured_bindings
              const auto& [left_part, right_part] = current_lead.split_by( len_vacancy );
#else
              const auto _division  = current_lead.split_by( len_vacancy );
              const auto &left_part = _division.first, &right_part = _division.second;
#endif
              const auto len_left_fill = virtual_point - right_part.width();

              pipeline
                << brush( details::console::resetcolor,
                          details::console::Dualcolor { this->lead_forecolor_, this->lead_backcolor_ } )
                << right_part
                << brush( details::console::resetcolor,
                          details::console::Dualcolor { this->filler_forecolor_, this->filler_backcolor_ } )
                << details::io::repeat( len_left_fill % this->filler_.width(), ' ' )
                << details::io::repeat( len_left_fill / this->filler_.width(), this->filler_ )
                << brush( details::console::resetcolor,
                          details::console::Dualcolor { this->lead_forecolor_, this->lead_backcolor_ } )
                << left_part << details::io::repeat( len_vacancy - left_part.width(), ' ' );
            }
          } else
            pipeline << details::io::repeat( this->bar_width_, ' ' );
        } else if ( this->filler_.empty() )
          pipeline << details::io::repeat( this->bar_width_, ' ' );
        else
          pipeline << brush(
            details::console::resetcolor,
            details::console::Dualcolor { this->filler_forecolor_, this->filler_backcolor_ } )
                   << details::io::repeat( this->bar_width_ / this->filler_.width(), this->filler_ )
                   << details::io::repeat( this->bar_width_ % this->filler_.width(), ' ' );

        return pipeline << brush( details::console::resetcolor,
                                  details::console::Dualcolor { this->end_forecolor_, this->end_backcolor_ } )
                        << this->ending_;
      }

      PACE__NODISCARD PACE__FORCEINLINE PACE__CXX20_CNSTXPR std::size_t fixed_length() const noexcept
      { return this->details::traits::BaseOf_t<Base, details::aspects::Bar>::fixed_length(); }

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
