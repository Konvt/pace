#ifndef PACE_RENDERABLE
#define PACE_RENDERABLE

#include "../../Indicator.hpp"
#include "../console/Escode.hpp"
#include "../console/Screen.hpp"
#include "../io/OStream.hpp"
#include "../render/Builder.hpp"
#include "../render/Renderer.hpp"
#include <mutex>

namespace pace {
  namespace details {
    namespace behaviors {
      template<typename Base, typename Derived>
      class Renderable;
      template<typename Base,
               template<typename, Channel, Policy, Region> class Derived,
               typename Soul,
               Channel Sink,
               Policy Mode,
               Region Zone>
      class Renderable<Base, Derived<Soul, Sink, Mode, Zone>> : public Base {
        static_assert( traits::is_instance_of<Soul, aspects::RenderRule>::value,
                       "the config type must derive from RenderRule" );
        using MostDerived = Derived<Soul, Sink, Mode, Zone>;

        virtual void do_halt( bool forced = false )
        {
          auto& executor = render::Renderer<Sink>::itself();
          PACE__ASSERT( executor.empty() == false );
          if ( !forced )
            executor.execute();
          executor.dismiss_then( []() noexcept { io::OStream<Sink>::itself().reset(); } );
        }
        virtual void do_boot() &
        {
          auto& executor = render::Renderer<Sink>::itself();
          if ( !executor.try_appoint( [this]() {
                 // No exceptions are caught here, this should be done by the thread manager.
                 auto& ostream        = io::OStream<Sink>::itself();
                 const auto istty     = console::Screen<Sink>::renderable();
                 const auto style_off = !istty && config::auto_style_off();
                 switch ( state_.load( std::memory_order_relaxed ) ) {
                 case Phase::Awake: {
                   auto guard = utils::make_scope_fail( [this]() noexcept {
                     concurrent::atomic_commit_all( state_, Phase::Stop, std::memory_order_release );
                   } );

                   if ( istty ) {
                     if PACE__CXX17_CNSTXPR ( Zone == Region::Fixed || Zone == Region::Relative ) {
                       ostream << console::linestart;
                       if PACE__CXX17_CNSTXPR ( Zone == Region::Fixed )
                         ostream << console::savecursor;
                     } else if PACE__CXX17_CNSTXPR ( Zone == Region::Bottom || Zone == Region::Top ) {
                       ostream << console::savecursor;
                       auto& screen      = console::Screen<Sink>::itself();
                       const auto height = screen.height();
                       if ( height > 1 ) {
                         if PACE__CXX17_CNSTXPR ( Zone == Region::Bottom )
                           ostream << console::nextline << io::setscroll( 1, height - 1 )
                                   << console::restorecursor << console::cursordown << console::cursorup
                                   << console::savecursor;
                         else
                           ostream << io::setscroll( 2, height ) << console::restorecursor
                                   << console::cursorup << console::cursordown << console::savecursor;
                       }
                       if PACE__CXX17_CNSTXPR ( Zone == Region::Bottom )
                         ostream << io::setline( height );
                       else
                         ostream << console::firstline;
                       (void)screen.vertical_alteration( height );
                     }
                   } else if PACE__CXX17_CNSTXPR ( Zone == Region::Bottom || Zone == Region::Top )
                     (void)console::Screen<Sink>::itself().vertical_alteration( 0 );

                   static_cast<MostDerived*>( this )->prologue( ostream, style_off );

                   if ( istty )
                     ostream << console::linewipe;
                   if ( ( Zone == Region::Bottom || Zone == Region::Top ) && istty )
                     // Unlike the conventional mode,
                     // Top and Bottom render in a dedicated canvas area on the terminal interface,
                     // where no other rendering content typically intrudes.
                     // Therefore, we don't need to consider interference from other elements,
                     // making it unnecessary to output an extra newline character.
                     ostream << console::restorecursor;
                   else
                     ostream << console::nextline;
                   ostream << io::flush;
                 } break;

                 case Phase::Refresh: {
                   if ( istty ) {
                     if PACE__CXX17_CNSTXPR ( Zone == Region::Fixed )
                       ostream << console::restorecursor;
                     else if PACE__CXX17_CNSTXPR ( Zone == Region::Relative )
                       ostream << console::prevline;
                     else if PACE__CXX17_CNSTXPR ( Zone == Region::Bottom || Zone == Region::Top ) {
                       ostream << console::savecursor;
                       auto& screen          = console::Screen<Sink>::itself();
                       const auto height     = screen.height();
                       const auto old_height = screen.vertical_alteration( height );
                       if ( height != old_height && height > 1 ) {
                         if PACE__CXX17_CNSTXPR ( Zone == Region::Bottom )
                           ostream << io::setscroll( 1, height - 1 )
                                   << io::when( height < old_height,
                                                io::concat( console::restorecursor,
                                                            console::cursordown,
                                                            console::cursorup,
                                                            console::savecursor ) );
                         else
                           ostream << io::setscroll( 2, height )
                                   << io::when( height < old_height,
                                                io::concat( console::restorecursor,
                                                            console::cursorup,
                                                            console::cursordown,
                                                            console::savecursor ) );
                       }
                       if PACE__CXX17_CNSTXPR ( Zone == Region::Bottom )
                         ostream << io::setline( height );
                       else
                         ostream << console::firstline;
                     }
                   } else if PACE__CXX17_CNSTXPR ( Zone == Region::Bottom || Zone == Region::Top )
                     (void)console::Screen<Sink>::itself().vertical_alteration( 0 );

                   static_cast<MostDerived*>( this )->monologue( ostream, style_off );

                   if ( istty )
                     ostream << console::linewipe;
                   if ( ( Zone == Region::Bottom || Zone == Region::Top ) && istty )
                     ostream << console::restorecursor;
                   else
                     ostream << console::nextline;
                   ostream << io::flush;
                 } break;

                 case Phase::Finish: {
                   if ( istty ) {
                     if PACE__CXX17_CNSTXPR ( Zone == Region::Fixed )
                       ostream << console::restorecursor;
                     else if PACE__CXX17_CNSTXPR ( Zone == Region::Relative )
                       ostream << console::prevline;
                     else if PACE__CXX17_CNSTXPR ( Zone == Region::Bottom )
                       ostream << console::savecursor << io::setline( console::Screen<Sink>::height() );
                     else if PACE__CXX17_CNSTXPR ( Zone == Region::Top )
                       ostream << console::savecursor << console::firstline;
                   }

                   static_cast<MostDerived*>( this )->epilogue( ostream, style_off );

                   const auto hide_done = config::hide_completed();
                   if ( istty ) {
                     if ( hide_done )
                       ostream << console::linestart;
                     ostream << console::linewipe;
                     if PACE__CXX17_CNSTXPR ( Zone == Region::Bottom || Zone == Region::Top )
                       ostream << console::resetscroll << console::restorecursor;
                     else if ( !hide_done )
                       ostream << console::nextline;
                   } else
                     ostream << console::nextline;
                   ostream << io::flush;
                 } break;

                 default: return;
                 }
               } ) )
            PACE__UNLIKELY throw exception::StateError(
              charcodes::make_literal( "pace: another progress bar instance is already running" ) );

          io::OStream<Sink>::itself() << io::reset; // reset the state.
          auto guard = utils::make_scope_fail( [&executor]() noexcept { executor.dismiss(); } );
          executor.template activate<Mode>();
        }

      protected:
        render::Builder<Soul> config_;
        mutable std::mutex mtx_;
        enum class Phase : std::uint8_t { Stop, Awake, Refresh, Finish };
        std::atomic<Phase> state_ { Phase::Stop };

        PACE__NODISCARD PACE__FORCEINLINE bool draw( io::CharPipeline& pipeline, bool style_off )
        {
          auto guard = utils::make_scope_fail( [this]() noexcept {
            concurrent::atomic_commit_all( state_, Phase::Stop, std::memory_order_release );
          } );
          switch ( state_.load( std::memory_order_relaxed ) ) {
          case Phase::Awake:   static_cast<MostDerived*>( this )->prologue( pipeline, style_off ); break;
          case Phase::Refresh: static_cast<MostDerived*>( this )->monologue( pipeline, style_off ); break;
          case Phase::Finish:  static_cast<MostDerived*>( this )->epilogue( pipeline, style_off ); break;
          default:             return false;
          }
          return true;
        }

        template<bool Forced>
        PACE__FORCEINLINE void do_reset() noexcept( Forced )
        {
          if ( state_.load( std::memory_order_relaxed ) != Phase::Stop ) {
            static_cast<MostDerived*>( this )->on_finish( Forced );
            if PACE__CXX17_CNSTXPR ( Forced )
              state_.store( Phase::Stop, std::memory_order_relaxed );
            else
              state_.store( Phase::Finish, std::memory_order_release );
            // `do_halt` throws an exception only when Forced == false;
            // if an exception is thrown, the state is guaranteed not to be in Phase::Stop,
            // so the object can still be properly reset on the next call.
            this->do_halt( Forced );
            PACE__ASSERT( state_ == Phase::Stop );
          } else
            state_.store( Phase::Stop, std::memory_order_relaxed );
        }
        /// @param ticker It must be an equivalent function-like object of the type `bool()`.
        ///               Its return value indicates whether there is any actual change in this tick.
        template<typename F>
        PACE__FORCEINLINE void do_tick( F&& ticker ) &
        {
          static_assert( traits::is_invocable_r<bool, F>::value,
                         "the ticker does not meet the interface requirements" );
          switch ( state_.load( std::memory_order_relaxed ) ) {
          case Phase::Stop: {
            std::lock_guard<std::mutex> lock { mtx_ };
            if ( state_.load( std::memory_order_relaxed ) == Phase::Stop ) {
              auto guard = utils::make_scope_fail(
                [this]() noexcept { state_.store( Phase::Stop, std::memory_order_relaxed ); } );
              state_.store( Phase::Awake, std::memory_order_relaxed );
              static_cast<MostDerived*>( this )->on_awaken();
              // make the initialization be atomic
              const bool modification = ticker();
              this->do_boot();
              // We need to confirm that the rendering method has been executed once,
              // but we do not want to incur the IO overhead, so we need to wait for the state change.
              concurrent::atomic_wait( state_, Phase::Awake, std::memory_order_acquire );
              if ( modification && static_cast<const MostDerived*>( this )->test_completion() )
                // Since `Reactive` can also attempt to acquire write locks,
                // it is necessary to always acquire write locks here.
                do_reset<false>();
              break;
            }
          }
            PACE__FALLTHROUGH;
          case Phase::Awake:   PACE__FALLTHROUGH;
          case Phase::Refresh: {
            if ( ticker() && static_cast<const MostDerived*>( this )->test_completion() )
              PACE__UNLIKELY
              {
                std::lock_guard<std::mutex> lock { mtx_ };
                do_reset<false>();
              }
            else
              render::Renderer<Sink>::itself().template commit<Mode>();
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
          this->Base::operator=( std::move( rhs ) );
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

        /// @brief Activate the object only, without any progress update.
        /// @note Simultaneous concurrent invocation with `tick` when the progress is approaching the end
        ///       may lead to race conditions.
        void poke()
        {
          do_tick( []() PACE__CXX17_CNSTXPR noexcept { return false; } );
        }

        PACE__NODISCARD std::size_t layout_height() const noexcept
        {
          if ( state_.load( std::memory_order_relaxed ) == Phase::Stop )
            return 0;
          return 2;
        }
        PACE__NODISCARD std::size_t layout_width() const noexcept { return config_.fixed_width(); }

        PACE__FORCEINLINE Soul& config() & noexcept { return config_; }
        PACE__FORCEINLINE const Soul& config() const& noexcept { return config_; }
        PACE__FORCEINLINE Soul&& config() && noexcept { return std::move( config_ ); }

        PACE__CXX20_CNSTXPR void swap( Renderable& other ) noexcept
        {
          config_.swap( other.config_ );
          this->Base::swap( other );
        }
      };
    } // namespace behaviors
  } // namespace details
} // namespace pace

#endif
