#ifndef PGBAR_BLOCK_PLOT
#define PGBAR_BLOCK_PLOT

#include "../details/aspects/Bar.hpp"
#include "../details/aspects/Capacity.hpp"
#include "../details/aspects/Filler.hpp"
#include "../details/aspects/Frame.hpp"
#include "../details/aspects/Remain.hpp"
#include "../details/aspects/Reversible.hpp"
#include "../details/behaviors/Determinate.hpp"
#include "../details/behaviors/Plain.hpp"
#include "../details/io/CharPipeline.hpp"
#include "../details/render/Parameter.hpp"
#include "BehaviorRegistry.hpp"

namespace pgbar {
  namespace facade {
    template<typename Base, typename Derived>
    class BlockPlot : public Base {
    protected:
      _details::io::CharPipeline& build( _details::io::CharPipeline& pipeline,
                                         const _details::render::Parameter& params ) const
      {
        if ( this->bar_width_ == 0 )
          return pipeline;

        const auto len_finished =
          static_cast<_details::types::Size>( this->bar_width_ * params.progress_ratio_ );
        const _details::types::Float fraction = ( this->bar_width_ * params.progress_ratio_ ) - len_finished;
        PGBAR__TRUST( fraction >= 0.0 );
        PGBAR__TRUST( fraction <= 1.0 );
        const auto incomplete_block = static_cast<_details::types::Size>( fraction * this->lead_.size() );
        PGBAR__ASSERT( incomplete_block <= this->lead_.size() );
        _details::types::Size len_vacancy = this->bar_width_ - len_finished;

        pipeline << this->reset_then_dye( this->start_col_, params.style_off_ ) << this->starting_;

        if ( !this->reversed_ ) {
          pipeline << this->reset_then_dye( this->filler_col_, params.style_off_ );
          pipeline.append( this->filler_, len_finished / this->filler_.width() )
            .append( ' ', len_finished % this->filler_.width() );

          if ( this->bar_width_ != len_finished && !this->lead_.empty()
               && this->lead_[incomplete_block].width() <= len_vacancy ) {
            pipeline << this->reset_then_dye( this->lead_col_, params.style_off_ );
            pipeline.append( this->lead_[incomplete_block] );
            len_vacancy -= this->lead_[incomplete_block].width();
          }

          pipeline << this->reset_then_dye( this->remain_col_, params.style_off_ );
          pipeline.append( ' ', len_vacancy % this->remain_.width() )
            .append( this->remain_, len_vacancy / this->remain_.width() );
        } else {
          const auto flag = this->bar_width_ != len_finished && !this->lead_.empty()
                         && this->lead_[incomplete_block].width() <= len_vacancy;
          if ( flag )
            len_vacancy -= this->lead_[incomplete_block].width();

          pipeline << this->reset_then_dye( this->remain_col_, params.style_off_ );
          pipeline.append( this->remain_, len_vacancy / this->remain_.width() )
            .append( ' ', len_vacancy % this->remain_.width() );

          if ( flag ) {
            pipeline << this->reset_then_dye( this->lead_col_, params.style_off_ );
            pipeline.append( this->lead_[incomplete_block] );
          }

          pipeline << this->reset_then_dye( this->filler_col_, params.style_off_ );
          pipeline.append( ' ', len_finished % this->filler_.width() )
            .append( this->filler_, len_finished / this->filler_.width() );
        }

        return pipeline << this->reset_then_dye( this->end_col_, params.style_off_ ) << this->ending_;
      }

      PGBAR__NODISCARD PGBAR__FORCEINLINE PGBAR__CXX20_CNSTXPR _details::types::Size fixed_length()
        const noexcept
      {
        return this->_details::traits::BaseOf_t<Base, _details::aspects::Bar>::fixed_length();
      }

      template<typename... Options>
      constexpr BlockPlot( _details::traits::TypeSet<Options...> tag ) noexcept : Base( tag )
      {}

      PGBAR__SPECIAL_MEMBERS( BlockPlot );
    };
  } // namespace facade

  PGBAR__INHERIT_REGISTER( facade::BlockPlot,
                           _details::aspects::Reversible,
                           _details::aspects::Remain,
                           _details::aspects::Filler,
                           _details::aspects::Frame,
                           _details::aspects::Bar,
                           _details::aspects::Capacity );

  PGBAR__BEHAVIOR_REGISTER( facade::BlockPlot, _details::behaviors::Determinate, _details::behaviors::Plain );
} // namespace pgbar

#endif
