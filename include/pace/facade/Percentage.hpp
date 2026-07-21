#ifndef PACE_PERCENTAGE
#define PACE_PERCENTAGE

#include "../details/aspects/Capacity.hpp"
#include "../details/aspects/Entailment.hpp"
#include "../details/behaviors/Incremental.hpp"
#include "../details/behaviors/Indeterminate.hpp"
#include "../details/behaviors/Plain.hpp"
#include "../details/render/Parameter.hpp"
#include "../details/render/TextAlign.hpp"
#include "../details/traits/C3.hpp"

namespace pace {
  namespace facade {
    template<typename Base, typename Derived>
    class Percentage : public Base {
      static PACE__FORCEINLINE constexpr details::charcodes::StringView default_text() noexcept
      { return { " nan. %" }; }

    protected:
      details::io::CharPipeline& build( details::io::CharPipeline& pipeline,
                                        const details::render::Parameter& params ) const
      {
        if ( params.progress_ratio <= 0.0 ) // 0.01%
          PACE__UNLIKELY return pipeline << default_text();

        std::string orig;
        details::utils::format_to( std::back_inserter( orig ), params.progress_ratio * 100.0, 2 );
        orig.push_back( '%' );
        pipeline << details::io::align<details::render::TextAlign::Right>( fixed_length(),
                                                                           std::move( orig ) );

        return pipeline;
      }

      PACE__NODISCARD static PACE__FORCEINLINE PACE__CNSTEVAL std::size_t fixed_length() noexcept
      { return default_text().size(); }

      template<typename... Options>
      constexpr Percentage( details::traits::TypeSet<Options...> tag ) noexcept : Base( tag )
      {}

      PACE__SPECIAL_MEMBERS( Percentage );
    };
  } // namespace facade

  PACE__INHERIT_REGISTER( facade::Percentage, details::aspects::Capacity );

  PACE__ENTAIL_REGISTER( facade::Percentage,
                         details::behaviors::Indeterminate,
                         details::behaviors::Plain,
                         details::behaviors::Incremental );
} // namespace pace

#endif
