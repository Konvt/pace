#ifndef PACE_RENDERABLE
#define PACE_RENDERABLE

#include "../../Indicator.hpp"
#include "../concurrent/Quiesce.hpp"
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
                 auto& ostream    = io::OStream<Sink>::itself();
                 const auto istty = console::Screen<Sink>::renderable();
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

                   static_cast<MostDerived*>( this )->prologue( ostream, !istty && config::auto_style_off() );

                   if ( istty ) {
                     ostream << console::linewipe;
                     if PACE__CXX17_CNSTXPR ( Zone == Region::Bottom || Zone == Region::Top )
                       // Unlike the conventional mode,
                       // Top and Bottom render in a dedicated canvas area on the terminal interface,
                       // where no other rendering content typically intrudes.
                       // Therefore, we don't need to consider interference from other elements,
                       // making it unnecessary to output an extra newline character.
                       ostream << console::restorecursor;
                     else
                       ostream << console::nextline;
                   } else
                     ostream << console::nextline;
                   concurrent::atomic_commit_all( state_,
                                                  Phase::Awake,
                                                  Phase::Refresh,
                                                  std::memory_order_release );
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

                   static_cast<MostDerived*>( this )->monologue( ostream,
                                                                 !istty && config::auto_style_off() );

                   if ( istty ) {
                     ostream << console::linewipe;
                     if PACE__CXX17_CNSTXPR ( Zone == Region::Bottom || Zone == Region::Top )
                       ostream << console::restorecursor;
                     else
                       ostream << console::nextline;
                   } else
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

                   static_cast<MostDerived*>( this )->epilogue( ostream, !istty && config::auto_style_off() );

                   const auto hide_done = config::hide_completed();
                   if ( istty ) {
                     ostream << io::when( hide_done, console::linestart ) << console::linewipe;
                     if PACE__CXX17_CNSTXPR ( Zone == Region::Bottom || Zone == Region::Top )
                       ostream << console::resetscroll << console::restorecursor;
                     else
                       ostream << io::when( !hide_done, console::nextline );
                   } else
                     ostream << console::nextline;
                   ostream << io::flush;
                   concurrent::atomic_commit_all( state_, Phase::Stop, std::memory_order_release );
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
        std::mutex mtx_;
        concurrent::Quiesce motion_;
        enum class Phase : std::uint8_t { Stop, Awake, Refresh, Finish };
        std::atomic<Phase> state_ { Phase::Stop };

        // This method always assumes that the cursor is at the beginning of the line.
        PACE__NODISCARD PACE__FORCEINLINE bool draw( io::CharPipeline& pipeline, bool style_off )
        {
          auto guard = utils::make_scope_fail( [this]() noexcept {
            concurrent::atomic_commit_all( state_, Phase::Stop, std::memory_order_release );
          } );
          switch ( state_.load( std::memory_order_relaxed ) ) {
          case Phase::Awake:
            static_cast<MostDerived*>( this )->prologue( pipeline, style_off );
            concurrent::atomic_commit_all( state_, Phase::Awake, Phase::Refresh, std::memory_order_release );
            break;
          case Phase::Refresh: static_cast<MostDerived*>( this )->monologue( pipeline, style_off ); break;
          case Phase::Finish:
            static_cast<MostDerived*>( this )->epilogue( pipeline, style_off );
            concurrent::atomic_commit_all( state_, Phase::Stop, std::memory_order_release );
            break;

          default: return false;
          }
          return true;
        }

        template<Phase Next>
        PACE__FORCEINLINE void do_reset() noexcept( Next == Phase::Stop )
        {
          auto expected = Phase::Refresh;
          if ( state_.compare_exchange_strong( expected, Next, std::memory_order_release ) ) {
            if PACE__CXX17_CNSTXPR ( Mode != Policy::Async )
              motion_.quiet();
            if PACE__CXX17_CNSTXPR ( Next == Phase::Stop )
              this->do_halt( true ); // exception free
            else {
              auto guard = utils::make_scope_fail( [this]() noexcept {
                auto expected = Next;
                // The `do_tick` state machine ensures that the previous state cannot be Phase::Awake.
                state_.compare_exchange_strong( expected, Phase::Refresh, std::memory_order_relaxed );
              } );
              this->do_halt( false );
            }
          }
        }
        template<typename F>
        PACE__FORCEINLINE void do_tick( F&& ticker ) &
        {
          switch ( state_.load( std::memory_order_relaxed ) ) {
            // Avoid allowing other threads to continue calling the `ticker` normally
            // when an exception is thrown by `do_boot`.
          case Phase::Awake: PACE__FALLTHROUGH;
          case Phase::Stop:  {
            // We cannot simply use CAS + std::atomic::wait to implement concurrent state transitions,
            // even if doing so would be performant. This is because if `do_boot` throws an exception,
            // a CAS will fail, and threads that entered the wait will be woken up at a wrong position;
            // they will then fall through directly into the Phase::Refresh state and start calling `ticker`.
            // We must ensure that, before the thread calling `do_tick` enters the Phase::Refresh case,
            // the progress bar’s state is already Phase::Refresh.
            std::lock_guard<std::mutex> lock { mtx_ };
            auto expected = Phase::Stop;
            if ( state_.compare_exchange_strong( expected, Phase::Awake, std::memory_order_relaxed ) ) {
              auto guard = utils::make_scope_fail(
                [this]() noexcept { state_.store( Phase::Stop, std::memory_order_relaxed ); } );
              static_cast<MostDerived*>( this )->on_awaken();
              ticker();
              this->do_boot();
              // We need to confirm that the rendering method has been executed once,
              // but we do not want to incur the IO overhead, so we need to wait for the state change.
              concurrent::atomic_wait( state_, Phase::Awake, std::memory_order_acquire );
              break;
            }
          }
            PACE__FALLTHROUGH;
          case Phase::Refresh: {
            if PACE__CXX17_CNSTXPR ( Mode != Policy::Async ) {
              auto hotspot = motion_.enter();
              ticker();
              render::Renderer<Sink>::itself().template commit<Mode>();
            } else
              ticker();
            // `commit` is empty when Mode == Async.
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
        ~Renderable() noexcept { abort(); }

      public:
        using Base::Base;
        Renderable( Soul config ) noexcept : config_ { std::move( config ) } {}

        // Normal closing of the object, all the information that has been output will remain on the screen.
        // This method will attempt to render the progress bar at least one more time.
        void finish() final { do_reset<Phase::Finish>(); }
        // Immediately shutdown the object, will not make any further output attempts.
        void abort() noexcept final { do_reset<Phase::Stop>(); }

        /// @brief Activate the object only, without any progress update.
        /// @note Simultaneous concurrent invocation with `tick` when the progress is approaching the end
        ///       may lead to race conditions.
        void poke() &
        {
          do_tick( []() noexcept {} );
        }

        PACE__NODISCARD PACE__FORCEINLINE bool active() const noexcept final
        { return state_.load( std::memory_order_relaxed ) != Phase::Stop; }

        // Return to the number of terminal lines the current object occupies.
        PACE__NODISCARD std::size_t layout_height() const noexcept
        { return state_.load( std::memory_order_relaxed ) == Phase::Stop ? 0 : 2; }
        // Return the IMMUTABLE layout width determined by the current config.
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
