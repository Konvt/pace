#ifndef PACE_BASIC_BAR
#define PACE_BASIC_BAR

#include "../Indicator.hpp"
#include "../details/aspects/Entailment.hpp"
#include "BasicConfig.hpp"

namespace pace {
  namespace prefab {
    template<typename Soul,
             Channel Sink = Channel::Err,
             Policy Mode  = Policy::Async,
             Region Zone  = Region::Fixed>
    class BasicBar
      : public details::traits::LI<details::aspects::EntailmentLinker_t<Soul>>::
          template type<Indicator, BasicBar<Soul, Sink, Mode, Zone>> {
      using Base = typename details::traits::LI<details::aspects::EntailmentLinker_t<Soul>>::
        template type<Indicator, BasicBar<Soul, Sink, Mode, Zone>>;

    public:
      using config_type                      = Soul;
      static constexpr Channel channel_value = Sink;
      static constexpr Policy policy_value   = Mode;
      static constexpr Region region_value   = Zone;

      using Base::Base;
      BasicBar() = default;

      template<typename... Args
#ifdef __cpp_concepts
               >
        requires std::is_constructible_v<Soul, Args...>
#else
               ,
               typename = typename std::enable_if<std::is_constructible<Soul, Args...>::value>::type>
#endif
      constexpr BasicBar( Args&&... args ) : Base( Soul( std::forward<Args>( args )... ) )
      {}

      BasicBar( const BasicBar& )            = delete;
      BasicBar& operator=( const BasicBar& ) = delete;

      BasicBar( BasicBar&& )              = default;
      BasicBar& operator=( BasicBar&& ) & = default;
      ~BasicBar()                         = default;

      void swap( BasicBar& other ) noexcept
      {
        PACE__TRUST( this != &other );
        PACE__ASSERT( this->active() == false );
        PACE__ASSERT( other.active() == false );
        this->Base::swap( other );
      }
      friend void swap( BasicBar& a, BasicBar& b ) noexcept { a.swap( b ); }
    };
  } // namespace prefab
} // namespace pace

#endif
