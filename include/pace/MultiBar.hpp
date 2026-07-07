#ifndef PACE_MULTI_BAR
#define PACE_MULTI_BAR

#include "details/assets/StaticLayout.hpp"
#include "details/core/Core.hpp"

namespace pace {
  template<typename Bar, typename... Bars>
  class MultiBar;
  template<Channel S, Policy M, Region Z, typename Config, typename... Configs>
  class MultiBar<prefab::BasicBar<Config, S, M, Z>, prefab::BasicBar<Configs, S, M, Z>...> {
    static_assert( details::traits::AllOf<details::traits::is_config<Config>,
                                          details::traits::is_config<Configs>...>::value,
                   "invalid config type" );

    template<details::types::Size Pos>
    using ConfigAt_t = details::traits::TypeAt_t<Pos, Config, Configs...>;
    template<details::types::Size Pos>
    using BarAt_t = details::traits::
      TypeAt_t<Pos, prefab::BasicBar<Config, S, M, Z>, prefab::BasicBar<Configs, S, M, Z>...>;

    details::assets::StaticLayout<details::traits::MakeIndexSequence<sizeof...( Configs ) + 1>,
                                  prefab::BasicBar<Config, S, M, Z>,
                                  prefab::BasicBar<Configs, S, M, Z>...>
      package_;

  public:
    static constexpr Channel sink = S;
    static constexpr Policy mode  = M;
    static constexpr Region zone  = Z;

    MultiBar() = default;

#ifdef __cpp_concepts
    template<typename Cfg, typename... Cfgs>
      requires(
        sizeof...( Cfgs ) <= sizeof...( Configs )
        && details::traits::TpStartsWith<details::traits::TypeList<std::decay_t<Cfg>, std::decay_t<Cfgs>...>,
                                         Config,
                                         Configs...>::value )
#else
    template<typename Cfg,
             typename... Cfgs,
             typename = typename std::enable_if<details::traits::AllOf<
               details::traits::BoolConstant<( sizeof...( Cfgs ) <= sizeof...( Configs ) )>,
               details::traits::TpStartsWith<details::traits::TypeList<typename std::decay<Cfg>::type,
                                                                       typename std::decay<Cfgs>::type...>,
                                             Config,
                                             Configs...>>::value>::type>
#endif
    MultiBar( Cfg&& cfg, Cfgs&&... cfgs ) noexcept( sizeof...( Cfgs ) == sizeof...( Configs ) )
      : package_ { std::forward<Cfg>( cfg ), std::forward<Cfgs>( cfgs )... }
    {}

    template<typename Cfg, typename... Cfgs
#ifdef __cpp_concepts
             >
      requires(
        sizeof...( Cfgs ) <= sizeof...( Configs )
        && details::traits::TpStartsWith<details::traits::TypeList<Cfg, Cfgs...>, Config, Configs...>::value )
#else
             ,
             typename = typename std::enable_if<details::traits::AllOf<
               details::traits::BoolConstant< ( sizeof...( Cfgs ) <= sizeof...( Configs ) )>,
               details::traits::TpStartsWith<
                 details::traits::TypeList<Cfg,
                 Cfgs...>,
                 Config,
                 Configs...>>::value>::type>
#endif
    MultiBar( prefab::BasicBar<Cfg, S, M, Z>&& bar, prefab::BasicBar<Cfgs, S, M, Z>&&... bars )
      noexcept( sizeof...( Cfgs ) == sizeof...( Configs ) )
      : package_ { std::move( bar ), std::move( bars )... }
    {}

    MultiBar( const MultiBar& )                      = delete;
    MultiBar& operator=( const MultiBar& ) &         = delete;
    MultiBar( MultiBar&& rhs ) noexcept              = default;
    MultiBar& operator=( MultiBar&& rhs ) & noexcept = default;
    ~MultiBar()                                      = default;

    // Reset all the progress bars.
    PACE__FORCEINLINE void reset() { package_.shut(); }
    // Abort all the progress bars.
    PACE__FORCEINLINE void abort() noexcept { package_.kill(); }
    // Returns the number of progress bars.
    PACE__NODISCARD static PACE__FORCEINLINE PACE__CNSTEVAL details::types::Size size() noexcept
    { return sizeof...( Configs ) + 1; }
    // Check whether a progress bar is running
    PACE__NODISCARD PACE__FORCEINLINE bool active() const noexcept { return package_.online(); }
    // Returns the number of progress bars which is running.
    PACE__NODISCARD PACE__FORCEINLINE details::types::Size active_count() const noexcept
    { return package_.online_count(); }

    template<details::types::Size Pos>
    PACE__FORCEINLINE PACE__CXX14_CNSTXPR BarAt_t<Pos>& at() & noexcept
    { return package_.template at<Pos>(); }
    template<details::types::Size Pos>
    PACE__FORCEINLINE PACE__CXX14_CNSTXPR const BarAt_t<Pos>& at() const& noexcept
    { return package_.template at<Pos>(); }
    template<details::types::Size Pos>
    PACE__FORCEINLINE PACE__CXX14_CNSTXPR BarAt_t<Pos>&& at() && noexcept
    { return std::move( package_.template at<Pos>() ); }

    void swap( MultiBar& other ) noexcept { package_.swap( other.package_ ); }
    friend void swap( MultiBar& a, MultiBar& b ) noexcept { a.swap( b ); }

    template<details::types::Size Pos>
    friend PACE__FORCEINLINE PACE__CXX14_CNSTXPR BarAt_t<Pos>& get( MultiBar& self ) noexcept
    { return self.template at<Pos>(); }
    template<details::types::Size Pos>
    friend PACE__FORCEINLINE PACE__CXX14_CNSTXPR const BarAt_t<Pos>& get( const MultiBar& self ) noexcept
    { return self.template at<Pos>(); }
    template<details::types::Size Pos>
    friend PACE__FORCEINLINE PACE__CXX14_CNSTXPR BarAt_t<Pos>&& get( MultiBar&& self ) noexcept
    { return std::move( self ).template at<Pos>(); }
  };

#ifdef __cpp_deduction_guides
  template<Channel S, Policy M, Region Z, typename Cfg, typename... Cfgs>
  MultiBar( prefab::BasicBar<Cfg, S, M, Z>&& bar, prefab::BasicBar<Cfgs, S, M, Z>&&... bars )
    -> MultiBar<prefab::BasicBar<Cfg, S, M, Z>, prefab::BasicBar<Cfgs, S, M, Z>...>;

  // CTAD, only generates the default version,
  // which means the the Sink is `Channel::Stderr` and Mode is `Policy::Async`.
  template<typename Config, typename... Configs
# ifdef __cpp_concepts
           >
    requires( details::traits::is_config<std::decay_t<Config>>::value
              && ( details::traits::is_config<std::decay_t<Configs>>::value && ... ) )
# else
           ,
    typename = std::enable_if_t<details::traits::AllOf<details::traits::is_config<std::decay_t<Config>>,
                                                         details::traits::is_config<std::decay_t<Configs>>...>::value>>
# endif
  MultiBar( Config, Configs... )
    -> MultiBar<prefab::BasicBar<std::decay_t<Config>, Channel::Stderr, Policy::Async, Region::Fixed>,
                prefab::BasicBar<std::decay_t<Configs>, Channel::Stderr, Policy::Async, Region::Fixed>...>;
#endif

  // Generates a MultiBar type containing Count instances of the given Bar type.
  template<typename Bar, details::types::Size Count>
  using MultiBar_t = details::traits::FillWith_t<MultiBar, Bar, Count>;

  // Creates a MultiBar using existing bar instances.
  template<typename Config, typename... Configs, Channel Sink, Policy Mode, Region Zone>
  PACE__NODISCARD PACE__FORCEINLINE auto make_multi(
    prefab::BasicBar<Config, Sink, Mode, Zone>&& bar,
    prefab::BasicBar<Configs, Sink, Mode, Zone>&&... bars ) noexcept
#ifdef __cpp_concepts
    -> MultiBar<prefab::BasicBar<Config, Sink, Mode, Zone>, prefab::BasicBar<Configs, Sink, Mode, Zone>...>
    requires( details::traits::is_config<Config>::value
              && ( details::traits::is_config<Configs>::value && ... ) )
#else
    -> typename std::enable_if<details::traits::AllOf<details::traits::is_config<Config>,
                                                      details::traits::is_config<Configs>...>::value,
                               MultiBar<prefab::BasicBar<Config, Sink, Mode, Zone>,
                                        prefab::BasicBar<Configs, Sink, Mode, Zone>...>>::type
#endif
  { return { std::move( bar ), std::move( bars )... }; }
  // Creates a MultiBar using configuration objects.
  template<Channel Sink = Channel::Stderr,
           Policy Mode  = Policy::Async,
           Region Zone  = Region::Fixed,
           typename Config,
           typename... Configs>
  PACE__NODISCARD PACE__FORCEINLINE auto make_multi( Config&& cfg, Configs&&... cfgs )
    noexcept( details::traits::Not<details::traits::AnyOf<std::is_lvalue_reference<Config&&>,
                                                          std::is_lvalue_reference<Configs&&>...>>::value )
#ifdef __cpp_concepts
      -> MultiBar<prefab::BasicBar<std::decay_t<Config>, Sink, Mode, Zone>,
                  prefab::BasicBar<std::decay_t<Configs>, Sink, Mode, Zone>...>
    requires( details::traits::is_config<std::decay_t<Config>>::value
              && ( details::traits::is_config<std::decay_t<Configs>>::value && ... ) )
#else
      -> typename std::enable_if<
        details::traits::AllOf<details::traits::is_config<typename std::decay<Config>::type>,
                               details::traits::is_config<typename std::decay<Configs>::type>...>::value,
        MultiBar<prefab::BasicBar<typename std::decay<Config>::type, Sink, Mode, Zone>,
                 prefab::BasicBar<typename std::decay<Configs>::type, Sink, Mode, Zone>...>>::type
#endif
  { return { std::forward<Config>( cfg ), std::forward<Configs>( cfgs )... }; }

  namespace details {
    namespace utils {
      template<types::Size Cnt, Channel S, Policy M, Region Z, typename B, types::Size... Is>
      PACE__NODISCARD PACE__FORCEINLINE typename std::enable_if<
        traits::is_bar<typename std::decay<B>::type>::value,
        MultiBar_t<prefab::BasicBar<typename std::decay<B>::type::config, S, M, Z>, Cnt>>::type
        make_multi_helper( B&& bar, traits::IndexSequence<Is...> )
          noexcept( traits::BoolConstant<( Cnt == 1 )>::value )
      {
        using Bar = typename std::decay<B>::type;
        std::array<typename Bar::config, Cnt - 1> cfgs { { ( (void)( Is ), bar.config() )... } };
        return { std::forward<B>( bar ), Bar( std::move( cfgs[Is] ) )... };
      }
      template<types::Size Cnt, Channel S, Policy M, Region Z, typename C, types::Size... Is>
      PACE__NODISCARD PACE__FORCEINLINE typename std::enable_if<
        traits::is_config<typename std::decay<C>::type>::value,
        MultiBar_t<prefab::BasicBar<typename std::decay<C>::type, S, M, Z>, Cnt>>::type
        make_multi_helper( C&& cfg, traits::IndexSequence<Is...> )
          noexcept( traits::AllOf<traits::BoolConstant<( Cnt == 1 )>,
                                  traits::Not<std::is_lvalue_reference<C&&>>>::value )
      {
        std::array<C, Cnt - 1> cfgs { { ( (void)( Is ), cfg )... } };
        return { std::forward<C>( cfg ), std::move( cfgs[Is] )... };
      }
    } // namespace utils
  } // namespace details

  /**
   * Creates a MultiBar with a fixed number of BasicBar instances using a single bar object.
   * **All BasicBar instances are initialized using the same configuration.**
   */
  template<details::types::Size Cnt, typename Config, Channel S, Policy M, Region Z>
  PACE__NODISCARD PACE__FORCEINLINE auto make_multi( prefab::BasicBar<Config, S, M, Z>&& bar )
    noexcept( Cnt == 1 )
#ifdef __cpp_concepts
    requires( Cnt > 0 && details::traits::is_config<Config>::value )
#else
      -> typename std::enable_if<details::traits::AllOf<details::traits::BoolConstant<( Cnt > 0 )>,
                                                        details::traits::is_config<Config>>::value,
                                 MultiBar_t<prefab::BasicBar<Config, S, M, Z>, Cnt>>::type
#endif
  {
    return details::utils::make_multi_helper<Cnt, S, M, Z>( std::move( bar ),
                                                            details::traits::MakeIndexSequence<Cnt - 1>() );
  }
  /**
   * Creates a MultiBar with a fixed number of BasicBar instances using a single configuration object.
   * **All BasicBar instances are initialized using the same configuration.**
   */
  template<details::types::Size Cnt,
           Channel Sink = Channel::Stderr,
           Policy Mode  = Policy::Async,
           Region Zone  = Region::Fixed,
           typename Config>
  PACE__NODISCARD PACE__FORCEINLINE auto make_multi( Config&& cfg )
    noexcept( details::traits::AllOf<details::traits::BoolConstant<( Cnt == 1 )>,
                                     details::traits::Not<std::is_lvalue_reference<Config&&>>>::value )
#ifdef __cpp_concepts
    requires( Cnt > 0 && details::traits::is_config<std::decay_t<Config>>::value )
#else
      -> typename std::enable_if<
        details::traits::AllOf<details::traits::BoolConstant<( Cnt > 0 )>,
                               details::traits::is_config<typename std::decay<Config>::type>>::value,
        MultiBar_t<prefab::BasicBar<typename std::decay<Config>::type, Sink, Mode, Zone>, Cnt>>::type
#endif
  {
    return details::utils::make_multi_helper<Cnt, Sink, Mode, Zone>(
      std::forward<Config>( cfg ),
      details::traits::MakeIndexSequence<Cnt - 1>() );
  }

  /**
   * Creates a MultiBar with a fixed number of bars using mutiple bar/configuration objects.
   * The ctor sequentially initializes the first few instances corresponding to the provided arguments;
   * **any remaining instances with no corresponding arguments will be default-initialized.**
   */
  template<typename Bar, details::types::Size Cnt, typename... Objs>
  PACE__NODISCARD PACE__FORCEINLINE auto make_multi( Objs&&... objs ) noexcept( sizeof...( Objs ) == Cnt )
#ifdef __cpp_concepts
    -> MultiBar_t<Bar, Cnt>
    requires( Cnt > 0 && sizeof...( Objs ) <= Cnt && details::traits::is_bar<Bar>::value
              && ( ( ( std::is_same_v<std::remove_cv_t<Bar>, std::decay_t<Objs>> && ... )
                     && !( std::is_lvalue_reference_v<Objs &&> || ... ) )
                   || ( std::is_same_v<typename Bar::config, std::decay_t<Objs>> && ... ) ) )
#else
    -> typename std::enable_if<
      details::traits::AllOf<
        details::traits::BoolConstant<( Cnt > 0 )>,
        details::traits::BoolConstant<( sizeof...( Objs ) <= Cnt )>,
        details::traits::is_bar<Bar>,
        details::traits::AnyOf<
          details::traits::AllOf<
            std::is_same<typename std::remove_cv<Bar>::type, typename std::decay<Objs>::type>...,
            details::traits::Not<details::traits::AnyOf<std::is_lvalue_reference<Objs&&>...>>>,
          details::traits::AllOf<std::is_same<typename Bar::config, typename std::decay<Objs>::type>...>>>::
        value,
      MultiBar_t<Bar, Cnt>>::type
#endif
  { return { std::forward<Objs>( objs )... }; }
  /**
   * Creates a MultiBar with a fixed number of BasicBar instances using mutiple configuration objects.
   * The ctor sequentially initializes the first few instances corresponding to the provided configurations;
   * **any remaining instances with no corresponding configurations will be default-initialized.**
   */
  template<typename Config,
           details::types::Size Cnt,
           Channel Sink = Channel::Stderr,
           Policy Mode  = Policy::Async,
           Region Zone  = Region::Fixed,
           typename... Configs>
  PACE__NODISCARD PACE__FORCEINLINE auto make_multi( Configs&&... configs )
    noexcept( sizeof...( Configs ) == Cnt )
#ifdef __cpp_concepts
      -> MultiBar_t<prefab::BasicBar<Config, Sink, Mode, Zone>, Cnt>
    requires( Cnt > 0 && sizeof...( Configs ) <= Cnt && details::traits::is_config<Config>::value
              && ( std::is_same_v<Config, std::decay_t<Configs>> && ... ) )
#else
      -> typename std::enable_if<
        details::traits::AllOf<details::traits::BoolConstant<( Cnt > 0 )>,
                               details::traits::BoolConstant<( sizeof...( Configs ) <= Cnt )>,
                               details::traits::is_config<Config>,
                               std::is_same<Config, typename std::decay<Configs>::type>...>::value,
        MultiBar_t<prefab::BasicBar<Config, Sink, Mode, Zone>, Cnt>>::type
#endif
  { return { std::forward<Configs>( configs )... }; }
  /**
   * Creates a MultiBar with a fixed number of BasicBar instances using mutiple bar objects.
   * The ctor sequentially initializes the first few instances corresponding to the provided objects;
   * **any remaining instances with no corresponding configurations will be default-initialized.**
   */
  template<typename Config,
           details::types::Size Cnt,
           Channel Sink = Channel::Stderr,
           Policy Mode  = Policy::Async,
           Region Zone  = Region::Fixed,
           typename... Configs>
  PACE__NODISCARD PACE__FORCEINLINE auto make_multi( prefab::BasicBar<Configs, Sink, Mode, Zone>&&... bars )
    noexcept( sizeof...( Configs ) == Cnt )
#ifdef __cpp_concepts
      -> MultiBar_t<prefab::BasicBar<Config, Sink, Mode, Zone>, Cnt>
    requires( Cnt > 0 && sizeof...( Configs ) <= Cnt && details::traits::is_config<Config>::value
              && ( std::is_same_v<Config, std::decay_t<Configs>> && ... ) )
#else
      -> typename std::enable_if<
        details::traits::AllOf<details::traits::BoolConstant<( Cnt > 0 )>,
                               details::traits::BoolConstant<( sizeof...( Configs ) <= Cnt )>,
                               details::traits::is_config<Config>,
                               std::is_same<Config, typename std::decay<Configs>::type>&&...>::value,
        MultiBar_t<prefab::BasicBar<Config, Sink, Mode, Zone>, Cnt>>::type
#endif
  { return { std::move( bars )... }; }
} // namespace pace

template<typename... Bs>
struct std::tuple_size<pace::MultiBar<Bs...>> : std::integral_constant<std::size_t, sizeof...( Bs )> {};

template<std::size_t I, typename... Bs>
struct std::tuple_element<I, pace::MultiBar<Bs...>> : pace::details::traits::TypeAt<I, Bs...> {};

#endif
