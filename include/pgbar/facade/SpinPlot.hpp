#ifndef PGBAR_SPIN_PLOT
#define PGBAR_SPIN_PLOT

#include "../details/aspects/Frame.hpp"
#include "../details/behaviors/Fancy.hpp"
#include "../details/behaviors/Indeterminate.hpp"
#include "../details/io/CharPipeline.hpp"
#include "../details/render/Parameter.hpp"
#include "BehaviorRegistry.hpp"

namespace pgbar {
  namespace facade {
    template<typename Base, typename Derived>
    class SpinPlot : public Base {
    protected:
      _details::io::CharPipeline& build( _details::io::CharPipeline& pipeline,
                                         const _details::render::Parameter& params ) const
      {
        if ( this->lead_.empty() )
          return pipeline;
        auto frame_cnt = static_cast<std::uint64_t>( params.frame_count_ * this->shift_factor_ );
        frame_cnt %= this->lead_.size();
        PGBAR__ASSERT( this->len_longest_lead_ >= this->lead_[frame_cnt].width() );

        return pipeline << this->reset_then_dye( this->lead_col_, params.style_off_ )
                        << _details::utils::format<_details::utils::TxtLayout::Left>(
                             this->len_longest_lead_,
                             this->lead_[frame_cnt] );
      }

      PGBAR__NODISCARD PGBAR__FORCEINLINE PGBAR__CXX20_CNSTXPR _details::types::Size fixed_length()
        const noexcept
      {
        return this->_details::traits::BaseOf_t<Base, _details::aspects::Frame>::fixed_length();
      }

      template<typename... Options>
      constexpr SpinPlot( _details::traits::TypeSet<Options...> tag ) noexcept : Base( tag )
      {}

      PGBAR__SPECIAL_MEMBERS( SpinPlot );
    };
  } // namespace facade

  PGBAR__INHERIT_REGISTER( facade::SpinPlot, _details::aspects::Frame );

  PGBAR__BEHAVIOR_REGISTER( facade::SpinPlot,
                            _details::behaviors::Indeterminate,
                            _details::behaviors::Fancy );
} // namespace pgbar

#endif
