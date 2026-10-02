#ifndef PACE_PERCENTAGE
#define PACE_PERCENTAGE

#include "../details/aspects/Capacity.hpp"
#include "../details/aspects/Entailment.hpp"
#include "../details/behaviors/Incremental.hpp"
#include "../details/behaviors/Renderable.hpp"
#include "../details/render/Parameter.hpp"
#include "../details/render/TextAlign.hpp"
#include "../details/traits/C3.hpp"

namespace pace {
  namespace option {
    // Control the length of the decimal part.
    struct PercentDecs : PACE__DERIVING_OPTION2( PercentDecs, std::uint8_t, _decimals );
  }

  namespace facade {
    template<typename Base, typename Derived>
    class Percentage : public Base {
      PACE__FORCEINLINE friend PACE__CXX14_CNSTXPR void unpack( Percentage& self,
                                                                option::PercentDecs val ) noexcept
      { self.decimals_ = val.value; }

      std::uint8_t decimals_;

    protected:
      details::io::CharPipeline& build( details::io::CharPipeline& pipeline,
                                        const details::render::Parameter& params ) const
      {
        if ( params.task_quota == 0 )
          PACE__UNLIKELY return pipeline
            << details::io::align<details::render::TextAlign::Right>( fixed_width(),
                                                                      decimals_ > 0 ? "nan.%" : "n/a%" );

        std::string orig;
        details::utils::format_to( std::back_inserter( orig ), params.progress_ratio * 100.0, decimals_ );
        orig.push_back( '%' );
        return pipeline << details::io::align<details::render::TextAlign::Right>( fixed_width(),
                                                                                  std::move( orig ) );
      }

      PACE__NODISCARD PACE__FORCEINLINE PACE__CXX14_CNSTXPR std::size_t fixed_width() const noexcept
      { return 4 /* the length of "100" and "%" */ + decimals_ + static_cast<std::size_t>( decimals_ > 0 ); }

      template<typename... Options>
      PACE__CXX14_CNSTXPR Percentage( details::traits::TypeSet<Options...> tag ) noexcept : Base( tag )
      {
        if PACE__CXX17_CNSTXPR ( !details::traits::TpContains<details::traits::TypeSet<Options...>,
                                                              option::PercentDecs>::value )
          unpack( *this, config::provide_for<Derived, option::PercentDecs>() );
      }

      PACE__SPECIAL_MEMBERS( Percentage );

    public:
#define PACE__METHOD( OptionName, ParamName, ReturnType )                   \
  std::lock_guard<details::concurrent::SharedMutex> lock { this->rw_mtx_ }; \
  unpack( *this, option::OptionName( ParamName ) );                         \
  return static_cast<ReturnType>( *this )

      // Control the length of the decimal part.
      Derived& percent_decs( std::uint8_t _decimals ) & { PACE__METHOD( PercentDecs, _decimals, Derived& ); }
      Derived&& percent_decs( std::uint8_t _decimals ) &&
      { PACE__METHOD( PercentDecs, _decimals, Derived&& ); }

#undef PACE__METHOD
    };
  } // namespace facade

  PACE__INHERIT_REGISTER( facade::Percentage, details::aspects::Capacity );

  PACE__ENTAIL_REGISTER( facade::Percentage,
                         details::behaviors::Incremental,
                         details::behaviors::Renderable );
} // namespace pace

#endif
