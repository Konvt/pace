#ifndef PACE_SWEEP_PLOT
#define PACE_SWEEP_PLOT

#include "../details/aspects/Bar.hpp"
#include "../details/aspects/Entailment.hpp"
#include "../details/aspects/Filler.hpp"
#include "../details/aspects/Frame.hpp"
#include "../details/behaviors/Fancy.hpp"
#include "../details/behaviors/Indeterminate.hpp"
#include "../details/io/Combinator.hpp"
#include "../details/render/Parameter.hpp"

namespace pace {
  namespace facade {
    template<typename Base, typename Derived>
    class SweepPlot : public Base {
    protected:
      details::io::CharPipeline& build( details::io::CharPipeline& pipeline,
                                        const details::render::Parameter& params ) const
      {
        if ( this->bar_width_ == 0 )
          return pipeline;

        const auto brush     = details::io::when( !params.style_off_ && this->colorful() );
        const auto frame_cnt = static_cast<std::uint64_t>( params.frame_count_ * this->shift_factor_ );

        pipeline << brush( details::console::resetcolor,
                           details::console::Dualcolor { this->start_forecolor_, this->start_backcolor_ } )
                 << this->starting_;

        PACE__ASSERT( this->filler_.width() > 0 );
        if ( !this->lead_.empty() ) {
          const auto& current_lead = this->lead_[frame_cnt % this->lead_.size()];
          if ( current_lead.width() <= this->bar_width_ ) {
            // virtual_point is a value between 1 and this->bar_width
            const auto virtual_point = [this, frame_cnt]() noexcept -> std::uint64_t {
              if ( this->bar_width_ == 1 )
                return 1;
              const auto period = 2 * this->bar_width_ - 2;
              const auto pos    = frame_cnt % period;
              return pos < this->bar_width_ ? pos + 1 : 2 * this->bar_width_ - pos - 1;
            }();
            const auto len_left_fill = [this, virtual_point, &current_lead]() noexcept -> std::uint64_t {
              const auto len_half_lead = ( current_lead.width() / 2 ) + current_lead.width() % 2;
              if ( virtual_point <= len_half_lead )
                return 0;
              const auto len_unreached = this->bar_width_ - virtual_point;
              if ( len_unreached <= len_half_lead - current_lead.width() % 2 )
                return this->bar_width_ - current_lead.width();

              return virtual_point - len_half_lead;
            }();
            const auto len_right_fill = this->bar_width_ - ( len_left_fill + current_lead.width() );
            PACE__ASSERT( len_left_fill + len_right_fill + current_lead.width() == this->bar_width_ );

            pipeline
              << brush( details::console::resetcolor,
                        details::console::Dualcolor { this->filler_forecolor_, this->filler_backcolor_ } )
              << details::io::repeat( len_left_fill / this->filler_.width(), this->filler_ )
              << details::io::repeat( len_left_fill % this->filler_.width(), ' ' )
              << brush( details::console::resetcolor,
                        details::console::Dualcolor { this->lead_forecolor_, this->lead_backcolor_ } )
              << current_lead
              << brush( details::console::resetcolor,
                        details::console::Dualcolor { this->filler_forecolor_, this->filler_backcolor_ } )
              << details::io::repeat( len_right_fill % this->filler_.width(), ' ' )
              << details::io::repeat( len_right_fill / this->filler_.width(), this->filler_ );
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
      constexpr SweepPlot( details::traits::TypeSet<Options...> tag ) noexcept : Base( tag )
      {}

      PACE__SPECIAL_MEMBERS( SweepPlot );
    };
  } // namespace facade

  PACE__INHERIT_REGISTER( facade::SweepPlot,
                          details::aspects::Filler,
                          details::aspects::Bar,
                          details::aspects::Frame );

  PACE__ENTAIL_REGISTER( facade::SweepPlot, details::behaviors::Indeterminate, details::behaviors::Fancy );
} // namespace pace

#endif
