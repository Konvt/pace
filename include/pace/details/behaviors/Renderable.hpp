#ifndef PACE_RENDERABLE
#define PACE_RENDERABLE

#include "../../Indicator.hpp"
#include "../behaviors/Incremental.hpp"
#include "../console/Escode.hpp"
#include "../io/OStream.hpp"
#include "../render/Builder.hpp"
#include "../render/Renderer.hpp"
#include "../traits/C3.hpp"
#include "Reactive.hpp"
#include <mutex>

namespace pace {
  namespace details {
    namespace behaviors {
      template<typename Base, typename Derived>
      class Renderable;
      template<typename Base,
               template<typename, Channel, Policy, Region> class Derived,
               typename Soul,
               Channel Outlet,
               Policy Mode,
               Region Area>
      class Renderable<Base, Derived<Soul, Outlet, Mode, Area>> : public Base {
        static_assert( traits::is_instance_of<Soul, aspects::RenderRule>::value,
                       "the config type must derive from RenderRule" );
        // to call "do_tick()"
        template<typename, typename>
        friend class Incremental;
        // to use "mtx_"
        template<typename, typename>
        friend class Reactive;
        using MostDerived = Derived<Soul, Outlet, Mode, Area>;

        friend PACE__FORCEINLINE void draw_content( Renderable& self ) { self.draw_content(); }

      protected:
        render::Builder<Soul> config_;
        mutable std::mutex mtx_;
        enum class Phase : std::uint8_t { Stop, Awake, Refresh, Finish };
        std::atomic<Phase> state_ { Phase::Stop };

      private:
        PACE__FORCEINLINE void draw_content()
        {
          switch ( state_.load( std::memory_order_acquire ) ) {
          case Phase::Awake:   static_cast<MostDerived*>( this )->prologue(); break;
          case Phase::Refresh: static_cast<MostDerived*>( this )->monologue(); break;
          case Phase::Finish:  static_cast<MostDerived*>( this )->epilogue(); break;
          default:             return;
          }
        }

        virtual void do_halt( bool forced = false ) noexcept
        {
          auto& executor = render::Renderer<Outlet>::itself();
          PACE__ASSERT( executor.empty() == false );
          if ( !forced )
            executor.template trigger<Mode>();
          executor.dismiss_then( []() noexcept { io::OStream<Outlet>::itself().release(); } );
        }
        virtual void do_boot() & noexcept( false )
        {
          auto& executor = render::Renderer<Outlet>::itself();
          if ( !executor.try_appoint( [this]() {
                 // No exceptions are caught here, this should be done by the thread manager.
                 auto& ostream    = io::OStream<Outlet>::itself();
                 const auto istty = console::TermContext<Outlet>::itself().connected();
                 switch ( state_.load( std::memory_order_acquire ) ) {
                 case Phase::Awake: {
                   if PACE__CXX17_CNSTXPR ( Area == Region::Fixed )
                     if ( istty )
                       ostream << console::savecursor;
                   static_cast<MostDerived*>( this )->prologue();
                   ostream << console::nextline;
                   ostream << io::flush;
                 } break;
                 case Phase::Refresh: {
                   if ( istty ) {
                     if PACE__CXX17_CNSTXPR ( Area == Region::Fixed )
                       ostream << console::resetcursor;
                     else
                       ostream << console::prevline << console::linestart;
                     ostream << console::linewipe;
                   }
                   static_cast<MostDerived*>( this )->monologue();
                   ostream << console::nextline;
                   ostream << io::flush;
                 } break;
                 case Phase::Finish: {
                   if ( istty ) {
                     if PACE__CXX17_CNSTXPR ( Area == Region::Fixed )
                       ostream << console::resetcursor;
                     else
                       ostream << console::prevline << console::linestart;
                     ostream << console::linewipe;
                   }
                   static_cast<MostDerived*>( this )->epilogue();
                   if ( istty && config::hide_completed() )
                     ostream << console::linestart << console::linewipe;
                   else
                     ostream << console::nextline;
                   ostream << io::flush;
                 } break;
                 default: return;
                 }
               } ) )
            PACE__UNLIKELY throw exception::InvalidState(
              charcodes::make_literal( "pace: another progress bar instance is already running" ) );

          io::OStream<Outlet>::itself() << io::release; // reset the state.
          auto guard = utils::make_scope_fail( [&executor]() noexcept { executor.dismiss(); } );
          executor.template activate<Mode>();
        }

      protected:
        template<bool Forced>
        PACE__FORCEINLINE void do_reset() noexcept( Forced )
        {
          if ( state_.load( std::memory_order_relaxed ) != Phase::Stop ) {
            if PACE__CXX17_CNSTXPR ( Forced )
              state_.store( Phase::Stop, std::memory_order_release );
            else {
              this->react();
              state_.store( Phase::Finish, std::memory_order_release );
            }
            this->do_halt( Forced );
          } else
            state_.store( Phase::Stop, std::memory_order_release );
        }
        template<typename F>
        void do_tick( F&& ticker ) & noexcept( false )
        {
          switch ( state_.load( std::memory_order_acquire ) ) {
          case Phase::Stop:  PACE__FALLTHROUGH;
          case Phase::Awake: {
            std::lock_guard<std::mutex> lock { mtx_ };
            if ( state_.load( std::memory_order_acquire ) == Phase::Stop ) {
              static_cast<MostDerived*>( this )->on_awaken();
              state_.store( Phase::Awake, std::memory_order_relaxed );

              auto guard = utils::make_scope_fail(
                [this]() noexcept { state_.store( Phase::Stop, std::memory_order_relaxed ); } );
              this->do_boot();
            }
          }
            PACE__FALLTHROUGH;
          case Phase::Refresh: {
            std::forward<F>( ticker )();

            if ( static_cast<const MostDerived*>( this )->test_completion() )
              PACE__UNLIKELY
              {
                // Since `Reactive` can also attempt to acquire write locks,
                // it is necessary to always acquire write locks here.
                std::lock_guard<std::mutex> lock { mtx_ };
                do_reset<false>();
              }
            else
              render::Renderer<Outlet>::itself().template commit<Mode>();
          } break;

          default: utils::unreachable();
          }
        }

        Renderable() = default;
        Renderable( Renderable&& rhs ) noexcept( std::is_nothrow_move_constructible<Base>::value )
          : Base( std::move( rhs ) ), config_ { std::move( rhs.config_ ) }
        {}
        Renderable& operator=( Renderable&& rhs ) & noexcept( std::is_nothrow_move_assignable<Base>::value )
        {
          config_ = std::move( rhs.config_ );
          Base::operator=( std::move( rhs ) );
          return *this;
        }
        ~Renderable() noexcept { do_reset<true>(); }

      public:
        using Base::Base;
        Renderable( Soul config ) noexcept : config_ { std::move( config ) } {}

        PACE__NODISCARD PACE__FORCEINLINE bool active() const noexcept final
        { return state_.load( std::memory_order_relaxed ) != Phase::Stop; }

        void reset() final
        {
          std::lock_guard<std::mutex> lock { mtx_ };
          do_reset<false>();
          PACE__ASSERT( active() == false );
        }
        void abort() noexcept final
        {
          std::lock_guard<std::mutex> lock { mtx_ };
          do_reset<true>();
          PACE__ASSERT( active() == false );
        }

        PACE__FORCEINLINE Soul& config() & noexcept { return config_; }
        PACE__FORCEINLINE const Soul& config() const& noexcept { return config_; }
        PACE__FORCEINLINE Soul&& config() && noexcept { return std::move( config_ ); }

        PACE__CXX20_CNSTXPR void swap( Renderable& other ) noexcept
        {
          Base::swap( other );
          config_.swap( other.config_ );
        }
      };
    } // namespace behaviors

    PACE__INHERIT_REGISTER( behaviors::Renderable, behaviors::Reactive, behaviors::Incremental );
  } // namespace details
} // namespace pace

#endif
