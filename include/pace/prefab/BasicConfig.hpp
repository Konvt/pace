#ifndef PACE_BASIC_CONFIG
#define PACE_BASIC_CONFIG

#include "../details/aspects/Schema.hpp"
#include "../details/aspects/Segment.hpp"
#include "../details/aspects/Text.hpp"

namespace pace {
  namespace option {
    /**
     * A special type, only used to construct the default value of BasicConfig.
     * Please do not manually create this type; instead, use the type below for static inference.
     */
    struct Projection : PACE__DERIVING_OPTION2( Projection, std::vector<bool>, projection );

    template<template<typename...> class... Facades>
    // If there is an error here, it indicates that there are duplicate types involved.
    struct Only : public details::traits::TemplateSet<Facades...> {};
    template<template<typename...> class... Facades>
    struct Except : public details::traits::TemplateSet<Facades...> {};

#define PACE__METHOD( ParamType, ReturnType )                                  \
  template<template<typename...> class... Facades>                             \
  constexpr ReturnType<Facades...> operator!( ParamType<Facades...> ) noexcept \
  { return {}; }
    PACE__METHOD( Except, Only );
    PACE__METHOD( Only, Except );
#undef PACE__METHOD
#define PACE__METHOD( ParamType )                                                                         \
  template<template<typename...> class... F1, template<typename...> class... F2>                          \
  constexpr details::traits::TmpNominalCast_t<                                                            \
    details::traits::Combine_t<details::traits::TemplateSet<F1...>, details::traits::TemplateSet<F2...>>, \
    ParamType>                                                                                            \
    operator|( ParamType<F1...>, ParamType<F2...> ) noexcept                                              \
  { return {}; }
    PACE__METHOD( Only );
    PACE__METHOD( Except );
#undef PACE__METHOD
  } // namespace option

  namespace details {
    namespace traits {
      template<typename S>
      struct _impl_is_selection {
      private:
        template<template<typename...> class... Fs>
        static constexpr std::true_type check( const option::Only<Fs...>& );
        template<template<typename...> class... Fs>
        static constexpr std::true_type check( const option::Except<Fs...>& );
        static constexpr std::false_type check( ... );

      public:
        using result = AllOf<Not<std::is_reference<S>>,
                             decltype( check( std::declval<typename std::remove_cv<S>::type>() ) )>;
      };
      template<typename S>
      using is_selection = typename _impl_is_selection<S>::result;
    } // namespace traits
  } // namespace details

  namespace prefab {
    template<template<typename...> class... Facades>
    class BasicConfig
      : public details::traits::
          LI_t<details::aspects::Prefix, details::aspects::Postfix, details::aspects::Segment, Facades...>::
            template type<details::aspects::Schema, BasicConfig<Facades...>> {
      static_assert( details::traits::is_unique<details::traits::TemplateList<Facades...>>::value,
                     "redundant Facades are not allowed" );
      using Element = details::traits::TemplateSet<Facades...>;
      template<typename Option>
      using is_setting =
        details::traits::AnyOf<details::traits::TpContains<details::traits::OptionLinker_t<
                                                             details::traits::C3_t<details::aspects::Prefix,
                                                                                   details::aspects::Postfix,
                                                                                   details::aspects::Segment,
                                                                                   Facades...>>,
                                                           Option>,
                               details::traits::is_selection<Option>>;

      friend PACE__FORCEINLINE void unpack( BasicConfig& self, option::Projection proj ) noexcept
      {
        const details::types::Size length = std::min( sizeof...( Facades ), proj.value().size() );
        for ( details::types::Size i = 0; i < length; ++i )
          self.projection_.set( i, proj.value()[i] );
      }
      template<template<typename...> class... Fs>
      friend PACE__FORCEINLINE PACE__CXX14_CNSTXPR void unpack( BasicConfig& self,
                                                                option::Only<Fs...> ) noexcept
      {
        static_assert(
          details::traits::AllOf<
            details::traits::TmpContains<details::traits::TemplateSet<Facades...>, Fs>...>::value,
          "try to modifiy an unkonwn Facade" );
        self.projection_.reset();
        (void)std::initializer_list<bool> {
          ( self.projection_.set( details::traits::IndexIn<Fs, Facades...>::value ), false )...
        };
      }
      template<template<typename...> class... Fs>
      friend PACE__FORCEINLINE PACE__CXX14_CNSTXPR void unpack( BasicConfig& self,
                                                                option::Except<Fs...> ) noexcept
      {
        static_assert(
          details::traits::AllOf<
            details::traits::TmpContains<details::traits::TemplateSet<Facades...>, Fs>...>::value,
          "try to modifiy an unkonwn Facade" );
        self.projection_.set();
        (void)std::initializer_list<bool> {
          ( self.projection_.reset( details::traits::IndexIn<Fs, Facades...>::value ), false )...
        };
      }

    protected:
      std::bitset<sizeof...( Facades )> projection_;

      template<typename... Args
#ifdef __cpp_concepts
               >
        requires( is_setting<Args>::value && ... )
#else
               ,
               typename = typename std::enable_if<details::traits::AllOf<is_setting<Args>...>::value>::type>
#endif
      PACE__CXX23_CNSTXPR BasicConfig( details::traits::TypeSet<Args...> tag ) : layout_type( tag )
      {
        // Projection is only used for injecting default values.
        if PACE__CXX17_CNSTXPR ( !details::traits::AnyOf<details::traits::is_selection<Args>...>::value )
          unpack( *this, config::provide_for<BasicConfig, option::Projection>() );
      }

    public:
      using layout_type =
        typename details::traits::LI_t<details::aspects::Prefix,
                                       details::aspects::Postfix,
                                       details::aspects::Segment,
                                       Facades...>::template type<details::aspects::Schema, BasicConfig>;

      /**
       * Build a Projection that explicitly enables a selected subset of Facades.

       * The resulting projection disables all Facades by default, and only marks
       * the specified ones as enabled.

       * This is typically used when a bar configuration should expose only a
       * minimal or curated set of components.
       */
      template<template<typename...> class... Fs>
      static option::Projection bake( option::Only<Fs...> )
      {
        static_assert(
          details::traits::AllOf<
            details::traits::TmpContains<details::traits::TemplateSet<Facades...>, Fs>...>::value,
          "try to modifiy an unkonwn Facade" );
        std::vector<bool> projection;
        projection.assign( sizeof...( Facades ), false );
        (void)std::initializer_list<bool> { ( projection[details::traits::IndexIn<Fs, Facades...>::value] =
                                                true )... };
        return { std::move( projection ) };
      }
      /**
       * Build a Projection that disables a subset of Facades while keeping
       * all others enabled.

       * The resulting projection starts with all Facades enabled, and then
       * removes the specified ones.

       * This is typically used for standard configurations where most components
       * are enabled by default, with only a few exclusions.
       */
      template<template<typename...> class... Fs>
      static option::Projection bake( option::Except<Fs...> )
      {
        static_assert(
          details::traits::AllOf<
            details::traits::TmpContains<details::traits::TemplateSet<Facades...>, Fs>...>::value,
          "try to modifiy an unkonwn Facade" );
        std::vector<bool> projection;
        projection.assign( sizeof...( Facades ), true );
        (void)std::initializer_list<bool> { ( projection[details::traits::IndexIn<Fs, Facades...>::value] =
                                                false )... };
        return { std::move( projection ) };
      }

      template<typename... Args
#ifdef __cpp_concepts
               >
        requires( details::traits::is_unique<details::traits::TypeList<Args...>>::value
                  && ( is_setting<Args>::value && ... ) )
#else
               ,
               typename = typename std::enable_if<
                 details::traits::AllOf<details::traits::is_unique<details::traits::TypeList<Args...>>,
                                        is_setting<Args>...>::value>::type>
#endif
      PACE__CXX23_CNSTXPR BasicConfig( Args... args )
        : BasicConfig( details::traits::TypeSet<typename std::decay<Args>::type...>() )
      { (void)std::initializer_list<bool> { ( unpack( *this, std::move( args ) ), false )... }; }

      BasicConfig( const BasicConfig& other ) noexcept( std::is_nothrow_copy_assignable<layout_type>::value )
      {
        std::lock_guard<details::concurrent::SharedMutex> lock { other.rw_mtx_ };
        layout_type::operator=( other );
        projection_ = other.projection_;
      }
      BasicConfig( BasicConfig&& rhs ) noexcept
      {
        std::lock_guard<details::concurrent::SharedMutex> lock { rhs.rw_mtx_ };
        layout_type::operator=( std::move( rhs ) );
        using std::swap;
        swap( projection_, rhs.projection_ );
      }
      BasicConfig& operator=( const BasicConfig& other ) & noexcept(
        std::is_nothrow_copy_assignable<layout_type>::value )
      {
        PACE__TRUST( this != &other );
        details::concurrent::SharedLock<details::concurrent::SharedMutex> lock1 { other.rw_mtx_,
                                                                                  std::defer_lock };
        std::lock( this->rw_mtx_, lock1 );
        std::lock_guard<details::concurrent::SharedMutex> lock2 { this->rw_mtx_, std::adopt_lock };

        projection_ = other.projection_;
        layout_type::operator=( other );
        return *this;
      }
      BasicConfig& operator=( BasicConfig&& rhs ) & noexcept
      { // To support concurrent modifications of a active progress bar,
        // we have to lock them during the movement
        std::lock( this->rw_mtx_, rhs.rw_mtx_ );
        std::lock_guard<details::concurrent::SharedMutex> lock1 { this->rw_mtx_, std::adopt_lock };
        std::lock_guard<details::concurrent::SharedMutex> lock2 { rhs.rw_mtx_, std::adopt_lock };

        PACE__TRUST( this != &rhs );
        using std::swap;
        swap( projection_, rhs.projection_ );
        layout_type::operator=( std::move( rhs ) );
        return *this;
      }
      /**
       * Note: Because there are no exposed public base classes for config type,
       * there should be no scenarios for managing derived objects using base class references.
       * So the destructor here is deliberately set to be non-virtual.
       */
      ~BasicConfig() = default;

      template<typename Arg, typename... Args>
      BasicConfig& with( Arg arg, Args... args ) &
#ifdef __cpp_concepts
        requires( details::traits::is_unique<details::traits::TypeList<Arg, Args...>>::value
                  && is_setting<Arg>::value && ( is_setting<Args>::value && ... ) )
#endif
      {
#ifndef __cpp_concepts
        static_assert( details::traits::is_unique<details::traits::TypeList<Arg, Args...>>::value,
                       "passed options cannot be repeated" );
        static_assert( details::traits::AllOf<is_setting<Arg>, is_setting<Args>...>::value,
                       "passed arguments must be valid options" );
#endif
        std::lock_guard<details::concurrent::SharedMutex> lock { this->rw_mtx_ };
        unpack( *this, std::move( arg ) );
        (void)std::initializer_list<bool> { ( unpack( *this, std::move( args ) ), false )... };
        return *this;
      }
      template<typename Arg, typename... Args>
      BasicConfig&& with( Arg arg, Args... args ) &&
#ifdef __cpp_concepts
        requires( details::traits::is_unique<details::traits::TypeList<Arg, Args...>>::value
                  && is_setting<Arg>::value && ( is_setting<Args>::value && ... ) )
#endif
      {
#ifndef __cpp_concepts
        static_assert( details::traits::is_unique<details::traits::TypeList<Arg, Args...>>::value,
                       "passed options cannot be repeated" );
        static_assert( details::traits::AllOf<is_setting<Arg>, is_setting<Args>...>::value,
                       "passed arguments must be valid options" );
#endif
        std::lock_guard<details::concurrent::SharedMutex> lock { this->rw_mtx_ };
        unpack( *this, std::move( arg ) );
        (void)std::initializer_list<bool> { ( unpack( *this, std::move( args ) ), false )... };
        return std::move( *this );
      }

      /**
       * Since the length of a progress bar is directly related to the way it is rendered,
       * do not obtain `fixed_width()` on a bare configuration type object;
       * instead, access the `fixed_width()` of the internal configuration object
       * through the `config()` method of a progress bar object.
       */
      PACE__NODISCARD virtual std::uint64_t fixed_width() const noexcept
      {
        PACE__TRUST( false );
        return ( std::numeric_limits<std::uint64_t>::max )();
      }

      BasicConfig& enable_all() & noexcept
      {
        std::lock_guard<details::concurrent::SharedMutex> lock { this->rw_mtx_ };
        unpack( *this, option::Except<>() );
        return *this;
      }
      BasicConfig&& enable_all() && noexcept
      {
        std::lock_guard<details::concurrent::SharedMutex> lock { this->rw_mtx_ };
        unpack( *this, option::Except<>() );
        return std::move( *this );
      }

      BasicConfig& disable_all() & noexcept
      {
        std::lock_guard<details::concurrent::SharedMutex> lock { this->rw_mtx_ };
        unpack( *this, option::Only<>() );
        return *this;
      }
      BasicConfig&& disable_all() && noexcept
      {
        std::lock_guard<details::concurrent::SharedMutex> lock { this->rw_mtx_ };
        unpack( *this, option::Only<>() );
        return std::move( *this );
      }

      // Enable the new facade based on the current state.
      template<template<typename...> class F, template<typename...> class... Fs>
      BasicConfig& enable() & noexcept
#ifdef __cpp_concepts
        requires( details::traits::TmpContains<Element, F>::value
                  && ( details::traits::TmpContains<Element, Fs>::value && ... ) )
#endif
      {
#ifndef __cpp_concepts
        static_assert( details::traits::AllOf<details::traits::TmpContains<Element, F>,
                                              details::traits::TmpContains<Element, Fs>...>::value,
                       "enabled facades must be part of the config object" );
#endif
        std::lock_guard<details::concurrent::SharedMutex> lock { this->rw_mtx_ };
        projection_.set( details::traits::IndexIn<F, Facades...>::value );
        (void)std::initializer_list<bool> {
          ( projection_.set( details::traits::IndexIn<Fs, Facades...>::value ), false )...
        };
        return *this;
      }
      // Enable the new facade based on the current state.
      template<template<typename...> class F, template<typename...> class... Fs>
      BasicConfig&& enable() && noexcept
#ifdef __cpp_concepts
        requires( details::traits::TmpContains<Element, F>::value
                  && ( details::traits::TmpContains<Element, Fs>::value && ... ) )
#endif
      {
#ifndef __cpp_concepts
        static_assert( details::traits::AllOf<details::traits::TmpContains<Element, F>,
                                              details::traits::TmpContains<Element, Fs>...>::value,
                       "enabled facades must be part of the config object" );
#endif
        std::lock_guard<details::concurrent::SharedMutex> lock { this->rw_mtx_ };
        projection_.set( details::traits::IndexIn<F, Facades...>::value );
        (void)std::initializer_list<bool> {
          ( projection_.set( details::traits::IndexIn<Fs, Facades...>::value ), false )...
        };
        return std::move( *this );
      }

      // Disable the new facade based on the current state.
      template<template<typename...> class F, template<typename...> class... Fs>
      BasicConfig& disable() & noexcept
#ifdef __cpp_concepts
        requires( details::traits::TmpContains<Element, F>::value
                  && ( details::traits::TmpContains<Element, Fs>::value && ... ) )
#endif
      {
#ifndef __cpp_concepts
        static_assert( details::traits::AllOf<details::traits::TmpContains<Element, F>,
                                              details::traits::TmpContains<Element, Fs>...>::value,
                       "disabled facades must be part of the config object" );
#endif
        std::lock_guard<details::concurrent::SharedMutex> lock { this->rw_mtx_ };
        projection_.reset( details::traits::IndexIn<F, Facades...>::value );
        (void)std::initializer_list<bool> {
          ( projection_.reset( details::traits::IndexIn<Fs, Facades...>::value ), false )...
        };
        return *this;
      }
      // Disable the new facade based on the current state.
      template<template<typename...> class F, template<typename...> class... Fs>
      BasicConfig&& disable() && noexcept
#ifdef __cpp_concepts
        requires( details::traits::TmpContains<Element, F>::value
                  && ( details::traits::TmpContains<Element, Fs>::value && ... ) )
#endif
      {
#ifndef __cpp_concepts
        static_assert( details::traits::AllOf<details::traits::TmpContains<Element, F>,
                                              details::traits::TmpContains<Element, Fs>...>::value,
                       "disabled facades must be part of the config object" );
#endif
        std::lock_guard<details::concurrent::SharedMutex> lock { this->rw_mtx_ };
        projection_.reset( details::traits::IndexIn<F, Facades...>::value );
        (void)std::initializer_list<bool> {
          ( projection_.reset( details::traits::IndexIn<Fs, Facades...>::value ), false )...
        };
        return std::move( *this );
      }

      PACE__CXX23_CNSTXPR void swap( BasicConfig& other ) noexcept
      {
        std::lock( this->rw_mtx_, other.rw_mtx_ );
        std::lock_guard<details::concurrent::SharedMutex> lock1 { this->rw_mtx_, std::adopt_lock };
        std::lock_guard<details::concurrent::SharedMutex> lock2 { other.rw_mtx_, std::adopt_lock };

        using std::swap;
        swap( projection_, other.projection_ );
        layout_type::swap( other );
      }
      friend PACE__CXX23_CNSTXPR void swap( BasicConfig& a, BasicConfig& b ) noexcept { a.swap( b ); }

      template<typename Option>
      friend auto operator<<( BasicConfig& cfg, Option&& opt )
#ifdef __cpp_concepts
        -> decltype( auto )
        requires is_setting<Option>::value
#else
        -> typename std::enable_if<is_setting<Option>::value, BasicConfig&>::type
#endif
      { return cfg.with( std::forward<Option>( opt ) ); }
      template<typename Option>
      friend auto operator<<( BasicConfig&& cfg, Option&& opt )
#ifdef __cpp_concepts
        -> decltype( auto )
        requires is_setting<std::decay_t<Option>>::value
#else
        -> typename std::enable_if<is_setting<typename std::decay<Option>::type>::value, BasicConfig&&>::type
#endif
      { return std::move( cfg.with( std::forward<Option>( opt ) ) ); }
    };
  } // namespace prefab

  namespace details {
    namespace traits {
      template<typename C>
      struct _impl_is_config {
      private:
        template<template<typename...> class... Fs>
        static constexpr std::true_type check( const prefab::BasicConfig<Fs...>& );
        static constexpr std::false_type check( ... );

      public:
        using result = AllOf<Not<std::is_reference<C>>,
                             decltype( check( std::declval<typename std::remove_cv<C>::type>() ) )>;
      };
      template<typename C>
      using is_config = typename _impl_is_config<C>::result;
    } // namespace traits
  } // namespace details
} // namespace pace

#endif
