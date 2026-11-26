#ifndef PACE_BASIC_BAR
#define PACE_BASIC_BAR

#include "../Automation.hpp"
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

  template<Channel Sink = Channel::Err,
           Policy Mode  = Policy::Async,
           Region Zone  = Region::Fixed,
           typename Soul>
  PACE__NODISCARD auto auto_finish( Soul&& cfg ) ->
#ifdef __cpp_concepts
    AutoFinish<prefab::BasicBar<std::decay_t<Soul>, Sink, Mode, Zone>>
    requires details::traits::is_config<std::decay_t<Soul>>::value
#else
    typename std::enable_if<
      details::traits::is_config<typename std::decay<Soul>::type>::value,
      AutoFinish<prefab::BasicBar<typename std::decay<Soul>::type, Sink, Mode, Zone>>>::type
#endif
  { return { std::forward<Soul>( cfg ) }; }
  template<typename Soul, Channel Sink, Policy Mode, Region Zone>
  PACE__NODISCARD auto auto_finish( prefab::BasicBar<Soul, Sink, Mode, Zone>&& bar )
    -> AutoFinish<prefab::BasicBar<Soul, Sink, Mode, Zone>>
  { return { std::move( bar ) }; }
  template<typename Config,
           Channel Sink = Channel::Err,
           Policy Mode  = Policy::Async,
           Region Zone  = Region::Fixed,
           typename... Options>
  PACE__NODISCARD auto auto_finish( Options&&... options ) ->
#ifdef __cpp_concepts
    AutoFinish<prefab::BasicBar<Config, Sink, Mode, Zone>>
    requires details::traits::is_config<Config>::value && std::is_constructible_v<Config, Options...>
#else
    typename std::enable_if<details::traits::AllOf<details::traits::is_config<Config>,
                                                   std::is_constructible<Config, Options...>>::value,
                            AutoFinish<prefab::BasicBar<Config, Sink, Mode, Zone>>>::type
#endif
  { return { std::forward<Options>( options )... }; }

  //////////////////////////////////////////////////

  template<Channel Sink = Channel::Err,
           Policy Mode  = Policy::Async,
           Region Zone  = Region::Fixed,
           typename Soul,
           typename F,
           typename... Options>
  PACE__NODISCARD auto auto_finish( Soul&& cfg, F&& callback ) ->
#ifdef __cpp_concepts
    AutoFinishWith<prefab::BasicBar<std::decay_t<Soul>, Sink, Mode, Zone>, typename std::decay<F>::type>
    requires details::traits::is_config<std::decay_t<Soul>>::value
          && details::traits::is_routine<std::decay_t<F>,
                                         prefab::BasicBar<std::decay_t<Soul>, Sink, Mode, Zone>>::value
#else
    typename std::enable_if<
      details::traits::AllOf<details::traits::is_config<typename std::decay<Soul>::type>,
                             details::traits::is_routine<
                               typename std::decay<F>::type,
                               prefab::BasicBar<typename std::decay<Soul>::type, Sink, Mode, Zone>>>::value,
      AutoFinishWith<prefab::BasicBar<typename std::decay<Soul>::type, Sink, Mode, Zone>,
                     typename std::decay<F>::type>>::type
#endif
  {
#ifndef __cpp_concepts
    static_assert( details::traits::is_config<typename std::decay<Soul>::type>::value, "not a valid object" );
#endif
    return { std::forward<Soul>( cfg ), std::forward<F>( callback ) };
  }
  template<typename Soul, Channel Sink, Policy Mode, Region Zone, typename F, typename... Options>
  PACE__NODISCARD auto auto_finish( prefab::BasicBar<Soul, Sink, Mode, Zone>&& bar, F&& callback ) ->
#ifdef __cpp_concepts
    AutoFinishWith<prefab::BasicBar<Soul, Sink, Mode, Zone>, typename std::decay<F>::type>
    requires details::traits::is_routine<std::decay_t<F>, prefab::BasicBar<Soul, Sink, Mode, Zone>>::value
#else
    typename std::enable_if<
      details::traits::is_routine<typename std::decay<F>::type,
                                  prefab::BasicBar<Soul, Sink, Mode, Zone>>::value,
      AutoFinishWith<prefab::BasicBar<Soul, Sink, Mode, Zone>, typename std::decay<F>::type>>::type
#endif
  { return { std::move( bar ), std::forward<F>( callback ) }; }
  template<typename Config,
           Channel Sink = Channel::Err,
           Policy Mode  = Policy::Async,
           Region Zone  = Region::Fixed,
           typename F,
           typename... Options>
  PACE__NODISCARD auto auto_finish( F&& callback, Options&&... options ) ->
#ifdef __cpp_concepts
    AutoFinishWith<prefab::BasicBar<Config, Sink, Mode, Zone>, typename std::decay<F>::type>
    requires details::traits::is_config<Config>::value
          && details::traits::is_routine<typename std::decay<F>::type,
                                         prefab::BasicBar<Config, Sink, Mode, Zone>>::value
#else
    typename std::enable_if<
      details::traits::AllOf<details::traits::is_config<Config>,
                             details::traits::is_routine<typename std::decay<F>::type,
                                                         prefab::BasicBar<Config, Sink, Mode, Zone>>>::value,
      AutoFinishWith<prefab::BasicBar<Config, Sink, Mode, Zone>, typename std::decay<F>::type>>::type
#endif
  { return { std::forward<F>( callback ), std::forward<Options>( options )... }; }

  //////////////////////////////////////////////////

  template<typename Config,
           Channel Sink = Channel::Err,
           Policy Mode  = Policy::Async,
           Region Zone  = Region::Fixed,
           typename... Options>
  PACE__NODISCARD auto auto_retire( Options&&... options ) ->
#ifdef __cpp_concepts
    AutoRetire<prefab::BasicBar<Config, Sink, Mode, Zone>>
    requires details::traits::is_config<Config>::value && std::is_constructible_v<Config, Options...>
#else
    typename std::enable_if<details::traits::AllOf<details::traits::is_config<Config>,
                                                   std::is_constructible<Config, Options...>>::value,
                            AutoRetire<prefab::BasicBar<Config, Sink, Mode, Zone>>>::type
#endif
  { return { std::forward<Options>( options )... }; }

#ifdef __cpp_deduction_guides
  template<typename Soul, Channel Sink, Policy Mode, Region Zone>
  AutoFinish( prefab::BasicBar<Soul, Sink, Mode, Zone>&& )
    -> AutoFinish<prefab::BasicBar<Soul, Sink, Mode, Zone>>;

  template<typename Soul, Channel Sink, Policy Mode, Region Zone, typename F>
  AutoFinishWith( prefab::BasicBar<Soul, Sink, Mode, Zone>&&, F&& )
    -> AutoFinishWith<prefab::BasicBar<Soul, Sink, Mode, Zone>, std::decay_t<F>>;

  template<typename Soul, Channel Sink, Policy Mode, Region Zone>
  AutoRetire( prefab::BasicBar<Soul, Sink, Mode, Zone>&& )
    -> AutoRetire<prefab::BasicBar<Soul, Sink, Mode, Zone>>;
#endif

  /**
   * A `Config`-based overload of `iterate` would normally be expected here,
   * but we intentionally omit one for the following reasons:

   * `iterate` does not express the *construction* of a progress bar.
   * Instead, it uses the contents of the argument list to create a side-effect object
   * (i.e. a progress bar object) for visualizing range traversal,
   * and then immediately uses it.

   * In principle, unless its returned `TrackedSpan` is explicitly stored,
   * the generated side-effect object essentially satisfies the affine property
   * (i.e. it is used at most once).

   * Furthermore, since `BasicBar`'s constructor supports forwarding constructor arguments
   * of `Config` types, this does not affect the resulting user experience.

   * Additionally, providing such overloads would form a Cartesian product
   * between different completion strategies and different range specifications,
   * leading to combinatorial explosion.
   * This would significantly increase the burden on overload resolution
   * and consequently lengthen compile times, which is not worth the cost.

   * Moreover, the parameter list required by the `Config`-based version would simply be too long
   * compared to the concise `Bar`-based version.
   */
} // namespace pace

#endif
