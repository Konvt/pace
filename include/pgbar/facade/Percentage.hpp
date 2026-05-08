#ifndef PGBAR_PERCENTAGE
#define PGBAR_PERCENTAGE

#include "../details/aspects/Capacity.hpp"
#include "../details/aspects/Entailment.hpp"
#include "../details/behaviors/Incremental.hpp"
#include "../details/io/CharPipeline.hpp"
#include "../details/render/Parameter.hpp"
#include "../details/traits/C3.hpp"

namespace pgbar {
  namespace facade {
    template<typename Base, typename Derived>
    class Percentage : public Base {
      static constexpr auto& _default_percent = u8" --.--%";

    protected:
      details::io::CharPipeline& build( details::io::CharPipeline& pipeline,
                                        const details::render::Parameter& params ) const
      {
        if ( params.progress_ratio_ <= 0.0 ) // 0.01%
          PGBAR__UNLIKELY return pipeline << _default_percent;

        auto orig = details::utils::format( params.progress_ratio_ * 100.0, 2 );
        orig.push_back( '%' );
        return pipeline << details::utils::format<details::utils::TxtLayout::Right>( fixed_length(),
                                                                                     std::move( orig ) );
      }

      PGBAR__NODISCARD static PGBAR__FORCEINLINE PGBAR__CNSTEVAL details::types::Size fixed_length() noexcept
      {
        return sizeof( _default_percent ) - 1;
      }

      template<typename... Options>
      constexpr Percentage( details::traits::TypeSet<Options...> tag ) noexcept : Base( tag )
      {}

      PGBAR__SPECIAL_MEMBERS( Percentage );
    };
  } // namespace facade

  PGBAR__INHERIT_REGISTER( facade::Percentage, details::aspects::Capacity );

  PGBAR__ENTAIL_REGISTER( facade::Percentage, details::behaviors::Incremental );
} // namespace pgbar

#endif
