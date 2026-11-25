#ifndef PACE_AUTOMATION
#define PACE_AUTOMATION

#include "Indicator.hpp"
#include "details/behaviors/Completion.hpp"
#include <memory>
#include <type_traits>

namespace pace {
  namespace details {
    namespace wrappers {
      template<typename UI>
      class UIView {
        static_assert( std::is_lvalue_reference<UI&>::value, "not a read-only type" );
        static_assert( std::is_base_of<Indicator, UI>::value, "the object must be an Indicator" );

      protected:
        UI& ui_;

        constexpr UIView( UI& ui ) noexcept : ui_ { ui } {}

      public:
        UI& operator*() noexcept { return ui_; };
        const UI& operator*() const noexcept { return ui_; }

        UI* operator->() noexcept { return std::addressof( ui_ ); }
        const UI* operator->() const noexcept { return std::addressof( ui_ ); }

        operator UI&() noexcept { return ui_; }
      };
      template<typename UI>
      class UIBox {
        static_assert( !std::is_reference<UI>::value, "not an owner type" );
        static_assert( std::is_base_of<Indicator, UI>::value, "the object must be an Indicator" );

      protected:
        UI ui_;

        template<typename... Args>
        constexpr UIBox( Args&&... args ) noexcept( std::is_nothrow_constructible<UI, Args...>::value )
          : ui_ { std::forward<Args>( args )... }
        {}

        UIBox( UIBox&& )            = default;
        UIBox& operator=( UIBox&& ) = default;

      public:
        UI& operator*() & noexcept { return ui_; };
        const UI& operator*() const& noexcept { return ui_; }
        UI&& operator*() && noexcept { return std::move( ui_ ); };

        UI* operator->() noexcept { return std::addressof( ui_ ); }
        const UI* operator->() const noexcept { return std::addressof( ui_ ); }

        explicit operator UI&() & noexcept { return ui_; }
      };

      template<typename Fn, bool Unary>
      class CompletionRoutine {
        template<typename T>
        friend struct behaviors::CompletionWith;

      protected:
        Fn hook_;

        template<typename F>
        constexpr CompletionRoutine( F&& hook ) noexcept( std::is_nothrow_constructible<Fn, F>::value )
          : hook_ { std::forward<F>( hook ) }
        {}

        template<typename UI>
        void on_completion( UI& ui ) &
        { utils::invoke( hook_, ui ); }
      };
      template<typename Fn>
      class CompletionRoutine<Fn, false> {
        template<typename T>
        friend struct behaviors::CompletionWith;

      protected:
        Fn hook_;

        template<typename F>
        constexpr CompletionRoutine( F&& hook ) noexcept( std::is_nothrow_constructible<Fn, F>::value )
          : hook_ { std::forward<F>( hook ) }
        {}

        template<typename UI>
        void on_completion( UI& ) &
        { (void)hook_(); }
      };
    } // namespace wrappers
  } // namespace details

  namespace views {
    template<typename UI>
    class AutoFinish
      : public details::wrappers::UIView<UI>
      , public details::behaviors::Iterable<AutoFinish<UI>> {
    public:
      constexpr AutoFinish( UI& ui ) noexcept : details::wrappers::UIView<UI>( ui ) {}

      // We do not guarantee that the current type is definitely movable,
      // so please do not rely on this feature.
#if !PACE__CXX17
      AutoFinish( AutoFinish&& ) = default;
#endif

      // Given that C++ lacks a lifecycle mechanism,
      // we can only require reference types to maintain the affine property.
      AutoFinish( const AutoFinish& )            = delete;
      AutoFinish& operator=( const AutoFinish& ) = delete;

      PACE__FORCEINLINE void tick()
      { // Lightweight reference objects can be constructed freely,
        // so there is no requirement that they must be in lvalue form when being called.
        this->ui_.tick();
        if ( this->ui_.completive() )
          this->ui_.finish();
      }
      PACE__FORCEINLINE void tick( std::uint64_t step )
      {
        this->ui_.tick( step );
        if ( this->ui_.completive() )
          this->ui_.finish();
      }
      PACE__FORCEINLINE void tick_to( std::uint8_t percentage )
      {
        this->ui_.tick_to( percentage );
        if ( this->ui_.completive() )
          this->ui_.finish();
      }
    };

    //////////////////////////////////////////////////

    template<typename UI, typename Fn>
    class AutoFinishWith
      : public details::wrappers::UIView<UI>
      // The unary form is more specific, so we will prioritize its detection.
      , public details::wrappers::CompletionRoutine<Fn, details::traits::is_invocable<Fn&, UI&>::value>
      , public details::behaviors::CompletionWith<AutoFinishWith<UI, Fn>> {
      using Routine =
        details::wrappers::CompletionRoutine<Fn, details::traits::is_invocable<Fn&, UI&>::value>;

    public:
      template<typename F>
      constexpr AutoFinishWith( UI& ui, F&& hook ) noexcept( std::is_nothrow_constructible<Fn, F>::value )
        : details::wrappers::UIView<UI>( ui ), Routine( std::forward<F>( hook ) )
      {}

#if !PACE__CXX17
      AutoFinishWith( AutoFinishWith&& ) = default;
#endif

      AutoFinishWith( const AutoFinishWith& )            = delete;
      AutoFinishWith& operator=( const AutoFinishWith& ) = delete;
    };

    //////////////////////////////////////////////////

    template<typename UI>
    class AutoRetire
      : public details::wrappers::UIView<UI>
      , public details::behaviors::Iterable<AutoRetire<UI>> {
    public:
      constexpr AutoRetire( UI& ui ) noexcept : details::wrappers::UIView<UI>( ui ) {}

#if !PACE__CXX17
      AutoRetire( AutoRetire&& ) = default;
#endif

      AutoRetire( const AutoRetire& )            = delete;
      AutoRetire& operator=( const AutoRetire& ) = delete;

      PACE__FORCEINLINE void tick()
      {
        this->ui_.tick();
        if ( this->ui_.completive() )
          this->ui_.retire();
      }
      PACE__FORCEINLINE void tick( std::uint64_t step )
      {
        this->ui_.tick( step );
        if ( this->ui_.completive() )
          this->ui_.retire();
      }
      PACE__FORCEINLINE void tick_to( std::uint8_t percentage )
      {
        this->ui_.tick_to( percentage );
        if ( this->ui_.completive() )
          this->ui_.retire();
      }
    };

    //////////////////////////////////////////////////

    template<typename UI>
    AutoFinish<UI> auto_finish( UI& ui ) noexcept
    { return { ui }; }
    template<typename UI, typename F>
    // accept lvalue callback
    AutoFinishWith<UI, F> auto_finish( UI& ui, F&& callback )
    { return { ui, std::forward<F>( callback ) }; }

    //////////////////////////////////////////////////

    template<typename UI>
    AutoRetire<UI> auto_retire( UI& ui ) noexcept
    { return { ui }; }
  } // namespace views

  template<typename UI>
  class AutoFinish
    : public details::wrappers::UIBox<UI>
    , public details::behaviors::Completion<AutoFinish<UI>> {
  public:
    /// @param args It can be many option arguments, a config object or even a bar object.
    template<typename... Args>
    constexpr AutoFinish( Args&&... args ) noexcept( std::is_nothrow_constructible<UI, Args...>::value )
#ifdef __cpp_concepts
      requires std::is_constructible_v<UI, Args...>
#endif
      : details::wrappers::UIBox<UI>( std::forward<Args>( args )... )
    {
#ifndef __cpp_concepts
      static_assert( std::is_constructible<UI, Args...>::value, "invalid arguments" );
#endif
    }

    AutoFinish( AutoFinish&& )              = default;
    AutoFinish& operator=( AutoFinish&& ) & = default;

    operator views::AutoFinish<UI>() & noexcept { return { **this }; }
  };
  // Prepared for the parameters returned by DynamicBar::insert.
  template<typename UI>
  class AutoFinish<std::unique_ptr<UI>>
    : public details::behaviors::Completion<AutoFinish<std::unique_ptr<UI>>> {
    std::unique_ptr<UI> ui_;

  public:
    PACE__CXX23_CNSTXPR AutoFinish( std::unique_ptr<UI>&& ui ) noexcept : ui_ { std::move( ui ) }
    { PACE__ASSERT( ui_ != nullptr ); }

    AutoFinish( AutoFinish&& )              = default;
    AutoFinish& operator=( AutoFinish&& ) & = default;

    UI& operator*() & noexcept { return *ui_; };
    const UI& operator*() const& noexcept { return *ui_; }
    UI&& operator*() && noexcept { return std::move( *ui_ ); };

    UI* operator->() noexcept { return ui_.get(); }
    const UI* operator->() const noexcept { return ui_.get(); }

    operator views::AutoFinish<UI>() & noexcept { return { **this }; }
  };

#ifdef __cpp_deduction_guides
  template<typename UI>
  AutoFinish( std::unique_ptr<UI>&& ) -> AutoFinish<std::unique_ptr<UI>>;
#endif

  //////////////////////////////////////////////////

  template<typename UI, typename Fn>
  class AutoFinishWith
    : public details::wrappers::UIBox<UI>
    , public details::wrappers::CompletionRoutine<Fn, details::traits::is_invocable<Fn&, UI&>::value>
    , public details::behaviors::CompletionWith<AutoFinishWith<UI, Fn>> {
    using Box     = details::wrappers::UIBox<UI>;
    using Routine = details::wrappers::CompletionRoutine<Fn, details::traits::is_invocable<Fn&, UI&>::value>;

  public:
    /// @param obj The bar object or the config object.
    template<typename Obj, typename F
#ifdef __cpp_concepts
             >
      requires( std::is_constructible<UI, Obj>::value
                && ( details::traits::is_invocable<Fn&>::value
                     || details::traits::is_invocable<Fn&, UI&>::value ) )
#else
             ,
             typename = typename std::enable_if<
               details::traits::AnyOf<details::traits::is_invocable<Fn&>,
                                      details::traits::is_invocable<Fn&, UI&>>::value>::type>
#endif
    constexpr AutoFinishWith( Obj&& obj, F&& hook )
      noexcept( std::is_nothrow_constructible<UI, Obj>::value && std::is_nothrow_constructible<Fn, F>::value )
      : Box( std::forward<Obj>( obj ) ), Routine( std::forward<F>( hook ) )
    {
#ifndef __cpp_concepts
      static_assert( std::is_constructible<UI, Obj>::value, "invalid arguments" );
#endif
    }

    /// @param options The option arguments.
    template<typename F, typename... Options
#ifdef __cpp_concepts
             >
      requires( std::is_constructible<UI, Options...>::value
                && ( details::traits::is_invocable<Fn&>::value
                     || details::traits::is_invocable<Fn&, UI&>::value ) )
#else
             ,
             typename = typename std::enable_if<
               details::traits::AnyOf<details::traits::is_invocable<Fn&>,
                                      details::traits::is_invocable<Fn&, UI&>>::value>::type>
#endif
    constexpr AutoFinishWith( F&& hook, Options&&... options )
      noexcept( std::is_nothrow_constructible<UI, Options...>::value
                && std::is_nothrow_constructible<Fn, F>::value )
      : Box( std::forward<Options>( options )... ), Routine( std::forward<F>( hook ) )
    {
#ifndef __cpp_concepts
      static_assert( std::is_constructible<UI, Options...>::value, "invalid arguments" );
#endif
    }

    AutoFinishWith( AutoFinishWith&& )              = default;
    AutoFinishWith& operator=( AutoFinishWith&& ) & = default;

    operator views::AutoFinishWith<UI, Fn&>() & noexcept { return { **this, this->hook_ }; }
  };
  template<typename UI, typename Fn>
  class AutoFinishWith<std::unique_ptr<UI>, Fn>
    : public details::wrappers::CompletionRoutine<Fn, details::traits::is_invocable<Fn&, UI&>::value>
    , public details::behaviors::CompletionWith<AutoFinishWith<std::unique_ptr<UI>, Fn>> {
    using Routine = details::wrappers::CompletionRoutine<Fn, details::traits::is_invocable<Fn&, UI&>::value>;
    std::unique_ptr<UI> ui_;

  public:
    template<typename F>
    PACE__CXX23_CNSTXPR AutoFinishWith( std::unique_ptr<UI>&& ui, F&& hook )
      noexcept( std::is_nothrow_constructible<Fn, F>::value )
      : Routine( std::forward<F>( hook ) ), ui_ { std::move( ui ) }
    { PACE__ASSERT( ui_ != nullptr ); }

    UI& operator*() & noexcept { return *ui_; };
    const UI& operator*() const& noexcept { return *ui_; }
    UI&& operator*() && noexcept { return std::move( *ui_ ); };

    UI* operator->() noexcept { return ui_.get(); }
    const UI* operator->() const noexcept { return ui_.get(); }

    AutoFinishWith( AutoFinishWith&& )              = default;
    AutoFinishWith& operator=( AutoFinishWith&& ) & = default;

    operator views::AutoFinishWith<UI, Fn&>() & noexcept { return { *ui_, this->hook_ }; }
  };

#ifdef __cpp_deduction_guides
  template<typename UI, typename F>
  AutoFinishWith( std::unique_ptr<UI>&&, F&& ) -> AutoFinishWith<std::unique_ptr<UI>, std::decay_t<F>>;
#endif

  //////////////////////////////////////////////////

  template<typename UI>
  class AutoRetire
    : public details::wrappers::UIBox<UI>
    , public details::behaviors::Retirement<AutoRetire<UI>> {
  public:
    /// @param args It can be many option arguments, a config object or even a bar object.
    template<typename... Args>
    constexpr AutoRetire( Args&&... args ) noexcept( std::is_nothrow_constructible<UI, Args...>::value )
#ifdef __cpp_concepts
      requires std::is_constructible_v<UI, Args...>
#endif
      : details::wrappers::UIBox<UI>( std::forward<Args>( args )... )
    {
#ifndef __cpp_concepts
      static_assert( std::is_constructible<UI, Args...>::value, "invalid arguments" );
#endif
    }

    AutoRetire( AutoRetire&& )              = default;
    AutoRetire& operator=( AutoRetire&& ) & = default;

    operator views::AutoRetire<UI>() & noexcept { return { **this }; }
  };
  template<typename UI>
  class AutoRetire<std::unique_ptr<UI>>
    : public details::behaviors::Retirement<AutoRetire<std::unique_ptr<UI>>> {
    std::unique_ptr<UI> ui_;

  public:
    PACE__CXX23_CNSTXPR AutoRetire( std::unique_ptr<UI>&& ui ) noexcept : ui_ { std::move( ui ) }
    { PACE__ASSERT( ui_ != nullptr ); }

    AutoRetire( AutoRetire&& )              = default;
    AutoRetire& operator=( AutoRetire&& ) & = default;

    UI& operator*() & noexcept { return *ui_; };
    const UI& operator*() const& noexcept { return *ui_; }
    UI&& operator*() && noexcept { return std::move( *ui_ ); };

    UI* operator->() noexcept { return ui_.get(); }
    const UI* operator->() const noexcept { return ui_.get(); }

    operator views::AutoRetire<UI>() & noexcept { return { **this }; }
  };

#ifdef __cpp_deduction_guides
  template<typename UI>
  AutoRetire( std::unique_ptr<UI>&& ) -> AutoRetire<std::unique_ptr<UI>>;
#endif

  //////////////////////////////////////////////////

  template<typename Bar, template<typename> class Automation, typename... Args>
  PACE__NODISCARD constexpr Automation<Bar> auto_finish( Args&&... args )
  {
    static_assert( std::is_move_constructible<Automation<Bar>>::value
                     && std::is_move_assignable<Automation<Bar>>::value,
                   "cannot create an Automation of the view property" );
    return { std::forward<Args>( args )... };
  }

  /// @param args It can be many option arguments, a config object or even a bar object.
  template<typename Bar, typename... Args>
  PACE__NODISCARD PACE__FORCEINLINE constexpr auto auto_finish( Args&&... args ) ->
#ifdef __cpp_concepts
    AutoFinish<Bar>
    requires( std::is_base_of_v<Indicator, Bar> && std::is_constructible_v<Bar, Args...> )
#else
    typename std::enable_if<
      details::traits::AllOf<std::is_base_of<Indicator, Bar>, std::is_constructible<Bar, Args...>>::value,
      AutoFinish<Bar>>::type
#endif
  { return { std::forward<Args>( args )... }; }
  template<typename Bar>
  PACE__NODISCARD PACE__FORCEINLINE constexpr auto auto_finish( std::unique_ptr<Bar>&& ui )
    -> AutoFinish<std::unique_ptr<Bar>>
  { return { std::move( ui ) }; }

  /// @param obj The bar object or the config object.
  template<typename Bar, typename Obj, typename F>
  PACE__NODISCARD constexpr auto auto_finish( Obj&& obj, F&& callback ) ->
#ifdef __cpp_concepts
    AutoFinishWith<Bar, std::decay_t<F>>
    requires( std::is_base_of_v<Indicator, Bar>
              && ( details::traits::is_invocable<std::decay_t<F>&>::value
                   || details::traits::is_invocable<std::decay_t<F>&, Bar&>::value ) )
#else
    typename std::enable_if<
      details::traits::AllOf<
        std::is_base_of<Indicator, Bar>,
        details::traits::AnyOf<details::traits::is_invocable<typename std::decay<F>::type&>,
                               details::traits::is_invocable<typename std::decay<F>::type&, Bar&>>>::value,
      AutoFinishWith<Bar, typename std::decay<F>::type>>::type
#endif
  { return { std::forward<Obj>( obj ), std::forward<F>( callback ) }; }
  /// @param options The option arguments.
  template<typename Bar, typename F, typename... Options>
  PACE__NODISCARD constexpr auto auto_finish( F&& callback, Options&&... options ) ->
#ifdef __cpp_concepts
    AutoFinishWith<Bar, std::decay_t<F>>
    requires( std::is_base_of_v<Indicator, Bar>
              && ( details::traits::is_invocable<std::decay_t<F>&>::value
                   || details::traits::is_invocable<std::decay_t<F>&, Bar&>::value ) )
#else
    typename std::enable_if<
      details::traits::AllOf<
        std::is_base_of<Indicator, Bar>,
        details::traits::AnyOf<details::traits::is_invocable<typename std::decay<F>::type&>,
                               details::traits::is_invocable<typename std::decay<F>::type&, Bar&>>>::value,
      AutoFinishWith<Bar, typename std::decay<F>::type>>::type
#endif
  { return { std::forward<F>( callback ), std::forward<Options>( options )... }; }
  template<typename Bar, typename F>
  PACE__NODISCARD constexpr auto auto_finish( std::unique_ptr<Bar>&& ui, F&& callback )
    -> AutoFinishWith<std::unique_ptr<Bar>, typename std::decay<F>::type>
  { return { std::move( ui ), std::forward<F>( callback ) }; }

  /// @param args It can be many option arguments, a config object or even a bar object.
  template<typename Bar, typename... Args>
  PACE__NODISCARD PACE__FORCEINLINE constexpr auto auto_retire( Args&&... args ) ->
#ifdef __cpp_concepts
    AutoRetire<Bar>
    requires std::is_constructible_v<Bar, Args...>
#else
    typename std::enable_if<std::is_constructible<Bar, Args...>::value, AutoRetire<Bar>>::type
#endif
  { return auto_finish<Bar, AutoRetire>( std::forward<Args>( args )... ); }
  template<typename Bar>
  PACE__NODISCARD constexpr auto auto_retire( std::unique_ptr<Bar>&& ui ) -> AutoRetire<std::unique_ptr<Bar>>
  { return { std::move( ui ) }; }

  //////////////////////////////////////////////////

  template<typename Bar, template<typename> class Automation = AutoFinish, typename N, typename... Options>
  PACE__NODISCARD PACE__FORCEINLINE auto iterate( N startpoint, N endpoint, N step, Options&&... options ) ->
#ifdef __cpp_concepts
    views::TrackedSpan<views::NumericSpan<N>, Automation<Bar>>
    requires( std::is_arithmetic_v<N> && std::is_constructible_v<Bar, Options...> )
#else
    typename std::enable_if<
      details::traits::AllOf<std::is_arithmetic<N>, std::is_constructible<Bar, Options...>>::value,
      views::TrackedSpan<views::NumericSpan<N>, Automation<Bar>>>::type
#endif
  {
    return {
      { startpoint, endpoint, step },
      auto_finish<Bar, Automation>( std::forward<Options>( options )... )
    };
  }
  template<typename Bar, typename N, typename Act, typename... Options>
  PACE__NODISCARD PACE__FORCEINLINE auto iterate( N startpoint,
                                                  N endpoint,
                                                  N step,
                                                  Act&& act,
                                                  Options&&... options ) ->
#ifdef __cpp_concepts
    views::TrackedSpan<views::NumericSpan<N>, AutoFinishWith<Bar, std::decay_t<Act>>>
    requires( std::is_arithmetic_v<N> && std::is_constructible_v<Bar, Options...>
              && ( details::traits::is_invocable<std::decay_t<Act>&>::value
                   || details::traits::is_invocable<std::decay_t<Act>&, Bar&>::value ) )
#else
    typename std::enable_if<
      details::traits::AllOf<
        std::is_arithmetic<N>,
        std::is_constructible<Bar, Options...>,
        details::traits::AnyOf<details::traits::is_invocable<typename std::decay<Act>::type&>,
                               details::traits::is_invocable<typename std::decay<Act>::type&, Bar&>>>::value,
      views::TrackedSpan<views::NumericSpan<N>, AutoFinishWith<Bar, typename std::decay<Act>::type>>>::type
#endif
  {
    return {
      { startpoint, endpoint, step },
      auto_finish<Bar>( std::forward<Act>( act ), std::forward<Options>( options )... )
    };
  }
  template<typename Bar,
           template<typename> class Automation = AutoFinish,
           typename N,
           typename Proc,
           typename... Options>
  PACE__FORCEINLINE auto iterate( N startpoint, N endpoint, N step, Proc&& op, Options&&... options )
#ifdef __cpp_concepts
    requires( std::is_arithmetic_v<N> && std::is_constructible_v<Bar, Options...>
              && details::traits::is_invocable<Proc&, N>::value )
#else
    -> typename std::enable_if<details::traits::AllOf<std::is_arithmetic<N>,
                                                      std::is_constructible<Bar, Options...>,
                                                      details::traits::is_invocable<Proc&, N>>::value>::type
#endif
  {
    auto_finish<Bar, Automation>( std::forward<Options>( options )... )
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
    requires( std::is_arithmetic_v<N> && std::is_constructible_v<Bar, Options...>
              && details::traits::is_invocable<Proc&, N>::value
              && ( details::traits::is_invocable<std::decay_t<Act>&>::value
                   || details::traits::is_invocable<std::decay_t<Act>&, Bar&>::value ) )
#else
    -> typename std::enable_if<details::traits::AllOf<std::is_arithmetic<N>,
                                                      std::is_constructible<Bar, Options...>,
                                                      details::traits::is_invocable<Proc&, N>>::value>::type
#endif
  {
#ifndef __cpp_concepts
    static_assert( details::traits::is_invocable<typename std::decay<Act>::type&>::value
                     || details::traits::is_invocable<typename std::decay<Act>::type&, Bar&>::value,
                   "unknown callback type" );
#endif
    auto_finish<Bar>( std::forward<Act>( act ), std::forward<Options>( options )... )
      .iterate( startpoint, endpoint, step, std::forward<Proc>( op ) );
  }

  //////////////////////////////////////////////////

  template<typename Bar, template<typename> class Automation = AutoFinish, typename N, typename... Options>
  PACE__NODISCARD PACE__FORCEINLINE auto iterate( N endpoint, N step, Options&&... options ) ->
#ifdef __cpp_concepts
    views::TrackedSpan<views::NumericSpan<N>, Automation<Bar>>
    requires( std::is_floating_point_v<N> && std::is_constructible_v<Bar, Options...> )
#else
    typename std::enable_if<
      details::traits::AllOf<std::is_floating_point<N>, std::is_constructible<Bar, Options...>>::value,
      views::TrackedSpan<views::NumericSpan<N>, Automation<Bar>>>::type
#endif
  {
    return {
      { {}, endpoint, step },
      auto_finish<Bar, Automation>( std::forward<Options>( options )... )
    };
  }
  template<typename Bar, typename N, typename Act, typename... Options>
  PACE__NODISCARD PACE__FORCEINLINE auto iterate( N endpoint, N step, Act&& act, Options&&... options ) ->
#ifdef __cpp_concepts
    views::TrackedSpan<views::NumericSpan<N>, AutoFinishWith<Bar, std::decay_t<Act>>>
    requires( std::is_floating_point_v<N> && std::is_constructible_v<Bar, Options...>
              && ( details::traits::is_invocable<std::decay_t<Act>&>::value
                   || details::traits::is_invocable<std::decay_t<Act>&, Bar&>::value ) )
#else
    typename std::enable_if<
      details::traits::AllOf<
        std::is_floating_point<N>,
        std::is_constructible<Bar, Options...>,
        details::traits::AnyOf<details::traits::is_invocable<typename std::decay<Act>::type&>,
                               details::traits::is_invocable<typename std::decay<Act>::type&, Bar&>>>::value,
      views::TrackedSpan<views::NumericSpan<N>, AutoFinishWith<Bar, typename std::decay<Act>::type>>>::type
#endif
  {
    return {
      { {}, endpoint, step },
      auto_finish<Bar>( std::forward<Act>( act ), std::forward<Options>( options )... )
    };
  }
  template<typename Bar,
           template<typename> class Automation = AutoFinish,
           typename N,
           typename Proc,
           typename... Options>
  PACE__FORCEINLINE auto iterate( N endpoint, N step, Proc&& op, Options&&... options )
#ifdef __cpp_concepts
    requires( std::is_floating_point_v<N> && std::is_constructible_v<Bar, Options...>
              && details::traits::is_invocable<Proc&, N>::value )
#else
    -> typename std::enable_if<details::traits::AllOf<std::is_floating_point<N>,
                                                      std::is_constructible<Bar, Options...>,
                                                      details::traits::is_invocable<Proc&, N>>::value>::type
#endif
  {
    auto_finish<Bar, Automation>( std::forward<Options>( options )... )
      .iterate( endpoint, step, std::forward<Proc>( op ) );
  }
  template<typename Bar, typename N, typename Proc, typename Act, typename... Options>
  PACE__FORCEINLINE auto iterate( N endpoint, N step, Proc&& op, Act&& act, Options&&... options )
#ifdef __cpp_concepts
    requires( std::is_floating_point_v<N> && std::is_constructible_v<Bar, Options...>
              && details::traits::is_invocable<Proc&, N>::value
              && ( details::traits::is_invocable<std::decay_t<Act>&>::value
                   || details::traits::is_invocable<std::decay_t<Act>&, Bar&>::value ) )
#else
    -> typename std::enable_if<details::traits::AllOf<std::is_floating_point<N>,
                                                      std::is_constructible<Bar, Options...>,
                                                      details::traits::is_invocable<Proc&, N>>::value>::type
#endif
  {
#ifndef __cpp_concepts
    static_assert( details::traits::is_invocable<typename std::decay<Act>::type&>::value
                     || details::traits::is_invocable<typename std::decay<Act>::type&, Bar&>::value,
                   "unknown callback type" );
#endif
    auto_finish<Bar>( std::forward<Act>( act ), std::forward<Options>( options )... )
      .iterate( endpoint, step, std::forward<Proc>( op ) );
  }

  //////////////////////////////////////////////////

  template<typename Bar, template<typename> class Automation = AutoFinish, typename N, typename... Options>
  PACE__NODISCARD PACE__FORCEINLINE auto iterate( N startpoint, N endpoint, Options&&... options ) ->
#ifdef __cpp_concepts
    views::TrackedSpan<views::NumericSpan<N>, Automation<Bar>>
    requires( std::is_integral_v<N> && std::is_constructible_v<Bar, Options...> )
#else
    typename std::enable_if<
      details::traits::AllOf<std::is_integral<N>, std::is_constructible<Bar, Options...>>::value,
      views::TrackedSpan<views::NumericSpan<N>, Automation<Bar>>>::type
#endif
  {
    return {
      { startpoint, endpoint },
      auto_finish<Bar, Automation>( std::forward<Options>( options )... )
    };
  }
  template<typename Bar, typename N, typename Act, typename... Options>
  PACE__NODISCARD PACE__FORCEINLINE auto iterate( N startpoint, N endpoint, Act&& act, Options&&... options )
    ->
#ifdef __cpp_concepts
    views::TrackedSpan<views::NumericSpan<N>, AutoFinishWith<Bar, std::decay_t<Act>>>
    requires( std::is_integral_v<N> && std::is_constructible_v<Bar, Options...>
              && ( details::traits::is_invocable<std::decay_t<Act>&>::value
                   || details::traits::is_invocable<std::decay_t<Act>&, Bar&>::value ) )
#else
    typename std::enable_if<
      details::traits::AllOf<
        std::is_integral<N>,
        std::is_constructible<Bar, Options...>,
        details::traits::AnyOf<details::traits::is_invocable<typename std::decay<Act>::type&>,
                               details::traits::is_invocable<typename std::decay<Act>::type&, Bar&>>>::value,
      views::TrackedSpan<views::NumericSpan<N>, AutoFinishWith<Bar, typename std::decay<Act>::type>>>::type
#endif
  {
    return {
      { startpoint, endpoint },
      auto_finish<Bar>( std::forward<Act>( act ), std::forward<Options>( options )... )
    };
  }
  template<typename Bar,
           template<typename> class Automation = AutoFinish,
           typename N,
           typename Proc,
           typename... Options>
  PACE__FORCEINLINE auto iterate( N startpoint, N endpoint, Proc&& op, Options&&... options )
#ifdef __cpp_concepts
    requires( std::is_integral_v<N> && std::is_constructible_v<Bar, Options...>
              && details::traits::is_invocable<Proc&, N>::value )
#else
    -> typename std::enable_if<details::traits::AllOf<std::is_integral<N>,
                                                      std::is_constructible<Bar, Options...>,
                                                      details::traits::is_invocable<Proc&, N>>::value>::type
#endif
  {
    auto_finish<Bar, Automation>( std::forward<Options>( options )... )
      .iterate( startpoint, endpoint, std::forward<Proc>( op ) );
  }
  template<typename Bar, typename N, typename Proc, typename Act, typename... Options>
  PACE__FORCEINLINE auto iterate( N startpoint, N endpoint, Proc&& op, Act&& act, Options&&... options )
#ifdef __cpp_concepts
    requires( std::is_integral_v<N> && std::is_constructible_v<Bar, Options...>
              && ( details::traits::is_invocable<std::decay_t<Act>&>::value
                   || details::traits::is_invocable<std::decay_t<Act>&, Bar&>::value ) )
#else
    -> typename std::enable_if<
      details::traits::AllOf<std::is_integral<N>, std::is_constructible<Bar, Options...>>::value>::type
#endif
  {
#ifndef __cpp_concepts
    static_assert( details::traits::is_invocable<typename std::decay<Act>::type&>::value
                     || details::traits::is_invocable<typename std::decay<Act>::type&, Bar&>::value,
                   "unknown callback type" );
#endif
    auto_finish<Bar>( std::forward<Act>( act ), std::forward<Options>( options )... )
      .iterate( startpoint, endpoint, std::forward<Proc>( op ) );
  }

  //////////////////////////////////////////////////

  template<typename Bar, template<typename> class Automation = AutoFinish, typename N, typename... Options>
  PACE__NODISCARD PACE__FORCEINLINE auto iterate( N endpoint, Options&&... options ) ->
#ifdef __cpp_concepts
    views::TrackedSpan<views::NumericSpan<N>, Automation<Bar>>
    requires( std::is_integral_v<N> && std::is_constructible_v<Bar, Options...> )
#else
    typename std::enable_if<
      details::traits::AllOf<std::is_integral<N>, std::is_constructible<Bar, Options...>>::value,
      views::TrackedSpan<views::NumericSpan<N>, Automation<Bar>>>::type
#endif
  { return { { endpoint }, auto_finish<Bar, Automation>( std::forward<Options>( options )... ) }; }
  template<typename Bar, typename N, typename Act, typename... Options>
  PACE__NODISCARD PACE__FORCEINLINE auto iterate( N endpoint, Act&& act, Options&&... options ) ->
#ifdef __cpp_concepts
    views::TrackedSpan<views::NumericSpan<N>, AutoFinishWith<Bar, std::decay_t<Act>>>
    requires( std::is_integral_v<N> && std::is_constructible_v<Bar, Options...>
              && ( details::traits::is_invocable<std::decay_t<Act>&>::value
                   || details::traits::is_invocable<std::decay_t<Act>&, Bar&>::value ) )
#else
    typename std::enable_if<
      details::traits::AllOf<
        std::is_integral<N>,
        std::is_constructible<Bar, Options...>,
        details::traits::AnyOf<details::traits::is_invocable<typename std::decay<Act>::type&>,
                               details::traits::is_invocable<typename std::decay<Act>::type&, Bar&>>>::value,
      views::TrackedSpan<views::NumericSpan<N>, AutoFinishWith<Bar, typename std::decay<Act>::type>>>::type
#endif
  {
    return { { endpoint },
             auto_finish<Bar>( std::forward<Act>( act ), std::forward<Options>( options )... ) };
  }
  template<typename Bar,
           template<typename> class Automation = AutoFinish,
           typename N,
           typename Proc,
           typename... Options>
  PACE__FORCEINLINE auto iterate( N endpoint, Proc&& op, Options&&... options )
#ifdef __cpp_concepts
    requires( std::is_integral_v<N> && std::is_constructible_v<Bar, Options...>
              && details::traits::is_invocable<Proc&, N>::value )
#else
    -> typename std::enable_if<details::traits::AllOf<std::is_integral<N>,
                                                      std::is_constructible<Bar, Options...>,
                                                      details::traits::is_invocable<Proc&, N>>::value>::type
#endif
  {
    auto_finish<Bar, Automation>( std::forward<Options>( options )... )
      .iterate( endpoint, std::forward<Proc>( op ) );
  }
  template<typename Bar, typename N, typename Proc, typename Act, typename... Options>
  PACE__FORCEINLINE auto iterate( N endpoint, Proc&& op, Act&& act, Options&&... options )
#ifdef __cpp_concepts
    requires( std::is_integral_v<N> && std::is_constructible_v<Bar, Options...>
              && details::traits::is_invocable<Proc&, N>::value
              && ( details::traits::is_invocable<std::decay_t<Act>&>::value
                   || details::traits::is_invocable<std::decay_t<Act>&, Bar&>::value ) )
#else
    -> typename std::enable_if<details::traits::AllOf<std::is_integral<N>,
                                                      std::is_constructible<Bar, Options...>,
                                                      details::traits::is_invocable<Proc&, N>>::value>::type
#endif
  {
#ifndef __cpp_concepts
    static_assert( details::traits::is_invocable<typename std::decay<Act>::type&>::value
                     || details::traits::is_invocable<typename std::decay<Act>::type&, Bar&>::value,
                   "unknown callback type" );
#endif
    auto_finish<Bar>( std::forward<Act>( act ), std::forward<Options>( options )... )
      .iterate( endpoint, std::forward<Proc>( op ) );
  }

  //////////////////////////////////////////////////

  template<typename Bar,
           template<typename> class Automation = AutoFinish,
           typename Itr,
           typename Snt,
           typename... Options>
  PACE__NODISCARD PACE__FORCEINLINE auto iterate( Itr startpoint, Snt endpoint, Options&&... options ) ->
#ifdef __cpp_concepts
    views::TrackedSpan<views::IteratorSpan<Itr, Snt>, Automation<Bar>>
    requires( details::traits::is_sized_cursor<Itr, Snt>::value && std::is_constructible_v<Bar, Options...> )
#else
    typename std::enable_if<details::traits::AllOf<details::traits::is_sized_cursor<Itr, Snt>,
                                                   std::is_constructible<Bar, Options...>>::value,
                            views::TrackedSpan<views::IteratorSpan<Itr, Snt>, Automation<Bar>>>::type
#endif
  {
    return {
      { std::move( startpoint ), std::move( endpoint ) },
      auto_finish<Bar, Automation>( std::forward<Options>( options )... )
    };
  }
  template<typename Bar, typename Itr, typename Snt, typename Act, typename... Options>
  PACE__NODISCARD PACE__FORCEINLINE auto iterate( Itr startpoint,
                                                  Snt endpoint,
                                                  Act&& act,
                                                  Options&&... options ) ->
#ifdef __cpp_concepts
    views::TrackedSpan<views::IteratorSpan<Itr, Snt>, AutoFinishWith<Bar, std::decay_t<Act>>>
    requires( details::traits::is_sized_cursor<Itr, Snt>::value && std::is_constructible_v<Bar, Options...>
              && ( details::traits::is_invocable<std::decay_t<Act>&>::value
                   || details::traits::is_invocable<std::decay_t<Act>&, Bar&>::value ) )
#else
    typename std::enable_if<
      details::traits::AllOf<
        details::traits::is_sized_cursor<Itr, Snt>,
        std::is_constructible<Bar, Options...>,
        details::traits::AnyOf<details::traits::is_invocable<typename std::decay<Act>::type&>,
                               details::traits::is_invocable<typename std::decay<Act>::type&, Bar&>>>::value,
      views::TrackedSpan<views::IteratorSpan<Itr, Snt>,
                         AutoFinishWith<Bar, typename std::decay<Act>::type>>>::type
#endif
  {
    return {
      { std::move( startpoint ), std::move( endpoint ) },
      auto_finish<Bar>( std::forward<Act>( act ), std::forward<Options>( options )... )
    };
  }
  template<typename Bar,
           template<typename> class Automation = AutoFinish,
           typename Itr,
           typename Snt,
           typename Proc,
           typename... Options>
  PACE__FORCEINLINE auto iterate( Itr startpoint, Snt endpoint, Proc&& op, Options&&... options )
#ifdef __cpp_concepts
    requires( details::traits::is_sized_cursor<Itr, Snt>::value && std::is_constructible_v<Bar, Options...>
              && details::traits::is_invocable<Proc&, details::traits::IterReference_t<Itr>>::value )
#else
    -> typename std::enable_if<details::traits::AllOf<
      details::traits::is_sized_cursor<Itr, Snt>,
      std::is_constructible<Bar, Options...>,
      details::traits::is_invocable<Proc&, details::traits::IterReference_t<Itr>>>::value>::type
#endif
  {
    auto_finish<Bar, Automation>( std::forward<Options>( options )... )
      .iterate( std::move( startpoint ), std::move( endpoint ), std::forward<Proc>( op ) );
  }
  template<typename Bar, typename Itr, typename Snt, typename Proc, typename Act, typename... Options>
  PACE__FORCEINLINE auto iterate( Itr startpoint, Snt endpoint, Proc&& op, Act&& act, Options&&... options )
#ifdef __cpp_concepts
    requires( details::traits::is_sized_cursor<Itr, Snt>::value && std::is_constructible_v<Bar, Options...>
              && details::traits::is_invocable<Proc&, details::traits::IterReference_t<Itr>>::value
              && ( details::traits::is_invocable<std::decay_t<Act>&>::value
                   || details::traits::is_invocable<std::decay_t<Act>&, Bar&>::value ) )
#else
    -> typename std::enable_if<details::traits::AllOf<
      details::traits::is_sized_cursor<Itr, Snt>,
      std::is_constructible<Bar, Options...>,
      details::traits::is_invocable<Proc&, details::traits::IterReference_t<Itr>>>::value>::type
#endif
  {
#ifndef __cpp_concepts
    static_assert( details::traits::is_invocable<typename std::decay<Act>::type&>::value
                     || details::traits::is_invocable<typename std::decay<Act>::type&, Bar&>::value,
                   "unknown callback type" );
#endif
    auto_finish<Bar>( std::forward<Act>( act ), std::forward<Options>( options )... )
      .iterate( std::move( startpoint ), std::move( endpoint ), std::forward<Proc>( op ) );
  }

  //////////////////////////////////////////////////

  template<typename Bar, template<typename> class Automation = AutoFinish, class R, typename... Options>
  PACE__NODISCARD PACE__FORCEINLINE auto iterate( R&& range, Options&&... options ) ->
#ifdef __cpp_concepts
    views::TrackedSpan<views::SizedSpan<std::remove_reference_t<R>>, Automation<Bar>>
    requires( details::traits::is_sized_range<R>::value
              && !std::ranges::view<std::remove_reference_t<R>> && std::is_constructible_v<Bar, Options...> )
#else
    typename std::enable_if<
      details::traits::AllOf<details::traits::is_sized_range<R>,
                             std::is_constructible<Bar, Options...>>::value,
      views::TrackedSpan<views::SizedSpan<typename std::remove_reference<R>::type>, Automation<Bar>>>::type
#endif
  {
    return { { std::forward<R>( range ) },
             auto_finish<Bar, Automation>( std::forward<Options>( options )... ) };
  }
  template<typename Bar, class R, typename Act, typename... Options>
  PACE__NODISCARD PACE__FORCEINLINE auto iterate( R&& range, Act&& act, Options&&... options ) ->
#ifdef __cpp_concepts
    views::TrackedSpan<views::SizedSpan<std::remove_reference_t<R>>, AutoFinishWith<Bar, std::decay_t<Act>>>
    requires( details::traits::is_sized_range<R>::value
              && !std::ranges::view<std::remove_reference_t<R>> && std::is_constructible_v<Bar, Options...>
              && ( details::traits::is_invocable<std::decay_t<Act>&>::value
                   || details::traits::is_invocable<std::decay_t<Act>&, Bar&>::value ) )
#else
    typename std::enable_if<
      details::traits::AllOf<
        details::traits::is_sized_range<R>,
        std::is_constructible<Bar, Options...>,
        details::traits::AnyOf<details::traits::is_invocable<typename std::decay<Act>::type&>,
                               details::traits::is_invocable<typename std::decay<Act>::type&, Bar&>>>::value,
      views::TrackedSpan<views::SizedSpan<typename std::remove_reference<R>::type>,
                         AutoFinishWith<Bar, typename std::decay<Act>::type>>>::type
#endif
  {
    return { { std::forward<R>( range ) },
             auto_finish<Bar>( std::forward<Act>( act ), std::forward<Options>( options )... ) };
  }
#ifdef __cpp_concepts
  template<typename Bar, template<typename> class Automation = AutoFinish, class R, typename... Options>
    requires( details::traits::is_sized_range<R>::value
              && std::ranges::view<R> && std::is_constructible_v<Bar, Options...> )
  PACE__NODISCARD PACE__FORCEINLINE auto iterate( R view, Options&&... options )
    -> views::TrackedSpan<R, Automation<Bar>>
  { return { std::move( view ), auto_finish<Bar, Automation>( std::forward<Options>( options )... ) }; }
  template<typename Bar, class R, typename Act, typename... Options>
    requires( details::traits::is_sized_range<R>::value
              && std::ranges::view<R> && std::is_constructible_v<Bar, Options...>
              && ( details::traits::is_invocable<std::decay_t<Act>&>::value
                   || details::traits::is_invocable<std::decay_t<Act>&, Bar&>::value ) )
  PACE__NODISCARD PACE__FORCEINLINE auto iterate( R view, Act&& act, Options&&... options )
    -> views::TrackedSpan<R, AutoFinishWith<Bar, std::decay_t<Act>>>
  {
    return { std::move( view ),
             auto_finish<Bar>( std::forward<Act>( act ), std::forward<Options>( options )... ) };
  }
#endif
  template<typename Bar,
           template<typename> class Automation = AutoFinish,
           class R,
           typename Proc,
           typename... Options>
  PACE__FORCEINLINE auto iterate( R&& range, Proc&& op, Options&&... options )
#ifdef __cpp_concepts
    requires( details::traits::is_sized_range<R>::value && std::is_constructible_v<Bar, Options...>
              && details::traits::is_invocable<
                Proc&,
                details::traits::IterReference_t<details::traits::IteratorOf_t<R>>>::value )
#else
    -> typename std::enable_if<details::traits::AllOf<
      details::traits::is_sized_range<R>,
      std::is_constructible<Bar, Options...>,
      details::traits::
        is_invocable<Proc&, details::traits::IterReference_t<details::traits::IteratorOf_t<R>>>>::value>::type
#endif
  {
    auto_finish<Bar, Automation>( std::forward<Options>( options )... )
      .iterate( std::forward<R>( range ), std::forward<Proc>( op ) );
  }
  template<typename Bar, class R, typename Proc, typename Act, typename... Options>
  PACE__FORCEINLINE auto iterate( R&& range, Proc&& op, Act&& act, Options&&... options )
#ifdef __cpp_concepts
    requires( details::traits::is_sized_range<R>::value && std::is_constructible_v<Bar, Options...>
              && ( details::traits::is_invocable<std::decay_t<Act>&>::value
                   || details::traits::is_invocable<std::decay_t<Act>&, Bar&>::value ) )
#else
    -> typename std::enable_if<details::traits::AllOf<details::traits::is_sized_range<R>,
                                                      std::is_constructible<Bar, Options...>>::value>::type
#endif
  {
#ifndef __cpp_concepts
    static_assert( details::traits::is_invocable<typename std::decay<Act>::type&>::value
                     || details::traits::is_invocable<typename std::decay<Act>::type&, Bar&>::value,
                   "unknown callback type" );
#endif
    auto_finish<Bar>( std::forward<Act>( act ), std::forward<Options>( options )... )
      .iterate( std::forward<R>( range ), std::forward<Proc>( op ) );
  }
} // namespace pace

#endif
