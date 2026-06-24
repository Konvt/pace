#ifndef PACE_BASIC_BAR
#define PACE_BASIC_BAR

#include "../Indicator.hpp"
#include "../details/aspects/Entailment.hpp"
#include "../details/behaviors/Incremental.hpp"
#include "../details/behaviors/Reactive.hpp"
#include "BasicConfig.hpp"

namespace pace {
  namespace prefab {
    template<typename Soul,
             Channel Sink = Channel::Stderr,
             Policy Mode  = Policy::Async,
             Region Zone  = Region::Fixed>
    class BasicBar
      : public details::traits::linearize<details::aspects::link_entailments_t<Soul>>::
          template type<Indicator, BasicBar<Soul, Sink, Mode, Zone>> {
      using Base = typename details::traits::linearize<details::aspects::link_entailments_t<Soul>>::
        template type<Indicator, BasicBar<Soul, Sink, Mode, Zone>>;

    public:
      using Config                  = Soul;
      static constexpr Channel sink = Sink;
      static constexpr Policy mode  = Mode;
      static constexpr Region zone  = Zone;

      using Base::Base;
      constexpr BasicBar() = default;
      template<typename... Args
#ifdef __cpp_concepts
               >
        requires( !( std::is_same_v<std::decay_t<Args>, Soul> || ... )
                  && std::is_constructible_v<Soul, Args && ...> )
#else
               ,
               typename = typename std::enable_if<details::traits::all_of<
                 details::traits::neg<std::is_same<typename std::decay<Args>::type, Soul>>...,
                 std::is_constructible<Soul, Args&&...>>::value>::type>
#endif
      constexpr BasicBar( Args&&... args ) noexcept( std::is_nothrow_constructible<Base, Args&&...>::value )
        : Base( Soul( std::forward<Args>( args )... ) )
      {}

      BasicBar( const BasicBar& )            = delete;
      BasicBar& operator=( const BasicBar& ) = delete;

      constexpr BasicBar( BasicBar&& )                            = default;
      PACE__CXX14_CNSTXPR BasicBar& operator=( BasicBar&& rhs ) & = default;
      PACE__CXX20_CNSTXPR virtual ~BasicBar()                     = default;

      void swap( BasicBar& other ) noexcept
      {
        PACE__TRUST( this != &other );
        PACE__ASSERT( this->active() == false );
        PACE__ASSERT( other.active() == false );
        Base::swap( other );
      }
      friend void swap( BasicBar& a, BasicBar& b ) noexcept { a.swap( b ); }
    };
  } // namespace prefab

  namespace details {
    namespace traits {
      template<typename B>
      struct _impl_is_bar {
      private:
        template<typename S, Channel O, Policy M, Region A>
        static constexpr std::true_type check( const prefab::BasicBar<S, O, M, A>& );
        static constexpr std::false_type check( ... );

      public:
        using result = all_of<neg<std::is_reference<B>>,
                              decltype( check( std::declval<typename std::remove_cv<B>::type>() ) )>;
      };
      template<typename B>
      using is_bar = typename _impl_is_bar<B>::result;
      template<typename B>
      using is_iterable_bar = all_of<is_bar<B>, is_instance_of<B, behaviors::Incremental>>;
      template<typename B>
      using is_reactive_bar = all_of<is_bar<B>, is_instance_of<B, behaviors::Reactive>>;
    } // namespace traits
  } // namespace details

  template<typename Bar, typename N, typename... Options>
  PACE__NODISCARD PACE__FORCEINLINE auto iterate( N startpoint, N endpoint, N step, Options&&... options ) ->
#ifdef __cpp_concepts
    slice::TrackedSpan<slice::NumericSpan<N>, std::shared_ptr<Bar>>
    requires( std::is_arithmetic_v<N> && details::traits::is_iterable_bar<Bar>::value
              && std::is_constructible_v<Bar, Options && ...> )
#else
    typename std::enable_if<details::traits::all_of<std::is_arithmetic<N>,
                                                    details::traits::is_iterable_bar<Bar>,
                                                    std::is_constructible<Bar, Options&&...>>::value,
                            slice::TrackedSpan<slice::NumericSpan<N>, std::shared_ptr<Bar>>>::type
#endif
  {
    return {
      { startpoint, endpoint, step },
      std::make_shared<Bar>( std::forward<Options>( options )... )
    };
  }
  template<typename Bar, typename N, typename Act, typename... Options>
  PACE__NODISCARD PACE__FORCEINLINE auto iterate( N startpoint,
                                                  N endpoint,
                                                  N step,
                                                  Act&& act,
                                                  Options&&... options ) ->
#ifdef __cpp_concepts
    slice::TrackedSpan<slice::NumericSpan<N>, std::shared_ptr<Bar>>
    requires(
      std::is_arithmetic_v<N> && details::traits::is_iterable_bar<Bar>::value
      && details::traits::is_reactive_bar<Bar>::value
      && std::is_same_v<std::remove_reference_t<decltype( std::declval<Bar>() | std::forward<Act>( act ) )>,
                        Bar>
      && std::is_constructible_v<Bar, Options && ...> )
#else
    typename std::enable_if<
      details::traits::all_of<
        std::is_arithmetic<N>,
        details::traits::is_iterable_bar<Bar>,
        details::traits::is_reactive_bar<Bar>,
        std::is_same<
          typename std::remove_reference<decltype( std::declval<Bar>() | std::forward<Act>( act ) )>::type,
          Bar>,
        std::is_constructible<Bar, Options&&...>>::value,
      slice::TrackedSpan<slice::NumericSpan<N>, std::shared_ptr<Bar>>>::type
#endif
  {
    return {
      { startpoint, endpoint, step },
      std::make_shared<Bar>( Bar( std::forward<Options>( options )... ) | std::forward<Act>( act ) )
    };
  }
  template<typename Bar, typename N, typename Proc, typename... Options>
  PACE__FORCEINLINE auto iterate( N startpoint, N endpoint, N step, Proc&& op, Options&&... options )
#ifdef __cpp_concepts
    requires( std::is_arithmetic_v<N> && details::traits::is_iterable_bar<Bar>::value
              && std::is_constructible_v<Bar, Options && ...> && requires( N ele, Proc&& op ) { op( ele ); } )
#else
    -> typename std::enable_if<details::traits::all_of<
      std::is_arithmetic<N>,
      details::traits::is_iterable_bar<Bar>,
      std::is_constructible<Bar, Options&&...>,
      std::is_void<decltype( std::declval<Proc&&>()( std::declval<N>() ), void() )>>::value>::type
#endif
  {
    Bar( std::forward<Options>( options )... )
      .iterate( startpoint, endpoint, step, std::forward<Proc>( op ) );
  }
  template<typename Bar, typename N, typename Proc, typename Act, typename... Options>
  PACE__FORCEINLINE auto iterate( N startpoint,
                                  N endpoint,
                                  N step,
                                  Proc&& op,
                                  Act&& act,
                                  Options&&... options )
#ifdef __cpp_concepts
    requires(
      std::is_arithmetic_v<N> && details::traits::is_iterable_bar<Bar>::value
      && details::traits::is_reactive_bar<Bar>::value
      && std::is_same_v<std::remove_reference_t<decltype( std::declval<Bar>() | std::forward<Act>( act ) )>,
                        Bar>
      && std::is_constructible_v<Bar, Options && ...> )
#else
    -> typename std::enable_if<details::traits::all_of<
      std::is_arithmetic<N>,
      details::traits::is_iterable_bar<Bar>,
      details::traits::is_reactive_bar<Bar>,
      std::is_same<
        typename std::remove_reference<decltype( std::declval<Bar>() | std::forward<Act>( act ) )>::type,
        Bar>,
      std::is_constructible<Bar, Options&&...>>::value>::type
#endif
  {
    ( Bar( std::forward<Options>( options )... ) | std::forward<Act>( act ) )
      .iterate( startpoint, endpoint, step, std::forward<Proc>( op ) );
  }
  template<typename Config,
           Channel Sink = Channel::Stderr,
           Policy Mode  = Policy::Async,
           Region Zone  = Region::Fixed,
           typename N,
           typename... Options
#ifdef __cpp_concepts
           >
    requires( std::is_arithmetic_v<N> && details::traits::is_config<Config>::value
              && details::traits::is_iterable_bar<prefab::BasicBar<Config, Sink, Mode, Zone>>::value
              && std::is_constructible_v<Config, Options && ...> )
#else
           ,
           typename std::enable_if<
             details::traits::all_of<std::is_arithmetic<N>, details::traits::is_config<Config>>::value,
             bool>::type = 0>

#endif
  PACE__NODISCARD PACE__FORCEINLINE auto iterate( N startpoint, N endpoint, N step, Options&&... options )
#ifndef __cpp_concepts
    -> typename std::enable_if<
      details::traits::all_of<details::traits::is_iterable_bar<prefab::BasicBar<Config, Sink, Mode, Zone>>,
                              std::is_constructible<Config, Options&&...>>::value,
      slice::TrackedSpan<slice::NumericSpan<N>,
                         std::shared_ptr<prefab::BasicBar<Config, Sink, Mode, Zone>>>>::type
#endif
  {
    return iterate<prefab::BasicBar<Config, Sink, Mode, Zone>>( startpoint,
                                                                endpoint,
                                                                step,
                                                                std::forward<Options>( options )... );
  }
  template<typename Config,
           Channel Sink = Channel::Stderr,
           Policy Mode  = Policy::Async,
           Region Zone  = Region::Fixed,
           typename N,
           typename Act,
           typename... Options
#ifdef __cpp_concepts
           >
    requires( std::is_arithmetic_v<N> && details::traits::is_config<Config>::value
              && details::traits::is_iterable_bar<prefab::BasicBar<Config, Sink, Mode, Zone>>::value
              && details::traits::is_reactive_bar<prefab::BasicBar<Config, Sink, Mode, Zone>>::value
              && std::is_same_v<
                std::remove_reference_t<decltype( std::declval<prefab::BasicBar<Config, Sink, Mode, Zone>>()
                                                  | std::declval<Act &&>() )>,
                prefab::BasicBar<Config, Sink, Mode, Zone>>
              && std::is_constructible_v<Config, Options && ...> )
#else
           ,
           typename std::enable_if<
             details::traits::all_of<std::is_arithmetic<N>, details::traits::is_config<Config>>::value,
             bool>::type = 0>
#endif
  PACE__NODISCARD PACE__FORCEINLINE auto iterate( N startpoint,
                                                  N endpoint,
                                                  N step,
                                                  Act&& act,
                                                  Options&&... options )
#ifndef __cpp_concepts
    -> typename std::enable_if<
      details::traits::all_of<
        details::traits::is_iterable_bar<prefab::BasicBar<Config, Sink, Mode, Zone>>,
        details::traits::is_reactive_bar<prefab::BasicBar<Config, Sink, Mode, Zone>>,
        std::is_same<
          typename std::remove_reference<decltype( std::declval<prefab::BasicBar<Config, Sink, Mode, Zone>>()
                                                   | std::declval<Act&&>() )>::type,
          prefab::BasicBar<Config, Sink, Mode, Zone>>,
        std::is_constructible<Config, Options&&...>>::value,
      slice::TrackedSpan<slice::NumericSpan<N>,
                         std::shared_ptr<prefab::BasicBar<Config, Sink, Mode, Zone>>>>::type
#endif
  {
    return iterate<prefab::BasicBar<Config, Sink, Mode, Zone>>( startpoint,
                                                                endpoint,
                                                                step,
                                                                std::forward<Act>( act ),
                                                                std::forward<Options>( options )... );
  }
  template<typename Config,
           Channel Sink = Channel::Stderr,
           Policy Mode  = Policy::Async,
           Region Zone  = Region::Fixed,
           typename N,
           typename Proc,
           typename... Options
#ifdef __cpp_concepts
           >
    requires( std::is_arithmetic_v<N> && details::traits::is_config<Config>::value
              && details::traits::is_iterable_bar<prefab::BasicBar<Config, Sink, Mode, Zone>>::value
              && std::is_constructible_v<Config, Options && ...>
              && requires( N ele, Proc&& op ) { op( ele ); } )
#else
           ,
           typename std::enable_if<
             details::traits::all_of<std::is_arithmetic<N>, details::traits::is_config<Config>>::value,
             bool>::type = 0>
#endif
  PACE__FORCEINLINE auto iterate( N startpoint, N endpoint, N step, Proc&& op, Options&&... options )
#ifndef __cpp_concepts
    -> typename std::enable_if<details::traits::all_of<
      details::traits::is_iterable_bar<prefab::BasicBar<Config, Sink, Mode, Zone>>,
      std::is_constructible<Config, Options&&...>,
      std::is_void<decltype( std::declval<Proc&&>( std::declval<N>() ), void() )>>::value>::type
#endif
  {
    iterate<prefab::BasicBar<Config, Sink, Mode, Zone>>( startpoint,
                                                         endpoint,
                                                         step,
                                                         std::forward<Proc>( op ),
                                                         std::forward<Options>( options )... );
  }
  template<typename Config,
           Channel Sink = Channel::Stderr,
           Policy Mode  = Policy::Async,
           Region Zone  = Region::Fixed,
           typename N,
           typename Proc,
           typename Act,
           typename... Options
#ifdef __cpp_concepts
           >
    requires( std::is_arithmetic_v<N> && details::traits::is_config<Config>::value
              && details::traits::is_iterable_bar<prefab::BasicBar<Config, Sink, Mode, Zone>>::value
              && details::traits::is_reactive_bar<prefab::BasicBar<Config, Sink, Mode, Zone>>::value
              && std::is_same_v<
                std::remove_reference_t<decltype( std::declval<prefab::BasicBar<Config, Sink, Mode, Zone>>()
                                                  | std::declval<Act &&>() )>,
                prefab::BasicBar<Config, Sink, Mode, Zone>>
              && std::is_constructible_v<Config, Options && ...> )
#else
           ,
           typename std::enable_if<
             details::traits::all_of<std::is_arithmetic<N>, details::traits::is_config<Config>>::value,
             bool>::type = 0>
#endif
  PACE__FORCEINLINE auto iterate( N startpoint,
                                  N endpoint,
                                  N step,
                                  Proc&& op,
                                  Act&& act,
                                  Options&&... options )
#ifndef __cpp_concepts
    -> typename std::enable_if<details::traits::all_of<
      details::traits::is_iterable_bar<prefab::BasicBar<Config, Sink, Mode, Zone>>,
      details::traits::is_reactive_bar<prefab::BasicBar<Config, Sink, Mode, Zone>>,
      std::is_same<
        typename std::remove_reference<decltype( std::declval<prefab::BasicBar<Config, Sink, Mode, Zone>>()
                                                 | std::declval<Act&&>() )>::type,
        prefab::BasicBar<Config, Sink, Mode, Zone>>,
      std::is_constructible<Config, Options&&...>>::value>::type
#endif
  {
    iterate<prefab::BasicBar<Config, Sink, Mode, Zone>>( startpoint,
                                                         endpoint,
                                                         step,
                                                         std::forward<Proc>( op ),
                                                         std::forward<Act>( act ),
                                                         std::forward<Options>( options )... );
  }

  //////////////////////////////////////////////////////////////////////////////////////////////////////////

  template<typename Bar, typename N, typename... Options>
  PACE__NODISCARD PACE__FORCEINLINE auto iterate( N endpoint, N step, Options&&... options ) ->
#ifdef __cpp_concepts
    slice::TrackedSpan<slice::NumericSpan<N>, std::shared_ptr<Bar>>
    requires( std::is_floating_point_v<N> && details::traits::is_iterable_bar<Bar>::value
              && std::is_constructible_v<Bar, Options && ...> )
#else
    typename std::enable_if<details::traits::all_of<std::is_floating_point<N>,
                                                    details::traits::is_iterable_bar<Bar>,
                                                    std::is_constructible<Bar, Options&&...>>::value,
                            slice::TrackedSpan<slice::NumericSpan<N>, std::shared_ptr<Bar>>>::type
#endif
  {
    return {
      { {}, endpoint, step },
      std::make_shared<Bar>( std::forward<Options>( options )... )
    };
  }
  template<typename Bar, typename N, typename Act, typename... Options>
  PACE__NODISCARD PACE__FORCEINLINE auto iterate( N endpoint, N step, Act&& act, Options&&... options ) ->
#ifdef __cpp_concepts
    slice::TrackedSpan<slice::NumericSpan<N>, std::shared_ptr<Bar>>
    requires(
      std::is_floating_point_v<N> && details::traits::is_iterable_bar<Bar>::value
      && details::traits::is_reactive_bar<Bar>::value
      && std::is_same_v<std::remove_reference_t<decltype( std::declval<Bar>() | std::forward<Act>( act ) )>,
                        Bar>
      && std::is_constructible_v<Bar, Options && ...> )
#else
    typename std::enable_if<
      details::traits::all_of<
        std::is_floating_point<N>,
        details::traits::is_iterable_bar<Bar>,
        details::traits::is_reactive_bar<Bar>,
        std::is_same<
          typename std::remove_reference<decltype( std::declval<Bar>() | std::forward<Act>( act ) )>::type,
          Bar>,
        std::is_constructible<Bar, Options&&...>>::value,
      slice::TrackedSpan<slice::NumericSpan<N>, std::shared_ptr<Bar>>>::type
#endif
  {
    return {
      { {}, endpoint, step },
      std::make_shared<Bar>( Bar( std::forward<Options>( options )... ) | std::forward<Act>( act ) )
    };
  }
  template<typename Bar, typename N, typename Proc, typename... Options>
  PACE__FORCEINLINE auto iterate( N endpoint, N step, Proc&& op, Options&&... options )
#ifdef __cpp_concepts
    requires( std::is_floating_point_v<N> && details::traits::is_iterable_bar<Bar>::value
              && std::is_constructible_v<Bar, Options && ...> && requires( N ele, Proc&& op ) { op( ele ); } )
#else
    -> typename std::enable_if<details::traits::all_of<
      std::is_floating_point<N>,
      details::traits::is_iterable_bar<Bar>,
      std::is_constructible<Bar, Options&&...>,
      std::is_void<decltype( std::declval<Proc&&>( std::declval<N>() ), void() )>>::value>::type
#endif
  { Bar( std::forward<Options>( options )... ).iterate( endpoint, step, std::forward<Proc>( op ) ); }
  template<typename Bar, typename N, typename Proc, typename Act, typename... Options>
  PACE__FORCEINLINE auto iterate( N endpoint, N step, Proc&& op, Act&& act, Options&&... options )
#ifdef __cpp_concepts
    requires(
      std::is_floating_point_v<N> && details::traits::is_iterable_bar<Bar>::value
      && details::traits::is_reactive_bar<Bar>::value
      && std::is_same_v<std::remove_reference_t<decltype( std::declval<Bar>() | std::forward<Act>( act ) )>,
                        Bar>
      && std::is_constructible_v<Bar, Options && ...> )
#else
    -> typename std::enable_if<details::traits::all_of<
      std::is_floating_point<N>,
      details::traits::is_iterable_bar<Bar>,
      details::traits::is_reactive_bar<Bar>,
      std::is_same<
        typename std::remove_reference<decltype( std::declval<Bar>() | std::forward<Act>( act ) )>::type,
        Bar>,
      std::is_constructible<Bar, Options&&...>>::value>::type
#endif
  {
    ( Bar( std::forward<Options>( options )... ) | std::forward<Act>( act ) )
      .iterate( endpoint, step, std::forward<Proc>( op ) );
  }
  template<typename Config,
           Channel Sink = Channel::Stderr,
           Policy Mode  = Policy::Async,
           Region Zone  = Region::Fixed,
           typename N,
           typename... Options
#ifdef __cpp_concepts
           >
    requires( std::is_floating_point_v<N> && details::traits::is_config<Config>::value
              && details::traits::is_iterable_bar<prefab::BasicBar<Config, Sink, Mode, Zone>>::value
              && std::is_constructible_v<Config, Options && ...> )
#else
           ,
           typename std::enable_if<
             details::traits::all_of<std::is_floating_point<N>, details::traits::is_config<Config>>::value,
             bool>::type = 0>
#endif
  PACE__NODISCARD PACE__FORCEINLINE auto iterate( N endpoint, N step, Options&&... options )
#ifndef __cpp_concepts
    -> typename std::enable_if<
      details::traits::all_of<std::is_floating_point<N>,
                              details::traits::is_config<Config>,
                              details::traits::is_iterable_bar<prefab::BasicBar<Config, Sink, Mode, Zone>>,
                              std::is_constructible<Config, Options&&...>>::value,
      slice::TrackedSpan<slice::NumericSpan<N>,
                         std::shared_ptr<prefab::BasicBar<Config, Sink, Mode, Zone>>>>::type
#endif
  {
    return iterate<prefab::BasicBar<Config, Sink, Mode, Zone>>( endpoint,
                                                                step,
                                                                std::forward<Options>( options )... );
  }
  template<typename Config,
           Channel Sink = Channel::Stderr,
           Policy Mode  = Policy::Async,
           Region Zone  = Region::Fixed,
           typename N,
           typename Act,
           typename... Options
#ifdef __cpp_concepts
           >
    requires( std::is_floating_point_v<N> && details::traits::is_config<Config>::value
              && details::traits::is_iterable_bar<prefab::BasicBar<Config, Sink, Mode, Zone>>::value
              && details::traits::is_reactive_bar<prefab::BasicBar<Config, Sink, Mode, Zone>>::value
              && std::is_same_v<
                std::remove_reference_t<decltype( std::declval<prefab::BasicBar<Config, Sink, Mode, Zone>>()
                                                  | std::declval<Act &&>() )>,
                prefab::BasicBar<Config, Sink, Mode, Zone>>
              && std::is_constructible_v<Config, Options && ...> )
#else
           ,
           typename std::enable_if<
             details::traits::all_of<std::is_floating_point<N>, details::traits::is_config<Config>>::value,
             bool>::type = 0>
#endif
  PACE__NODISCARD PACE__FORCEINLINE auto iterate( N endpoint, N step, Act&& act, Options&&... options )
#ifndef __cpp_concepts
    -> typename std::enable_if<
      details::traits::all_of<
        details::traits::is_iterable_bar<prefab::BasicBar<Config, Sink, Mode, Zone>>,
        details::traits::is_reactive_bar<prefab::BasicBar<Config, Sink, Mode, Zone>>,
        std::is_same<
          typename std::remove_reference<decltype( std::declval<prefab::BasicBar<Config, Sink, Mode, Zone>>()
                                                   | std::declval<Act&&>() )>::type,
          prefab::BasicBar<Config, Sink, Mode, Zone>>,
        std::is_constructible<Config, Options&&...>>::value,
      slice::TrackedSpan<slice::NumericSpan<N>,
                         std::shared_ptr<prefab::BasicBar<Config, Sink, Mode, Zone>>>>::type
#endif
  {
    return iterate<prefab::BasicBar<Config, Sink, Mode, Zone>>( endpoint,
                                                                step,
                                                                std::forward<Act>( act ),
                                                                std::forward<Options>( options )... );
  }
  template<typename Config,
           Channel Sink = Channel::Stderr,
           Policy Mode  = Policy::Async,
           Region Zone  = Region::Fixed,
           typename N,
           typename Proc,
           typename... Options
#ifdef __cpp_concepts
           >
    requires( std::is_floating_point_v<N> && details::traits::is_config<Config>::value
              && details::traits::is_iterable_bar<prefab::BasicBar<Config, Sink, Mode, Zone>>::value
              && std::is_constructible_v<Config, Options && ...>
              && requires( N ele, Proc&& op ) { op( ele ); } )
#else
           ,
           typename std::enable_if<
             details::traits::all_of<std::is_floating_point<N>, details::traits::is_config<Config>>::value,
             bool>::type = 0>
#endif
  PACE__FORCEINLINE auto iterate( N endpoint, N step, Proc&& op, Options&&... options )
#ifndef __cpp_concepts
    -> typename std::enable_if<details::traits::all_of<
      details::traits::is_iterable_bar<prefab::BasicBar<Config, Sink, Mode, Zone>>,
      std::is_constructible<Config, Options&&...>,
      std::is_void<decltype( std::declval<Proc&&>( std::declval<N>() ), void() )>>::value>::type
#endif
  {
    iterate<prefab::BasicBar<Config, Sink, Mode, Zone>>( endpoint,
                                                         step,
                                                         std::forward<Proc>( op ),
                                                         std::forward<Options>( options )... );
  }
  template<typename Config,
           Channel Sink = Channel::Stderr,
           Policy Mode  = Policy::Async,
           Region Zone  = Region::Fixed,
           typename N,
           typename Proc,
           typename Act,
           typename... Options
#ifdef __cpp_concepts
           >
    requires( std::is_floating_point_v<N> && details::traits::is_config<Config>::value
              && details::traits::is_iterable_bar<prefab::BasicBar<Config, Sink, Mode, Zone>>::value
              && details::traits::is_reactive_bar<prefab::BasicBar<Config, Sink, Mode, Zone>>::value
              && std::is_same_v<
                std::remove_reference_t<decltype( std::declval<prefab::BasicBar<Config, Sink, Mode, Zone>>()
                                                  | std::declval<Act &&>() )>,
                prefab::BasicBar<Config, Sink, Mode, Zone>>
              && std::is_constructible_v<Config, Options && ...> )
#else
           ,
           typename std::enable_if<
             details::traits::all_of<std::is_floating_point<N>, details::traits::is_config<Config>>::value,
             bool>::type = 0>
#endif
  PACE__FORCEINLINE auto iterate( N endpoint, N step, Proc&& op, Act&& act, Options&&... options )
#ifndef __cpp_concepts
    -> typename std::enable_if<details::traits::all_of<
      details::traits::is_iterable_bar<prefab::BasicBar<Config, Sink, Mode, Zone>>,
      details::traits::is_reactive_bar<prefab::BasicBar<Config, Sink, Mode, Zone>>,
      std::is_same<
        typename std::remove_reference<decltype( std::declval<prefab::BasicBar<Config, Sink, Mode, Zone>>()
                                                 | std::declval<Act&&>() )>::type,
        prefab::BasicBar<Config, Sink, Mode, Zone>>,
      std::is_constructible<Config, Options&&...>>::value>::type
#endif
  {
    iterate<prefab::BasicBar<Config, Sink, Mode, Zone>>( endpoint,
                                                         step,
                                                         std::forward<Proc>( op ),
                                                         std::forward<Act>( act ),
                                                         std::forward<Options>( options )... );
  }

  //////////////////////////////////////////////////////////////////////////////////////////////////////////

  template<typename Bar, typename N, typename... Options>
  PACE__NODISCARD PACE__FORCEINLINE auto iterate( N startpoint, N endpoint, Options&&... options ) ->
#ifdef __cpp_concepts
    slice::TrackedSpan<slice::NumericSpan<N>, std::shared_ptr<Bar>>
    requires( std::is_integral_v<N> && details::traits::is_iterable_bar<Bar>::value
              && std::is_constructible_v<Bar, Options && ...> )
#else
    typename std::enable_if<details::traits::all_of<std::is_integral<N>,
                                                    details::traits::is_iterable_bar<Bar>,
                                                    std::is_constructible<Bar, Options&&...>>::value,
                            slice::TrackedSpan<slice::NumericSpan<N>, std::shared_ptr<Bar>>>::type
#endif
  {
    return {
      { startpoint, endpoint },
      std::make_shared<Bar>( std::forward<Options>( options )... )
    };
  }
  template<typename Bar, typename N, typename Act, typename... Options>
  PACE__NODISCARD PACE__FORCEINLINE auto iterate( N startpoint, N endpoint, Act&& act, Options&&... options )
    ->
#ifdef __cpp_concepts
    slice::TrackedSpan<slice::NumericSpan<N>, std::shared_ptr<Bar>>
    requires(
      std::is_integral_v<N> && details::traits::is_iterable_bar<Bar>::value
      && details::traits::is_reactive_bar<Bar>::value
      && std::is_same_v<std::remove_reference_t<decltype( std::declval<Bar>() | std::forward<Act>( act ) )>,
                        Bar>
      && std::is_constructible_v<Bar, Options && ...> )
#else
    typename std::enable_if<
      details::traits::all_of<
        std::is_integral<N>,
        details::traits::is_iterable_bar<Bar>,
        std::is_same<
          typename std::remove_reference<decltype( std::declval<Bar>() | std::forward<Act>( act ) )>::type,
          Bar>,
        std::is_constructible<Bar, Options&&...>>::value,
      slice::TrackedSpan<slice::NumericSpan<N>, std::shared_ptr<Bar>>>::type
#endif
  {
    return {
      { startpoint, endpoint },
      std::make_shared<Bar>( Bar( std::forward<Options>( options )... ) | std::forward<Act>( act ) )
    };
  }
  template<typename Bar, typename N, typename Proc, typename... Options>
  PACE__FORCEINLINE auto iterate( N startpoint, N endpoint, Proc&& op, Options&&... options )
#ifdef __cpp_concepts
    requires( std::is_integral_v<N> && details::traits::is_iterable_bar<Bar>::value
              && std::is_constructible_v<Bar, Options && ...> && requires( N ele, Proc&& op ) { op( ele ); } )
#else
    -> typename std::enable_if<details::traits::all_of<
      std::is_integral<N>,
      details::traits::is_iterable_bar<Bar>,
      std::is_constructible<Bar, Options&&...>,
      std::is_void<decltype( std::declval<Proc&&>( std::declval<N>() ), void() )>>::value>::type
#endif
  { Bar( std::forward<Options>( options )... ).iterate( startpoint, endpoint, std::forward<Proc>( op ) ); }
  template<typename Bar, typename N, typename Proc, typename Act, typename... Options>
  PACE__FORCEINLINE auto iterate( N startpoint, N endpoint, Proc&& op, Act&& act, Options&&... options )
#ifdef __cpp_concepts
    requires(
      std::is_integral_v<N> && details::traits::is_iterable_bar<Bar>::value
      && details::traits::is_reactive_bar<Bar>::value
      && std::is_same_v<std::remove_reference_t<decltype( std::declval<Bar>() | std::forward<Act>( act ) )>,
                        Bar>
      && std::is_constructible_v<Bar, Options && ...> )
#else
    -> typename std::enable_if<details::traits::all_of<
      std::is_integral<N>,
      details::traits::is_iterable_bar<Bar>,
      std::is_same<
        typename std::remove_reference<decltype( std::declval<Bar>() | std::forward<Act>( act ) )>::type,
        Bar>,
      std::is_constructible<Bar, Options&&...>>::value>::type
#endif
  {
    ( Bar( std::forward<Options>( options )... ) | std::forward<Act>( act ) )
      .iterate( startpoint, endpoint, std::forward<Proc>( op ) );
  }
  template<typename Config,
           Channel Sink = Channel::Stderr,
           Policy Mode  = Policy::Async,
           Region Zone  = Region::Fixed,
           typename N,
           typename... Options
#ifdef __cpp_concepts
           >
    requires( std::is_integral_v<N> && details::traits::is_config<Config>::value
              && details::traits::is_iterable_bar<prefab::BasicBar<Config, Sink, Mode, Zone>>::value
              && std::is_constructible_v<Config, Options && ...> )
#else
           ,
           typename std::enable_if<
             details::traits::all_of<std::is_integral<N>, details::traits::is_config<Config>>::value,
             bool>::type = 0>
#endif
  PACE__NODISCARD PACE__FORCEINLINE auto iterate( N startpoint, N endpoint, Options&&... options )
#ifndef __cpp_concepts
    -> typename std::enable_if<
      details::traits::all_of<details::traits::is_iterable_bar<prefab::BasicBar<Config, Sink, Mode, Zone>>,
                              std::is_constructible<Config, Options&&...>>::value,
      slice::TrackedSpan<slice::NumericSpan<N>,
                         std::shared_ptr<prefab::BasicBar<Config, Sink, Mode, Zone>>>>::type
#endif
  {
    return iterate<prefab::BasicBar<Config, Sink, Mode, Zone>>( startpoint,
                                                                endpoint,
                                                                std::forward<Options>( options )... );
  }
  template<typename Config,
           Channel Sink = Channel::Stderr,
           Policy Mode  = Policy::Async,
           Region Zone  = Region::Fixed,
           typename N,
           typename Act,
           typename... Options
#ifdef __cpp_concepts
           >
    requires( std::is_integral_v<N> && details::traits::is_config<Config>::value
              && details::traits::is_iterable_bar<prefab::BasicBar<Config, Sink, Mode, Zone>>::value
              && details::traits::is_reactive_bar<prefab::BasicBar<Config, Sink, Mode, Zone>>::value
              && std::is_same_v<
                std::remove_reference_t<decltype( std::declval<prefab::BasicBar<Config, Sink, Mode, Zone>>()
                                                  | std::declval<Act &&>() )>,
                prefab::BasicBar<Config, Sink, Mode, Zone>>
              && std::is_constructible_v<Config, Options && ...> )
#else
           ,
           typename std::enable_if<
             details::traits::all_of<std::is_integral<N>, details::traits::is_config<Config>>::value,
             bool>::type = 0>
#endif
  PACE__NODISCARD PACE__FORCEINLINE auto iterate( N startpoint, N endpoint, Act&& act, Options&&... options )
#ifndef __cpp_concepts
    -> typename std::enable_if<
      details::traits::all_of<
        details::traits::is_iterable_bar<prefab::BasicBar<Config, Sink, Mode, Zone>>,
        details::traits::is_reactive_bar<prefab::BasicBar<Config, Sink, Mode, Zone>>,
        std::is_same<
          typename std::remove_reference<decltype( std::declval<prefab::BasicBar<Config, Sink, Mode, Zone>>()
                                                   | std::declval<Act&&>() )>::type,
          prefab::BasicBar<Config, Sink, Mode, Zone>>,
        std::is_constructible<Config, Options&&...>>::value,
      slice::TrackedSpan<slice::NumericSpan<N>,
                         std::shared_ptr<prefab::BasicBar<Config, Sink, Mode, Zone>>>>::type
#endif
  {
    return iterate<prefab::BasicBar<Config, Sink, Mode, Zone>>( startpoint,
                                                                endpoint,
                                                                std::forward<Act>( act ),
                                                                std::forward<Options>( options )... );
  }
  template<typename Config,
           Channel Sink = Channel::Stderr,
           Policy Mode  = Policy::Async,
           Region Zone  = Region::Fixed,
           typename N,
           typename Proc,
           typename... Options
#ifdef __cpp_concepts
           >
    requires( std::is_integral_v<N> && details::traits::is_config<Config>::value
              && details::traits::is_iterable_bar<prefab::BasicBar<Config, Sink, Mode, Zone>>::value
              && std::is_constructible_v<Config, Options && ...>
              && requires( N ele, Proc&& op ) { op( ele ); } )
#else
           ,
           typename std::enable_if<
             details::traits::all_of<std::is_integral<N>, details::traits::is_config<Config>>::value,
             bool>::type = 0>
#endif
  PACE__FORCEINLINE auto iterate( N startpoint, N endpoint, Proc&& op, Options&&... options )
#ifndef __cpp_concepts
    -> typename std::enable_if<details::traits::all_of<
      details::traits::is_iterable_bar<prefab::BasicBar<Config, Sink, Mode, Zone>>,
      std::is_constructible<Config, Options&&...>,
      std::is_void<decltype( std::declval<Proc&&>( std::declval<N>() ), void() )>>::value>::type
#endif
  {
    iterate<prefab::BasicBar<Config, Sink, Mode, Zone>>( startpoint,
                                                         endpoint,
                                                         std::forward<Proc>( op ),
                                                         std::forward<Options>( options )... );
  }
  template<typename Config,
           Channel Sink = Channel::Stderr,
           Policy Mode  = Policy::Async,
           Region Zone  = Region::Fixed,
           typename N,
           typename Proc,
           typename Act,
           typename... Options
#ifdef __cpp_concepts
           >
    requires( std::is_integral_v<N> && details::traits::is_config<Config>::value
              && details::traits::is_iterable_bar<prefab::BasicBar<Config, Sink, Mode, Zone>>::value
              && details::traits::is_reactive_bar<prefab::BasicBar<Config, Sink, Mode, Zone>>::value
              && std::is_same_v<
                std::remove_reference_t<decltype( std::declval<prefab::BasicBar<Config, Sink, Mode, Zone>>()
                                                  | std::declval<Act &&>() )>,
                prefab::BasicBar<Config, Sink, Mode, Zone>>
              && std::is_constructible_v<Config, Options && ...> )
#else
           ,
           typename std::enable_if<
             details::traits::all_of<std::is_integral<N>, details::traits::is_config<Config>>::value,
             bool>::type = 0>
#endif
  PACE__FORCEINLINE auto iterate( N startpoint, N endpoint, Proc&& op, Act&& act, Options&&... options )
#ifndef __cpp_concepts
    -> typename std::enable_if<details::traits::all_of<
      details::traits::is_iterable_bar<prefab::BasicBar<Config, Sink, Mode, Zone>>,
      details::traits::is_reactive_bar<prefab::BasicBar<Config, Sink, Mode, Zone>>,
      std::is_same<
        typename std::remove_reference<decltype( std::declval<prefab::BasicBar<Config, Sink, Mode, Zone>>()
                                                 | std::declval<Act&&>() )>::type,
        prefab::BasicBar<Config, Sink, Mode, Zone>>,
      std::is_constructible<Config, Options&&...>>::value>::type
#endif
  {
    iterate<prefab::BasicBar<Config, Sink, Mode, Zone>>( startpoint,
                                                         endpoint,
                                                         std::forward<Proc>( op ),
                                                         std::forward<Act>( act ),
                                                         std::forward<Options>( options )... );
  }

  //////////////////////////////////////////////////////////////////////////////////////////////////////////

  template<typename Bar, typename N, typename... Options>
  PACE__NODISCARD PACE__FORCEINLINE auto iterate( N endpoint, Options&&... options ) ->
#ifdef __cpp_concepts
    slice::TrackedSpan<slice::NumericSpan<N>, std::shared_ptr<Bar>>
    requires( std::is_integral_v<N> && details::traits::is_iterable_bar<Bar>::value
              && std::is_constructible_v<Bar, Options && ...> )
#else
    typename std::enable_if<details::traits::all_of<std::is_integral<N>,
                                                    details::traits::is_iterable_bar<Bar>,
                                                    std::is_constructible<Bar, Options&&...>>::value,
                            slice::TrackedSpan<slice::NumericSpan<N>, std::shared_ptr<Bar>>>::type
#endif
  { return { { endpoint }, std::make_shared<Bar>( std::forward<Options>( options )... ) }; }
  template<typename Bar, typename N, typename Act, typename... Options>
  PACE__NODISCARD PACE__FORCEINLINE auto iterate( N endpoint, Act&& act, Options&&... options ) ->
#ifdef __cpp_concepts
    slice::TrackedSpan<slice::NumericSpan<N>, std::shared_ptr<Bar>>
    requires(
      std::is_integral_v<N> && details::traits::is_iterable_bar<Bar>::value
      && details::traits::is_reactive_bar<Bar>::value
      && std::is_same_v<std::remove_reference_t<decltype( std::declval<Bar>() | std::forward<Act>( act ) )>,
                        Bar>
      && std::is_constructible_v<Bar, Options && ...> )
#else
    typename std::enable_if<
      details::traits::all_of<
        std::is_integral<N>,
        details::traits::is_iterable_bar<Bar>,
        details::traits::is_reactive_bar<Bar>,
        std::is_same<
          typename std::remove_reference<decltype( std::declval<Bar>() | std::forward<Act>( act ) )>::type,
          Bar>,
        std::is_constructible<Bar, Options&&...>>::value,
      slice::TrackedSpan<slice::NumericSpan<N>, std::shared_ptr<Bar>>>::type
#endif
  {
    return { { endpoint },
             std::make_shared<Bar>( Bar( std::forward<Options>( options )... ) | std::forward<Act>( act ) ) };
  }
  template<typename Bar, typename N, typename Proc, typename... Options>
  PACE__FORCEINLINE auto iterate( N endpoint, Proc&& op, Options&&... options )
#ifdef __cpp_concepts
    requires( std::is_integral_v<N> && details::traits::is_iterable_bar<Bar>::value
              && std::is_constructible_v<Bar, Options && ...> && requires( N ele, Proc&& op ) { op( ele ); } )
#else
    -> typename std::enable_if<details::traits::all_of<
      std::is_integral<N>,
      details::traits::is_iterable_bar<Bar>,
      std::is_constructible<Bar, Options&&...>,
      std::is_void<decltype( std::declval<Proc&&>( std::declval<N>() ), void() )>>::value>::type
#endif
  { Bar( std::forward<Options>( options )... ).iterate( endpoint, std::forward<Proc>( op ) ); }
  template<typename Bar, typename N, typename Proc, typename Act, typename... Options>
  PACE__FORCEINLINE auto iterate( N endpoint, Proc&& op, Act&& act, Options&&... options )
#ifdef __cpp_concepts
    requires(
      std::is_integral_v<N> && details::traits::is_iterable_bar<Bar>::value
      && details::traits::is_reactive_bar<Bar>::value
      && std::is_same_v<std::remove_reference_t<decltype( std::declval<Bar>() | std::forward<Act>( act ) )>,
                        Bar>
      && std::is_constructible_v<Bar, Options && ...> )
#else
    -> typename std::enable_if<details::traits::all_of<
      std::is_integral<N>,
      details::traits::is_iterable_bar<Bar>,
      details::traits::is_reactive_bar<Bar>,
      std::is_same<
        typename std::remove_reference<decltype( std::declval<Bar>() | std::forward<Act>( act ) )>::type,
        Bar>,
      std::is_constructible<Bar, Options&&...>>::value>::type
#endif
  {
    ( Bar( std::forward<Options>( options )... ) | std::forward<Act>( act ) )
      .iterate( endpoint, std::forward<Proc>( op ) );
  }
  template<typename Config,
           Channel Sink = Channel::Stderr,
           Policy Mode  = Policy::Async,
           Region Zone  = Region::Fixed,
           typename N,
           typename... Options
#ifdef __cpp_concepts
           >
    requires( std::is_integral_v<N> && details::traits::is_config<Config>::value
              && details::traits::is_iterable_bar<prefab::BasicBar<Config, Sink, Mode, Zone>>::value
              && std::is_constructible_v<Config, Options && ...> )
#else
           ,
           typename std::enable_if<
             details::traits::all_of<std::is_integral<N>, details::traits::is_config<Config>>::value,
             bool>::type = 0>
#endif
  PACE__NODISCARD PACE__FORCEINLINE auto iterate( N endpoint, Options&&... options )
#ifndef __cpp_concepts
    -> typename std::enable_if<
      details::traits::all_of<details::traits::is_iterable_bar<prefab::BasicBar<Config, Sink, Mode, Zone>>,
                              std::is_constructible<Config, Options&&...>>::value,
      slice::TrackedSpan<slice::NumericSpan<N>,
                         std::shared_ptr<prefab::BasicBar<Config, Sink, Mode, Zone>>>>::type
#endif
  {
    return iterate<prefab::BasicBar<Config, Sink, Mode, Zone>>( endpoint,
                                                                std::forward<Options>( options )... );
  }
  template<typename Config,
           Channel Sink = Channel::Stderr,
           Policy Mode  = Policy::Async,
           Region Zone  = Region::Fixed,
           typename N,
           typename Act,
           typename... Options
#ifdef __cpp_concepts
           >
    requires( std::is_integral_v<N> && details::traits::is_config<Config>::value
              && details::traits::is_iterable_bar<prefab::BasicBar<Config, Sink, Mode, Zone>>::value
              && details::traits::is_reactive_bar<prefab::BasicBar<Config, Sink, Mode, Zone>>::value
              && std::is_same_v<
                std::remove_reference_t<decltype( std::declval<prefab::BasicBar<Config, Sink, Mode, Zone>>()
                                                  | std::declval<Act &&>() )>,
                prefab::BasicBar<Config, Sink, Mode, Zone>>
              && std::is_constructible_v<Config, Options && ...> )
#else
           ,
           typename std::enable_if<
             details::traits::all_of<std::is_integral<N>, details::traits::is_config<Config>>::value,
             bool>::type = 0>
#endif
  PACE__NODISCARD PACE__FORCEINLINE auto iterate( N endpoint, Act&& act, Options&&... options )
#ifndef __cpp_concepts
    -> typename std::enable_if<
      details::traits::all_of<
        details::traits::is_iterable_bar<prefab::BasicBar<Config, Sink, Mode, Zone>>,
        details::traits::is_reactive_bar<prefab::BasicBar<Config, Sink, Mode, Zone>>,
        std::is_same<
          typename std::remove_reference<decltype( std::declval<prefab::BasicBar<Config, Sink, Mode, Zone>>()
                                                   | std::declval<Act&&>() )>::type,
          prefab::BasicBar<Config, Sink, Mode, Zone>>,
        std::is_constructible<Config, Options&&...>>::value,
      slice::TrackedSpan<slice::NumericSpan<N>,
                         std::shared_ptr<prefab::BasicBar<Config, Sink, Mode, Zone>>>>::type
#endif
  {
    return iterate<prefab::BasicBar<Config, Sink, Mode, Zone>>( endpoint,
                                                                std::forward<Act>( act ),
                                                                std::forward<Options>( options )... );
  }
  template<typename Config,
           Channel Sink = Channel::Stderr,
           Policy Mode  = Policy::Async,
           Region Zone  = Region::Fixed,
           typename N,
           typename Proc,
           typename... Options
#ifdef __cpp_concepts
           >
    requires( std::is_integral_v<N> && details::traits::is_config<Config>::value
              && details::traits::is_iterable_bar<prefab::BasicBar<Config, Sink, Mode, Zone>>::value
              && std::is_constructible_v<Config, Options && ...>
              && requires( N ele, Proc&& op ) { op( ele ); } )
#else
           ,
           typename std::enable_if<
             details::traits::all_of<std::is_integral<N>, details::traits::is_config<Config>>::value,
             bool>::type = 0>
#endif
  PACE__FORCEINLINE auto iterate( N endpoint, Proc&& op, Options&&... options )
#ifndef __cpp_concepts
    -> typename std::enable_if<details::traits::all_of<
      details::traits::is_iterable_bar<prefab::BasicBar<Config, Sink, Mode, Zone>>,
      std::is_constructible<Config, Options&&...>,
      std::is_void<decltype( std::declval<Proc&&>( std::declval<N>() ), void() )>>::value>::type
#endif
  {
    iterate<prefab::BasicBar<Config, Sink, Mode, Zone>>( endpoint,
                                                         std::forward<Proc>( op ),
                                                         std::forward<Options>( options )... );
  }
  template<typename Config,
           Channel Sink = Channel::Stderr,
           Policy Mode  = Policy::Async,
           Region Zone  = Region::Fixed,
           typename N,
           typename Proc,
           typename Act,
           typename... Options
#ifdef __cpp_concepts
           >
    requires( std::is_integral_v<N> && details::traits::is_config<Config>::value
              && details::traits::is_iterable_bar<prefab::BasicBar<Config, Sink, Mode, Zone>>::value
              && details::traits::is_reactive_bar<prefab::BasicBar<Config, Sink, Mode, Zone>>::value
              && std::is_same_v<
                std::remove_reference_t<decltype( std::declval<prefab::BasicBar<Config, Sink, Mode, Zone>>()
                                                  | std::declval<Act &&>() )>,
                prefab::BasicBar<Config, Sink, Mode, Zone>>
              && std::is_constructible_v<Config, Options && ...> )
#else
           ,
           typename std::enable_if<
             details::traits::all_of<std::is_integral<N>, details::traits::is_config<Config>>::value,
             bool>::type = 0>
#endif
  PACE__FORCEINLINE auto iterate( N endpoint, Proc&& op, Act&& act, Options&&... options )
#ifndef __cpp_concepts
    -> typename std::enable_if<details::traits::all_of<
      details::traits::is_iterable_bar<prefab::BasicBar<Config, Sink, Mode, Zone>>,
      details::traits::is_reactive_bar<prefab::BasicBar<Config, Sink, Mode, Zone>>,
      std::is_same<
        typename std::remove_reference<decltype( std::declval<prefab::BasicBar<Config, Sink, Mode, Zone>>()
                                                 | std::declval<Act&&>() )>::type,
        prefab::BasicBar<Config, Sink, Mode, Zone>>,
      std::is_constructible<Config, Options&&...>>::value>::type
#endif
  {
    iterate<prefab::BasicBar<Config, Sink, Mode, Zone>>( endpoint,
                                                         std::forward<Proc>( op ),
                                                         std::forward<Act>( act ),
                                                         std::forward<Options>( options )... );
  }

  //////////////////////////////////////////////////////////////////////////////////////////////////////////

  template<typename Bar, typename Itr, typename Snt, typename... Options>
  PACE__NODISCARD PACE__FORCEINLINE auto iterate( Itr startpoint, Snt endpoint, Options&&... options ) ->
#ifdef __cpp_concepts
    slice::TrackedSpan<slice::IteratorSpan<Itr, Snt>, std::shared_ptr<Bar>>
    requires( details::traits::is_sized_cursor<Itr, Snt>::value
              && details::traits::is_iterable_bar<Bar>::value
              && std::is_constructible_v<Bar, Options && ...> )
#else
    typename std::enable_if<details::traits::all_of<details::traits::is_sized_cursor<Itr, Snt>,
                                                    details::traits::is_iterable_bar<Bar>,
                                                    std::is_constructible<Bar, Options&&...>>::value,
                            slice::TrackedSpan<slice::IteratorSpan<Itr, Snt>, std::shared_ptr<Bar>>>::type
#endif
  {
    return {
      { std::move( startpoint ), std::move( endpoint ) },
      std::make_shared<Bar>( std::forward<Options>( options )... )
    };
  }
  template<typename Bar, typename Itr, typename Snt, typename Act, typename... Options>
  PACE__NODISCARD PACE__FORCEINLINE auto iterate( Itr startpoint,
                                                  Snt endpoint,
                                                  Act&& act,
                                                  Options&&... options ) ->
#ifdef __cpp_concepts
    slice::TrackedSpan<slice::IteratorSpan<Itr, Snt>, std::shared_ptr<Bar>>
    requires(
      details::traits::is_sized_cursor<Itr, Snt>::value && details::traits::is_iterable_bar<Bar>::value
      && details::traits::is_reactive_bar<Bar>::value
      && std::is_same_v<std::remove_reference_t<decltype( std::declval<Bar>() | std::forward<Act>( act ) )>,
                        Bar>
      && std::is_constructible_v<Bar, Options && ...> )
#else
    typename std::enable_if<
      details::traits::all_of<
        details::traits::is_sized_cursor<Itr, Snt>,
        details::traits::is_iterable_bar<Bar>,
        details::traits::is_reactive_bar<Bar>,
        std::is_same<
          typename std::remove_reference<decltype( std::declval<Bar>() | std::forward<Act>( act ) )>::type,
          Bar>,
        std::is_constructible<Bar, Options&&...>>::value,
      slice::TrackedSpan<slice::IteratorSpan<Itr, Snt>, std::shared_ptr<Bar>>>::type
#endif
  {
    return {
      { std::move( startpoint ), std::move( endpoint ) },
      std::make_shared<Bar>( Bar( std::forward<Options>( options )... ) | std::forward<Act>( act ) )
    };
  }
  template<typename Bar, typename Itr, typename Snt, typename Proc, typename... Options>
  PACE__FORCEINLINE auto iterate( Itr startpoint, Snt endpoint, Proc&& op, Options&&... options )
#ifdef __cpp_concepts
    requires( details::traits::is_sized_cursor<Itr, Snt>::value
              && details::traits::is_iterable_bar<Bar>::value && std::is_constructible_v<Bar, Options && ...>
              && requires( details::traits::iter_value_t<Itr> ele, Proc&& op ) { op( ele ); } )
#else
    -> typename std::enable_if<details::traits::all_of<
      details::traits::is_sized_cursor<Itr, Snt>,
      details::traits::is_iterable_bar<Bar>,
      std::is_constructible<Bar, Options&&...>,
      std::is_void<decltype( std::declval<Proc&&>( std::declval<details::traits::iter_value_t<Itr>>() ),
                             void() )>>::value>::type
#endif
  {
    Bar( std::forward<Options>( options )... )
      .iterate( std::move( startpoint ), std::move( endpoint ), std::forward<Proc>( op ) );
  }
  template<typename Bar, typename Itr, typename Snt, typename Proc, typename Act, typename... Options>
  PACE__FORCEINLINE auto iterate( Itr startpoint, Snt endpoint, Proc&& op, Act&& act, Options&&... options )
#ifdef __cpp_concepts
    requires(
      details::traits::is_sized_cursor<Itr, Snt>::value && details::traits::is_iterable_bar<Bar>::value
      && details::traits::is_reactive_bar<Bar>::value
      && std::is_same_v<std::remove_reference_t<decltype( std::declval<Bar>() | std::forward<Act>( act ) )>,
                        Bar>
      && std::is_constructible_v<Bar, Options && ...> )
#else
    -> typename std::enable_if<details::traits::all_of<
      details::traits::is_sized_cursor<Itr, Snt>,
      details::traits::is_iterable_bar<Bar>,
      details::traits::is_reactive_bar<Bar>,
      std::is_same<
        typename std::remove_reference<decltype( std::declval<Bar>() | std::forward<Act>( act ) )>::type,
        Bar>,
      std::is_constructible<Bar, Options&&...>>::value>::type
#endif
  {
    ( Bar( std::forward<Options>( options )... ) | std::forward<Act>( act ) )
      .iterate( std::move( startpoint ), std::move( endpoint ), std::forward<Proc>( op ) );
  }
  template<typename Config,
           Channel Sink = Channel::Stderr,
           Policy Mode  = Policy::Async,
           Region Zone  = Region::Fixed,
           typename Itr,
           typename Snt,
           typename... Options
#ifdef __cpp_concepts
           >
    requires( details::traits::is_sized_cursor<Itr, Snt>::value && details::traits::is_config<Config>::value
              && details::traits::is_iterable_bar<prefab::BasicBar<Config, Sink, Mode, Zone>>::value
              && std::is_constructible_v<Config, Options && ...> )
#else
           ,
           typename std::enable_if<details::traits::all_of<details::traits::is_sized_cursor<Itr, Snt>,
                                                           details::traits::is_config<Config>>::value,
                                   bool>::type = 0>
#endif
  PACE__NODISCARD PACE__FORCEINLINE auto iterate( Itr startpoint, Snt endpoint, Options&&... options )
#ifndef __cpp_concepts
    -> typename std::enable_if<
      details::traits::all_of<details::traits::is_iterable_bar<prefab::BasicBar<Config, Sink, Mode, Zone>>,
                              std::is_constructible<Config, Options&&...>>::value,
      slice::TrackedSpan<slice::IteratorSpan<Itr, Snt>,
                         std::shared_ptr<prefab::BasicBar<Config, Sink, Mode, Zone>>>>::type
#endif
  {
    return iterate<prefab::BasicBar<Config, Sink, Mode, Zone>>( std::move( startpoint ),
                                                                std::move( endpoint ),
                                                                std::forward<Options>( options )... );
  }
  template<typename Config,
           Channel Sink = Channel::Stderr,
           Policy Mode  = Policy::Async,
           Region Zone  = Region::Fixed,
           typename Itr,
           typename Snt,
           typename Act,
           typename... Options
#ifdef __cpp_concepts
           >
    requires( details::traits::is_sized_cursor<Itr, Snt>::value && details::traits::is_config<Config>::value
              && details::traits::is_iterable_bar<prefab::BasicBar<Config, Sink, Mode, Zone>>::value
              && details::traits::is_reactive_bar<prefab::BasicBar<Config, Sink, Mode, Zone>>::value
              && std::is_same_v<
                std::remove_reference_t<decltype( std::declval<prefab::BasicBar<Config, Sink, Mode, Zone>>()
                                                  | std::declval<Act &&>() )>,
                prefab::BasicBar<Config, Sink, Mode, Zone>>
              && std::is_constructible_v<Config, Options && ...> )
#else
           ,
           typename std::enable_if<details::traits::all_of<details::traits::is_sized_cursor<Itr, Snt>,
                                                           details::traits::is_config<Config>>::value,
                                   bool>::type = 0>
#endif
  PACE__NODISCARD PACE__FORCEINLINE auto iterate( Itr startpoint,
                                                  Snt endpoint,
                                                  Act&& act,
                                                  Options&&... options )
#ifndef __cpp_concepts
    -> typename std::enable_if<
      details::traits::all_of<
        details::traits::is_iterable_bar<prefab::BasicBar<Config, Sink, Mode, Zone>>,
        details::traits::is_reactive_bar<prefab::BasicBar<Config, Sink, Mode, Zone>>,
        std::is_same<
          typename std::remove_reference<decltype( std::declval<prefab::BasicBar<Config, Sink, Mode, Zone>>()
                                                   | std::declval<Act&&>() )>::type,
          prefab::BasicBar<Config, Sink, Mode, Zone>>,
        std::is_constructible<Config, Options&&...>>::value,
      slice::TrackedSpan<slice::IteratorSpan<Itr, Snt>,
                         std::shared_ptr<prefab::BasicBar<Config, Sink, Mode, Zone>>>>::type
#endif
  {
    return iterate<prefab::BasicBar<Config, Sink, Mode, Zone>>( std::move( startpoint ),
                                                                std::move( endpoint ),
                                                                std::forward<Act>( act ),
                                                                std::forward<Options>( options )... );
  }
  template<typename Config,
           Channel Sink = Channel::Stderr,
           Policy Mode  = Policy::Async,
           Region Zone  = Region::Fixed,
           typename Itr,
           typename Snt,
           typename Proc,
           typename... Options
#ifdef __cpp_concepts
           >
    requires( details::traits::is_sized_cursor<Itr, Snt>::value && details::traits::is_config<Config>::value
              && details::traits::is_iterable_bar<prefab::BasicBar<Config, Sink, Mode, Zone>>::value
              && std::is_constructible_v<Config, Options && ...>
              && requires( details::traits::iter_value_t<Itr> ele, Proc&& op ) { op( ele ); } )
#else
           ,
           typename std::enable_if<details::traits::all_of<details::traits::is_sized_cursor<Itr, Snt>,
                                                           details::traits::is_config<Config>>::value,
                                   bool>::type = 0>
#endif
  PACE__FORCEINLINE auto iterate( Itr startpoint, Snt endpoint, Proc&& op, Options&&... options )
#ifndef __cpp_concepts
    -> typename std::enable_if<details::traits::all_of<
      details::traits::is_iterable_bar<prefab::BasicBar<Config, Sink, Mode, Zone>>,
      std::is_constructible<Config, Options&&...>,
      std::is_void<decltype( std::declval<Proc&&>( std::declval<details::traits::iter_value_t<Itr>>() ),
                             void() )>>::value>::type
#endif
  {
    iterate<prefab::BasicBar<Config, Sink, Mode, Zone>>( std::move( startpoint ),
                                                         std::move( endpoint ),
                                                         std::forward<Proc>( op ),
                                                         std::forward<Options>( options )... );
  }
  template<typename Config,
           Channel Sink = Channel::Stderr,
           Policy Mode  = Policy::Async,
           Region Zone  = Region::Fixed,
           typename Itr,
           typename Snt,
           typename Proc,
           typename Act,
           typename... Options
#ifdef __cpp_concepts
           >
    requires( details::traits::is_sized_cursor<Itr, Snt>::value && details::traits::is_config<Config>::value
              && details::traits::is_iterable_bar<prefab::BasicBar<Config, Sink, Mode, Zone>>::value
              && details::traits::is_reactive_bar<prefab::BasicBar<Config, Sink, Mode, Zone>>::value
              && std::is_same_v<
                std::remove_reference_t<decltype( std::declval<prefab::BasicBar<Config, Sink, Mode, Zone>>()
                                                  | std::declval<Act &&>() )>,
                prefab::BasicBar<Config, Sink, Mode, Zone>>
              && std::is_constructible_v<Config, Options && ...> )
#else
           ,
           typename std::enable_if<details::traits::all_of<details::traits::is_sized_cursor<Itr, Snt>,
                                                           details::traits::is_config<Config>>::value,
                                   bool>::type = 0>
#endif
  PACE__FORCEINLINE auto iterate( Itr startpoint, Snt endpoint, Proc&& op, Act&& act, Options&&... options )
#ifndef __cpp_concepts
    -> typename std::enable_if<details::traits::all_of<
      details::traits::is_iterable_bar<prefab::BasicBar<Config, Sink, Mode, Zone>>,
      details::traits::is_reactive_bar<prefab::BasicBar<Config, Sink, Mode, Zone>>,
      std::is_same<
        typename std::remove_reference<decltype( std::declval<prefab::BasicBar<Config, Sink, Mode, Zone>>()
                                                 | std::declval<Act&&>() )>::type,
        prefab::BasicBar<Config, Sink, Mode, Zone>>,
      std::is_constructible<Config, Options&&...>>::value>::type
#endif
  {
    iterate<prefab::BasicBar<Config, Sink, Mode, Zone>>( std::move( startpoint ),
                                                         std::move( endpoint ),
                                                         std::forward<Proc>( op ),
                                                         std::forward<Act>( act ),
                                                         std::forward<Options>( options )... );
  }

  //////////////////////////////////////////////////////////////////////////////////////////////////////////

  template<typename Bar, class R, typename... Options>
  PACE__NODISCARD PACE__FORCEINLINE auto iterate( R&& range, Options&&... options ) ->
#ifdef __cpp_concepts
    slice::TrackedSpan<slice::SizedSpan<std::remove_reference_t<R>>, std::shared_ptr<Bar>>
    requires( details::traits::is_sized_range<R>::value && !std::ranges::view<std::remove_reference_t<R>>
              && details::traits::is_iterable_bar<Bar>::value
              && std::is_constructible_v<Bar, Options && ...> )
#else
    typename std::enable_if<details::traits::all_of<details::traits::is_sized_range<R>,
                                                    details::traits::is_iterable_bar<Bar>,
                                                    std::is_constructible<Bar, Options&&...>>::value,
                            slice::TrackedSpan<slice::SizedSpan<typename std::remove_reference<R>::type>,
                                               std::shared_ptr<Bar>>>::type
#endif
  { return { { std::forward<R>( range ) }, std::make_shared<Bar>( std::forward<Options>( options )... ) }; }
  template<typename Bar, class R, typename Act, typename... Options>
  PACE__NODISCARD PACE__FORCEINLINE auto iterate( R&& range, Act&& act, Options&&... options ) ->
#ifdef __cpp_concepts
    slice::TrackedSpan<slice::SizedSpan<std::remove_reference_t<R>>, std::shared_ptr<Bar>>
    requires(
      details::traits::is_sized_range<R>::value && !std::ranges::view<std::remove_reference_t<R>>
      && details::traits::is_iterable_bar<Bar>::value && details::traits::is_reactive_bar<Bar>::value
      && std::is_same_v<std::remove_reference_t<decltype( std::declval<Bar>() | std::forward<Act>( act ) )>,
                        Bar>
      && std::is_constructible_v<Bar, Options && ...> )
#else
    typename std::enable_if<
      details::traits::all_of<
        details::traits::is_sized_range<R>,
        details::traits::is_iterable_bar<Bar>,
        details::traits::is_reactive_bar<Bar>,
        std::is_same<
          typename std::remove_reference<decltype( std::declval<Bar>() | std::forward<Act>( act ) )>::type,
          Bar>,
        std::is_constructible<Bar, Options&&...>>::value,
      slice::TrackedSpan<slice::SizedSpan<typename std::remove_reference<R>::type>,
                         std::shared_ptr<Bar>>>::type
#endif
  {
    return { { std::forward<R>( range ) },
             std::make_shared<Bar>( Bar( std::forward<Options>( options )... ) | std::forward<Act>( act ) ) };
  }
#ifdef __cpp_concepts
  template<typename Bar, class R, typename... Options>
    requires( details::traits::is_sized_range<R>::value
              && std::ranges::view<R> && details::traits::is_iterable_bar<Bar>::value
              && std::is_constructible_v<Bar, Options && ...> )
  PACE__NODISCARD PACE__FORCEINLINE
    slice::TrackedSpan<R, std::shared_ptr<Bar>> iterate( R view, Options&&... options )
  { return { std::move( view ), std::make_shared<Bar>( std::forward<Options>( options )... ) }; }
  template<typename Bar, class R, typename Act, typename... Options>
  PACE__NODISCARD PACE__FORCEINLINE slice::TrackedSpan<R, std::shared_ptr<Bar>>
    iterate( R view, Act&& act, Options&&... options )
    requires(
      details::traits::is_sized_range<R>::value && std::ranges::view<R>
      && details::traits::is_iterable_bar<Bar>::value && details::traits::is_reactive_bar<Bar>::value
      && std::is_same_v<std::remove_reference_t<decltype( std::declval<Bar>() | std::forward<Act>( act ) )>,
                        Bar>
      && std::is_constructible_v<Bar, Options && ...> )
  {
    return { std::move( view ),
             std::make_shared<Bar>( Bar( std::forward<Options>( options )... ) | std::forward<Act>( act ) ) };
  }
#endif
  template<typename Bar, class R, typename Proc, typename... Options>
  PACE__FORCEINLINE auto iterate( R&& range, Proc&& op, Options&&... options )
#ifdef __cpp_concepts
    requires( details::traits::is_sized_range<R>::value && details::traits::is_iterable_bar<Bar>::value
              && std::is_constructible_v<Bar, Options && ...>
              && requires( details::traits::iter_value_t<details::traits::iterator_of_t<R>> ele, Proc&& op ) {
                   op( ele );
                 } )
#else
    -> typename std::enable_if<details::traits::all_of<
      details::traits::is_sized_range<R>,
      details::traits::is_iterable_bar<Bar>,
      std::is_constructible<Bar, Options&&...>,
      std::is_void<decltype( std::declval<Proc&&>( std::declval<details::traits::iter_value_t<
                                                     details::traits::iterator_of_t<R>>>() ),
                             void() )>>::value>::type
#endif
  {
    Bar( std::forward<Options>( options )... ).iterate( std::forward<R>( range ), std::forward<Proc>( op ) );
  }
  template<typename Bar, class R, typename Proc, typename Act, typename... Options>
  PACE__FORCEINLINE auto iterate( R&& range, Proc&& op, Act&& act, Options&&... options )
#ifdef __cpp_concepts
    requires(
      details::traits::is_sized_range<R>::value && details::traits::is_iterable_bar<Bar>::value
      && details::traits::is_reactive_bar<Bar>::value
      && std::is_same_v<std::remove_reference_t<decltype( std::declval<Bar>() | std::forward<Act>( act ) )>,
                        Bar>
      && std::is_constructible_v<Bar, Options && ...> )
#else
    -> typename std::enable_if<details::traits::all_of<
      details::traits::is_sized_range<R>,
      details::traits::is_iterable_bar<Bar>,
      details::traits::is_reactive_bar<Bar>,
      std::is_same<
        typename std::remove_reference<decltype( std::declval<Bar>() | std::forward<Act>( act ) )>::type,
        Bar>,
      std::is_constructible<Bar, Options&&...>>::value>::type
#endif
  {
    ( Bar( std::forward<Options>( options )... ) | std::forward<Act>( act ) )
      .iterate( std::forward<R>( range ), std::forward<Proc>( op ) );
  }
  template<typename Config,
           Channel Sink = Channel::Stderr,
           Policy Mode  = Policy::Async,
           Region Zone  = Region::Fixed,
           class R,
           typename... Options
#ifdef __cpp_concepts
           >
    requires( details::traits::is_sized_range<R>::value
              && !std::ranges::view<std::remove_reference_t<R>> && details::traits::is_config<Config>::value
              && details::traits::is_iterable_bar<prefab::BasicBar<Config, Sink, Mode, Zone>>::value
              && std::is_constructible_v<Config, Options && ...> )
#else
           ,
           typename std::enable_if<details::traits::all_of<details::traits::is_sized_range<R>,
                                                           details::traits::is_config<Config>>::value,
                                   bool>::type = 0>
#endif
  PACE__NODISCARD PACE__FORCEINLINE auto iterate( R&& range, Options&&... options )
#ifndef __cpp_concepts
    -> typename std::enable_if<
      details::traits::all_of<details::traits::is_iterable_bar<prefab::BasicBar<Config, Sink, Mode, Zone>>,
                              std::is_constructible<Config, Options&&...>>::value,
      slice::TrackedSpan<slice::SizedSpan<typename std::remove_reference<R>::type>,
                         std::shared_ptr<prefab::BasicBar<Config, Sink, Mode, Zone>>>>::type
#endif
  {
    return iterate<prefab::BasicBar<Config, Sink, Mode, Zone>>( std::forward<R>( range ),
                                                                std::forward<Options>( options )... );
  }
  template<typename Config,
           Channel Sink = Channel::Stderr,
           Policy Mode  = Policy::Async,
           Region Zone  = Region::Fixed,
           class R,
           typename Act,
           typename... Options
#ifdef __cpp_concepts
           >
    requires( details::traits::is_sized_range<R>::value
              && !std::ranges::view<std::remove_reference_t<R>> && details::traits::is_config<Config>::value
              && details::traits::is_iterable_bar<prefab::BasicBar<Config, Sink, Mode, Zone>>::value
              && details::traits::is_reactive_bar<prefab::BasicBar<Config, Sink, Mode, Zone>>::value
              && std::is_same_v<
                std::remove_reference_t<decltype( std::declval<prefab::BasicBar<Config, Sink, Mode, Zone>>()
                                                  | std::declval<Act &&>() )>,
                prefab::BasicBar<Config, Sink, Mode, Zone>>
              && std::is_constructible_v<Config, Options && ...> )
#else
           ,
           typename std::enable_if<details::traits::all_of<details::traits::is_sized_range<R>,
                                                           details::traits::is_config<Config>>::value,
                                   bool>::type = 0>
#endif
  PACE__NODISCARD PACE__FORCEINLINE auto iterate( R&& range, Act&& act, Options&&... options )
#ifndef __cpp_concepts
    -> typename std::enable_if<
      details::traits::all_of<
        details::traits::is_iterable_bar<prefab::BasicBar<Config, Sink, Mode, Zone>>,
        details::traits::is_reactive_bar<prefab::BasicBar<Config, Sink, Mode, Zone>>,
        std::is_same<
          typename std::remove_reference<decltype( std::declval<prefab::BasicBar<Config, Sink, Mode, Zone>>()
                                                   | std::declval<Act&&>() )>::type,
          prefab::BasicBar<Config, Sink, Mode, Zone>>,
        std::is_constructible<Config, Options&&...>>::value,
      slice::TrackedSpan<slice::SizedSpan<typename std::remove_reference<R>::type>,
                         std::shared_ptr<prefab::BasicBar<Config, Sink, Mode, Zone>>>>::type
#endif
  {
    return iterate<prefab::BasicBar<Config, Sink, Mode, Zone>>( std::forward<R>( range ),
                                                                std::forward<Act>( act ),
                                                                std::forward<Options>( options )... );
  }
#ifdef __cpp_concepts
  template<typename Config,
           Channel Sink = Channel::Stderr,
           Policy Mode  = Policy::Async,
           Region Zone  = Region::Fixed,
           class R,
           typename... Options>
    requires( details::traits::is_sized_range<R>::value
              && std::ranges::view<R> && details::traits::is_config<Config>::value
              && details::traits::is_iterable_bar<prefab::BasicBar<Config, Sink, Mode, Zone>>::value
              && std::is_constructible_v<Config, Options && ...> )
  PACE__NODISCARD PACE__FORCEINLINE
    slice::TrackedSpan<R, std::shared_ptr<prefab::BasicBar<Config, Sink, Mode, Zone>>> iterate(
      R view,
      Options&&... options )
  {
    return { std::move( view ),
             std::make_shared<prefab::BasicBar<Config, Sink, Mode, Zone>>(
               std::forward<Options>( options )... ) };
  }
  template<typename Config,
           Channel Sink = Channel::Stderr,
           Policy Mode  = Policy::Async,
           Region Zone  = Region::Fixed,
           class R,
           typename Act,
           typename... Options>
  PACE__NODISCARD PACE__FORCEINLINE
    slice::TrackedSpan<R, std::shared_ptr<prefab::BasicBar<Config, Sink, Mode, Zone>>>
    iterate( R view, Act&& act, Options&&... options )
    requires( details::traits::is_sized_range<R>::value
              && std::ranges::view<R> && details::traits::is_config<Config>::value
              && details::traits::is_iterable_bar<prefab::BasicBar<Config, Sink, Mode, Zone>>::value
              && details::traits::is_reactive_bar<prefab::BasicBar<Config, Sink, Mode, Zone>>::value
              && std::is_same_v<
                std::remove_reference_t<decltype( std::declval<prefab::BasicBar<Config, Sink, Mode, Zone>>()
                                                  | std::forward<Act>( act ) )>,
                prefab::BasicBar<Config, Sink, Mode, Zone>>
              && std::is_constructible_v<Config, Options && ...> )
  {
    return { std::move( view ),
             std::make_shared<prefab::BasicBar<Config, Sink, Mode, Zone>>(
               Bar( std::forward<Options>( options )... ) | std::forward<Act>( act ) ) };
  }
#endif
  template<typename Config,
           Channel Sink = Channel::Stderr,
           Policy Mode  = Policy::Async,
           Region Zone  = Region::Fixed,
           class R,
           typename Proc,
           typename... Options
#ifdef __cpp_concepts
           >
    requires( details::traits::is_sized_range<R>::value && details::traits::is_config<Config>::value
              && details::traits::is_iterable_bar<prefab::BasicBar<Config, Sink, Mode, Zone>>::value
              && std::is_constructible_v<Config, Options && ...>
              && requires( details::traits::iter_value_t<details::traits::iterator_of_t<R>> ele,
                           Proc&& op ) { op( ele ); } )
#else
           ,
           typename std::enable_if<details::traits::all_of<details::traits::is_sized_range<R>,
                                                           details::traits::is_config<Config>>::value,
                                   bool>::type = 0>
#endif
  PACE__FORCEINLINE auto iterate( R&& range, Proc&& op, Options&&... options )
#ifndef __cpp_concepts
    -> typename std::enable_if<details::traits::all_of<
      details::traits::is_iterable_bar<prefab::BasicBar<Config, Sink, Mode, Zone>>,
      std::is_constructible<Config, Options&&...>,
      std::is_void<decltype( std::declval<Proc&&>( std::declval<details::traits::iter_value_t<
                                                     details::traits::iterator_of_t<R>>>() ),
                             void() )>>::value>::type
#endif
  {
    iterate<prefab::BasicBar<Config, Sink, Mode, Zone>>( std::forward<R>( range ),
                                                         std::forward<Proc>( op ),
                                                         std::forward<Options>( options )... );
  }
  template<typename Config,
           Channel Sink = Channel::Stderr,
           Policy Mode  = Policy::Async,
           Region Zone  = Region::Fixed,
           class R,
           typename Proc,
           typename Act,
           typename... Options
#ifdef __cpp_concepts
           >
    requires( details::traits::is_sized_range<R>::value && details::traits::is_config<Config>::value
              && details::traits::is_iterable_bar<prefab::BasicBar<Config, Sink, Mode, Zone>>::value
              && details::traits::is_reactive_bar<prefab::BasicBar<Config, Sink, Mode, Zone>>::value
              && std::is_same_v<
                std::remove_reference_t<decltype( std::declval<prefab::BasicBar<Config, Sink, Mode, Zone>>()
                                                  | std::declval<Act &&>() )>,
                prefab::BasicBar<Config, Sink, Mode, Zone>>
              && std::is_constructible_v<Config, Options && ...> )
#else
           ,
           typename std::enable_if<details::traits::all_of<details::traits::is_sized_range<R>,
                                                           details::traits::is_config<Config>>::value,
                                   bool>::type = 0>
#endif
  PACE__FORCEINLINE auto iterate( R&& range, Proc&& op, Act&& act, Options&&... options )
#ifndef __cpp_concepts
    -> typename std::enable_if<details::traits::all_of<
      details::traits::is_iterable_bar<prefab::BasicBar<Config, Sink, Mode, Zone>>,
      details::traits::is_reactive_bar<prefab::BasicBar<Config, Sink, Mode, Zone>>,
      std::is_same<
        typename std::remove_reference<decltype( std::declval<prefab::BasicBar<Config, Sink, Mode, Zone>>()
                                                 | std::declval<Act&&>() )>::type,
        prefab::BasicBar<Config, Sink, Mode, Zone>>,
      std::is_constructible<Config, Options&&...>>::value>::type
#endif
  {
    iterate<prefab::BasicBar<Config, Sink, Mode, Zone>>( std::forward<R>( range ),
                                                         std::forward<Proc>( op ),
                                                         std::forward<Act>( act ),
                                                         std::forward<Options>( options )... );
  }
} // namespace pace

#endif
