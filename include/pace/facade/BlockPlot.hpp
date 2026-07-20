#ifndef PACE_BLOCK_PLOT
#define PACE_BLOCK_PLOT

#include "../details/aspects/Bar.hpp"
#include "../details/aspects/Capacity.hpp"
#include "../details/aspects/Entailment.hpp"
#include "../details/aspects/Filler.hpp"
#include "../details/aspects/Frame.hpp"
#include "../details/aspects/Remain.hpp"
#include "../details/aspects/Reversible.hpp"
#include "../details/behaviors/Determinate.hpp"
#include "../details/behaviors/Plain.hpp"
#include "../details/io/Combinator.hpp"
#include "../details/render/Parameter.hpp"

namespace pace {
  namespace facade {
    template<typename Base, typename Derived>
    class BlockPlot : public Base {
    protected:
      details::io::CharPipeline& build( details::io::CharPipeline& pipeline,
                                        const details::render::Parameter& params ) const
      {
        if ( this->bar_width_ == 0 )
          return pipeline;

        const auto brush        = details::io::when( !params.style_off_ && this->colorful() );
        const auto len_finished = static_cast<std::size_t>( this->bar_width_ * params.progress_ratio_ );
        const details::types::Float fraction = ( this->bar_width_ * params.progress_ratio_ ) - len_finished;
        PACE__TRUST( fraction >= 0.0 );
        PACE__TRUST( fraction <= 1.0 );
        const auto incomplete_block = static_cast<std::size_t>( fraction * this->lead_.size() );
        PACE__ASSERT( incomplete_block <= this->lead_.size() );
        std::size_t len_vacancy = this->bar_width_ - len_finished;

        pipeline << brush( details::console::resetcolor,
                           details::console::Dualcolor { this->start_forecolor_, this->start_backcolor_ } )
                 << this->starting_;

        PACE__ASSERT( this->filler_.width() > 0 );
        PACE__ASSERT( this->remain_.width() > 0 );
        if ( !this->reversed_ ) {
          pipeline << brush(
            details::console::resetcolor,
            details::console::Dualcolor { this->filler_forecolor_, this->filler_backcolor_ } )
                   << details::io::repeat( len_finished / this->filler_.width(), this->filler_ )
                   << details::io::repeat( len_finished % this->filler_.width(), ' ' );

          if ( this->bar_width_ != len_finished && !this->lead_.empty()
               && this->lead_[incomplete_block].width() <= len_vacancy ) {
            pipeline << brush( details::console::resetcolor,
                               details::console::Dualcolor { this->lead_forecolor_, this->lead_backcolor_ } )
                     << this->lead_[incomplete_block];
            len_vacancy -= this->lead_[incomplete_block].width();
          }

          pipeline << brush(
            details::console::resetcolor,
            details::console::Dualcolor { this->remain_forecolor_, this->remain_backcolor_ } )
                   << details::io::repeat( len_vacancy % this->remain_.width(), ' ' )
                   << details::io::repeat( len_vacancy / this->remain_.width(), this->remain_ );
        } else {
          const auto flag = this->bar_width_ != len_finished && !this->lead_.empty()
                         && this->lead_[incomplete_block].width() <= len_vacancy;
          if ( flag )
            len_vacancy -= this->lead_[incomplete_block].width();

          pipeline << brush(
            details::console::resetcolor,
            details::console::Dualcolor { this->remain_forecolor_, this->remain_backcolor_ } )
                   << details::io::repeat( len_vacancy / this->remain_.width(), this->remain_ )
                   << details::io::repeat( len_vacancy % this->remain_.width(), ' ' );

          if ( flag )
            pipeline << brush( details::console::resetcolor,
                               details::console::Dualcolor { this->lead_forecolor_, this->lead_backcolor_ } )
                     << this->lead_[incomplete_block];

          pipeline << brush(
            details::console::resetcolor,
            details::console::Dualcolor { this->filler_forecolor_, this->filler_backcolor_ } )
                   << details::io::repeat( len_finished % this->filler_.width(), ' ' )
                   << details::io::repeat( len_finished / this->filler_.width(), this->filler_ );
        }

        return pipeline << brush( details::console::resetcolor,
                                  details::console::Dualcolor { this->end_forecolor_, this->end_backcolor_ } )
                        << this->ending_;
      }

      PACE__NODISCARD PACE__FORCEINLINE PACE__CXX20_CNSTXPR std::size_t fixed_length() const noexcept
      { return this->details::traits::BaseOf_t<Base, details::aspects::Bar>::fixed_length(); }

      template<typename... Options>
      constexpr BlockPlot( details::traits::TypeSet<Options...> tag ) noexcept : Base( tag )
      {}

      PACE__SPECIAL_MEMBERS( BlockPlot );
    };
  } // namespace facade

  PACE__INHERIT_REGISTER( facade::BlockPlot,
                          details::aspects::Reversible,
                          details::aspects::Remain,
                          details::aspects::Filler,
                          details::aspects::Bar,
                          details::aspects::Frame,
                          details::aspects::Capacity );

  PACE__ENTAIL_REGISTER( facade::BlockPlot, details::behaviors::Determinate, details::behaviors::Plain );
} // namespace pace

#endif
