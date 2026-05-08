#ifndef PGBAR_CHAR_PLOT
#define PGBAR_CHAR_PLOT

#include "../details/aspects/Bar.hpp"
#include "../details/aspects/Capacity.hpp"
#include "../details/aspects/Filler.hpp"
#include "../details/aspects/Frame.hpp"
#include "../details/aspects/Remain.hpp"
#include "../details/aspects/Reversible.hpp"
#include "../details/behaviors/Determinate.hpp"
#include "../details/behaviors/Fancy.hpp"
#include "../details/io/CharPipeline.hpp"
#include "../details/render/Parameter.hpp"
#include "BehaviorRegistry.hpp"

namespace pgbar {
  namespace facade {
    template<typename Base, typename Derived>
    class CharPlot : public Base {
    protected:
      _details::io::CharPipeline& build( _details::io::CharPipeline& pipeline,
                                         const _details::render::Parameter& params ) const
      {
        if ( this->bar_width_ == 0 )
          return pipeline;

        auto frame_cnt = params.frame_count_;
        const auto len_finished =
          static_cast<_details::types::Size>( std::round( this->bar_width_ * params.progress_ratio_ ) );
        _details::types::Size len_vacancy = this->bar_width_ - len_finished;

        pipeline << this->reset_then_dye( this->start_col_, params.style_off_ ) << this->starting_;

        if ( !this->reversed_ ) {
          pipeline << this->reset_then_dye( this->filler_col_, params.style_off_ );
          pipeline.append( this->filler_, len_finished / this->filler_.width() )
            .append( ' ', len_finished % this->filler_.width() );

          if ( !this->lead_.empty() ) {
            frame_cnt = static_cast<std::uint32_t>(
              static_cast<std::uint64_t>( params.frame_count_ * this->shift_factor_ ) % this->lead_.size() );
            const auto& current_lead = this->lead_[frame_cnt];
            if ( current_lead.width() <= len_vacancy ) {
              pipeline << this->reset_then_dye( this->lead_col_, params.style_off_ );
              pipeline.append( current_lead );
              len_vacancy -= current_lead.width();
            }
          }

          pipeline << this->reset_then_dye( this->remain_col_, params.style_off_ );
          pipeline.append( ' ', len_vacancy % this->remain_.width() )
            .append( this->remain_, len_vacancy / this->remain_.width() );
        } else {
          const auto flag = [this, frame_cnt, &len_vacancy]() noexcept {
            if ( !this->lead_.empty() ) {
              const auto offset =
                static_cast<std::uint64_t>( frame_cnt * this->shift_factor_ ) % this->lead_.size();
              if ( this->lead_[offset].width() <= len_vacancy ) {
                len_vacancy -= this->lead_[offset].width();
                return true;
              }
            }
            return false;
          }();

          pipeline << this->reset_then_dye( this->remain_col_, params.style_off_ );
          pipeline.append( ' ', len_vacancy % this->remain_.width() )
            .append( this->remain_, len_vacancy / this->remain_.width() );

          if ( flag ) {
            pipeline << this->reset_then_dye( this->lead_col_, params.style_off_ );
            pipeline.append( this->lead_[frame_cnt] );
          }

          pipeline << this->reset_then_dye( this->filler_col_, params.style_off_ );
          pipeline.append( this->filler_, len_finished / this->filler_.width() )
            .append( ' ', len_finished % this->filler_.width() );
        }

        return pipeline << this->reset_then_dye( this->end_col_, params.style_off_ ) << this->ending_;
      }

      PGBAR__NODISCARD PGBAR__FORCEINLINE PGBAR__CXX20_CNSTXPR _details::types::Size fixed_length()
        const noexcept
      {
        return this->_details::traits::BaseOf_t<Base, _details::aspects::Bar>::fixed_length();
      }

      template<typename... Options>
      constexpr CharPlot( _details::traits::TypeSet<Options...> tag ) noexcept : Base( tag )
      {}

      PGBAR__SPECIAL_MEMBERS( CharPlot );
    };
  } // namespace facade

  PGBAR__INHERIT_REGISTER( facade::CharPlot,
                           _details::aspects::Reversible,
                           _details::aspects::Remain,
                           _details::aspects::Filler,
                           _details::aspects::Bar,
                           _details::aspects::Frame,
                           _details::aspects::Capacity );

  PGBAR__BEHAVIOR_REGISTER( facade::CharPlot, _details::behaviors::Determinate, _details::behaviors::Fancy );
} // namespace pgbar

#endif
