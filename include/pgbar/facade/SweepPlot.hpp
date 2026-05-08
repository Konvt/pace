#ifndef PGBAR_SWEEP_PLOT
#define PGBAR_SWEEP_PLOT

#include "../details/aspects/Bar.hpp"
#include "../details/aspects/Filler.hpp"
#include "../details/aspects/Frame.hpp"
#include "../details/behaviors/Fancy.hpp"
#include "../details/behaviors/Indeterminate.hpp"
#include "../details/io/CharPipeline.hpp"
#include "../details/render/Parameter.hpp"
#include "BehaviorRegistry.hpp"

namespace pgbar {
  namespace facade {
    template<typename Base, typename Derived>
    class SweepPlot : public Base {
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
            PGBAR__ASSERT( len_left_fill + len_right_fill + current_lead.width() == this->bar_width_ );

            pipeline << this->reset_then_dye( this->filler_col_, params.style_off_ );
            pipeline.append( this->filler_, len_left_fill / this->filler_.width() )
              .append( ' ', len_left_fill % this->filler_.width() );

            pipeline << this->reset_then_dye( this->lead_col_, params.style_off_ ) << current_lead
                     << this->reset_then_dye( this->filler_col_, params.style_off_ );
            pipeline.append( ' ', len_right_fill % this->filler_.width() )
              .append( this->filler_, len_right_fill / this->filler_.width() );
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
      constexpr SweepPlot( _details::traits::TypeSet<Options...> tag ) noexcept : Base( tag )
      {}

      PGBAR__SPECIAL_MEMBERS( SweepPlot );
    };
  } // namespace facade

  PGBAR__INHERIT_REGISTER( facade::SweepPlot,
                           _details::aspects::Filler,
                           _details::aspects::Bar,
                           _details::aspects::Frame );

  PGBAR__BEHAVIOR_REGISTER( facade::SweepPlot,
                            _details::behaviors::Indeterminate,
                            _details::behaviors::Fancy );
} // namespace pgbar

#endif
