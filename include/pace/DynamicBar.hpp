#ifndef PACE_DYNAMIC_BAR
#define PACE_DYNAMIC_BAR

#include "details/assets/DynamicLayout.hpp"

namespace pace {
  template<Channel Sink = Channel::Err, Policy Mode = Policy::Async, Region Zone = Region::Fixed>
  class DynamicBar {
    using Context = details::assets::DynamicLayout<Sink, Mode, Zone>;

    std::shared_ptr<Context> core_;
    mutable details::concurrent::SharedMutex mtx_;
    std::atomic<bool> initr_ { false };

    PACE__FORCEINLINE void setup_if_null() &
    {
      if ( !initr_.load( std::memory_order_relaxed ) ) {
        std::lock_guard<details::concurrent::SharedMutex> lock { mtx_ };
        if ( !initr_.load( std::memory_order_relaxed ) ) {
          PACE__ASSERT( core_ == nullptr );
          core_ = std::make_shared<Context>();
          initr_.store( true, std::memory_order_release );
        }
      }
    }

  public:
    static constexpr Channel channel_value = Sink;
    static constexpr Policy policy_value   = Mode;
    static constexpr Region region_value   = Zone;

    DynamicBar()                               = default;
    DynamicBar( const DynamicBar& )            = delete;
    DynamicBar& operator=( const DynamicBar& ) = delete;
    DynamicBar( DynamicBar&& rhs ) noexcept
    {
      PACE__ASSERT( rhs.active() == false );
      core_ = std::move( rhs.core_ );
      if ( core_ != nullptr )
        initr_.store( true, std::memory_order_relaxed );
      rhs.initr_.store( false, std::memory_order_release );
    }
    DynamicBar& operator=( DynamicBar&& rhs ) & noexcept
    { // The thread insecurity here is deliberately designed.
      // Because for a move-only type, transferring ownership simultaneously
      // in multiple locations should not occur.
      PACE__TRUST( this != &rhs );
      PACE__ASSERT( active() == false );
      PACE__ASSERT( rhs.active() == false );
      core_ = std::move( rhs.core_ );
      initr_.store( core_ != nullptr, std::memory_order_release );
      rhs.initr_.store( false, std::memory_order_release );
      return *this;
    }
    ~DynamicBar() = default;

    PACE__NODISCARD PACE__FORCEINLINE bool active() const noexcept
    {
      details::concurrent::SharedLock<details::concurrent::SharedMutex> lock { mtx_ };
      return core_ != nullptr && core_->online();
    }
    /// @return The number of progress bars associated with the current object.
    PACE__NODISCARD PACE__FORCEINLINE std::size_t size() const noexcept
    {
      details::concurrent::SharedLock<details::concurrent::SharedMutex> lock { mtx_ };
      return core_ != nullptr ? core_.use_count() - 1 : 0;
    }
    /// @return The number of terminal lines currently occupied by the progress bar,
    //          which is useful when using `Region::Relative` layout and needing to output extra information.
    PACE__NODISCARD PACE__FORCEINLINE std::size_t layout_height() const noexcept
    {
      details::concurrent::SharedLock<details::concurrent::SharedMutex> lock { mtx_ };
      return core_ != nullptr ? core_->height() : 0;
    }

    template<typename Soul>
    PACE__NODISCARD auto insert( prefab::BasicBar<Soul, Sink, Mode, Zone>&& bar )
      -> std::unique_ptr<prefab::BasicBar<Soul, Sink, Mode, Zone>>
    {
      setup_if_null();
      return details::utils::make_unique<details::assets::ManagedBar<Soul, Sink, Mode, Zone>>(
        core_,
        std::move( bar ) );
    }
    template<typename Soul>
    PACE__NODISCARD auto insert( Soul cfg ) -> std::unique_ptr<prefab::BasicBar<Soul, Sink, Mode, Zone>>
#ifdef __cpp_concepts
      requires details::traits::is_config<typename std::decay<Soul>::type>::value
#endif
    {
#ifndef __cpp_concepts
      static_assert( details::traits::is_config<typename std::decay<Soul>::type>::value,
                     "not a configuration" );
#endif
      setup_if_null();
      return details::utils::make_unique<details::assets::ManagedBar<Soul, Sink, Mode, Zone>>(
        core_,
        std::move( cfg ) );
    }

    template<typename Bar, typename... Options>
    PACE__NODISCARD auto insert( Options&&... options )
#ifdef __cpp_concepts
      -> std::unique_ptr<Bar>
      requires( std::is_base_of_v<Indicator, Bar> && Bar::channel_value == Sink && Bar::policy_value == Mode
                && Bar::region_value == Zone && std::is_constructible_v<Bar, Options...> )
#else
      -> typename std::enable_if<
        details::traits::AllOf<std::is_base_of<Indicator, Bar>,
                               std::is_constructible<Bar, Options...>,
                               details::traits::BoolConstant<( Bar::channel_value == Sink )>,
                               details::traits::BoolConstant<( Bar::policy_value == Mode )>,
                               details::traits::BoolConstant<( Bar::region_value == Zone )>>::value,
        std::unique_ptr<Bar>>::type
#endif
    {
      setup_if_null();
      return details::utils::make_unique<
        details::assets::ManagedBar<typename Bar::config_type, Sink, Mode, Zone>>(
        core_,
        std::forward<Options>( options )... );
    }
    template<typename Soul, typename... Options>
    PACE__NODISCARD auto insert( Options&&... options )
#ifdef __cpp_concepts
      -> std::unique_ptr<prefab::BasicBar<Soul, Sink, Mode, Zone>>
      requires( details::traits::is_config<Soul>::value && std::is_constructible_v<Soul, Options...> )
#else
      -> typename std::enable_if<details::traits::AllOf<details::traits::is_config<Soul>,
                                                        std::is_constructible<Soul, Options...>>::value,
                                 std::unique_ptr<prefab::BasicBar<Soul, Sink, Mode, Zone>>>::type
#endif
    {
      setup_if_null();
      return details::utils::make_unique<details::assets::ManagedBar<Soul, Sink, Mode, Zone>>(
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
      initr_.store( core_ != nullptr, std::memory_order_release );
      other.initr_.store( other.core_ != nullptr, std::memory_order_release );
    }
    friend void swap( DynamicBar& a, DynamicBar& b ) noexcept { a.swap( b ); }
  };

  // Creates a tuple of unique_ptr pointing to bars using existing bar instances.
  template<typename Soul, typename... Souls, Channel S, Policy M, Region Z>
  PACE__NODISCARD PACE__FORCEINLINE auto make_dynamic( prefab::BasicBar<Soul, S, M, Z>&& bar,
                                                       prefab::BasicBar<Souls, S, M, Z>&&... bars )
    -> std::tuple<std::unique_ptr<prefab::BasicBar<Soul, S, M, Z>>,
                  std::unique_ptr<prefab::BasicBar<Souls, S, M, Z>>...>
  {
    DynamicBar<S, M, Z> factory;
    return std::make_tuple( factory.insert( std::move( bar ) ), factory.insert( std::move( bars ) )... );
  }
  // Creates a tuple of unique_ptr pointing to bars using configuration objects.
  template<Channel Sink = Channel::Err,
           Policy Mode  = Policy::Async,
           Region Zone  = Region::Fixed,
           typename Soul,
           typename... Souls>
  PACE__NODISCARD PACE__FORCEINLINE auto make_dynamic( Soul&& cfg, Souls&&... cfgs )
#ifdef __cpp_concepts
    -> std::tuple<std::unique_ptr<prefab::BasicBar<std::decay_t<Soul>, Sink, Mode, Zone>>,
                  std::unique_ptr<prefab::BasicBar<std::decay_t<Souls>, Sink, Mode, Zone>>...>
    requires( details::traits::is_config<std::decay_t<Soul>>::value
              && ( details::traits::is_config<std::decay_t<Souls>>::value && ... ) )
#else
    -> typename std::enable_if<
      details::traits::AllOf<details::traits::is_config<typename std::decay<Soul>::type>,
                             details::traits::is_config<typename std::decay<Souls>::type>...>::value,
      std::tuple<std::unique_ptr<prefab::BasicBar<typename std::decay<Soul>::type, Sink, Mode, Zone>>,
                 std::unique_ptr<prefab::BasicBar<typename std::decay<Souls>::type, Sink, Mode, Zone>>...>>::
      type
#endif
  {
    DynamicBar<Sink, Mode, Zone> factory;
    return std::make_tuple( factory.insert( std::forward<Soul>( cfg ) ),
                            factory.insert( std::forward<Souls>( cfgs ) )... );
  }

  /**
   * Creates a vector of unique_ptr pointing to bars with a fixed number of BasicBar instances.
   * **All BasicBar instances are initialized using the same configuration.**
   */
  template<typename Soul, Channel S, Policy M, Region Z>
  PACE__NODISCARD PACE__FORCEINLINE auto make_dynamic( prefab::BasicBar<Soul, S, M, Z>&& bar,
                                                       std::size_t count )
    -> std::vector<std::unique_ptr<prefab::BasicBar<Soul, S, M, Z>>>
  {
    std::vector<std::unique_ptr<prefab::BasicBar<Soul, S, M, Z>>> products;
    if ( count == 0 )
      PACE__UNLIKELY return products;
    DynamicBar<S, M, Z> factory;
    std::generate_n( std::back_inserter( products ), count - 1, [&factory, &bar]() {
      return factory.insert( bar.Soul() );
    } );
    products.emplace_back( factory.insert( std::move( bar ) ) );
    return products;
  }
  /**
   * Creates a vector of unique_ptr pointing to bars with a fixed number of BasicBar instances.
   * **All BasicBar instances are initialized using the same configuration.**
   */
  template<Channel Sink = Channel::Err,
           Policy Mode  = Policy::Async,
           Region Zone  = Region::Fixed,
           typename Soul>
  PACE__NODISCARD PACE__FORCEINLINE auto make_dynamic( Soul&& cfg, std::size_t count )
    -> std::vector<std::unique_ptr<prefab::BasicBar<typename std::decay<Soul>::type, Sink, Mode, Zone>>>
#ifdef __cpp_concepts
    requires details::traits::is_config<std::decay_t<Soul>>::value
#endif
  {
#ifndef __cpp_concepts
    static_assert( details::traits::is_config<typename std::decay<Soul>::type>::value,
                   "not a configuration" );
#endif
    std::vector<std::unique_ptr<prefab::BasicBar<typename std::decay<Soul>::type, Sink, Mode, Zone>>>
      products;
    if ( count == 0 )
      PACE__UNLIKELY return products;
    DynamicBar<Sink, Mode, Zone> factory;
    std::generate_n( std::back_inserter( products ), count - 1, [&factory, &cfg]() {
      return factory.insert( cfg );
    } );
    products.emplace_back( factory.insert( std::forward<Soul>( cfg ) ) );
    return products;
  }

  /**
   * Creates a vector of unique_ptr with a fixed number of bars using mutiple bar/configuration objects.
   * The ctor sequentially initializes the first few instances corresponding to the provided arguments;
   * **An unmatched count and Bars number will cause an exception `pace::exception::InvalidArgument`.**
   */
  template<typename Bar, typename... Objs>
  PACE__NODISCARD PACE__FORCEINLINE auto make_dynamic( std::size_t count, Objs&&... objs )
#ifdef __cpp_concepts
    -> std::vector<std::unique_ptr<Bar>>
    requires( ( ( std::is_same_v<std::remove_cv_t<Bar>, std::remove_cv_t<Objs>> && ... )
                && !( std::is_lvalue_reference_v<Objs&&> || ... ) )
              || ( std::is_same<typename Bar::config_type, std::decay_t<Objs>>::value && ... ) )
#else
    -> typename std::enable_if<
      details::traits::AnyOf<
        details::traits::AllOf<
          std::is_same<typename std::remove_cv<Bar>::type, typename std::remove_cv<Objs>::type>...,
          details::traits::Not<details::traits::AnyOf<std::is_lvalue_reference<Objs&&>...>>>,
        details::traits::AllOf<std::is_same<typename Bar::config_type, typename std::decay<Objs>::type>...>>::
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
        details::charcodes::SharedString message =
          details::charcodes::make_literal( "pace: provided object count (" );
        details::utils::format_to( std::back_inserter( message ), sizeof...( Objs ) );
        message.append( ") exceeds the specified count (" );
        details::utils::format_to( std::back_inserter( message ), count );
        message.push_back( ')' );
        throw exception::InvalidArgument( std::move( message ) );
      }

    DynamicBar<Bar::channel_value, Bar::policy_value, Bar::region_value> factory;
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
  template<typename Soul,
           Channel Sink = Channel::Err,
           Policy Mode  = Policy::Async,
           Region Zone  = Region::Fixed,
           typename... Souls>
  PACE__NODISCARD PACE__FORCEINLINE auto make_dynamic( std::size_t count, Souls&&... cfgs )
#ifdef __cpp_concepts
    -> std::vector<std::unique_ptr<prefab::BasicBar<Soul, Sink, Mode, Zone>>>
    requires( details::traits::is_config<Soul>::value
              && ( std::is_same_v<std::remove_cv_t<Soul>, std::decay_t<Souls>> && ... ) )
#else
    ->
    typename std::enable_if<details::traits::AllOf<details::traits::is_config<Soul>,
                                                   std::is_same<typename std::remove_cv<Soul>::type,
                                                                typename std::decay<Souls>::type>...>::value,
                            std::vector<std::unique_ptr<prefab::BasicBar<Soul, Sink, Mode, Zone>>>>::type
#endif
  {
    std::vector<std::unique_ptr<prefab::BasicBar<Soul, Sink, Mode, Zone>>> products;
    if ( count == 0 )
      PACE__UNLIKELY return products;
    else if ( count < sizeof...( Souls ) )
      PACE__UNLIKELY
      {
        details::charcodes::SharedString message =
          details::charcodes::make_literal( "pace: provided Souls count (" );
        details::utils::format_to( std::back_inserter( message ), sizeof...( Souls ) );
        message.append( ") exceeds the specified count (" );
        details::utils::format_to( std::back_inserter( message ), count );
        message.push_back( ')' );
        throw exception::InvalidArgument( std::move( message ) );
      }

    DynamicBar<Sink, Mode, Zone> factory;
    (void)std::initializer_list<bool> {
      ( products.emplace_back( factory.insert( std::forward<Souls>( cfgs ) ) ), false )...
    };
    std::generate_n( std::back_inserter( products ), count - sizeof...( Souls ) - 1, [&factory]() {
      return factory.template insert<Soul>();
    } );
    return products;
  }
  /**
   * Creates a vector of unique_ptr with a fixed number of BasicBar instances using multiple bar objects.
   * The ctor sequentially initializes the first few instances corresponding to the provided objects;
   * **An unmatched count and Bars number will cause an exception `pace::exception::InvalidArgument`.**
   */
  template<typename Soul,
           Channel Sink = Channel::Err,
           Policy Mode  = Policy::Async,
           Region Zone  = Region::Fixed,
           typename... Souls>
  PACE__NODISCARD PACE__FORCEINLINE auto make_dynamic( std::size_t count,
                                                       prefab::BasicBar<Souls, Sink, Mode, Zone>&&... bars )
#ifdef __cpp_concepts
    -> std::vector<std::unique_ptr<prefab::BasicBar<Soul, Sink, Mode, Zone>>>
    requires( std::is_same_v<Soul, std::decay_t<Souls>> && ... )
#else
    -> typename std::enable_if<
      details::traits::AllOf<std::is_same<Soul, typename std::decay<Souls>::type>...>::value,
      std::vector<std::unique_ptr<prefab::BasicBar<Soul, Sink, Mode, Zone>>>>::type
#endif
  {
    std::vector<std::unique_ptr<prefab::BasicBar<Soul, Sink, Mode, Zone>>> products;
    if ( count == 0 )
      PACE__UNLIKELY return products;
    else if ( count < sizeof...( Souls ) )
      PACE__UNLIKELY
      {
        details::charcodes::SharedString message =
          details::charcodes::make_literal( "pace: provided bar count (" );
        details::utils::format_to( std::back_inserter( message ), sizeof...( Souls ) );
        message.append( ") exceeds the specified count (" );
        details::utils::format_to( std::back_inserter( message ), count );
        message.push_back( ')' );
        throw exception::InvalidArgument( std::move( message ) );
      }

    DynamicBar<Sink, Mode, Zone> factory;
    (void)std::initializer_list<bool> { ( products.emplace_back( factory.insert( std::move( bars ) ) ),
                                          false )... };
    std::generate_n( std::back_inserter( products ), count - sizeof...( Souls ) - 1, [&factory]() {
      return factory.template insert<Soul>();
    } );
    return products;
  }
} // namespace pace

#endif
