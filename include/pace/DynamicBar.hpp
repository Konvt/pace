#ifndef PACE_DYNAMIC_BAR
#define PACE_DYNAMIC_BAR

#include "details/assets/DynamicLayout.hpp"

namespace pace {
  template<Channel Sink = Channel::Stderr, Policy Mode = Policy::Async, Region Zone = Region::Fixed>
  class DynamicBar {
    using Context = details::assets::DynamicLayout<Sink, Mode, Zone>;

    std::shared_ptr<Context> core_;
    mutable details::concurrent::SharedMutex mtx_;

    PACE__FORCEINLINE void setup_if_null() &
    {
      if ( core_ == nullptr )
        PACE__UNLIKELY
        {
          std::lock_guard<details::concurrent::SharedMutex> lock { mtx_ };
          if ( core_ == nullptr )
            core_ = std::make_shared<Context>();
        }
    }

  public:
    DynamicBar()                               = default;
    DynamicBar( const DynamicBar& )            = delete;
    DynamicBar& operator=( const DynamicBar& ) = delete;
    DynamicBar( DynamicBar&& rhs ) noexcept
    {
      PACE__ASSERT( rhs.active() == false );
      core_ = std::move( rhs.core_ );
    }
    DynamicBar& operator=( DynamicBar&& rhs ) & noexcept
    { // The thread insecurity here is deliberately designed.
      // Because for a move-only type, transferring ownership simultaneously
      // in multiple locations should not occur.
      PACE__TRUST( this != &rhs );
      PACE__ASSERT( active() == false );
      PACE__ASSERT( rhs.active() == false );
      core_ = std::move( rhs.core_ );
      return *this;
    }
    ~DynamicBar() = default;

    PACE__NODISCARD PACE__FORCEINLINE bool active() const noexcept
    {
      details::concurrent::SharedLock<details::concurrent::SharedMutex> lock { mtx_ };
      return core_ != nullptr && core_->online_count() != 0;
    }
    PACE__NODISCARD PACE__FORCEINLINE details::types::Size size() const noexcept
    {
      details::concurrent::SharedLock<details::concurrent::SharedMutex> lock { mtx_ };
      return core_ != nullptr ? core_.use_count() - 1 : 0;
    }
    PACE__NODISCARD PACE__FORCEINLINE details::types::Size active_count() const noexcept
    {
      details::concurrent::SharedLock<details::concurrent::SharedMutex> lock { mtx_ };
      return core_ != nullptr ? core_->online_count() : 0;
    }
    PACE__FORCEINLINE void reset()
    {
      std::lock_guard<details::concurrent::SharedMutex> lock { mtx_ };
      if ( core_ != nullptr )
        core_->shut();
    }
    PACE__FORCEINLINE void abort() noexcept
    {
      std::lock_guard<details::concurrent::SharedMutex> lock { mtx_ };
      if ( core_ != nullptr )
        core_->kill();
    }

    // Wait until the indicator is Stop.
    void wait() const noexcept
    {
      details::concurrent::spin_wait( [this]() noexcept { return !active(); } );
    }
    // Wait for the indicator is Stop or timed out.
    template<class Rep, class Period>
    PACE__NODISCARD bool wait_for( const std::chrono::duration<Rep, Period>& timeout ) const noexcept
    {
      return details::concurrent::spin_wait_for( [this]() noexcept { return !active(); }, timeout );
    }

    template<typename Config>
    PACE__NODISCARD auto insert( prefab::BasicBar<Config, Sink, Mode, Zone>&& bar )
#ifdef __cpp_concepts
      -> std::unique_ptr<prefab::BasicBar<Config, Sink, Mode, Zone>>
      requires details::traits::is_config<Config>::value
#else
      -> typename std::enable_if<details::traits::is_config<Config>::value,
                                 std::unique_ptr<prefab::BasicBar<Config, Sink, Mode, Zone>>>::type
#endif
    {
      setup_if_null();
      return details::utils::make_unique<details::assets::ManagedBar<Config, Sink, Mode, Zone>>(
        core_,
        std::move( bar ) );
    }
    template<typename Config>
    PACE__NODISCARD auto insert( Config cfg )
#ifdef __cpp_concepts
      -> std::unique_ptr<prefab::BasicBar<Config, Sink, Mode, Zone>>
      requires details::traits::is_config<Config>::value
#else
      -> typename std::enable_if<details::traits::is_config<Config>::value,
                                 std::unique_ptr<prefab::BasicBar<Config, Sink, Mode, Zone>>>::type
#endif
    {
      setup_if_null();
      return details::utils::make_unique<details::assets::ManagedBar<Config, Sink, Mode, Zone>>(
        core_,
        std::move( cfg ) );
    }

    template<typename Bar, typename... Options>
    PACE__NODISCARD auto insert( Options&&... options )
#ifdef __cpp_concepts
      -> std::unique_ptr<Bar>
      requires( details::traits::is_bar<Bar>::value && Bar::sink == Sink && Bar::mode == Mode
                && Bar::zone == Zone && std::is_constructible_v<Bar, Options && ...> )
#else
      -> typename std::enable_if<
        details::traits::AllOf<details::traits::is_bar<Bar>,
                               std::is_constructible<Bar, Options&&...>,
                               details::traits::BoolConstant<( Bar::sink == Sink )>,
                               details::traits::BoolConstant<( Bar::mode == Mode )>,
                               details::traits::BoolConstant<( Bar::zone == Zone )>>::value,
        std::unique_ptr<Bar>>::type
#endif
    {
      setup_if_null();
      return details::utils::make_unique<details::assets::ManagedBar<typename Bar::Config, Sink, Mode, Zone>>(
        core_,
        std::forward<Options>( options )... );
    }
    template<typename Config, typename... Options>
    PACE__NODISCARD auto insert( Options&&... options )
#ifdef __cpp_concepts
      -> std::unique_ptr<prefab::BasicBar<Config, Sink, Mode, Zone>>
      requires( details::traits::is_config<Config>::value && std::is_constructible_v<Config, Options && ...> )
#else
      -> typename std::enable_if<details::traits::AllOf<details::traits::is_config<Config>,
                                                        std::is_constructible<Config, Options&&...>>::value,
                                 std::unique_ptr<prefab::BasicBar<Config, Sink, Mode, Zone>>>::type
#endif
    {
      setup_if_null();
      return details::utils::make_unique<details::assets::ManagedBar<Config, Sink, Mode, Zone>>(
        core_,
        std::forward<Options>( options )... );
    }

    void swap( DynamicBar& other ) noexcept
    { // The thread insecurity here is deliberately designed.
      // The reason can be found in the move assignment.
      PACE__TRUST( this != &other );
      PACE__ASSERT( active() == false );
      PACE__ASSERT( other.active() == false );
      core_.swap( other.core_ );
    }
    friend void swap( DynamicBar& a, DynamicBar& b ) noexcept { a.swap( b ); }
  };

  // Creates a tuple of unique_ptr pointing to bars using existing bar instances.
  template<typename Config, typename... Configs, Channel O, Policy M, Region A>
  PACE__NODISCARD PACE__FORCEINLINE auto make_dynamic( prefab::BasicBar<Config, O, M, A>&& bar,
                                                       prefab::BasicBar<Configs, O, M, A>&&... bars )
#ifdef __cpp_concepts
    requires( details::traits::is_config<Config>::value
              && ( details::traits::is_config<Configs>::value && ... ) )
#else
    -> typename std::enable_if<details::traits::AllOf<details::traits::is_config<Config>,
                                                      details::traits::is_config<Configs>...>::value,
                               std::tuple<std::unique_ptr<prefab::BasicBar<Config, O, M, A>>,
                                          std::unique_ptr<prefab::BasicBar<Configs, O, M, A>>...>>::type
#endif
  {
    DynamicBar<O, M, A> factory;
    return std::make_tuple( factory.insert( std::move( bar ) ), factory.insert( std::move( bars ) )... );
  }
  // Creates a tuple of unique_ptr pointing to bars using configuration objects.
  template<Channel Sink = Channel::Stderr,
           Policy Mode  = Policy::Async,
           Region Zone  = Region::Fixed,
           typename Config,
           typename... Configs>
  PACE__NODISCARD PACE__FORCEINLINE auto make_dynamic( Config&& cfg, Configs&&... cfgs )
#ifdef __cpp_concepts
    requires( details::traits::is_config<std::decay_t<Config>>::value
              && ( details::traits::is_config<std::decay_t<Configs>>::value && ... ) )
#else
    -> typename std::enable_if<
      details::traits::AllOf<details::traits::is_config<typename std::decay<Config>::type>,
                             details::traits::is_config<typename std::decay<Configs>::type>...>::value,
      std::tuple<
        std::unique_ptr<prefab::BasicBar<typename std::decay<Config>::type, Sink, Mode, Zone>>,
        std::unique_ptr<prefab::BasicBar<typename std::decay<Configs>::type, Sink, Mode, Zone>>...>>::type
#endif
  {
    DynamicBar<Sink, Mode, Zone> factory;
    return std::make_tuple( factory.insert( std::forward<Config>( cfg ) ),
                            factory.insert( std::forward<Configs>( cfgs ) )... );
  }

  /**
   * Creates a vector of unique_ptr pointing to bars with a fixed number of BasicBar instances.
   * **All BasicBar instances are initialized using the same configuration.**
   */
  template<typename Config, Channel O, Policy M, Region A>
  PACE__NODISCARD PACE__FORCEINLINE auto make_dynamic( prefab::BasicBar<Config, O, M, A>&& bar,
                                                       details::types::Size count )
#ifdef __cpp_concepts
    requires details::traits::is_config<Config>::value
#else
    -> typename std::enable_if<details::traits::is_config<Config>::value,
                               std::vector<std::unique_ptr<prefab::BasicBar<Config, O, M, A>>>>::type
#endif
  {
    std::vector<std::unique_ptr<prefab::BasicBar<Config, O, M, A>>> products;
    if ( count == 0 )
      PACE__UNLIKELY return products;
    DynamicBar<O, M, A> factory;
    std::generate_n( std::back_inserter( products ), count - 1, [&factory, &bar]() {
      return factory.insert( bar.config() );
    } );
    products.emplace_back( factory.insert( std::move( bar ) ) );
    return products;
  }
  /**
   * Creates a vector of unique_ptr pointing to bars with a fixed number of BasicBar instances.
   * **All BasicBar instances are initialized using the same configuration.**
   */
  template<Channel Sink = Channel::Stderr,
           Policy Mode  = Policy::Async,
           Region Zone  = Region::Fixed,
           typename Config>
  PACE__NODISCARD PACE__FORCEINLINE auto make_dynamic( Config&& cfg, details::types::Size count )
#ifdef __cpp_concepts
    requires details::traits::is_config<std::decay_t<Config>>::value
#else
    ->
    typename std::enable_if<details::traits::is_config<typename std::decay<Config>::type>::value,
                            std::vector<std::unique_ptr<
                              prefab::BasicBar<typename std::decay<Config>::type, Sink, Mode, Zone>>>>::type
#endif
  {
    std::vector<std::unique_ptr<prefab::BasicBar<typename std::decay<Config>::type, Sink, Mode, Zone>>>
      products;
    if ( count == 0 )
      PACE__UNLIKELY return products;
    DynamicBar<Sink, Mode, Zone> factory;
    std::generate_n( std::back_inserter( products ), count - 1, [&factory, &cfg]() {
      return factory.insert( cfg );
    } );
    products.emplace_back( factory.insert( std::forward<Config>( cfg ) ) );
    return products;
  }

  /**
   * Creates a vector of unique_ptr with a fixed number of bars using mutiple bar/configuration objects.
   * The ctor sequentially initializes the first few instances corresponding to the provided arguments;
   * **An unmatched count and Bars number will cause an exception `pace::exception::InvalidArgument`.**
   */
  template<typename Bar, typename... Objs>
  PACE__NODISCARD PACE__FORCEINLINE auto make_dynamic( details::types::Size count, Objs&&... objs )
#ifdef __cpp_concepts
    requires( details::traits::is_bar<Bar>::value
              && ( ( ( std::is_same_v<std::remove_cv_t<Bar>, std::remove_cv_t<Objs>> && ... )
                     && !( std::is_lvalue_reference_v<Objs &&> || ... ) )
                   || ( std::is_same<typename Bar::Config, std::decay_t<Objs>>::value && ... ) ) )
#else
    -> typename std::enable_if<
      details::traits::AllOf<
        details::traits::is_bar<Bar>,
        details::traits::AnyOf<
          details::traits::AllOf<
            std::is_same<typename std::remove_cv<Bar>::type, typename std::remove_cv<Objs>::type>...,
            details::traits::Not<details::traits::AnyOf<std::is_lvalue_reference<Objs&&>...>>>,
          details::traits::AllOf<std::is_same<typename Bar::Config, typename std::decay<Objs>::type>...>>>::
        value,
      std::vector<std::unique_ptr<Bar>>>::type
#endif
  {
    std::vector<std::unique_ptr<Bar>> products;
    if ( count == 0 )
      PACE__UNLIKELY return products;
    else if ( count < sizeof...( Objs ) )
      PACE__UNLIKELY
      {
        details::charcodes::CoWString message =
          details::charcodes::make_literal( "pace: provided object count (" );
        details::utils::format_to( std::back_inserter( message ), sizeof...( Objs ) );
        message.append( ") exceeds the specified count (" );
        details::utils::format_to( std::back_inserter( message ), count );
        message.push_back( ')' );
        throw exception::InvalidArgument( std::move( message ) );
      }

    DynamicBar<Bar::sink, Bar::mode, Bar::zone> factory;
    (void)std::initializer_list<bool> {
      ( products.emplace_back( factory.insert( std::forward<Objs>( objs ) ) ), false )...
    };
    std::generate_n( std::back_inserter( products ), count - sizeof...( Objs ), [&factory]() {
      return factory.template insert<Bar>();
    } );
    return products;
  }
  /**
   * Creates a vector of unique_ptr with a fixed number of BasicBar instances using multiple configurations.
   * The ctor sequentially initializes the first few instances corresponding to the provided configurations;
   * **An unmatched count and Bars number will cause an exception `pace::exception::InvalidArgument`.**
   */
  template<typename Config,
           Channel Sink = Channel::Stderr,
           Policy Mode  = Policy::Async,
           Region Zone  = Region::Fixed,
           typename... Configs>
  PACE__NODISCARD PACE__FORCEINLINE auto make_dynamic( details::types::Size count, Configs&&... cfgs )
#ifdef __cpp_concepts
    requires( details::traits::is_config<Config>::value
              && ( std::is_same_v<std::remove_cv_t<Config>, std::decay_t<Configs>> && ... ) )
#else
    -> typename std::enable_if<
      details::traits::AllOf<
        details::traits::is_config<Config>,
        std::is_same<typename std::remove_cv<Config>::type, typename std::decay<Configs>::type>...>::value,
      std::vector<std::unique_ptr<prefab::BasicBar<Config, Sink, Mode, Zone>>>>::type
#endif
  {
    std::vector<std::unique_ptr<prefab::BasicBar<Config, Sink, Mode, Zone>>> products;
    if ( count == 0 )
      PACE__UNLIKELY return products;
    else if ( count < sizeof...( Configs ) )
      PACE__UNLIKELY
      {
        details::charcodes::CoWString message =
          details::charcodes::make_literal( "pace: provided configs count (" );
        details::utils::format_to( std::back_inserter( message ), sizeof...( Configs ) );
        message.append( ") exceeds the specified count (" );
        details::utils::format_to( std::back_inserter( message ), count );
        message.push_back( ')' );
        throw exception::InvalidArgument( std::move( message ) );
      }

    DynamicBar<Sink, Mode, Zone> factory;
    (void)std::initializer_list<bool> {
      ( products.emplace_back( factory.insert( std::forward<Configs>( cfgs ) ) ), false )...
    };
    std::generate_n( std::back_inserter( products ), count - sizeof...( Configs ) - 1, [&factory]() {
      return factory.template insert<Config>();
    } );
    return products;
  }
  /**
   * Creates a vector of unique_ptr with a fixed number of BasicBar instances using multiple bar objects.
   * The ctor sequentially initializes the first few instances corresponding to the provided objects;
   * **An unmatched count and Bars number will cause an exception `pace::exception::InvalidArgument`.**
   */
  template<typename Config,
           Channel Sink = Channel::Stderr,
           Policy Mode  = Policy::Async,
           Region Zone  = Region::Fixed,
           typename... Configs>
  PACE__NODISCARD PACE__FORCEINLINE auto make_dynamic( details::types::Size count,
                                                       prefab::BasicBar<Configs, Sink, Mode, Zone>&&... bars )
#ifdef __cpp_concepts
    requires( details::traits::is_config<Config>::value
              && ( std::is_same_v<Config, std::decay_t<Configs>> && ... ) )
#else
    -> typename std::enable_if<
      details::traits::AllOf<details::traits::is_config<Config>,
                             std::is_same<Config, typename std::decay<Configs>::type>...>::value,
      std::vector<std::unique_ptr<prefab::BasicBar<Config, Sink, Mode, Zone>>>>::type
#endif
  {
    std::vector<std::unique_ptr<prefab::BasicBar<Config, Sink, Mode, Zone>>> products;
    if ( count == 0 )
      PACE__UNLIKELY return products;
    else if ( count < sizeof...( Configs ) )
      PACE__UNLIKELY
      {
        details::charcodes::CoWString message =
          details::charcodes::make_literal( "pace: provided bar count (" );
        details::utils::format_to( std::back_inserter( message ), sizeof...( Configs ) );
        message.append( ") exceeds the specified count (" );
        details::utils::format_to( std::back_inserter( message ), count );
        message.push_back( ')' );
        throw exception::InvalidArgument( std::move( message ) );
      }

    DynamicBar<Sink, Mode, Zone> factory;
    (void)std::initializer_list<bool> { ( products.emplace_back( factory.insert( std::move( bars ) ) ),
                                          false )... };
    std::generate_n( std::back_inserter( products ), count - sizeof...( Configs ) - 1, [&factory]() {
      return factory.template insert<Config>();
    } );
    return products;
  }
} // namespace pace

#endif
