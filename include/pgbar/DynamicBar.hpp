#ifndef PGBAR_DYNAMIC_BAR
#define PGBAR_DYNAMIC_BAR

#include "details/prefabs/DynamicLayout.hpp"

namespace pgbar {
  template<Channel Outlet = Channel::Stderr, Policy Mode = Policy::Async, Region Area = Region::Fixed>
  class DynamicBar {
    using Context = _details::prefabs::DynamicLayout<Outlet, Mode, Area>;

    std::shared_ptr<Context> core_;
    mutable _details::concurrent::SharedMutex mtx_;

    PGBAR__FORCEINLINE void setup_if_null() &
    {
      if ( core_ == nullptr )
        PGBAR__UNLIKELY
        {
          std::lock_guard<_details::concurrent::SharedMutex> lock { mtx_ };
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
      PGBAR__ASSERT( rhs.active() == false );
      core_ = std::move( rhs.core_ );
    }
    DynamicBar& operator=( DynamicBar&& rhs ) & noexcept
    { // The thread insecurity here is deliberately designed.
      // Because for a move-only type, transferring ownership simultaneously
      // in multiple locations should not occur.
      PGBAR__TRUST( this != &rhs );
      PGBAR__ASSERT( active() == false );
      PGBAR__ASSERT( rhs.active() == false );
      core_ = std::move( rhs.core_ );
      return *this;
    }
    ~DynamicBar() = default;

    PGBAR__NODISCARD PGBAR__FORCEINLINE bool active() const noexcept
    {
      _details::concurrent::SharedLock<_details::concurrent::SharedMutex> lock { mtx_ };
      return core_ != nullptr && core_->online_count() != 0;
    }
    PGBAR__NODISCARD PGBAR__FORCEINLINE _details::types::Size size() const noexcept
    {
      _details::concurrent::SharedLock<_details::concurrent::SharedMutex> lock { mtx_ };
      return core_ != nullptr ? core_.use_count() - 1 : 0;
    }
    PGBAR__NODISCARD PGBAR__FORCEINLINE _details::types::Size active_count() const noexcept
    {
      _details::concurrent::SharedLock<_details::concurrent::SharedMutex> lock { mtx_ };
      return core_ != nullptr ? core_->online_count() : 0;
    }
    PGBAR__FORCEINLINE void reset()
    {
      std::lock_guard<_details::concurrent::SharedMutex> lock { mtx_ };
      if ( core_ != nullptr )
        core_->shut();
    }
    PGBAR__FORCEINLINE void abort() noexcept
    {
      std::lock_guard<_details::concurrent::SharedMutex> lock { mtx_ };
      if ( core_ != nullptr )
        core_->kill();
    }

    // Wait until the indicator is Stop.
    void wait() const noexcept
    {
      _details::concurrent::spin_wait( [this]() noexcept { return !active(); } );
    }
    // Wait for the indicator is Stop or timed out.
    template<class Rep, class Period>
    PGBAR__NODISCARD bool wait_for( const std::chrono::duration<Rep, Period>& timeout ) const noexcept
    {
      return _details::concurrent::spin_wait_for( [this]() noexcept { return !active(); }, timeout );
    }

    template<typename Config>
    PGBAR__NODISCARD auto insert( _details::prefabs::BasicBar<Config, Outlet, Mode, Area>&& bar )
#ifdef __cpp_concepts
      requires _details::traits::is_config<Config>::value
#else
      ->
      typename std::enable_if<_details::traits::is_config<Config>::value,
                              std::unique_ptr<_details::prefabs::BasicBar<Config, Outlet, Mode, Area>>>::type
#endif
    {
      setup_if_null();
      return _details::utils::make_unique<_details::prefabs::ManagedBar<Config, Outlet, Mode, Area>>(
        core_,
        std::move( bar ) );
    }
    template<typename Config>
    PGBAR__NODISCARD auto insert( Config cfg )
#ifdef __cpp_concepts
      requires _details::traits::is_config<Config>::value
#else
      ->
      typename std::enable_if<_details::traits::is_config<Config>::value,
                              std::unique_ptr<_details::prefabs::BasicBar<Config, Outlet, Mode, Area>>>::type
#endif
    {
      setup_if_null();
      return _details::utils::make_unique<_details::prefabs::ManagedBar<Config, Outlet, Mode, Area>>(
        core_,
        std::move( cfg ) );
    }

    template<typename Bar, typename... Options>
    PGBAR__NODISCARD auto insert( Options&&... options )
#ifdef __cpp_concepts
      requires( _details::traits::is_bar<Bar>::value && Bar::Channel == Outlet && Bar::Policy == Mode
                && Bar::Region == Area && std::is_constructible_v<Bar, Options && ...> )
#else
      -> typename std::enable_if<
        _details::traits::AllOf<_details::traits::is_bar<Bar>,
                                std::is_constructible<Bar, Options&&...>,
                                _details::traits::BoolConstant<( Bar::Channel == Outlet )>,
                                _details::traits::BoolConstant<( Bar::Policy == Mode )>,
                                _details::traits::BoolConstant<( Bar::Region == Area )>>::value,
        std::unique_ptr<Bar>>::type
#endif
    {
      setup_if_null();
      return _details::utils::make_unique<
        _details::prefabs::ManagedBar<typename Bar::Config, Outlet, Mode, Area>>(
        core_,
        std::forward<Options>( options )... );
    }
    template<typename Config, typename... Options>
    PGBAR__NODISCARD auto insert( Options&&... options )
#ifdef __cpp_concepts
      requires( _details::traits::is_config<Config>::value
                && std::is_constructible_v<Config, Options && ...> )
#else
      ->
      typename std::enable_if<_details::traits::AllOf<_details::traits::is_config<Config>,
                                                      std::is_constructible<Config, Options&&...>>::value,
                              std::unique_ptr<_details::prefabs::BasicBar<Config, Outlet, Mode, Area>>>::type
#endif
    {
      setup_if_null();
      return _details::utils::make_unique<_details::prefabs::ManagedBar<Config, Outlet, Mode, Area>>(
        core_,
        std::forward<Options>( options )... );
    }

    void swap( DynamicBar& other ) noexcept
    { // The thread insecurity here is deliberately designed.
      // The reason can be found in the move assignment.
      PGBAR__TRUST( this != &other );
      PGBAR__ASSERT( active() == false );
      PGBAR__ASSERT( lhs.active() == false );
      core_.swap( other.core_ );
    }
    friend void swap( DynamicBar& a, DynamicBar& b ) noexcept { a.swap( b ); }
  };

  // Creates a tuple of unique_ptr pointing to bars using existing bar instances.
  template<typename Config, typename... Configs, Channel O, Policy M, Region A>
  PGBAR__NODISCARD PGBAR__FORCEINLINE auto make_dynamic(
    _details::prefabs::BasicBar<Config, O, M, A>&& bar,
    _details::prefabs::BasicBar<Configs, O, M, A>&&... bars )
#ifdef __cpp_concepts
    requires( _details::traits::is_config<Config>::value
              && ( _details::traits::is_config<Configs>::value && ... ) )
#else
    -> typename std::enable_if<
      _details::traits::AllOf<_details::traits::is_config<Config>,
                              _details::traits::is_config<Configs>...>::value,
      std::tuple<std::unique_ptr<_details::prefabs::BasicBar<Config, O, M, A>>,
                 std::unique_ptr<_details::prefabs::BasicBar<Configs, O, M, A>>...>>::type
#endif
  {
    DynamicBar<O, M, A> factory;
    return std::make_tuple( factory.insert( std::move( bar ) ), factory.insert( std::move( bars ) )... );
  }
  // Creates a tuple of unique_ptr pointing to bars using configuration objects.
  template<Channel Outlet = Channel::Stderr,
           Policy Mode    = Policy::Async,
           Region Area    = Region::Fixed,
           typename Config,
           typename... Configs>
  PGBAR__NODISCARD PGBAR__FORCEINLINE auto make_dynamic( Config&& cfg, Configs&&... cfgs )
#ifdef __cpp_concepts
    requires( _details::traits::is_config<std::decay_t<Config>>::value
              && ( _details::traits::is_config<std::decay_t<Configs>>::value && ... ) )
#else
    -> typename std::enable_if<
      _details::traits::AllOf<_details::traits::is_config<typename std::decay<Config>::type>,
                              _details::traits::is_config<typename std::decay<Configs>::type>...>::value,
      std::tuple<
        std::unique_ptr<_details::prefabs::BasicBar<typename std::decay<Config>::type, Outlet, Mode, Area>>,
        std::unique_ptr<
          _details::prefabs::BasicBar<typename std::decay<Configs>::type, Outlet, Mode, Area>>...>>::type
#endif
  {
    DynamicBar<Outlet, Mode, Area> factory;
    return std::make_tuple( factory.insert( std::forward<Config>( cfg ) ),
                            factory.insert( std::forward<Configs>( cfgs ) )... );
  }

  /**
   * Creates a vector of unique_ptr pointing to bars with a fixed number of BasicBar instances.
   * **All BasicBar instances are initialized using the same configuration.**
   */
  template<typename Config, Channel O, Policy M, Region A>
  PGBAR__NODISCARD PGBAR__FORCEINLINE auto make_dynamic( _details::prefabs::BasicBar<Config, O, M, A>&& bar,
                                                         _details::types::Size count )
#ifdef __cpp_concepts
    requires _details::traits::is_config<Config>::value
#else
    ->
    typename std::enable_if<_details::traits::is_config<Config>::value,
                            std::vector<std::unique_ptr<_details::prefabs::BasicBar<Config, O, M, A>>>>::type
#endif
  {
    std::vector<std::unique_ptr<_details::prefabs::BasicBar<Config, O, M, A>>> products;
    if ( count == 0 )
      PGBAR__UNLIKELY return products;
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
  template<Channel Outlet = Channel::Stderr,
           Policy Mode    = Policy::Async,
           Region Area    = Region::Fixed,
           typename Config>
  PGBAR__NODISCARD PGBAR__FORCEINLINE auto make_dynamic( Config&& cfg, _details::types::Size count )
#ifdef __cpp_concepts
    requires _details::traits::is_config<std::decay_t<Config>>::value
#else
    -> typename std::enable_if<
      _details::traits::is_config<typename std::decay<Config>::type>::value,
      std::vector<std::unique_ptr<
        _details::prefabs::BasicBar<typename std::decay<Config>::type, Outlet, Mode, Area>>>>::type
#endif
  {
    std::vector<
      std::unique_ptr<_details::prefabs::BasicBar<typename std::decay<Config>::type, Outlet, Mode, Area>>>
      products;
    if ( count == 0 )
      PGBAR__UNLIKELY return products;
    DynamicBar<Outlet, Mode, Area> factory;
    std::generate_n( std::back_inserter( products ), count - 1, [&factory, &cfg]() {
      return factory.insert( cfg );
    } );
    products.emplace_back( factory.insert( std::forward<Config>( cfg ) ) );
    return products;
  }

  /**
   * Creates a vector of unique_ptr with a fixed number of bars using mutiple bar/configuration objects.
   * The ctor sequentially initializes the first few instances corresponding to the provided arguments;
   * **An unmatched count and Bars number will cause an exception `pgbar::exception::InvalidArgument`.**
   */
  template<typename Bar, typename... Objs>
  PGBAR__NODISCARD PGBAR__FORCEINLINE auto make_dynamic( _details::types::Size count, Objs&&... objs )
    noexcept( false )
#ifdef __cpp_concepts
    requires( _details::traits::is_bar<Bar>::value
              && ( ( ( std::is_same_v<std::remove_cv_t<Bar>, std::remove_cv_t<Objs>> && ... )
                     && !( std::is_lvalue_reference_v<Objs &&> || ... ) )
                   || ( std::is_same<typename Bar::Config, std::decay_t<Objs>>::value && ... ) ) )
#else
      ->
    typename std::enable_if<
      _details::traits::AllOf<
        _details::traits::is_bar<Bar>,
        _details::traits::AnyOf<
          _details::traits::AllOf<
            std::is_same<typename std::remove_cv<Bar>::type, typename std::remove_cv<Objs>::type>...,
            _details::traits::Not<_details::traits::AnyOf<std::is_lvalue_reference<Objs&&>...>>>,
          _details::traits::AllOf<std::is_same<typename Bar::Config, typename std::decay<Objs>::type>...>>>::
        value,
      std::vector<std::unique_ptr<Bar>>>::type
#endif
  {
    std::vector<std::unique_ptr<Bar>> products;
    if ( count == 0 )
      PGBAR__UNLIKELY return products;
    else if ( count < sizeof...( Objs ) )
      PGBAR__UNLIKELY
      {
        _details::charcodes::CoWString message =
          _details::charcodes::make_literal( "pgbar: provided object count (" );
        message.append( _details::utils::format( sizeof...( Objs ) ) )
          .append( ") exceeds the specified count (" )
          .append( _details::utils::format( count ) )
          .push_back( ')' );
        throw exception::InvalidArgument( std::move( message ) );
      }

    DynamicBar<Bar::Channel, Bar::Policy, Bar::Region> factory;
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
   * **An unmatched count and Bars number will cause an exception `pgbar::exception::InvalidArgument`.**
   */
  template<typename Config,
           Channel Outlet = Channel::Stderr,
           Policy Mode    = Policy::Async,
           Region Area    = Region::Fixed,
           typename... Configs>
  PGBAR__NODISCARD PGBAR__FORCEINLINE auto make_dynamic( _details::types::Size count, Configs&&... cfgs )
    noexcept( false )
#ifdef __cpp_concepts
    requires( _details::traits::is_config<Config>::value
              && ( std::is_same_v<std::remove_cv_t<Config>, std::decay_t<Configs>> && ... ) )
#else
      -> typename std::enable_if<
        _details::traits::AllOf<
          _details::traits::is_config<Config>,
          std::is_same<typename std::remove_cv<Config>::type, typename std::decay<Configs>::type>...>::value,
        std::vector<std::unique_ptr<_details::prefabs::BasicBar<Config, Outlet, Mode, Area>>>>::type
#endif
  {
    std::vector<std::unique_ptr<_details::prefabs::BasicBar<Config, Outlet, Mode, Area>>> products;
    if ( count == 0 )
      PGBAR__UNLIKELY return products;
    else if ( count < sizeof...( Configs ) )
      PGBAR__UNLIKELY
      {
        _details::charcodes::CoWString message =
          _details::charcodes::make_literal( "pgbar: provided configs count (" );
        message.append( _details::utils::format( sizeof...( Configs ) ) )
          .append( ") exceeds the specified count (" )
          .append( _details::utils::format( count ) )
          .push_back( ')' );
        throw exception::InvalidArgument( std::move( message ) );
      }

    DynamicBar<Outlet, Mode, Area> factory;
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
   * **An unmatched count and Bars number will cause an exception `pgbar::exception::InvalidArgument`.**
   */
  template<typename Config,
           Channel Outlet = Channel::Stderr,
           Policy Mode    = Policy::Async,
           Region Area    = Region::Fixed,
           typename... Configs>
  PGBAR__NODISCARD PGBAR__FORCEINLINE auto make_dynamic(
    _details::types::Size count,
    _details::prefabs::BasicBar<Configs, Outlet, Mode, Area>&&... bars ) noexcept( false )
#ifdef __cpp_concepts
    requires( _details::traits::is_config<Config>::value
              && ( std::is_same_v<Config, std::decay_t<Configs>> && ... ) )
#else
    -> typename std::enable_if<
      _details::traits::AllOf<_details::traits::is_config<Config>,
                              std::is_same<Config, typename std::decay<Configs>::type>...>::value,
      std::vector<std::unique_ptr<_details::prefabs::BasicBar<Config, Outlet, Mode, Area>>>>::type
#endif
  {
    std::vector<std::unique_ptr<_details::prefabs::BasicBar<Config, Outlet, Mode, Area>>> products;
    if ( count == 0 )
      PGBAR__UNLIKELY return products;
    else if ( count < sizeof...( Configs ) )
      PGBAR__UNLIKELY
      {
        _details::charcodes::CoWString message =
          _details::charcodes::make_literal( "pgbar: provided bar count (" );
        message.append( _details::utils::format( sizeof...( Configs ) ) )
          .append( ") exceeds the specified count (" )
          .append( _details::utils::format( count ) )
          .push_back( ')' );
        throw exception::InvalidArgument( std::move( message ) );
      }

    DynamicBar<Outlet, Mode, Area> factory;
    (void)std::initializer_list<bool> { ( products.emplace_back( factory.insert( std::move( bars ) ) ),
                                          false )... };
    std::generate_n( std::back_inserter( products ), count - sizeof...( Configs ) - 1, [&factory]() {
      return factory.template insert<Config>();
    } );
    return products;
  }
} // namespace pgbar

#endif
