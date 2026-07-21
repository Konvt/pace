#ifndef PACE_CHAR_PLOT
#define PACE_CHAR_PLOT

#include "../details/aspects/Bar.hpp"
#include "../details/aspects/Capacity.hpp"
#include "../details/aspects/Entailment.hpp"
#include "../details/aspects/Filler.hpp"
#include "../details/aspects/Frame.hpp"
#include "../details/aspects/Remain.hpp"
#include "../details/aspects/Reversible.hpp"
#include "../details/behaviors/Determinate.hpp"
#include "../details/behaviors/Fancy.hpp"
#include "../details/render/Parameter.hpp"

namespace pace {
  namespace facade {
    template<typename Base, typename Derived>
    class CharPlot : public Base {
    protected:
      details::io::CharPipeline& build( details::io::CharPipeline& pipeline,
                                        const details::render::Parameter& params ) const
      {
        if ( this->bar_width_ == 0 )
          return pipeline;

        const auto brush = details::io::when( !params.style_off && this->colorful() );
        auto frame_cnt   = params.frame_count;
        const auto len_finished =
          static_cast<std::size_t>( std::round( this->bar_width_ * params.progress_ratio ) );
        std::size_t len_vacancy = this->bar_width_ - len_finished;

        pipeline << brush( details::io::join(
          details::console::resetcolor,
          details::console::Dualcolor { this->start_forecolor_, this->start_backcolor_ } ) )
                 << this->starting_;

        PACE__ASSERT( this->filler_.width() > 0 );
        PACE__ASSERT( this->remain_.width() > 0 );
        if ( !this->reversed_ ) {
          pipeline << brush( details::io::join(
            details::console::resetcolor,
            details::console::Dualcolor { this->filler_forecolor_, this->filler_backcolor_ } ) )
                   << details::io::repeat( len_finished / this->filler_.width(), this->filler_ )
                   << details::io::repeat( len_finished % this->filler_.width(), ' ' );

          if ( !this->lead_.empty() ) {
            frame_cnt = static_cast<std::uint32_t>(
              static_cast<std::uint64_t>( params.frame_count * this->shift_factor_ ) % this->lead_.size() );
            const auto& current_lead = this->lead_[frame_cnt];
            if ( current_lead.width() <= len_vacancy ) {
              pipeline << brush( details::io::join(
                details::console::resetcolor,
                details::console::Dualcolor { this->lead_forecolor_, this->lead_backcolor_ } ) )
                       << current_lead;
              len_vacancy -= current_lead.width();
            }
          }

          pipeline << brush( details::io::join(
            details::console::resetcolor,
            details::console::Dualcolor { this->remain_forecolor_, this->remain_backcolor_ } ) )
                   << details::io::repeat( len_vacancy % this->remain_.width(), ' ' )
                   << details::io::repeat( len_vacancy / this->remain_.width(), ' ' );
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

          pipeline << brush( details::io::join(
            details::console::resetcolor,
            details::console::Dualcolor { this->remain_forecolor_, this->remain_backcolor_ } ) )
                   << details::io::repeat( len_vacancy % this->remain_.width(), ' ' )
                   << details::io::repeat( len_vacancy / this->remain_.width(), this->remain_ );

          if ( flag )
            pipeline << brush( details::io::join(
              details::console::resetcolor,
              details::console::Dualcolor { this->lead_forecolor_, this->lead_backcolor_ } ) )
                     << this->lead_[frame_cnt];

          pipeline << brush( details::io::join(
            details::console::resetcolor,
            details::console::Dualcolor { this->filler_forecolor_, this->filler_backcolor_ } ) )
                   << details::io::repeat( len_finished / this->filler_.width(), this->filler_ )
                   << details::io::repeat( len_finished % this->filler_.width(), ' ' );
        }

        return pipeline << brush( details::io::join(
                 details::console::resetcolor,
                 details::console::Dualcolor { this->end_forecolor_, this->end_backcolor_ } ) )
                        << this->ending_;
      }

      PACE__NODISCARD PACE__FORCEINLINE PACE__CXX20_CNSTXPR std::size_t fixed_length() const noexcept
      { return this->details::traits::BaseOf_t<Base, details::aspects::Bar>::fixed_length(); }

      template<typename... Options>
      constexpr CharPlot( details::traits::TypeSet<Options...> tag ) noexcept : Base( tag )
      {}

      PACE__SPECIAL_MEMBERS( CharPlot );
    };
  } // namespace facade

  PACE__INHERIT_REGISTER( facade::CharPlot,
                          details::aspects::Reversible,
                          details::aspects::Remain,
                          details::aspects::Filler,
                          details::aspects::Bar,
                          details::aspects::Frame,
                          details::aspects::Capacity );

  PACE__ENTAIL_REGISTER( facade::CharPlot, details::behaviors::Determinate, details::behaviors::Fancy );
} // namespace pace

#endif
