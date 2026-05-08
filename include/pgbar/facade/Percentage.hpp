#ifndef PGBAR_PERCENTAGE
#define PGBAR_PERCENTAGE

#include "../details/aspects/Capacity.hpp"
#include "../details/behaviors/Incremental.hpp"
#include "../details/io/CharPipeline.hpp"
#include "../details/render/Parameter.hpp"
#include "../details/traits/C3.hpp"
#include "BehaviorRegistry.hpp"

namespace pgbar {
  namespace facade {
    template<typename Base, typename Derived>
    class Percentage : public Base {
      static constexpr auto& _default_percent = u8" --.--%";

    protected:
      _details::io::CharPipeline& build( _details::io::CharPipeline& pipeline,
                                         const _details::render::Parameter& params ) const
      {
        if ( params.progress_ratio_ <= 0.0 ) // 0.01%
          PGBAR__UNLIKELY return pipeline << _default_percent;

        auto orig = _details::utils::format( params.progress_ratio_ * 100.0, 2 );
        orig.push_back( '%' );
        return pipeline << _details::utils::format<_details::utils::TxtLayout::Right>( fixed_length(),
                                                                                       std::move( orig ) );
      }

      PGBAR__NODISCARD static PGBAR__FORCEINLINE PGBAR__CNSTEVAL _details::types::Size fixed_length() noexcept
      {
        return sizeof( _default_percent ) - 1;
      }

      template<typename... Options>
      constexpr Percentage( _details::traits::TypeSet<Options...> tag ) noexcept : Base( tag )
      {}

      PGBAR__SPECIAL_MEMBERS( Percentage );
    };
  } // namespace facade

  PGBAR__INHERIT_REGISTER( facade::Percentage, _details::aspects::Capacity );

  PGBAR__BEHAVIOR_REGISTER( facade::Percentage, _details::behaviors::Incremental );
} // namespace pgbar

#endif
