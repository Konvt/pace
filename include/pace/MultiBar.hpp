#ifndef PACE_MULTI_BAR
#define PACE_MULTI_BAR

#include "details/assets/StaticLayout.hpp"

namespace pace {
  template<typename Bar, typename... Bars>
  class MultiBar;
  template<Channel S, Policy M, Region Z, typename Soul, typename... Souls>
  class MultiBar<prefab::BasicBar<Soul, S, M, Z>, prefab::BasicBar<Souls, S, M, Z>...> {
    template<std::size_t Pos>
    using ConfigAt_t = details::traits::TypeAt_t<Pos, Soul, Souls...>;
    template<std::size_t Pos>
    using BarAt_t = prefab::BasicBar<ConfigAt_t<Pos>, S, M, Z>;

    details::assets::
      StaticLayout<S, M, Z, details::traits::MakeIndexSequence<sizeof...( Souls ) + 1>, Soul, Souls...>
        package_;

  public:
    static constexpr Channel channel_value = S;
    static constexpr Policy policy_value   = M;
    static constexpr Region region_value   = Z;

    MultiBar() = default;

    template<typename Cfg, typename... Cfgs>
    MultiBar( Cfg&& cfg, Cfgs&&... cfgs ) noexcept( sizeof...( Cfgs ) == sizeof...( Souls ) )
#ifdef __cpp_concepts
      requires( sizeof...( Cfgs ) <= sizeof...( Souls ) )
           && details::traits::TpStartsWith<
                details::traits::TypeList<typename std::decay<Cfg>::type, typename std::decay<Cfgs>::type...>,
                Soul,
                Souls...>::value
#endif
      : package_ { std::forward<Cfg>( cfg ), std::forward<Cfgs>( cfgs )... }
    {
#ifndef __cpp_concepts
      static_assert( sizeof...( Cfgs ) <= sizeof...( Souls ), "too many configurations" );
      static_assert(
        details::traits::TpStartsWith<
          details::traits::TypeList<typename std::decay<Cfg>::type, typename std::decay<Cfgs>::type...>,
          Soul,
          Souls...>::value,
        "inconsistent configuration parameters" );
#endif
    }

    template<typename Cfg, typename... Cfgs>
    MultiBar( prefab::BasicBar<Cfg, S, M, Z>&& bar, prefab::BasicBar<Cfgs, S, M, Z>&&... bars )
      noexcept( sizeof...( Cfgs ) == sizeof...( Souls ) )
#ifdef __cpp_concepts
      requires( sizeof...( Cfgs ) <= sizeof...( Souls ) )
           && details::traits::TpStartsWith<
                details::traits::TypeList<typename std::decay<Cfg>::type, typename std::decay<Cfgs>::type...>,
                Soul,
                Souls...>::value
#endif
      : package_ { std::move( bar ), std::move( bars )... }
    {
#ifndef __cpp_concepts
      static_assert( sizeof...( Cfgs ) <= sizeof...( Souls ), "too many configurations" );
      static_assert(
        details::traits::TpStartsWith<
          details::traits::TypeList<typename std::decay<Cfg>::type, typename std::decay<Cfgs>::type...>,
          Soul,
          Souls...>::value,
        "inconsistent configuration parameters" );
#endif
    }

    MultiBar( const MultiBar& )                  = delete;
    MultiBar& operator=( const MultiBar& ) &     = delete;
    MultiBar( MultiBar&& ) noexcept              = default;
    MultiBar& operator=( MultiBar&& ) & noexcept = default;
    ~MultiBar()                                  = default;

    // Mark all the progress bars as completed.
    PACE__FORCEINLINE void finish() { package_.close(); }
    // Mark all the progress bars as retired.
    PACE__FORCEINLINE void retire() { package_.hide(); }
    // Abort all the progress bars.
    PACE__FORCEINLINE void abort() noexcept { package_.cancel(); }

    // Returns the number of progress bars.
    PACE__NODISCARD static PACE__FORCEINLINE PACE__CNSTEVAL std::size_t size() noexcept
    { return sizeof...( Souls ) + 1; }
    // Check whether a progress bar is running
    PACE__NODISCARD PACE__FORCEINLINE bool active() const noexcept { return package_.online(); }
    /// @return The number of terminal lines currently occupied by the progress bar,
    //          which is useful when using `Region::Relative` layout and needing to output extra information.
    PACE__NODISCARD PACE__FORCEINLINE std::size_t layout_height() const noexcept { return package_.height(); }

    template<std::size_t Pos>
    PACE__FORCEINLINE PACE__CXX14_CNSTXPR BarAt_t<Pos>& at() & noexcept
    { return package_.template at<Pos>(); }
    template<std::size_t Pos>
    PACE__FORCEINLINE constexpr const BarAt_t<Pos>& at() const& noexcept
    { return package_.template at<Pos>(); }
    template<std::size_t Pos>
    PACE__FORCEINLINE PACE__CXX14_CNSTXPR BarAt_t<Pos>&& at() && noexcept
    { return std::move( package_.template at<Pos>() ); }

    void swap( MultiBar& other ) noexcept { package_.swap( other.package_ ); }
    friend void swap( MultiBar& a, MultiBar& b ) noexcept { a.swap( b ); }

    template<std::size_t Pos>
    PACE__FORCEINLINE friend PACE__CXX14_CNSTXPR BarAt_t<Pos>& get( MultiBar& self ) noexcept
    { return self.template at<Pos>(); }
    template<std::size_t Pos>
    PACE__FORCEINLINE friend constexpr const BarAt_t<Pos>& get( const MultiBar& self ) noexcept
    { return self.template at<Pos>(); }
    template<std::size_t Pos>
    PACE__FORCEINLINE friend PACE__CXX14_CNSTXPR BarAt_t<Pos>&& get( MultiBar&& self ) noexcept
    { return std::move( self ).template at<Pos>(); }

    template<typename T>
    PACE__FORCEINLINE friend PACE__CXX14_CNSTXPR auto get( MultiBar& self ) noexcept
      -> decltype( self.package_.template get<T>() )
    { return self.package_.template get<T>(); }
    template<typename T>
    PACE__FORCEINLINE friend constexpr auto get( const MultiBar& self ) noexcept
      -> decltype( self.package_.template get<T>() )
    { return self.package_.template get<T>( self.package_ ); }
    template<typename T>
    PACE__FORCEINLINE friend PACE__CXX14_CNSTXPR auto get( MultiBar&& self ) noexcept
      -> decltype( self.package_.template get<T>() )
    { return std::move( self.package_ ).template get<T>( self.package_ ); }
  };

#ifdef __cpp_deduction_guides
  template<Channel S, Policy M, Region Z, typename Cfg, typename... Cfgs>
  MultiBar( prefab::BasicBar<Cfg, S, M, Z>&&, prefab::BasicBar<Cfgs, S, M, Z>&&... )
    -> MultiBar<prefab::BasicBar<Cfg, S, M, Z>, prefab::BasicBar<Cfgs, S, M, Z>...>;
#endif

  // Generates a MultiBar type containing Count instances of the given Bar type.
  template<typename Bar, std::size_t Count>
  using MultiBar_t = details::traits::FillWith_t<MultiBar, Bar, Count>;

  // Creates a MultiBar using existing bar instances.
  template<typename Soul, typename... Souls, Channel Sink, Policy Mode, Region Zone>
  PACE__NODISCARD PACE__FORCEINLINE auto make_multi(
    prefab::BasicBar<Soul, Sink, Mode, Zone>&& bar,
    prefab::BasicBar<Souls, Sink, Mode, Zone>&&... bars ) noexcept
    -> MultiBar<prefab::BasicBar<Soul, Sink, Mode, Zone>, prefab::BasicBar<Souls, Sink, Mode, Zone>...>
  { return { std::move( bar ), std::move( bars )... }; }
  // Creates a MultiBar using configuration objects.
  template<Channel Sink = Channel::Err,
           Policy Mode  = Policy::Async,
           Region Zone  = Region::Fixed,
           typename Soul,
           typename... Souls>
  PACE__NODISCARD PACE__FORCEINLINE auto make_multi( Soul&& cfg, Souls&&... cfgs ) noexcept(
    details::traits::Not<
      details::traits::AnyOf<std::is_lvalue_reference<Soul&&>, std::is_lvalue_reference<Souls&&>...>>::value )
#ifdef __cpp_concepts
    -> MultiBar<prefab::BasicBar<std::decay_t<Soul>, Sink, Mode, Zone>,
                prefab::BasicBar<std::decay_t<Souls>, Sink, Mode, Zone>...>
    requires details::traits::is_config<std::decay_t<Soul>>::value
          && ( details::traits::is_config<std::decay_t<Souls>>::value && ... )
#else
    -> typename std::enable_if<
      details::traits::AllOf<details::traits::is_config<typename std::decay<Soul>::type>,
                             details::traits::is_config<typename std::decay<Souls>::type>...>::value,
      MultiBar<prefab::BasicBar<typename std::decay<Soul>::type, Sink, Mode, Zone>,
               prefab::BasicBar<typename std::decay<Souls>::type, Sink, Mode, Zone>...>>::type
#endif
  { return { std::forward<Soul>( cfg ), std::forward<Souls>( cfgs )... }; }

  namespace details {
    namespace utils {
      template<std::size_t Cnt, typename Soul, Channel S, Policy M, Region Z, typename B, std::size_t... Is>
      PACE__NODISCARD PACE__FORCEINLINE auto _impl_make_multi( prefab::BasicBar<Soul, S, M, Z>&& bar,
                                                               traits::IndexSequence<Is...> )
        noexcept( traits::BoolConstant<( Cnt == 1 )>::value )
          -> MultiBar_t<prefab::BasicBar<Soul, S, M, Z>, Cnt>
      {
        std::array<Soul, Cnt - 1> cfgs { { ( (void)( Is ), bar.config() )... } };
        return { std::move( bar ).config(), std::move( cfgs[Is] )... };
      }
      template<std::size_t Cnt, Channel S, Policy M, Region Z, typename Soul, std::size_t... Is>
      PACE__NODISCARD PACE__FORCEINLINE auto _impl_make_multi( Soul&& cfg, traits::IndexSequence<Is...> )
        noexcept( traits::AllOf<traits::BoolConstant<( Cnt == 1 )>,
                                traits::Not<std::is_lvalue_reference<Soul&&>>>::value )
          -> MultiBar_t<prefab::BasicBar<typename std::decay<Soul>::type, S, M, Z>, Cnt>
      {
        static_assert( details::traits::is_config<typename std::decay<Soul>::type>::value,
                       "not a configuration" );
        std::array<Soul, Cnt - 1> cfgs { { ( (void)( Is ), cfg )... } };
        return { std::forward<Soul>( cfg ), std::move( cfgs[Is] )... };
      }
    } // namespace utils
  } // namespace details

  /**
   * Creates a MultiBar with a fixed number of BasicBar instances using a single bar object.
   * **All BasicBar instances are initialized using the same configuration.**
   */
  template<std::size_t Cnt, typename Soul, Channel S, Policy M, Region Z>
  PACE__NODISCARD PACE__FORCEINLINE auto make_multi( prefab::BasicBar<Soul, S, M, Z>&& bar )
    noexcept( Cnt == 1 )
#ifdef __cpp_concepts
      -> MultiBar_t<prefab::BasicBar<Soul, S, M, Z>, Cnt>
    requires( Cnt > 0 )
#else
      -> typename std::enable_if<( Cnt > 0 ), MultiBar_t<prefab::BasicBar<Soul, S, M, Z>, Cnt>>::type
#endif
  {
    return details::utils::_impl_make_multi<Cnt, S, M, Z>( std::move( bar ),
                                                           details::traits::MakeIndexSequence<Cnt - 1>() );
  }
  /**
   * Creates a MultiBar with a fixed number of BasicBar instances using a single configuration object.
   * **All BasicBar instances are initialized using the same configuration.**
   */
  template<std::size_t Cnt,
           Channel Sink = Channel::Err,
           Policy Mode  = Policy::Async,
           Region Zone  = Region::Fixed,
           typename Soul>
  PACE__NODISCARD PACE__FORCEINLINE auto make_multi( Soul&& cfg )
    noexcept( details::traits::AllOf<details::traits::BoolConstant<( Cnt == 1 )>,
                                     details::traits::Not<std::is_lvalue_reference<Soul&&>>>::value )
#ifdef __cpp_concepts
      -> MultiBar_t<prefab::BasicBar<std::decay_t<Soul>, Sink, Mode, Zone>, Cnt>
    requires( Cnt > 0 ) && details::traits::is_config<typename std::decay<Soul>::type>::value
#else
      -> typename std::enable_if<
        ( Cnt > 0 ),
        MultiBar_t<prefab::BasicBar<typename std::decay<Soul>::type, Sink, Mode, Zone>, Cnt>>::type
#endif
  {
#ifndef __cpp_concepts
    static_assert( details::traits::is_config<typename std::decay<Soul>::type>::value,
                   "not a configuration" );
#endif
    return details::utils::_impl_make_multi<Cnt, Sink, Mode, Zone>(
      std::forward<Soul>( cfg ),
      details::traits::MakeIndexSequence<Cnt - 1>() );
  }

  /**
   * Creates a MultiBar with a fixed number of bars using mutiple bar/configuration objects.
   * The ctor sequentially initializes the first few instances corresponding to the provided arguments;
   * **any remaining instances with no corresponding arguments will be default-initialized.**
   */
  template<typename Bar, std::size_t Cnt, typename... Objs>
  PACE__NODISCARD PACE__FORCEINLINE auto make_multi( Objs&&... objs ) noexcept( sizeof...( Objs ) == Cnt )
#ifdef __cpp_concepts
    -> MultiBar_t<Bar, Cnt>
    requires( Cnt > 0 ) && ( sizeof...( Objs ) <= Cnt )
         && ( ( ( std::is_same_v<Bar, std::decay_t<Objs>> && ... )
                && !( std::is_lvalue_reference_v<Objs&&> || ... ) )
              || ( std::is_same_v<typename Bar::config_type, std::decay_t<Objs>> && ... ) )
#else
    -> typename std::enable_if<
      details::traits::AllOf<
        details::traits::BoolConstant<( Cnt > 0 )>,
        details::traits::BoolConstant<( sizeof...( Objs ) <= Cnt )>,
        details::traits::AnyOf<
          details::traits::AllOf<
            std::is_same<Bar, typename std::decay<Objs>::type>...,
            details::traits::Not<details::traits::AnyOf<std::is_lvalue_reference<Objs&&>...>>>,
          details::traits::AllOf<
            std::is_same<typename Bar::config_type, typename std::decay<Objs>::type>...>>>::value,
      MultiBar_t<Bar, Cnt>>::type
#endif
  { return { std::forward<Objs>( objs )... }; }
  /**
   * Creates a MultiBar with a fixed number of BasicBar instances using mutiple configuration objects.
   * The ctor sequentially initializes the first few instances corresponding to the provided configurations;
   * **any remaining instances with no corresponding configurations will be default-initialized.**
   */
  template<typename Config,
           std::size_t Cnt,
           Channel Sink = Channel::Err,
           Policy Mode  = Policy::Async,
           Region Zone  = Region::Fixed,
           typename... Souls>
  PACE__NODISCARD PACE__FORCEINLINE auto make_multi( Souls&&... cfgs ) noexcept( sizeof...( Souls ) == Cnt )
#ifdef __cpp_concepts
    -> MultiBar_t<prefab::BasicBar<Config, Sink, Mode, Zone>, Cnt>
    requires( Cnt > 0 ) && ( sizeof...( Souls ) <= Cnt ) && details::traits::is_config<Config>::value
         && ( std::is_same_v<Config, std::decay_t<Souls>> && ... )
#else
    -> typename std::enable_if<
      details::traits::AllOf<details::traits::BoolConstant<( Cnt > 0 )>,
                             details::traits::BoolConstant<( sizeof...( Souls ) <= Cnt )>,
                             details::traits::is_config<Config>,
                             std::is_same<Config, typename std::decay<Souls>::type>...>::value,
      MultiBar_t<prefab::BasicBar<Config, Sink, Mode, Zone>, Cnt>>::type
#endif
  { return { std::forward<Souls>( cfgs )... }; }
  /**
   * Creates a MultiBar with a fixed number of BasicBar instances using mutiple bar objects.
   * The ctor sequentially initializes the first few instances corresponding to the provided objects;
   * **any remaining instances with no corresponding configurations will be default-initialized.**
   */
  template<typename Config,
           std::size_t Cnt,
           Channel Sink = Channel::Err,
           Policy Mode  = Policy::Async,
           Region Zone  = Region::Fixed,
           typename... Souls>
  PACE__NODISCARD PACE__FORCEINLINE auto make_multi( prefab::BasicBar<Souls, Sink, Mode, Zone>&&... bars )
    noexcept( sizeof...( Souls ) == Cnt )
#ifdef __cpp_concepts
      -> MultiBar_t<prefab::BasicBar<Config, Sink, Mode, Zone>, Cnt>
    requires( Cnt > 0 ) && ( sizeof...( Souls ) <= Cnt )
         && ( std::is_same_v<Config, std::decay_t<Souls>> && ... )
#else
      -> typename std::enable_if<
        details::traits::AllOf<details::traits::BoolConstant<( Cnt > 0 )>,
                               details::traits::BoolConstant<( sizeof...( Souls ) <= Cnt )>,
                               std::is_same<Config, typename std::decay<Souls>::type>&&...>::value,
        MultiBar_t<prefab::BasicBar<Config, Sink, Mode, Zone>, Cnt>>::type
#endif
  { return { std::move( bars )... }; }
} // namespace pace

template<typename... Bs>
struct std::tuple_size<pace::MultiBar<Bs...>> : std::integral_constant<std::size_t, sizeof...( Bs )> {};

template<std::size_t I, typename... Bs>
struct std::tuple_element<I, pace::MultiBar<Bs...>> : pace::details::traits::TypeAt<I, Bs...> {};

#endif
