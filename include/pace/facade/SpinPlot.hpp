#ifndef PACE_SPIN_PLOT
#define PACE_SPIN_PLOT

#include "../details/aspects/Entailment.hpp"
#include "../details/aspects/Frame.hpp"
#include "../details/behaviors/Fancy.hpp"
#include "../details/behaviors/Indeterminate.hpp"
#include "../details/render/Parameter.hpp"
#include "../details/render/TextAlign.hpp"

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
        auto frame_cnt = static_cast<std::uint64_t>( params.frame_count * this->shift_factor_ );
        frame_cnt %= this->lead_.size();
        PACE__ASSERT( this->len_longest_lead_ >= this->lead_[frame_cnt].width() );

        pipeline << details::io::when(
          !params.style_off && this->colorful(),
          details::io::join( details::console::resetcolor,
                             details::console::Dualcolor { this->lead_forecolor_, this->lead_backcolor_ } ) )
                 << details::io::align<details::render::TextAlign::Left>( this->len_longest_lead_,
                                                                          this->lead_[frame_cnt] );

        return pipeline;
      }

      PACE__NODISCARD PACE__FORCEINLINE PACE__CXX20_CNSTXPR std::size_t fixed_length() const noexcept
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
