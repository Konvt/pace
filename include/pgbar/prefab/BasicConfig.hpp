#ifndef PGBAR_BASIC_CONFIG
#define PGBAR_BASIC_CONFIG

#include "../details/aspects/Schema.hpp"
#include "../details/aspects/Segment.hpp"
#include "../details/aspects/Text.hpp"

namespace pgbar {
  namespace option {
    /**
     * A special type, only used to construct the default value of BasicConfig.
     * Please do not manually create this type; instead, use the type below for static inference.
     */
    struct Projection : PGBAR__DERIVING_OPTION1( Projection, std::vector<bool>, projection );

    template<template<typename...> class... Facades>
    // If there is an error here, it indicates that there are duplicate types involved.
    struct Only : public details::traits::TemplateSet<Facades...> {};
    template<template<typename...> class... Facades>
    struct Except : public details::traits::TemplateSet<Facades...> {};

#define PGBAR__METHOD( ParamType, ReturnType )                                 \
  template<template<typename...> class... Facades>                             \
  constexpr ReturnType<Facades...> operator!( ParamType<Facades...> ) noexcept \
  {                                                                            \
    return {};                                                                 \
  }
    PGBAR__METHOD( Except, Only );
    PGBAR__METHOD( Only, Except );
#undef PGBAR__METHOD
#define PGBAR__METHOD( ParamType )                                                                        \
  template<template<typename...> class... F1, template<typename...> class... F2>                          \
  constexpr details::traits::TmpNominalCast_t<                                                            \
    details::traits::Combine_t<details::traits::TemplateSet<F1...>, details::traits::TemplateSet<F2...>>, \
    ParamType>                                                                                            \
    operator|( ParamType<F1...>, ParamType<F2...> ) noexcept                                              \
  {                                                                                                       \
    return {};                                                                                            \
  }
    PGBAR__METHOD( Only );
    PGBAR__METHOD( Except );
#undef PGBAR__METHOD
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
      static_assert( details::traits::Distinct<details::traits::TemplateList<Facades...>>::value,
                     "redundant Facades are not allowed" );
      using Element = details::traits::TemplateSet<Facades...>;
      template<typename Option>
      using is_setting =
        details::traits::AnyOf<details::traits::TpContain<details::traits::OptionLinker_t<
                                                            details::traits::C3_t<details::aspects::Prefix,
                                                                                  details::aspects::Postfix,
                                                                                  details::aspects::Segment,
                                                                                  Facades...>>,
                                                          Option>,
                               details::traits::is_selection<Option>>;

      friend PGBAR__FORCEINLINE void unpack( BasicConfig& self, option::Projection proj ) noexcept
      {
        const size_t length = std::min( sizeof...( Facades ), proj.value().size() );
        for ( size_t i = 0; i < length; ++i )
          self.projection_.set( i, proj.value()[i] );
      }
      template<template<typename...> class... Fs>
      friend PGBAR__FORCEINLINE PGBAR__CXX14_CNSTXPR void unpack( BasicConfig& self,
                                                                  option::Only<Fs...> ) noexcept
      {
        static_assert( details::traits::AllOf<
                         details::traits::TmpContain<details::traits::TemplateSet<Facades...>, Fs>...>::value,
                       "try to modifiy an unkonwn Facade" );
        self.projection_.reset();
        (void)std::initializer_list<bool> {
          ( self.projection_.set( details::traits::IndexIn<Fs, Facades...>::value ), false )...
        };
      }
      template<template<typename...> class... Fs>
      friend PGBAR__FORCEINLINE PGBAR__CXX14_CNSTXPR void unpack( BasicConfig& self,
                                                                  option::Except<Fs...> ) noexcept
      {
        static_assert( details::traits::AllOf<
                         details::traits::TmpContain<details::traits::TemplateSet<Facades...>, Fs>...>::value,
                       "try to modifiy an unkonwn Facade" );
        self.projection_.set();
        (void)std::initializer_list<bool> {
          ( self.projection_.reset( details::traits::IndexIn<Fs, Facades...>::value ), false )...
        };
      }

    protected:
      std::bitset<sizeof...( Facades )> projection_;

      template<typename... Options>
      PGBAR__CXX23_CNSTXPR BasicConfig( details::traits::TypeSet<Options...> tag ) : Layout( tag )
      {
        // Projection is only used for injecting default values.
        if PGBAR__CXX17_CNSTXPR ( !details::traits::AnyOf<details::traits::is_selection<Options>...>::value )
          unpack( *this, config::provide_for<BasicConfig, option::Projection>() );
      }

    public:
      using Layout =
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
        static_assert( details::traits::AllOf<
                         details::traits::TmpContain<details::traits::TemplateSet<Facades...>, Fs>...>::value,
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
        static_assert( details::traits::AllOf<
                         details::traits::TmpContain<details::traits::TemplateSet<Facades...>, Fs>...>::value,
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
        requires( details::traits::Distinct<details::traits::TypeList<Args...>>::value
                  && ( is_setting<Args>::value && ... ) )
#else
               ,
               typename = typename std::enable_if<
                 details::traits::AllOf<details::traits::Distinct<details::traits::TypeList<Args...>>,
                                        is_setting<Args>...>::value>::type>
#endif
      PGBAR__CXX23_CNSTXPR BasicConfig( Args... args )
        : BasicConfig( details::traits::TypeSet<typename std::decay<Args>::type...>() )
      {
        (void)std::initializer_list<bool> { ( unpack( *this, std::move( args ) ), false )... };
      }

      BasicConfig( const BasicConfig& other ) noexcept( std::is_nothrow_copy_assignable<Layout>::value )
      {
        std::lock_guard<details::concurrent::SharedMutex> lock { other.rw_mtx_ };
        Layout::operator=( other );
        projection_ = other.projection_;
      }
      BasicConfig( BasicConfig&& rhs ) noexcept
      {
        Layout::operator=( std::move( rhs ) );
        using std::swap;
        swap( projection_, rhs.projection_ );
      }
      BasicConfig& operator=( const BasicConfig& other ) & noexcept(
        std::is_nothrow_copy_assignable<Layout>::value )
      {
        PGBAR__TRUST( this != &other );
        details::concurrent::SharedLock<details::concurrent::SharedMutex> lock1 { other.rw_mtx_,
                                                                                  std::defer_lock };
        std::lock( this->rw_mtx_, lock1 );
        std::lock_guard<details::concurrent::SharedMutex> lock2 { this->rw_mtx_, std::adopt_lock };

        projection_ = other.projection_;
        Layout::operator=( other );
        return *this;
      }
      BasicConfig& operator=( BasicConfig&& rhs ) & noexcept
      {
        PGBAR__TRUST( this != &rhs );
        using std::swap;
        swap( projection_, rhs.projection_ );
        Layout::operator=( std::move( rhs ) );
        return *this;
      }
      /**
       * Note: Because there are no exposed public base classes for config type,
       * there should be no scenarios for managing derived objects using base class references.
       * So the destructor here is deliberately set to be non-virtual.
       */
      ~BasicConfig() = default;

      template<typename Arg, typename... Args>
      auto with( Arg arg, Args... args ) &
#ifdef __cpp_concepts
        -> decltype( auto )
        requires( details::traits::Distinct<details::traits::TypeList<Arg, Args...>>::value
                  && is_setting<Arg>::value && ( is_setting<Args>::value && ... ) )
#else
        -> typename std::enable_if<
          details::traits::AllOf<details::traits::Distinct<details::traits::TypeList<Arg, Args...>>,
                                 is_setting<Arg>,
                                 is_setting<Args>...>::value,
          BasicConfig&>::type
#endif
      {
        std::lock_guard<details::concurrent::SharedMutex> lock { this->rw_mtx_ };
        unpack( *this, std::move( arg ) );
        (void)std::initializer_list<bool> { ( unpack( *this, std::move( args ) ), false )... };
        return *this;
      }
      template<typename Arg, typename... Args>
      auto with( Arg arg, Args... args ) &&
#ifdef __cpp_concepts
        -> decltype( auto )
        requires( details::traits::Distinct<details::traits::TypeList<Arg, Args...>>::value
                  && is_setting<Arg>::value && ( is_setting<Args>::value && ... ) )
#else
        -> typename std::enable_if<
          details::traits::AllOf<details::traits::Distinct<details::traits::TypeList<Arg, Args...>>,
                                 is_setting<Arg>,
                                 is_setting<Args>...>::value,
          BasicConfig&>::type
#endif
      {
        std::lock_guard<details::concurrent::SharedMutex> lock { this->rw_mtx_ };
        unpack( *this, std::move( arg ) );
        (void)std::initializer_list<bool> { ( unpack( *this, std::move( args ) ), false )... };
        return std::move( *this );
      }

      PGBAR__NODISCARD std::uint64_t fixed_width() const noexcept
      {
        details::concurrent::SharedLock<details::concurrent::SharedMutex> lock { this->rw_mtx_ };
        details::types::Size num_enabled = 0;
        std::uint64_t width              = 0;
        (void)std::initializer_list<bool> { ( ++num_enabled,
                                              width +=
                                              ( projection_.test( details::traits::IndexIn<Facades>::value )
                                                  ? details::traits::BaseOf_t<Layout, Facades>::fixed_length()
                                                  : 0 ),
                                              false )... };
        // Before the first element and the last element, we do not set a divider.
        return width + details::traits::BaseOf_t<Layout, details::aspects::Prefix>::fixed_length()
             + details::traits::BaseOf_t<Layout, details::aspects::Postfix>::fixed_length()
             + details::traits::BaseOf_t<Layout, details::aspects::Segment>::fixed_length( num_enabled );
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
      auto enable() & noexcept
#ifdef __cpp_concepts
        -> decltype( auto )
        requires( details::traits::TmpContain<Element, F>::value
                  && ( details::traits::TmpContain<Element, Fs>::value && ... ) )
#else
        -> typename std::enable_if<details::traits::AllOf<details::traits::TmpContain<Element, F>,
                                                          details::traits::TmpContain<Element, Fs>...>::value,
                                   BasicConfig&>::type
#endif
      {
        std::lock_guard<details::concurrent::SharedMutex> lock { this->rw_mtx_ };
        projection_.set( details::traits::IndexIn<F, Facades...>::value );
        (void)std::initializer_list<bool> {
          ( projection_.set( details::traits::IndexIn<Fs, Facades...>::value ), false )...
        };
        return *this;
      }
      // Enable the new facade based on the current state.
      template<template<typename...> class F, template<typename...> class... Fs>
      auto enable() && noexcept
#ifdef __cpp_concepts
        -> decltype( auto )
        requires( details::traits::TmpContain<Element, F>::value
                  && ( details::traits::TmpContain<Element, Fs>::value && ... ) )
#else
        -> typename std::enable_if<details::traits::AllOf<details::traits::TmpContain<Element, F>,
                                                          details::traits::TmpContain<Element, Fs>...>::value,
                                   BasicConfig&&>::type
#endif
      {
        std::lock_guard<details::concurrent::SharedMutex> lock { this->rw_mtx_ };
        projection_.set( details::traits::IndexIn<F, Facades...>::value );
        (void)std::initializer_list<bool> {
          ( projection_.set( details::traits::IndexIn<Fs, Facades...>::value ), false )...
        };
        return std::move( *this );
      }

      // Disable the new facade based on the current state.
      template<template<typename...> class F, template<typename...> class... Fs>
      auto disable() & noexcept
#ifdef __cpp_concepts
        -> decltype( auto )
        requires( details::traits::TmpContain<Element, F>::value
                  && ( details::traits::TmpContain<Element, Fs>::value && ... ) )
#else
        -> typename std::enable_if<details::traits::AllOf<details::traits::TmpContain<Element, F>,
                                                          details::traits::TmpContain<Element, Fs>...>::value,
                                   BasicConfig&>::type
#endif
      {
        std::lock_guard<details::concurrent::SharedMutex> lock { this->rw_mtx_ };
        projection_.set( details::traits::IndexIn<F, Facades...>::value );
        (void)std::initializer_list<bool> {
          ( projection_.reset( details::traits::IndexIn<Fs, Facades...>::value ), false )...
        };
        return *this;
      }
      // Disable the new facade based on the current state.
      template<template<typename...> class F, template<typename...> class... Fs>
      auto disable() && noexcept
#ifdef __cpp_concepts
        -> decltype( auto )
        requires( details::traits::TmpContain<Element, F>::value
                  && ( details::traits::TmpContain<Element, Fs>::value && ... ) )
#else
        -> typename std::enable_if<details::traits::AllOf<details::traits::TmpContain<Element, F>,
                                                          details::traits::TmpContain<Element, Fs>...>::value,
                                   BasicConfig&&>::type
#endif
      {
        std::lock_guard<details::concurrent::SharedMutex> lock { this->rw_mtx_ };
        projection_.set( details::traits::IndexIn<F, Facades...>::value );
        (void)std::initializer_list<bool> {
          ( projection_.reset( details::traits::IndexIn<Fs, Facades...>::value ), false )...
        };
        return std::move( *this );
      }

      PGBAR__CXX23_CNSTXPR void swap( BasicConfig& other ) noexcept
      {
        using std::swap;
        swap( projection_, other.projection_ );
        Layout::swap( other );
      }
      friend PGBAR__CXX23_CNSTXPR void swap( BasicConfig& a, BasicConfig& b ) noexcept { a.swap( b ); }

      template<typename Option>
      friend auto operator|=( BasicConfig& cfg, Option&& opt )
#ifdef __cpp_concepts
        requires is_setting<std::decay_t<Option>>::value
#else
        -> typename std::enable_if<is_setting<typename std::decay<Option>::type>::value>::type
#endif
      {
        cfg.with( std::forward<Option>( opt ) );
      }
      template<typename Option>
      friend auto operator|( BasicConfig& cfg, Option&& opt )
#ifdef __cpp_concepts
        -> decltype( auto )
        requires is_setting<Option>::value
#else
        -> typename std::enable_if<is_setting<Option>::value, BasicConfig&>::type
#endif
      {
        return cfg.with( std::forward<Option>( opt ) );
      }
      template<typename Option>
      friend auto operator|( BasicConfig&& cfg, Option&& opt )
#ifdef __cpp_concepts
        -> decltype( auto )
        requires is_setting<std::decay_t<Option>>::value
#else
        -> typename std::enable_if<is_setting<typename std::decay<Option>::type>::value, BasicConfig&&>::type
#endif
      {
        return std::move( cfg.with( std::forward<Option>( opt ) ) );
      }
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
} // namespace pgbar

#endif
