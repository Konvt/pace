#ifndef PGBAR_BASIC_BAR
#define PGBAR_BASIC_BAR

#include "../../Indicator.hpp"
#include "../../facade/BehaviorRegistry.hpp"
#include "../behaviors/Incremental.hpp"
#include "../behaviors/Reactive.hpp"
#include "BasicConfig.hpp"

namespace pgbar {
  namespace _details {
    namespace prefabs {
      template<typename Soul, Channel Outlet, Policy Mode, Region Area>
      class BasicBar
        : public traits::LI<
            traits::BehaviorLinker_t<Soul>>::template type<Indicator, BasicBar<Soul, Outlet, Mode, Area>> {
        using Base = typename traits::LI<
          traits::BehaviorLinker_t<Soul>>::template type<Indicator, BasicBar<Soul, Outlet, Mode, Area>>;

      public:
        using Config                     = Soul;
        static constexpr Channel Channel = Outlet;
        static constexpr Policy Policy   = Mode;
        static constexpr Region Region   = Area;

        using Base::Base;
        constexpr BasicBar() = default;
#ifdef __cpp_concepts
        template<typename... Args>
          requires( !( std::is_same_v<std::decay_t<Args>, Soul> || ... )
                    && std::is_constructible_v<Soul, Args && ...> )
#else
        template<typename... Args,
                 typename = typename std::enable_if<
                   traits::AllOf<traits::Not<std::is_same<typename std::decay<Args>::type, Soul>>...,
                                 std::is_constructible<Soul, Args&&...>>::value>::type>
#endif
        constexpr BasicBar( Args&&... args ) noexcept( std::is_nothrow_constructible<Base, Args&&...>::value )
          : Base( Soul( std::forward<Args>( args )... ) )
        {}

        BasicBar( const BasicBar& )            = delete;
        BasicBar& operator=( const BasicBar& ) = delete;

        constexpr BasicBar( BasicBar&& )                             = default;
        PGBAR__CXX14_CNSTXPR BasicBar& operator=( BasicBar&& rhs ) & = default;
        PGBAR__CXX20_CNSTXPR virtual ~BasicBar()                     = default;

        void swap( BasicBar& other ) noexcept
        {
          PGBAR__TRUST( this != &other );
          PGBAR__ASSERT( this->active() == false );
          PGBAR__ASSERT( other.active() == false );
          Base::swap( other );
        }
        friend void swap( BasicBar& a, BasicBar& b ) noexcept { a.swap( b ); }
      };
    } // namespace prefabs

    namespace traits {
      template<typename B>
      struct _impl_is_bar {
      private:
        template<typename S, Channel O, Policy M, Region A>
        static constexpr std::true_type check( const prefabs::BasicBar<S, O, M, A>& );
        static constexpr std::false_type check( ... );

      public:
        using result = AllOf<Not<std::is_reference<B>>,
                             decltype( check( std::declval<typename std::remove_cv<B>::type>() ) )>;
      };
      template<typename B>
      using is_bar = typename _impl_is_bar<B>::result;
      template<typename B>
      using is_iterable_bar = AllOf<is_bar<B>, is_instance_of<B, behaviors::Incremental>>;
      template<typename B>
      using is_reactive_bar = AllOf<is_bar<B>, is_instance_of<B, behaviors::Reactive>>;
    } // namespace traits
  } // namespace _details

  template<typename Bar, typename N, typename... Options>
  PGBAR__NODISCARD PGBAR__FORCEINLINE auto iterate( N startpoint, N endpoint, N step, Options&&... options )
    ->
#ifdef __cpp_concepts
    slice::TrackedSpan<slice::NumericSpan<N>, std::shared_ptr<Bar>>
    requires( std::is_arithmetic_v<N> && _details::traits::is_iterable_bar<Bar>::value
              && std::is_constructible_v<Bar, Options && ...> )
#else
    typename std::enable_if<_details::traits::AllOf<std::is_arithmetic<N>,
                                                    _details::traits::is_iterable_bar<Bar>,
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
  PGBAR__NODISCARD PGBAR__FORCEINLINE auto iterate( N startpoint,
                                                    N endpoint,
                                                    N step,
                                                    Act&& act,
                                                    Options&&... options ) ->
#ifdef __cpp_concepts
    slice::TrackedSpan<slice::NumericSpan<N>, std::shared_ptr<Bar>>
    requires(
      std::is_arithmetic_v<N> && _details::traits::is_iterable_bar<Bar>::value
      && _details::traits::is_reactive_bar<Bar>::value
      && std::is_same_v<std::remove_reference_t<decltype( std::declval<Bar>() | std::forward<Act>( act ) )>,
                        Bar>
      && std::is_constructible_v<Bar, Options && ...> )
#else
    typename std::enable_if<
      _details::traits::AllOf<
        std::is_arithmetic<N>,
        _details::traits::is_iterable_bar<Bar>,
        _details::traits::is_reactive_bar<Bar>,
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
  PGBAR__FORCEINLINE auto iterate( N startpoint, N endpoint, N step, Proc&& op, Options&&... options )
#ifdef __cpp_concepts
    requires( std::is_arithmetic_v<N> && _details::traits::is_iterable_bar<Bar>::value
              && std::is_constructible_v<Bar, Options && ...> && requires( N ele, Proc&& op ) { op( ele ); } )
#else
    -> typename std::enable_if<_details::traits::AllOf<
      std::is_arithmetic<N>,
      _details::traits::is_iterable_bar<Bar>,
      std::is_constructible<Bar, Options&&...>,
      std::is_void<decltype( std::declval<Proc&&>()( std::declval<N>() ), void() )>>::value>::type
#endif
  {
    Bar( std::forward<Options>( options )... )
      .iterate( startpoint, endpoint, step, std::forward<Proc>( op ) );
  }
  template<typename Bar, typename N, typename Proc, typename Act, typename... Options>
  PGBAR__FORCEINLINE auto iterate( N startpoint,
                                   N endpoint,
                                   N step,
                                   Proc&& op,
                                   Act&& act,
                                   Options&&... options )
#ifdef __cpp_concepts
    requires(
      std::is_arithmetic_v<N> && _details::traits::is_iterable_bar<Bar>::value
      && _details::traits::is_reactive_bar<Bar>::value
      && std::is_same_v<std::remove_reference_t<decltype( std::declval<Bar>() | std::forward<Act>( act ) )>,
                        Bar>
      && std::is_constructible_v<Bar, Options && ...> )
#else
    -> typename std::enable_if<_details::traits::AllOf<
      std::is_arithmetic<N>,
      _details::traits::is_iterable_bar<Bar>,
      _details::traits::is_reactive_bar<Bar>,
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
           Channel Outlet = Channel::Stderr,
           Policy Mode    = Policy::Async,
           Region Area    = Region::Fixed,
           typename N,
           typename... Options
#ifdef __cpp_concepts
           >
    requires(
      std::is_arithmetic_v<N> && _details::traits::is_config<Config>::value
      && _details::traits::is_iterable_bar<_details::prefabs::BasicBar<Config, Outlet, Mode, Area>>::value
      && std::is_constructible_v<Config, Options && ...> )
#else
           ,
           typename std::enable_if<
             _details::traits::AllOf<std::is_arithmetic<N>, _details::traits::is_config<Config>>::value,
             bool>::type = 0>

#endif
  PGBAR__NODISCARD PGBAR__FORCEINLINE auto iterate( N startpoint, N endpoint, N step, Options&&... options )
#ifndef __cpp_concepts
    -> typename std::enable_if<
      _details::traits::AllOf<
        _details::traits::is_iterable_bar<_details::prefabs::BasicBar<Config, Outlet, Mode, Area>>,
        std::is_constructible<Config, Options&&...>>::value,
      slice::TrackedSpan<slice::NumericSpan<N>,
                         std::shared_ptr<_details::prefabs::BasicBar<Config, Outlet, Mode, Area>>>>::type
#endif
  {
    return iterate<_details::prefabs::BasicBar<Config, Outlet, Mode, Area>>(
      startpoint,
      endpoint,
      step,
      std::forward<Options>( options )... );
  }
  template<typename Config,
           Channel Outlet = Channel::Stderr,
           Policy Mode    = Policy::Async,
           Region Area    = Region::Fixed,
           typename N,
           typename Act,
           typename... Options
#ifdef __cpp_concepts
           >
    requires(
      std::is_arithmetic_v<N> && _details::traits::is_config<Config>::value
      && _details::traits::is_iterable_bar<_details::prefabs::BasicBar<Config, Outlet, Mode, Area>>::value
      && _details::traits::is_reactive_bar<_details::prefabs::BasicBar<Config, Outlet, Mode, Area>>::value
      && std::is_same_v<std::remove_reference_t<
                          decltype( std::declval<_details::prefabs::BasicBar<Config, Outlet, Mode, Area>>()
                                    | std::declval<Act &&>() )>,
                        _details::prefabs::BasicBar<Config, Outlet, Mode, Area>>
      && std::is_constructible_v<Config, Options && ...> )
#else
           ,
           typename std::enable_if<
             _details::traits::AllOf<std::is_arithmetic<N>, _details::traits::is_config<Config>>::value,
             bool>::type = 0>
#endif
  PGBAR__NODISCARD PGBAR__FORCEINLINE auto iterate( N startpoint,
                                                    N endpoint,
                                                    N step,
                                                    Act&& act,
                                                    Options&&... options )
#ifndef __cpp_concepts
    -> typename std::enable_if<
      _details::traits::AllOf<
        _details::traits::is_iterable_bar<_details::prefabs::BasicBar<Config, Outlet, Mode, Area>>,
        _details::traits::is_reactive_bar<_details::prefabs::BasicBar<Config, Outlet, Mode, Area>>,
        std::is_same<typename std::remove_reference<
                       decltype( std::declval<_details::prefabs::BasicBar<Config, Outlet, Mode, Area>>()
                                 | std::declval<Act&&>() )>::type,
                     _details::prefabs::BasicBar<Config, Outlet, Mode, Area>>,
        std::is_constructible<Config, Options&&...>>::value,
      slice::TrackedSpan<slice::NumericSpan<N>,
                         std::shared_ptr<_details::prefabs::BasicBar<Config, Outlet, Mode, Area>>>>::type
#endif
  {
    return iterate<_details::prefabs::BasicBar<Config, Outlet, Mode, Area>>(
      startpoint,
      endpoint,
      step,
      std::forward<Act>( act ),
      std::forward<Options>( options )... );
  }
  template<typename Config,
           Channel Outlet = Channel::Stderr,
           Policy Mode    = Policy::Async,
           Region Area    = Region::Fixed,
           typename N,
           typename Proc,
           typename... Options
#ifdef __cpp_concepts
           >
    requires(
      std::is_arithmetic_v<N> && _details::traits::is_config<Config>::value
      && _details::traits::is_iterable_bar<_details::prefabs::BasicBar<Config, Outlet, Mode, Area>>::value
      && std::is_constructible_v<Config, Options && ...> && requires( N ele, Proc&& op ) { op( ele ); } )
#else
           ,
           typename std::enable_if<
             _details::traits::AllOf<std::is_arithmetic<N>, _details::traits::is_config<Config>>::value,
             bool>::type = 0>
#endif
  PGBAR__FORCEINLINE auto iterate( N startpoint, N endpoint, N step, Proc&& op, Options&&... options )
#ifndef __cpp_concepts
    -> typename std::enable_if<_details::traits::AllOf<
      _details::traits::is_iterable_bar<_details::prefabs::BasicBar<Config, Outlet, Mode, Area>>,
      std::is_constructible<Config, Options&&...>,
      std::is_void<decltype( std::declval<Proc&&>( std::declval<N>() ), void() )>>::value>::type
#endif
  {
    iterate<_details::prefabs::BasicBar<Config, Outlet, Mode, Area>>( startpoint,
                                                                      endpoint,
                                                                      step,
                                                                      std::forward<Proc>( op ),
                                                                      std::forward<Options>( options )... );
  }
  template<typename Config,
           Channel Outlet = Channel::Stderr,
           Policy Mode    = Policy::Async,
           Region Area    = Region::Fixed,
           typename N,
           typename Proc,
           typename Act,
           typename... Options
#ifdef __cpp_concepts
           >
    requires(
      std::is_arithmetic_v<N> && _details::traits::is_config<Config>::value
      && _details::traits::is_iterable_bar<_details::prefabs::BasicBar<Config, Outlet, Mode, Area>>::value
      && _details::traits::is_reactive_bar<_details::prefabs::BasicBar<Config, Outlet, Mode, Area>>::value
      && std::is_same_v<std::remove_reference_t<
                          decltype( std::declval<_details::prefabs::BasicBar<Config, Outlet, Mode, Area>>()
                                    | std::declval<Act &&>() )>,
                        _details::prefabs::BasicBar<Config, Outlet, Mode, Area>>
      && std::is_constructible_v<Config, Options && ...> )
#else
           ,
           typename std::enable_if<
             _details::traits::AllOf<std::is_arithmetic<N>, _details::traits::is_config<Config>>::value,
             bool>::type = 0>
#endif
  PGBAR__FORCEINLINE auto iterate( N startpoint,
                                   N endpoint,
                                   N step,
                                   Proc&& op,
                                   Act&& act,
                                   Options&&... options )
#ifndef __cpp_concepts
    -> typename std::enable_if<_details::traits::AllOf<
      _details::traits::is_iterable_bar<_details::prefabs::BasicBar<Config, Outlet, Mode, Area>>,
      _details::traits::is_reactive_bar<_details::prefabs::BasicBar<Config, Outlet, Mode, Area>>,
      std::is_same<typename std::remove_reference<
                     decltype( std::declval<_details::prefabs::BasicBar<Config, Outlet, Mode, Area>>()
                               | std::declval<Act&&>() )>::type,
                   _details::prefabs::BasicBar<Config, Outlet, Mode, Area>>,
      std::is_constructible<Config, Options&&...>>::value>::type
#endif
  {
    iterate<_details::prefabs::BasicBar<Config, Outlet, Mode, Area>>( startpoint,
                                                                      endpoint,
                                                                      step,
                                                                      std::forward<Proc>( op ),
                                                                      std::forward<Act>( act ),
                                                                      std::forward<Options>( options )... );
  }

  //////////////////////////////////////////////////////////////////////////////////////////////////////////

  template<typename Bar, typename N, typename... Options>
  PGBAR__NODISCARD PGBAR__FORCEINLINE auto iterate( N endpoint, N step, Options&&... options ) ->
#ifdef __cpp_concepts
    slice::TrackedSpan<slice::NumericSpan<N>, std::shared_ptr<Bar>>
    requires( std::is_floating_point_v<N> && _details::traits::is_iterable_bar<Bar>::value
              && std::is_constructible_v<Bar, Options && ...> )
#else
    typename std::enable_if<_details::traits::AllOf<std::is_floating_point<N>,
                                                    _details::traits::is_iterable_bar<Bar>,
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
  PGBAR__NODISCARD PGBAR__FORCEINLINE auto iterate( N endpoint, N step, Act&& act, Options&&... options ) ->
#ifdef __cpp_concepts
    slice::TrackedSpan<slice::NumericSpan<N>, std::shared_ptr<Bar>>
    requires(
      std::is_floating_point_v<N> && _details::traits::is_iterable_bar<Bar>::value
      && _details::traits::is_reactive_bar<Bar>::value
      && std::is_same_v<std::remove_reference_t<decltype( std::declval<Bar>() | std::forward<Act>( act ) )>,
                        Bar>
      && std::is_constructible_v<Bar, Options && ...> )
#else
    typename std::enable_if<
      _details::traits::AllOf<
        std::is_floating_point<N>,
        _details::traits::is_iterable_bar<Bar>,
        _details::traits::is_reactive_bar<Bar>,
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
  PGBAR__FORCEINLINE auto iterate( N endpoint, N step, Proc&& op, Options&&... options )
#ifdef __cpp_concepts
    requires( std::is_floating_point_v<N> && _details::traits::is_iterable_bar<Bar>::value
              && std::is_constructible_v<Bar, Options && ...> && requires( N ele, Proc&& op ) { op( ele ); } )
#else
    -> typename std::enable_if<_details::traits::AllOf<
      std::is_floating_point<N>,
      _details::traits::is_iterable_bar<Bar>,
      std::is_constructible<Bar, Options&&...>,
      std::is_void<decltype( std::declval<Proc&&>( std::declval<N>() ), void() )>>::value>::type
#endif
  {
    Bar( std::forward<Options>( options )... ).iterate( endpoint, step, std::forward<Proc>( op ) );
  }
  template<typename Bar, typename N, typename Proc, typename Act, typename... Options>
  PGBAR__FORCEINLINE auto iterate( N endpoint, N step, Proc&& op, Act&& act, Options&&... options )
#ifdef __cpp_concepts
    requires(
      std::is_floating_point_v<N> && _details::traits::is_iterable_bar<Bar>::value
      && _details::traits::is_reactive_bar<Bar>::value
      && std::is_same_v<std::remove_reference_t<decltype( std::declval<Bar>() | std::forward<Act>( act ) )>,
                        Bar>
      && std::is_constructible_v<Bar, Options && ...> )
#else
    -> typename std::enable_if<_details::traits::AllOf<
      std::is_floating_point<N>,
      _details::traits::is_iterable_bar<Bar>,
      _details::traits::is_reactive_bar<Bar>,
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
           Channel Outlet = Channel::Stderr,
           Policy Mode    = Policy::Async,
           Region Area    = Region::Fixed,
           typename N,
           typename... Options
#ifdef __cpp_concepts
           >
    requires(
      std::is_floating_point_v<N> && _details::traits::is_config<Config>::value
      && _details::traits::is_iterable_bar<_details::prefabs::BasicBar<Config, Outlet, Mode, Area>>::value
      && std::is_constructible_v<Config, Options && ...> )
#else
           ,
           typename std::enable_if<
             _details::traits::AllOf<std::is_floating_point<N>, _details::traits::is_config<Config>>::value,
             bool>::type = 0>
#endif
  PGBAR__NODISCARD PGBAR__FORCEINLINE auto iterate( N endpoint, N step, Options&&... options )
#ifndef __cpp_concepts
    -> typename std::enable_if<
      _details::traits::AllOf<
        std::is_floating_point<N>,
        _details::traits::is_config<Config>,
        _details::traits::is_iterable_bar<_details::prefabs::BasicBar<Config, Outlet, Mode, Area>>,
        std::is_constructible<Config, Options&&...>>::value,
      slice::TrackedSpan<slice::NumericSpan<N>,
                         std::shared_ptr<_details::prefabs::BasicBar<Config, Outlet, Mode, Area>>>>::type
#endif
  {
    return iterate<_details::prefabs::BasicBar<Config, Outlet, Mode, Area>>(
      endpoint,
      step,
      std::forward<Options>( options )... );
  }
  template<typename Config,
           Channel Outlet = Channel::Stderr,
           Policy Mode    = Policy::Async,
           Region Area    = Region::Fixed,
           typename N,
           typename Act,
           typename... Options
#ifdef __cpp_concepts
           >
    requires(
      std::is_floating_point_v<N> && _details::traits::is_config<Config>::value
      && _details::traits::is_iterable_bar<_details::prefabs::BasicBar<Config, Outlet, Mode, Area>>::value
      && _details::traits::is_reactive_bar<_details::prefabs::BasicBar<Config, Outlet, Mode, Area>>::value
      && std::is_same_v<std::remove_reference_t<
                          decltype( std::declval<_details::prefabs::BasicBar<Config, Outlet, Mode, Area>>()
                                    | std::declval<Act &&>() )>,
                        _details::prefabs::BasicBar<Config, Outlet, Mode, Area>>
      && std::is_constructible_v<Config, Options && ...> )
#else
           ,
           typename std::enable_if<
             _details::traits::AllOf<std::is_floating_point<N>, _details::traits::is_config<Config>>::value,
             bool>::type = 0>
#endif
  PGBAR__NODISCARD PGBAR__FORCEINLINE auto iterate( N endpoint, N step, Act&& act, Options&&... options )
#ifndef __cpp_concepts
    -> typename std::enable_if<
      _details::traits::AllOf<
        _details::traits::is_iterable_bar<_details::prefabs::BasicBar<Config, Outlet, Mode, Area>>,
        _details::traits::is_reactive_bar<_details::prefabs::BasicBar<Config, Outlet, Mode, Area>>,
        std::is_same<typename std::remove_reference<
                       decltype( std::declval<_details::prefabs::BasicBar<Config, Outlet, Mode, Area>>()
                                 | std::declval<Act&&>() )>::type,
                     _details::prefabs::BasicBar<Config, Outlet, Mode, Area>>,
        std::is_constructible<Config, Options&&...>>::value,
      slice::TrackedSpan<slice::NumericSpan<N>,
                         std::shared_ptr<_details::prefabs::BasicBar<Config, Outlet, Mode, Area>>>>::type
#endif
  {
    return iterate<_details::prefabs::BasicBar<Config, Outlet, Mode, Area>>(
      endpoint,
      step,
      std::forward<Act>( act ),
      std::forward<Options>( options )... );
  }
  template<typename Config,
           Channel Outlet = Channel::Stderr,
           Policy Mode    = Policy::Async,
           Region Area    = Region::Fixed,
           typename N,
           typename Proc,
           typename... Options
#ifdef __cpp_concepts
           >
    requires(
      std::is_floating_point_v<N> && _details::traits::is_config<Config>::value
      && _details::traits::is_iterable_bar<_details::prefabs::BasicBar<Config, Outlet, Mode, Area>>::value
      && std::is_constructible_v<Config, Options && ...> && requires( N ele, Proc&& op ) { op( ele ); } )
#else
           ,
           typename std::enable_if<
             _details::traits::AllOf<std::is_floating_point<N>, _details::traits::is_config<Config>>::value,
             bool>::type = 0>
#endif
  PGBAR__FORCEINLINE auto iterate( N endpoint, N step, Proc&& op, Options&&... options )
#ifndef __cpp_concepts
    -> typename std::enable_if<_details::traits::AllOf<
      _details::traits::is_iterable_bar<_details::prefabs::BasicBar<Config, Outlet, Mode, Area>>,
      std::is_constructible<Config, Options&&...>,
      std::is_void<decltype( std::declval<Proc&&>( std::declval<N>() ), void() )>>::value>::type
#endif
  {
    iterate<_details::prefabs::BasicBar<Config, Outlet, Mode, Area>>( endpoint,
                                                                      step,
                                                                      std::forward<Proc>( op ),
                                                                      std::forward<Options>( options )... );
  }
  template<typename Config,
           Channel Outlet = Channel::Stderr,
           Policy Mode    = Policy::Async,
           Region Area    = Region::Fixed,
           typename N,
           typename Proc,
           typename Act,
           typename... Options
#ifdef __cpp_concepts
           >
    requires(
      std::is_floating_point_v<N> && _details::traits::is_config<Config>::value
      && _details::traits::is_iterable_bar<_details::prefabs::BasicBar<Config, Outlet, Mode, Area>>::value
      && _details::traits::is_reactive_bar<_details::prefabs::BasicBar<Config, Outlet, Mode, Area>>::value
      && std::is_same_v<std::remove_reference_t<
                          decltype( std::declval<_details::prefabs::BasicBar<Config, Outlet, Mode, Area>>()
                                    | std::declval<Act &&>() )>,
                        _details::prefabs::BasicBar<Config, Outlet, Mode, Area>>
      && std::is_constructible_v<Config, Options && ...> )
#else
           ,
           typename std::enable_if<
             _details::traits::AllOf<std::is_floating_point<N>, _details::traits::is_config<Config>>::value,
             bool>::type = 0>
#endif
  PGBAR__FORCEINLINE auto iterate( N endpoint, N step, Proc&& op, Act&& act, Options&&... options )
#ifndef __cpp_concepts
    -> typename std::enable_if<_details::traits::AllOf<
      _details::traits::is_iterable_bar<_details::prefabs::BasicBar<Config, Outlet, Mode, Area>>,
      _details::traits::is_reactive_bar<_details::prefabs::BasicBar<Config, Outlet, Mode, Area>>,
      std::is_same<typename std::remove_reference<
                     decltype( std::declval<_details::prefabs::BasicBar<Config, Outlet, Mode, Area>>()
                               | std::declval<Act&&>() )>::type,
                   _details::prefabs::BasicBar<Config, Outlet, Mode, Area>>,
      std::is_constructible<Config, Options&&...>>::value>::type
#endif
  {
    iterate<_details::prefabs::BasicBar<Config, Outlet, Mode, Area>>( endpoint,
                                                                      step,
                                                                      std::forward<Proc>( op ),
                                                                      std::forward<Act>( act ),
                                                                      std::forward<Options>( options )... );
  }

  //////////////////////////////////////////////////////////////////////////////////////////////////////////

  template<typename Bar, typename N, typename... Options>
  PGBAR__NODISCARD PGBAR__FORCEINLINE auto iterate( N startpoint, N endpoint, Options&&... options ) ->
#ifdef __cpp_concepts
    slice::TrackedSpan<slice::NumericSpan<N>, std::shared_ptr<Bar>>
    requires( std::is_integral_v<N> && _details::traits::is_iterable_bar<Bar>::value
              && std::is_constructible_v<Bar, Options && ...> )
#else
    typename std::enable_if<_details::traits::AllOf<std::is_integral<N>,
                                                    _details::traits::is_iterable_bar<Bar>,
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
  PGBAR__NODISCARD PGBAR__FORCEINLINE auto iterate( N startpoint,
                                                    N endpoint,
                                                    Act&& act,
                                                    Options&&... options ) ->
#ifdef __cpp_concepts
    slice::TrackedSpan<slice::NumericSpan<N>, std::shared_ptr<Bar>>
    requires(
      std::is_integral_v<N> && _details::traits::is_iterable_bar<Bar>::value
      && _details::traits::is_reactive_bar<Bar>::value
      && std::is_same_v<std::remove_reference_t<decltype( std::declval<Bar>() | std::forward<Act>( act ) )>,
                        Bar>
      && std::is_constructible_v<Bar, Options && ...> )
#else
    typename std::enable_if<
      _details::traits::AllOf<
        std::is_integral<N>,
        _details::traits::is_iterable_bar<Bar>,
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
  PGBAR__FORCEINLINE auto iterate( N startpoint, N endpoint, Proc&& op, Options&&... options )
#ifdef __cpp_concepts
    requires( std::is_integral_v<N> && _details::traits::is_iterable_bar<Bar>::value
              && std::is_constructible_v<Bar, Options && ...> && requires( N ele, Proc&& op ) { op( ele ); } )
#else
    -> typename std::enable_if<_details::traits::AllOf<
      std::is_integral<N>,
      _details::traits::is_iterable_bar<Bar>,
      std::is_constructible<Bar, Options&&...>,
      std::is_void<decltype( std::declval<Proc&&>( std::declval<N>() ), void() )>>::value>::type
#endif
  {
    Bar( std::forward<Options>( options )... ).iterate( startpoint, endpoint, std::forward<Proc>( op ) );
  }
  template<typename Bar, typename N, typename Proc, typename Act, typename... Options>
  PGBAR__FORCEINLINE auto iterate( N startpoint, N endpoint, Proc&& op, Act&& act, Options&&... options )
#ifdef __cpp_concepts
    requires(
      std::is_integral_v<N> && _details::traits::is_iterable_bar<Bar>::value
      && _details::traits::is_reactive_bar<Bar>::value
      && std::is_same_v<std::remove_reference_t<decltype( std::declval<Bar>() | std::forward<Act>( act ) )>,
                        Bar>
      && std::is_constructible_v<Bar, Options && ...> )
#else
    -> typename std::enable_if<_details::traits::AllOf<
      std::is_integral<N>,
      _details::traits::is_iterable_bar<Bar>,
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
           Channel Outlet = Channel::Stderr,
           Policy Mode    = Policy::Async,
           Region Area    = Region::Fixed,
           typename N,
           typename... Options
#ifdef __cpp_concepts
           >
    requires(
      std::is_integral_v<N> && _details::traits::is_config<Config>::value
      && _details::traits::is_iterable_bar<_details::prefabs::BasicBar<Config, Outlet, Mode, Area>>::value
      && std::is_constructible_v<Config, Options && ...> )
#else
           ,
           typename std::enable_if<
             _details::traits::AllOf<std::is_integral<N>, _details::traits::is_config<Config>>::value,
             bool>::type = 0>
#endif
  PGBAR__NODISCARD PGBAR__FORCEINLINE auto iterate( N startpoint, N endpoint, Options&&... options )
#ifndef __cpp_concepts
    -> typename std::enable_if<
      _details::traits::AllOf<
        _details::traits::is_iterable_bar<_details::prefabs::BasicBar<Config, Outlet, Mode, Area>>,
        std::is_constructible<Config, Options&&...>>::value,
      slice::TrackedSpan<slice::NumericSpan<N>,
                         std::shared_ptr<_details::prefabs::BasicBar<Config, Outlet, Mode, Area>>>>::type
#endif
  {
    return iterate<_details::prefabs::BasicBar<Config, Outlet, Mode, Area>>(
      startpoint,
      endpoint,
      std::forward<Options>( options )... );
  }
  template<typename Config,
           Channel Outlet = Channel::Stderr,
           Policy Mode    = Policy::Async,
           Region Area    = Region::Fixed,
           typename N,
           typename Act,
           typename... Options
#ifdef __cpp_concepts
           >
    requires(
      std::is_integral_v<N> && _details::traits::is_config<Config>::value
      && _details::traits::is_iterable_bar<_details::prefabs::BasicBar<Config, Outlet, Mode, Area>>::value
      && _details::traits::is_reactive_bar<_details::prefabs::BasicBar<Config, Outlet, Mode, Area>>::value
      && std::is_same_v<std::remove_reference_t<
                          decltype( std::declval<_details::prefabs::BasicBar<Config, Outlet, Mode, Area>>()
                                    | std::declval<Act &&>() )>,
                        _details::prefabs::BasicBar<Config, Outlet, Mode, Area>>
      && std::is_constructible_v<Config, Options && ...> )
#else
           ,
           typename std::enable_if<
             _details::traits::AllOf<std::is_integral<N>, _details::traits::is_config<Config>>::value,
             bool>::type = 0>
#endif
  PGBAR__NODISCARD PGBAR__FORCEINLINE auto iterate( N startpoint,
                                                    N endpoint,
                                                    Act&& act,
                                                    Options&&... options )
#ifndef __cpp_concepts
    -> typename std::enable_if<
      _details::traits::AllOf<
        _details::traits::is_iterable_bar<_details::prefabs::BasicBar<Config, Outlet, Mode, Area>>,
        _details::traits::is_reactive_bar<_details::prefabs::BasicBar<Config, Outlet, Mode, Area>>,
        std::is_same<typename std::remove_reference<
                       decltype( std::declval<_details::prefabs::BasicBar<Config, Outlet, Mode, Area>>()
                                 | std::declval<Act&&>() )>::type,
                     _details::prefabs::BasicBar<Config, Outlet, Mode, Area>>,
        std::is_constructible<Config, Options&&...>>::value,
      slice::TrackedSpan<slice::NumericSpan<N>,
                         std::shared_ptr<_details::prefabs::BasicBar<Config, Outlet, Mode, Area>>>>::type
#endif
  {
    return iterate<_details::prefabs::BasicBar<Config, Outlet, Mode, Area>>(
      startpoint,
      endpoint,
      std::forward<Act>( act ),
      std::forward<Options>( options )... );
  }
  template<typename Config,
           Channel Outlet = Channel::Stderr,
           Policy Mode    = Policy::Async,
           Region Area    = Region::Fixed,
           typename N,
           typename Proc,
           typename... Options
#ifdef __cpp_concepts
           >
    requires(
      std::is_integral_v<N> && _details::traits::is_config<Config>::value
      && _details::traits::is_iterable_bar<_details::prefabs::BasicBar<Config, Outlet, Mode, Area>>::value
      && std::is_constructible_v<Config, Options && ...> && requires( N ele, Proc&& op ) { op( ele ); } )
#else
           ,
           typename std::enable_if<
             _details::traits::AllOf<std::is_integral<N>, _details::traits::is_config<Config>>::value,
             bool>::type = 0>
#endif
  PGBAR__FORCEINLINE auto iterate( N startpoint, N endpoint, Proc&& op, Options&&... options )
#ifndef __cpp_concepts
    -> typename std::enable_if<_details::traits::AllOf<
      _details::traits::is_iterable_bar<_details::prefabs::BasicBar<Config, Outlet, Mode, Area>>,
      std::is_constructible<Config, Options&&...>,
      std::is_void<decltype( std::declval<Proc&&>( std::declval<N>() ), void() )>>::value>::type
#endif
  {
    iterate<_details::prefabs::BasicBar<Config, Outlet, Mode, Area>>( startpoint,
                                                                      endpoint,
                                                                      std::forward<Proc>( op ),
                                                                      std::forward<Options>( options )... );
  }
  template<typename Config,
           Channel Outlet = Channel::Stderr,
           Policy Mode    = Policy::Async,
           Region Area    = Region::Fixed,
           typename N,
           typename Proc,
           typename Act,
           typename... Options
#ifdef __cpp_concepts
           >
    requires(
      std::is_integral_v<N> && _details::traits::is_config<Config>::value
      && _details::traits::is_iterable_bar<_details::prefabs::BasicBar<Config, Outlet, Mode, Area>>::value
      && _details::traits::is_reactive_bar<_details::prefabs::BasicBar<Config, Outlet, Mode, Area>>::value
      && std::is_same_v<std::remove_reference_t<
                          decltype( std::declval<_details::prefabs::BasicBar<Config, Outlet, Mode, Area>>()
                                    | std::declval<Act &&>() )>,
                        _details::prefabs::BasicBar<Config, Outlet, Mode, Area>>
      && std::is_constructible_v<Config, Options && ...> )
#else
           ,
           typename std::enable_if<
             _details::traits::AllOf<std::is_integral<N>, _details::traits::is_config<Config>>::value,
             bool>::type = 0>
#endif
  PGBAR__FORCEINLINE auto iterate( N startpoint, N endpoint, Proc&& op, Act&& act, Options&&... options )
#ifndef __cpp_concepts
    -> typename std::enable_if<_details::traits::AllOf<
      _details::traits::is_iterable_bar<_details::prefabs::BasicBar<Config, Outlet, Mode, Area>>,
      _details::traits::is_reactive_bar<_details::prefabs::BasicBar<Config, Outlet, Mode, Area>>,
      std::is_same<typename std::remove_reference<
                     decltype( std::declval<_details::prefabs::BasicBar<Config, Outlet, Mode, Area>>()
                               | std::declval<Act&&>() )>::type,
                   _details::prefabs::BasicBar<Config, Outlet, Mode, Area>>,
      std::is_constructible<Config, Options&&...>>::value>::type
#endif
  {
    iterate<_details::prefabs::BasicBar<Config, Outlet, Mode, Area>>( startpoint,
                                                                      endpoint,
                                                                      std::forward<Proc>( op ),
                                                                      std::forward<Act>( act ),
                                                                      std::forward<Options>( options )... );
  }

  //////////////////////////////////////////////////////////////////////////////////////////////////////////

  template<typename Bar, typename N, typename... Options>
  PGBAR__NODISCARD PGBAR__FORCEINLINE auto iterate( N endpoint, Options&&... options ) ->
#ifdef __cpp_concepts
    slice::TrackedSpan<slice::NumericSpan<N>, std::shared_ptr<Bar>>
    requires( std::is_integral_v<N> && _details::traits::is_iterable_bar<Bar>::value
              && std::is_constructible_v<Bar, Options && ...> )
#else
    typename std::enable_if<_details::traits::AllOf<std::is_integral<N>,
                                                    _details::traits::is_iterable_bar<Bar>,
                                                    std::is_constructible<Bar, Options&&...>>::value,
                            slice::TrackedSpan<slice::NumericSpan<N>, std::shared_ptr<Bar>>>::type
#endif
  {
    return { { endpoint }, std::make_shared<Bar>( std::forward<Options>( options )... ) };
  }
  template<typename Bar, typename N, typename Act, typename... Options>
  PGBAR__NODISCARD PGBAR__FORCEINLINE auto iterate( N endpoint, Act&& act, Options&&... options ) ->
#ifdef __cpp_concepts
    slice::TrackedSpan<slice::NumericSpan<N>, std::shared_ptr<Bar>>
    requires(
      std::is_integral_v<N> && _details::traits::is_iterable_bar<Bar>::value
      && _details::traits::is_reactive_bar<Bar>::value
      && std::is_same_v<std::remove_reference_t<decltype( std::declval<Bar>() | std::forward<Act>( act ) )>,
                        Bar>
      && std::is_constructible_v<Bar, Options && ...> )
#else
    typename std::enable_if<
      _details::traits::AllOf<
        std::is_integral<N>,
        _details::traits::is_iterable_bar<Bar>,
        _details::traits::is_reactive_bar<Bar>,
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
  PGBAR__FORCEINLINE auto iterate( N endpoint, Proc&& op, Options&&... options )
#ifdef __cpp_concepts
    requires( std::is_integral_v<N> && _details::traits::is_iterable_bar<Bar>::value
              && std::is_constructible_v<Bar, Options && ...> && requires( N ele, Proc&& op ) { op( ele ); } )
#else
    -> typename std::enable_if<_details::traits::AllOf<
      std::is_integral<N>,
      _details::traits::is_iterable_bar<Bar>,
      std::is_constructible<Bar, Options&&...>,
      std::is_void<decltype( std::declval<Proc&&>( std::declval<N>() ), void() )>>::value>::type
#endif
  {
    Bar( std::forward<Options>( options )... ).iterate( endpoint, std::forward<Proc>( op ) );
  }
  template<typename Bar, typename N, typename Proc, typename Act, typename... Options>
  PGBAR__FORCEINLINE auto iterate( N endpoint, Proc&& op, Act&& act, Options&&... options )
#ifdef __cpp_concepts
    requires(
      std::is_integral_v<N> && _details::traits::is_iterable_bar<Bar>::value
      && _details::traits::is_reactive_bar<Bar>::value
      && std::is_same_v<std::remove_reference_t<decltype( std::declval<Bar>() | std::forward<Act>( act ) )>,
                        Bar>
      && std::is_constructible_v<Bar, Options && ...> )
#else
    -> typename std::enable_if<_details::traits::AllOf<
      std::is_integral<N>,
      _details::traits::is_iterable_bar<Bar>,
      _details::traits::is_reactive_bar<Bar>,
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
           Channel Outlet = Channel::Stderr,
           Policy Mode    = Policy::Async,
           Region Area    = Region::Fixed,
           typename N,
           typename... Options
#ifdef __cpp_concepts
           >
    requires(
      std::is_integral_v<N> && _details::traits::is_config<Config>::value
      && _details::traits::is_iterable_bar<_details::prefabs::BasicBar<Config, Outlet, Mode, Area>>::value
      && std::is_constructible_v<Config, Options && ...> )
#else
           ,
           typename std::enable_if<
             _details::traits::AllOf<std::is_integral<N>, _details::traits::is_config<Config>>::value,
             bool>::type = 0>
#endif
  PGBAR__NODISCARD PGBAR__FORCEINLINE auto iterate( N endpoint, Options&&... options )
#ifndef __cpp_concepts
    -> typename std::enable_if<
      _details::traits::AllOf<
        _details::traits::is_iterable_bar<_details::prefabs::BasicBar<Config, Outlet, Mode, Area>>,
        std::is_constructible<Config, Options&&...>>::value,
      slice::TrackedSpan<slice::NumericSpan<N>,
                         std::shared_ptr<_details::prefabs::BasicBar<Config, Outlet, Mode, Area>>>>::type
#endif
  {
    return iterate<_details::prefabs::BasicBar<Config, Outlet, Mode, Area>>(
      endpoint,
      std::forward<Options>( options )... );
  }
  template<typename Config,
           Channel Outlet = Channel::Stderr,
           Policy Mode    = Policy::Async,
           Region Area    = Region::Fixed,
           typename N,
           typename Act,
           typename... Options
#ifdef __cpp_concepts
           >
    requires(
      std::is_integral_v<N> && _details::traits::is_config<Config>::value
      && _details::traits::is_iterable_bar<_details::prefabs::BasicBar<Config, Outlet, Mode, Area>>::value
      && _details::traits::is_reactive_bar<_details::prefabs::BasicBar<Config, Outlet, Mode, Area>>::value
      && std::is_same_v<std::remove_reference_t<
                          decltype( std::declval<_details::prefabs::BasicBar<Config, Outlet, Mode, Area>>()
                                    | std::declval<Act &&>() )>,
                        _details::prefabs::BasicBar<Config, Outlet, Mode, Area>>
      && std::is_constructible_v<Config, Options && ...> )
#else
           ,
           typename std::enable_if<
             _details::traits::AllOf<std::is_integral<N>, _details::traits::is_config<Config>>::value,
             bool>::type = 0>
#endif
  PGBAR__NODISCARD PGBAR__FORCEINLINE auto iterate( N endpoint, Act&& act, Options&&... options )
#ifndef __cpp_concepts
    -> typename std::enable_if<
      _details::traits::AllOf<
        _details::traits::is_iterable_bar<_details::prefabs::BasicBar<Config, Outlet, Mode, Area>>,
        _details::traits::is_reactive_bar<_details::prefabs::BasicBar<Config, Outlet, Mode, Area>>,
        std::is_same<typename std::remove_reference<
                       decltype( std::declval<_details::prefabs::BasicBar<Config, Outlet, Mode, Area>>()
                                 | std::declval<Act&&>() )>::type,
                     _details::prefabs::BasicBar<Config, Outlet, Mode, Area>>,
        std::is_constructible<Config, Options&&...>>::value,
      slice::TrackedSpan<slice::NumericSpan<N>,
                         std::shared_ptr<_details::prefabs::BasicBar<Config, Outlet, Mode, Area>>>>::type
#endif
  {
    return iterate<_details::prefabs::BasicBar<Config, Outlet, Mode, Area>>(
      endpoint,
      std::forward<Act>( act ),
      std::forward<Options>( options )... );
  }
  template<typename Config,
           Channel Outlet = Channel::Stderr,
           Policy Mode    = Policy::Async,
           Region Area    = Region::Fixed,
           typename N,
           typename Proc,
           typename... Options
#ifdef __cpp_concepts
           >
    requires(
      std::is_integral_v<N> && _details::traits::is_config<Config>::value
      && _details::traits::is_iterable_bar<_details::prefabs::BasicBar<Config, Outlet, Mode, Area>>::value
      && std::is_constructible_v<Config, Options && ...> && requires( N ele, Proc&& op ) { op( ele ); } )
#else
           ,
           typename std::enable_if<
             _details::traits::AllOf<std::is_integral<N>, _details::traits::is_config<Config>>::value,
             bool>::type = 0>
#endif
  PGBAR__FORCEINLINE auto iterate( N endpoint, Proc&& op, Options&&... options )
#ifndef __cpp_concepts
    -> typename std::enable_if<_details::traits::AllOf<
      _details::traits::is_iterable_bar<_details::prefabs::BasicBar<Config, Outlet, Mode, Area>>,
      std::is_constructible<Config, Options&&...>,
      std::is_void<decltype( std::declval<Proc&&>( std::declval<N>() ), void() )>>::value>::type
#endif
  {
    iterate<_details::prefabs::BasicBar<Config, Outlet, Mode, Area>>( endpoint,
                                                                      std::forward<Proc>( op ),
                                                                      std::forward<Options>( options )... );
  }
  template<typename Config,
           Channel Outlet = Channel::Stderr,
           Policy Mode    = Policy::Async,
           Region Area    = Region::Fixed,
           typename N,
           typename Proc,
           typename Act,
           typename... Options
#ifdef __cpp_concepts
           >
    requires(
      std::is_integral_v<N> && _details::traits::is_config<Config>::value
      && _details::traits::is_iterable_bar<_details::prefabs::BasicBar<Config, Outlet, Mode, Area>>::value
      && _details::traits::is_reactive_bar<_details::prefabs::BasicBar<Config, Outlet, Mode, Area>>::value
      && std::is_same_v<std::remove_reference_t<
                          decltype( std::declval<_details::prefabs::BasicBar<Config, Outlet, Mode, Area>>()
                                    | std::declval<Act &&>() )>,
                        _details::prefabs::BasicBar<Config, Outlet, Mode, Area>>
      && std::is_constructible_v<Config, Options && ...> )
#else
           ,
           typename std::enable_if<
             _details::traits::AllOf<std::is_integral<N>, _details::traits::is_config<Config>>::value,
             bool>::type = 0>
#endif
  PGBAR__FORCEINLINE auto iterate( N endpoint, Proc&& op, Act&& act, Options&&... options )
#ifndef __cpp_concepts
    -> typename std::enable_if<_details::traits::AllOf<
      _details::traits::is_iterable_bar<_details::prefabs::BasicBar<Config, Outlet, Mode, Area>>,
      _details::traits::is_reactive_bar<_details::prefabs::BasicBar<Config, Outlet, Mode, Area>>,
      std::is_same<typename std::remove_reference<
                     decltype( std::declval<_details::prefabs::BasicBar<Config, Outlet, Mode, Area>>()
                               | std::declval<Act&&>() )>::type,
                   _details::prefabs::BasicBar<Config, Outlet, Mode, Area>>,
      std::is_constructible<Config, Options&&...>>::value>::type
#endif
  {
    iterate<_details::prefabs::BasicBar<Config, Outlet, Mode, Area>>( endpoint,
                                                                      std::forward<Proc>( op ),
                                                                      std::forward<Act>( act ),
                                                                      std::forward<Options>( options )... );
  }

  //////////////////////////////////////////////////////////////////////////////////////////////////////////

  template<typename Bar, typename Itr, typename Snt, typename... Options>
  PGBAR__NODISCARD PGBAR__FORCEINLINE auto iterate( Itr startpoint, Snt endpoint, Options&&... options ) ->
#ifdef __cpp_concepts
    slice::TrackedSpan<slice::IteratorSpan<Itr, Snt>, std::shared_ptr<Bar>>
    requires( _details::traits::is_sized_cursor<Itr, Snt>::value
              && _details::traits::is_iterable_bar<Bar>::value
              && std::is_constructible_v<Bar, Options && ...> )
#else
    typename std::enable_if<_details::traits::AllOf<_details::traits::is_sized_cursor<Itr, Snt>,
                                                    _details::traits::is_iterable_bar<Bar>,
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
  PGBAR__NODISCARD PGBAR__FORCEINLINE auto iterate( Itr startpoint,
                                                    Snt endpoint,
                                                    Act&& act,
                                                    Options&&... options ) ->
#ifdef __cpp_concepts
    slice::TrackedSpan<slice::IteratorSpan<Itr, Snt>, std::shared_ptr<Bar>>
    requires(
      _details::traits::is_sized_cursor<Itr, Snt>::value && _details::traits::is_iterable_bar<Bar>::value
      && _details::traits::is_reactive_bar<Bar>::value
      && std::is_same_v<std::remove_reference_t<decltype( std::declval<Bar>() | std::forward<Act>( act ) )>,
                        Bar>
      && std::is_constructible_v<Bar, Options && ...> )
#else
    typename std::enable_if<
      _details::traits::AllOf<
        _details::traits::is_sized_cursor<Itr, Snt>,
        _details::traits::is_iterable_bar<Bar>,
        _details::traits::is_reactive_bar<Bar>,
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
  PGBAR__FORCEINLINE auto iterate( Itr startpoint, Snt endpoint, Proc&& op, Options&&... options )
#ifdef __cpp_concepts
    requires( _details::traits::is_sized_cursor<Itr, Snt>::value
              && _details::traits::is_iterable_bar<Bar>::value && std::is_constructible_v<Bar, Options && ...>
              && requires( _details::traits::IterValue_t<Itr> ele, Proc&& op ) { op( ele ); } )
#else
    -> typename std::enable_if<_details::traits::AllOf<
      _details::traits::is_sized_cursor<Itr, Snt>,
      _details::traits::is_iterable_bar<Bar>,
      std::is_constructible<Bar, Options&&...>,
      std::is_void<decltype( std::declval<Proc&&>( std::declval<_details::traits::IterValue_t<Itr>>() ),
                             void() )>>::value>::type
#endif
  {
    Bar( std::forward<Options>( options )... )
      .iterate( std::move( startpoint ), std::move( endpoint ), std::forward<Proc>( op ) );
  }
  template<typename Bar, typename Itr, typename Snt, typename Proc, typename Act, typename... Options>
  PGBAR__FORCEINLINE auto iterate( Itr startpoint, Snt endpoint, Proc&& op, Act&& act, Options&&... options )
#ifdef __cpp_concepts
    requires(
      _details::traits::is_sized_cursor<Itr, Snt>::value && _details::traits::is_iterable_bar<Bar>::value
      && _details::traits::is_reactive_bar<Bar>::value
      && std::is_same_v<std::remove_reference_t<decltype( std::declval<Bar>() | std::forward<Act>( act ) )>,
                        Bar>
      && std::is_constructible_v<Bar, Options && ...> )
#else
    -> typename std::enable_if<_details::traits::AllOf<
      _details::traits::is_sized_cursor<Itr, Snt>,
      _details::traits::is_iterable_bar<Bar>,
      _details::traits::is_reactive_bar<Bar>,
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
           Channel Outlet = Channel::Stderr,
           Policy Mode    = Policy::Async,
           Region Area    = Region::Fixed,
           typename Itr,
           typename Snt,
           typename... Options
#ifdef __cpp_concepts
           >
    requires(
      _details::traits::is_sized_cursor<Itr, Snt>::value && _details::traits::is_config<Config>::value
      && _details::traits::is_iterable_bar<_details::prefabs::BasicBar<Config, Outlet, Mode, Area>>::value
      && std::is_constructible_v<Config, Options && ...> )
#else
           ,
           typename std::enable_if<_details::traits::AllOf<_details::traits::is_sized_cursor<Itr, Snt>,
                                                           _details::traits::is_config<Config>>::value,
                                   bool>::type = 0>
#endif
  PGBAR__NODISCARD PGBAR__FORCEINLINE auto iterate( Itr startpoint, Snt endpoint, Options&&... options )
#ifndef __cpp_concepts
    -> typename std::enable_if<
      _details::traits::AllOf<
        _details::traits::is_iterable_bar<_details::prefabs::BasicBar<Config, Outlet, Mode, Area>>,
        std::is_constructible<Config, Options&&...>>::value,
      slice::TrackedSpan<slice::IteratorSpan<Itr, Snt>,
                         std::shared_ptr<_details::prefabs::BasicBar<Config, Outlet, Mode, Area>>>>::type
#endif
  {
    return iterate<_details::prefabs::BasicBar<Config, Outlet, Mode, Area>>(
      std::move( startpoint ),
      std::move( endpoint ),
      std::forward<Options>( options )... );
  }
  template<typename Config,
           Channel Outlet = Channel::Stderr,
           Policy Mode    = Policy::Async,
           Region Area    = Region::Fixed,
           typename Itr,
           typename Snt,
           typename Act,
           typename... Options
#ifdef __cpp_concepts
           >
    requires(
      _details::traits::is_sized_cursor<Itr, Snt>::value && _details::traits::is_config<Config>::value
      && _details::traits::is_iterable_bar<_details::prefabs::BasicBar<Config, Outlet, Mode, Area>>::value
      && _details::traits::is_reactive_bar<_details::prefabs::BasicBar<Config, Outlet, Mode, Area>>::value
      && std::is_same_v<std::remove_reference_t<
                          decltype( std::declval<_details::prefabs::BasicBar<Config, Outlet, Mode, Area>>()
                                    | std::declval<Act &&>() )>,
                        _details::prefabs::BasicBar<Config, Outlet, Mode, Area>>
      && std::is_constructible_v<Config, Options && ...> )
#else
           ,
           typename std::enable_if<_details::traits::AllOf<_details::traits::is_sized_cursor<Itr, Snt>,
                                                           _details::traits::is_config<Config>>::value,
                                   bool>::type = 0>
#endif
  PGBAR__NODISCARD PGBAR__FORCEINLINE auto iterate( Itr startpoint,
                                                    Snt endpoint,
                                                    Act&& act,
                                                    Options&&... options )
#ifndef __cpp_concepts
    -> typename std::enable_if<
      _details::traits::AllOf<
        _details::traits::is_iterable_bar<_details::prefabs::BasicBar<Config, Outlet, Mode, Area>>,
        _details::traits::is_reactive_bar<_details::prefabs::BasicBar<Config, Outlet, Mode, Area>>,
        std::is_same<typename std::remove_reference<
                       decltype( std::declval<_details::prefabs::BasicBar<Config, Outlet, Mode, Area>>()
                                 | std::declval<Act&&>() )>::type,
                     _details::prefabs::BasicBar<Config, Outlet, Mode, Area>>,
        std::is_constructible<Config, Options&&...>>::value,
      slice::TrackedSpan<slice::IteratorSpan<Itr, Snt>,
                         std::shared_ptr<_details::prefabs::BasicBar<Config, Outlet, Mode, Area>>>>::type
#endif
  {
    return iterate<_details::prefabs::BasicBar<Config, Outlet, Mode, Area>>(
      std::move( startpoint ),
      std::move( endpoint ),
      std::forward<Act>( act ),
      std::forward<Options>( options )... );
  }
  template<typename Config,
           Channel Outlet = Channel::Stderr,
           Policy Mode    = Policy::Async,
           Region Area    = Region::Fixed,
           typename Itr,
           typename Snt,
           typename Proc,
           typename... Options
#ifdef __cpp_concepts
           >
    requires(
      _details::traits::is_sized_cursor<Itr, Snt>::value && _details::traits::is_config<Config>::value
      && _details::traits::is_iterable_bar<_details::prefabs::BasicBar<Config, Outlet, Mode, Area>>::value
      && std::is_constructible_v<Config, Options && ...>
      && requires( _details::traits::IterValue_t<Itr> ele, Proc&& op ) { op( ele ); } )
#else
           ,
           typename std::enable_if<_details::traits::AllOf<_details::traits::is_sized_cursor<Itr, Snt>,
                                                           _details::traits::is_config<Config>>::value,
                                   bool>::type = 0>
#endif
  PGBAR__FORCEINLINE auto iterate( Itr startpoint, Snt endpoint, Proc&& op, Options&&... options )
#ifndef __cpp_concepts
    -> typename std::enable_if<_details::traits::AllOf<
      _details::traits::is_iterable_bar<_details::prefabs::BasicBar<Config, Outlet, Mode, Area>>,
      std::is_constructible<Config, Options&&...>,
      std::is_void<decltype( std::declval<Proc&&>( std::declval<_details::traits::IterValue_t<Itr>>() ),
                             void() )>>::value>::type
#endif
  {
    iterate<_details::prefabs::BasicBar<Config, Outlet, Mode, Area>>( std::move( startpoint ),
                                                                      std::move( endpoint ),
                                                                      std::forward<Proc>( op ),
                                                                      std::forward<Options>( options )... );
  }
  template<typename Config,
           Channel Outlet = Channel::Stderr,
           Policy Mode    = Policy::Async,
           Region Area    = Region::Fixed,
           typename Itr,
           typename Snt,
           typename Proc,
           typename Act,
           typename... Options
#ifdef __cpp_concepts
           >
    requires(
      _details::traits::is_sized_cursor<Itr, Snt>::value && _details::traits::is_config<Config>::value
      && _details::traits::is_iterable_bar<_details::prefabs::BasicBar<Config, Outlet, Mode, Area>>::value
      && _details::traits::is_reactive_bar<_details::prefabs::BasicBar<Config, Outlet, Mode, Area>>::value
      && std::is_same_v<std::remove_reference_t<
                          decltype( std::declval<_details::prefabs::BasicBar<Config, Outlet, Mode, Area>>()
                                    | std::declval<Act &&>() )>,
                        _details::prefabs::BasicBar<Config, Outlet, Mode, Area>>
      && std::is_constructible_v<Config, Options && ...> )
#else
           ,
           typename std::enable_if<_details::traits::AllOf<_details::traits::is_sized_cursor<Itr, Snt>,
                                                           _details::traits::is_config<Config>>::value,
                                   bool>::type = 0>
#endif
  PGBAR__FORCEINLINE auto iterate( Itr startpoint, Snt endpoint, Proc&& op, Act&& act, Options&&... options )
#ifndef __cpp_concepts
    -> typename std::enable_if<_details::traits::AllOf<
      _details::traits::is_iterable_bar<_details::prefabs::BasicBar<Config, Outlet, Mode, Area>>,
      _details::traits::is_reactive_bar<_details::prefabs::BasicBar<Config, Outlet, Mode, Area>>,
      std::is_same<typename std::remove_reference<
                     decltype( std::declval<_details::prefabs::BasicBar<Config, Outlet, Mode, Area>>()
                               | std::declval<Act&&>() )>::type,
                   _details::prefabs::BasicBar<Config, Outlet, Mode, Area>>,
      std::is_constructible<Config, Options&&...>>::value>::type
#endif
  {
    iterate<_details::prefabs::BasicBar<Config, Outlet, Mode, Area>>( std::move( startpoint ),
                                                                      std::move( endpoint ),
                                                                      std::forward<Proc>( op ),
                                                                      std::forward<Act>( act ),
                                                                      std::forward<Options>( options )... );
  }

  //////////////////////////////////////////////////////////////////////////////////////////////////////////

  template<typename Bar, class R, typename... Options>
  PGBAR__NODISCARD PGBAR__FORCEINLINE auto iterate( R&& range, Options&&... options ) ->
#ifdef __cpp_concepts
    slice::TrackedSpan<slice::BoundedSpan<std::remove_reference_t<R>>, std::shared_ptr<Bar>>
    requires( _details::traits::is_bounded_range<R>::value && !std::ranges::view<std::remove_reference_t<R>>
              && _details::traits::is_iterable_bar<Bar>::value
              && std::is_constructible_v<Bar, Options && ...> )
#else
    typename std::enable_if<_details::traits::AllOf<_details::traits::is_bounded_range<R>,
                                                    _details::traits::is_iterable_bar<Bar>,
                                                    std::is_constructible<Bar, Options&&...>>::value,
                            slice::TrackedSpan<slice::BoundedSpan<typename std::remove_reference<R>::type>,
                                               std::shared_ptr<Bar>>>::type
#endif
  {
    return { { std::forward<R>( range ) }, std::make_shared<Bar>( std::forward<Options>( options )... ) };
  }
  template<typename Bar, class R, typename Act, typename... Options>
  PGBAR__NODISCARD PGBAR__FORCEINLINE auto iterate( R&& range, Act&& act, Options&&... options ) ->
#ifdef __cpp_concepts
    slice::TrackedSpan<slice::BoundedSpan<std::remove_reference_t<R>>, std::shared_ptr<Bar>>
    requires(
      _details::traits::is_bounded_range<R>::value && !std::ranges::view<std::remove_reference_t<R>>
      && _details::traits::is_iterable_bar<Bar>::value && _details::traits::is_reactive_bar<Bar>::value
      && std::is_same_v<std::remove_reference_t<decltype( std::declval<Bar>() | std::forward<Act>( act ) )>,
                        Bar>
      && std::is_constructible_v<Bar, Options && ...> )
#else
    typename std::enable_if<
      _details::traits::AllOf<
        _details::traits::is_bounded_range<R>,
        _details::traits::is_iterable_bar<Bar>,
        _details::traits::is_reactive_bar<Bar>,
        std::is_same<
          typename std::remove_reference<decltype( std::declval<Bar>() | std::forward<Act>( act ) )>::type,
          Bar>,
        std::is_constructible<Bar, Options&&...>>::value,
      slice::TrackedSpan<slice::BoundedSpan<typename std::remove_reference<R>::type>,
                         std::shared_ptr<Bar>>>::type
#endif
  {
    return { { std::forward<R>( range ) },
             std::make_shared<Bar>( Bar( std::forward<Options>( options )... ) | std::forward<Act>( act ) ) };
  }
#ifdef __cpp_concepts
  template<typename Bar, class R, typename... Options>
    requires( _details::traits::is_bounded_range<R>::value
              && std::ranges::view<R> && _details::traits::is_iterable_bar<Bar>::value
              && std::is_constructible_v<Bar, Options && ...> )
  PGBAR__NODISCARD PGBAR__FORCEINLINE
    slice::TrackedSpan<R, std::shared_ptr<Bar>> iterate( R view, Options&&... options )
  {
    return { std::move( view ), std::make_shared<Bar>( std::forward<Options>( options )... ) };
  }
  template<typename Bar, class R, typename Act, typename... Options>
  PGBAR__NODISCARD PGBAR__FORCEINLINE slice::TrackedSpan<R, std::shared_ptr<Bar>>
    iterate( R view, Act&& act, Options&&... options )
    requires(
      _details::traits::is_bounded_range<R>::value && std::ranges::view<R>
      && _details::traits::is_iterable_bar<Bar>::value && _details::traits::is_reactive_bar<Bar>::value
      && std::is_same_v<std::remove_reference_t<decltype( std::declval<Bar>() | std::forward<Act>( act ) )>,
                        Bar>
      && std::is_constructible_v<Bar, Options && ...> )
  {
    return { std::move( view ),
             std::make_shared<Bar>( Bar( std::forward<Options>( options )... ) | std::forward<Act>( act ) ) };
  }
#endif
  template<typename Bar, class R, typename Proc, typename... Options>
  PGBAR__FORCEINLINE auto iterate( R&& range, Proc&& op, Options&&... options )
#ifdef __cpp_concepts
    requires( _details::traits::is_bounded_range<R>::value && _details::traits::is_iterable_bar<Bar>::value
              && std::is_constructible_v<Bar, Options && ...>
              && requires( _details::traits::IterValue_t<_details::traits::IteratorOf_t<R>> ele, Proc&& op ) {
                   op( ele );
                 } )
#else
    -> typename std::enable_if<_details::traits::AllOf<
      _details::traits::is_bounded_range<R>,
      _details::traits::is_iterable_bar<Bar>,
      std::is_constructible<Bar, Options&&...>,
      std::is_void<decltype( std::declval<Proc&&>( std::declval<_details::traits::IterValue_t<
                                                     _details::traits::IteratorOf_t<R>>>() ),
                             void() )>>::value>::type
#endif
  {
    Bar( std::forward<Options>( options )... ).iterate( std::forward<R>( range ), std::forward<Proc>( op ) );
  }
  template<typename Bar, class R, typename Proc, typename Act, typename... Options>
  PGBAR__FORCEINLINE auto iterate( R&& range, Proc&& op, Act&& act, Options&&... options )
#ifdef __cpp_concepts
    requires(
      _details::traits::is_bounded_range<R>::value && _details::traits::is_iterable_bar<Bar>::value
      && _details::traits::is_reactive_bar<Bar>::value
      && std::is_same_v<std::remove_reference_t<decltype( std::declval<Bar>() | std::forward<Act>( act ) )>,
                        Bar>
      && std::is_constructible_v<Bar, Options && ...> )
#else
    -> typename std::enable_if<_details::traits::AllOf<
      _details::traits::is_bounded_range<R>,
      _details::traits::is_iterable_bar<Bar>,
      _details::traits::is_reactive_bar<Bar>,
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
           Channel Outlet = Channel::Stderr,
           Policy Mode    = Policy::Async,
           Region Area    = Region::Fixed,
           class R,
           typename... Options
#ifdef __cpp_concepts
           >
    requires(
      _details::traits::is_bounded_range<R>::value
      && !std::ranges::view<std::remove_reference_t<R>> && _details::traits::is_config<Config>::value
      && _details::traits::is_iterable_bar<_details::prefabs::BasicBar<Config, Outlet, Mode, Area>>::value
      && std::is_constructible_v<Config, Options && ...> )
#else
           ,
           typename std::enable_if<_details::traits::AllOf<_details::traits::is_bounded_range<R>,
                                                           _details::traits::is_config<Config>>::value,
                                   bool>::type = 0>
#endif
  PGBAR__NODISCARD PGBAR__FORCEINLINE auto iterate( R&& range, Options&&... options )
#ifndef __cpp_concepts
    -> typename std::enable_if<
      _details::traits::AllOf<
        _details::traits::is_iterable_bar<_details::prefabs::BasicBar<Config, Outlet, Mode, Area>>,
        std::is_constructible<Config, Options&&...>>::value,
      slice::TrackedSpan<slice::BoundedSpan<typename std::remove_reference<R>::type>,
                         std::shared_ptr<_details::prefabs::BasicBar<Config, Outlet, Mode, Area>>>>::type
#endif
  {
    return iterate<_details::prefabs::BasicBar<Config, Outlet, Mode, Area>>(
      std::forward<R>( range ),
      std::forward<Options>( options )... );
  }
  template<typename Config,
           Channel Outlet = Channel::Stderr,
           Policy Mode    = Policy::Async,
           Region Area    = Region::Fixed,
           class R,
           typename Act,
           typename... Options
#ifdef __cpp_concepts
           >
    requires(
      _details::traits::is_bounded_range<R>::value
      && !std::ranges::view<std::remove_reference_t<R>> && _details::traits::is_config<Config>::value
      && _details::traits::is_iterable_bar<_details::prefabs::BasicBar<Config, Outlet, Mode, Area>>::value
      && _details::traits::is_reactive_bar<_details::prefabs::BasicBar<Config, Outlet, Mode, Area>>::value
      && std::is_same_v<std::remove_reference_t<
                          decltype( std::declval<_details::prefabs::BasicBar<Config, Outlet, Mode, Area>>()
                                    | std::declval<Act &&>() )>,
                        _details::prefabs::BasicBar<Config, Outlet, Mode, Area>>
      && std::is_constructible_v<Config, Options && ...> )
#else
           ,
           typename std::enable_if<_details::traits::AllOf<_details::traits::is_bounded_range<R>,
                                                           _details::traits::is_config<Config>>::value,
                                   bool>::type = 0>
#endif
  PGBAR__NODISCARD PGBAR__FORCEINLINE auto iterate( R&& range, Act&& act, Options&&... options )
#ifndef __cpp_concepts
    -> typename std::enable_if<
      _details::traits::AllOf<
        _details::traits::is_iterable_bar<_details::prefabs::BasicBar<Config, Outlet, Mode, Area>>,
        _details::traits::is_reactive_bar<_details::prefabs::BasicBar<Config, Outlet, Mode, Area>>,
        std::is_same<typename std::remove_reference<
                       decltype( std::declval<_details::prefabs::BasicBar<Config, Outlet, Mode, Area>>()
                                 | std::declval<Act&&>() )>::type,
                     _details::prefabs::BasicBar<Config, Outlet, Mode, Area>>,
        std::is_constructible<Config, Options&&...>>::value,
      slice::TrackedSpan<slice::BoundedSpan<typename std::remove_reference<R>::type>,
                         std::shared_ptr<_details::prefabs::BasicBar<Config, Outlet, Mode, Area>>>>::type
#endif
  {
    return iterate<_details::prefabs::BasicBar<Config, Outlet, Mode, Area>>(
      std::forward<R>( range ),
      std::forward<Act>( act ),
      std::forward<Options>( options )... );
  }
#ifdef __cpp_concepts
  template<typename Config,
           Channel Outlet = Channel::Stderr,
           Policy Mode    = Policy::Async,
           Region Area    = Region::Fixed,
           class R,
           typename... Options>
    requires(
      _details::traits::is_bounded_range<R>::value
      && std::ranges::view<R> && _details::traits::is_config<Config>::value
      && _details::traits::is_iterable_bar<_details::prefabs::BasicBar<Config, Outlet, Mode, Area>>::value
      && std::is_constructible_v<Config, Options && ...> )
  PGBAR__NODISCARD PGBAR__FORCEINLINE
    slice::TrackedSpan<R, std::shared_ptr<_details::prefabs::BasicBar<Config, Outlet, Mode, Area>>> iterate(
      R view,
      Options&&... options )
  {
    return { std::move( view ),
             std::make_shared<_details::prefabs::BasicBar<Config, Outlet, Mode, Area>>(
               std::forward<Options>( options )... ) };
  }
  template<typename Config,
           Channel Outlet = Channel::Stderr,
           Policy Mode    = Policy::Async,
           Region Area    = Region::Fixed,
           class R,
           typename Act,
           typename... Options>
  PGBAR__NODISCARD PGBAR__FORCEINLINE
    slice::TrackedSpan<R, std::shared_ptr<_details::prefabs::BasicBar<Config, Outlet, Mode, Area>>>
    iterate( R view, Act&& act, Options&&... options )
    requires(
      _details::traits::is_bounded_range<R>::value
      && std::ranges::view<R> && _details::traits::is_config<Config>::value
      && _details::traits::is_iterable_bar<_details::prefabs::BasicBar<Config, Outlet, Mode, Area>>::value
      && _details::traits::is_reactive_bar<_details::prefabs::BasicBar<Config, Outlet, Mode, Area>>::value
      && std::is_same_v<std::remove_reference_t<
                          decltype( std::declval<_details::prefabs::BasicBar<Config, Outlet, Mode, Area>>()
                                    | std::forward<Act>( act ) )>,
                        _details::prefabs::BasicBar<Config, Outlet, Mode, Area>>
      && std::is_constructible_v<Config, Options && ...> )
  {
    return { std::move( view ),
             std::make_shared<_details::prefabs::BasicBar<Config, Outlet, Mode, Area>>(
               Bar( std::forward<Options>( options )... ) | std::forward<Act>( act ) ) };
  }
#endif
  template<typename Config,
           Channel Outlet = Channel::Stderr,
           Policy Mode    = Policy::Async,
           Region Area    = Region::Fixed,
           class R,
           typename Proc,
           typename... Options
#ifdef __cpp_concepts
           >
    requires(
      _details::traits::is_bounded_range<R>::value && _details::traits::is_config<Config>::value
      && _details::traits::is_iterable_bar<_details::prefabs::BasicBar<Config, Outlet, Mode, Area>>::value
      && std::is_constructible_v<Config, Options && ...>
      && requires( _details::traits::IterValue_t<_details::traits::IteratorOf_t<R>> ele,
                   Proc&& op ) { op( ele ); } )
#else
           ,
           typename std::enable_if<_details::traits::AllOf<_details::traits::is_bounded_range<R>,
                                                           _details::traits::is_config<Config>>::value,
                                   bool>::type = 0>
#endif
  PGBAR__FORCEINLINE auto iterate( R&& range, Proc&& op, Options&&... options )
#ifndef __cpp_concepts
    -> typename std::enable_if<_details::traits::AllOf<
      _details::traits::is_iterable_bar<_details::prefabs::BasicBar<Config, Outlet, Mode, Area>>,
      std::is_constructible<Config, Options&&...>,
      std::is_void<decltype( std::declval<Proc&&>( std::declval<_details::traits::IterValue_t<
                                                     _details::traits::IteratorOf_t<R>>>() ),
                             void() )>>::value>::type
#endif
  {
    iterate<_details::prefabs::BasicBar<Config, Outlet, Mode, Area>>( std::forward<R>( range ),
                                                                      std::forward<Proc>( op ),
                                                                      std::forward<Options>( options )... );
  }
  template<typename Config,
           Channel Outlet = Channel::Stderr,
           Policy Mode    = Policy::Async,
           Region Area    = Region::Fixed,
           class R,
           typename Proc,
           typename Act,
           typename... Options
#ifdef __cpp_concepts
           >
    requires(
      _details::traits::is_bounded_range<R>::value && _details::traits::is_config<Config>::value
      && _details::traits::is_iterable_bar<_details::prefabs::BasicBar<Config, Outlet, Mode, Area>>::value
      && _details::traits::is_reactive_bar<_details::prefabs::BasicBar<Config, Outlet, Mode, Area>>::value
      && std::is_same_v<std::remove_reference_t<
                          decltype( std::declval<_details::prefabs::BasicBar<Config, Outlet, Mode, Area>>()
                                    | std::declval<Act &&>() )>,
                        _details::prefabs::BasicBar<Config, Outlet, Mode, Area>>
      && std::is_constructible_v<Config, Options && ...> )
#else
           ,
           typename std::enable_if<_details::traits::AllOf<_details::traits::is_bounded_range<R>,
                                                           _details::traits::is_config<Config>>::value,
                                   bool>::type = 0>
#endif
  PGBAR__FORCEINLINE auto iterate( R&& range, Proc&& op, Act&& act, Options&&... options )
#ifndef __cpp_concepts
    -> typename std::enable_if<_details::traits::AllOf<
      _details::traits::is_iterable_bar<_details::prefabs::BasicBar<Config, Outlet, Mode, Area>>,
      _details::traits::is_reactive_bar<_details::prefabs::BasicBar<Config, Outlet, Mode, Area>>,
      std::is_same<typename std::remove_reference<
                     decltype( std::declval<_details::prefabs::BasicBar<Config, Outlet, Mode, Area>>()
                               | std::declval<Act&&>() )>::type,
                   _details::prefabs::BasicBar<Config, Outlet, Mode, Area>>,
      std::is_constructible<Config, Options&&...>>::value>::type
#endif
  {
    iterate<_details::prefabs::BasicBar<Config, Outlet, Mode, Area>>( std::forward<R>( range ),
                                                                      std::forward<Proc>( op ),
                                                                      std::forward<Act>( act ),
                                                                      std::forward<Options>( options )... );
  }
} // namespace pgbar

#endif
