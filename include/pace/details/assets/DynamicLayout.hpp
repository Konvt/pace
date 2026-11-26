#ifndef PACE_DYNAMIC_LAYOUT
#define PACE_DYNAMIC_LAYOUT

#include "../console/Screen.hpp"
#include "ManagedBar.hpp"

namespace pace {
  namespace details {
    namespace assets {
      template<Channel Sink, Policy Mode, Region Zone>
      class DynamicLayout final {
        enum class Locus : std::uint8_t { Offstage, Backstage, Onstage, Echo };
        struct Slot {
        private:
          template<typename Derived>
          PACE__NODISCARD static details::types::Visage draw( Indicator* item,
                                                              io::CharPipeline& pipeline,
                                                              bool style_off )
          {
            static_assert( std::is_base_of<Indicator, Derived>::value,
                           "Derived must inherit from Indicator" );
            PACE__TRUST( item != nullptr );
            return static_cast<Derived*>( item )->draw( pipeline, style_off );
          }

        public:
          details::types::Visage ( *render )( Indicator*, io::CharPipeline&, bool );
          Indicator* target;
          // Only the active bar will be constructed as a slot, hence, the default value is Backstage.
          Locus stage { Locus::Backstage };

          template<typename Config>
          explicit Slot( assets::ManagedBar<Config, Sink, Mode, Zone>* item ) noexcept
            : render { draw<assets::ManagedBar<Config, Sink, Mode, Zone>> }, target { item }
          {}

          PACE__NODISCARD bool aborted() const noexcept { return target == nullptr; }
        };

        std::vector<Slot> items_;
        std::size_t alive_cnt_ = 0;
        // For details on selecting lock types, see the comments in the @ref `do_shut`.
        mutable concurrent::SharedMutex res_mtx_;
        mutable std::mutex sched_mtx_;

        enum class Phase : std::uint8_t { Stop, Awake, Refresh, Finish };
        std::atomic<Phase> state_ { Phase::Stop };

        /// @return Indicates whether the current rendering has occupied a separate line.
        PACE__NODISCARD bool do_render( std::size_t pos,
                                        io::CharPipeline& pipeline,
                                        bool istty,
                                        bool style_off )
        {
          PACE__ASSERT( pos < items_.size() );
          auto& item = items_[pos];
          switch ( item.stage ) {
          case Locus::Echo: {
            if ( istty ) {
              if PACE__CXX17_CNSTXPR ( Zone == Region::Bottom )
                pipeline << console::prevline;
              else
                pipeline << console::linefeed;
              return true;
            }
            item.stage = Locus::Offstage;
          } break;

          case Locus::Backstage: {
            // There is a potential race condition:
            // in the asynchronous (with Policy::Signal) mode,
            // if a progress bar object `bar` is aborted after it has been activated
            // but before the asynchronous thread has rendered it,
            // and assuming there are other progress bars still present in `DynamicLayout`
            // (i.e., the termination condition for `do_halt` is not met),
            // then later when the asynchronous thread accesses the slot
            // of this bar — which has not yet been removed — it may result in dereferencing a null pointer.

            // Therefore, we require that **any bar object must ensure it is rendered at least once**
            // after being activated (see the @ref `do_tick` implementation in Renderable.hpp).
            PACE__ASSERT( item.aborted() == false );
            if PACE__CXX17_CNSTXPR ( Zone == Region::Bottom || Zone == Region::Top ) {
              // We don't want the terminal space occupied by this layout to expand indefinitely.
              const auto phantom =
                std::find_if( items_.rbegin() + items_.size() - pos, items_.rend(), []( const Slot& slot ) {
                  return slot.stage == Locus::Echo || slot.aborted();
                } );
              if ( phantom != items_.rend() )
                phantom->stage = Locus::Offstage;
            }
            item.stage = Locus::Onstage;
          }
            goto Portrayal; // NOLINT(cppcoreguidelines-avoid-goto): avoid nullptr check
          case Locus::Onstage: {
            if ( item.aborted() )
              PACE__UNLIKELY
              {
                // being aborted or even destructed
                if ( istty ) {
                  if PACE__CXX17_CNSTXPR ( Zone == Region::Bottom )
                    pipeline << console::prevline;
                  else
                    pipeline << console::linefeed;
                  item.stage = Locus::Echo;
                  return true;
                }
                item.stage = Locus::Offstage;
                break;
              }

Portrayal:
            const auto status = ( *item.render )( item.target, pipeline, style_off );

            if ( status != types::Visage::Existent ) {
              //
              PACE__ASSERT( this->ElementAt_t<Pos>::active() == false );
              // Mark it as nullptr to prevent it from being found during `pop`.
              item.target = nullptr;
              // There is a legitimate race condition here: when a progress bar is `abort`ed,
              // if other progress bars are still active and being updated,
              // the rendering thread may access that progress bar before the thread
              // calling `abort` marks its target as `nullptr`,
              // because the `pop` function and the rendering thread have no synchronization relationship
              // in asynchronous mode. At that point, `draw` does nothing.
              // Therefore, here we should treat types::Visage::Completed and types::Visage::Extinct
              // as equivalent cases for state transitions.
              item.stage  = istty && status != types::Visage::Effaced ? Locus::Echo : Locus::Offstage;
            }
            if ( status != types::Visage::Effaced ) {
              pipeline << io::when( status != types::Visage::Extinct, console::eraseafter );
              if PACE__CXX17_CNSTXPR ( Zone == Region::Bottom )
                pipeline << io::choice( istty, console::prevline, console::linefeed );
              else
                pipeline << console::linefeed;
              return true;
            } else
              pipeline << io::when( istty, console::deleteline );
          } break;

          default: break;
          }
          return false;
        }
        /// @return The number of lines occupied in this rendering.
        std::size_t do_schedule( io::CharPipeline& pipeline, bool istty, bool style_off ) &
        {
          std::size_t num_newlines = 0;
          if ( Zone != Region::Bottom || !istty )
            for ( std::size_t i = 0; i < items_.size(); ++i )
              num_newlines += do_render( i, pipeline, istty, style_off );
          else
            for ( std::size_t i = items_.size(); i-- > 0; )
              num_newlines += do_render( i, pipeline, istty, style_off );
          return num_newlines;
        }

        PACE__FORCEINLINE void eliminate() noexcept
        {
          // Search for the extinct progress bars and remove them.
          items_.erase(
            std::remove_if( items_.begin(),
                            items_.end(),
                            []( const Slot& slot ) noexcept { return slot.stage == Locus::Offstage; } ),
            items_.end() );
        }

      public:
        DynamicLayout()                                  = default;
        DynamicLayout( const DynamicLayout& )            = delete;
        DynamicLayout& operator=( const DynamicLayout& ) = delete;

        // Since DynamicLayout is created within the space pointed to by `std::shared_ptr`,
        // it will only be destroyed after all progress bars have been destructed.
        // Given the destruction guarantee of `Renderable`,
        // rendering has clearly stopped by then, so no additional work is required.
        ~DynamicLayout() = default;

        template<typename C>
        void append( assets::ManagedBar<C, Sink, Mode, Zone>* item ) &
        {
          auto& executor = render::Renderer<Sink>::itself();
          std::unique_lock<std::mutex> lock1 { sched_mtx_ };
          if ( state_.load( std::memory_order_relaxed ) == Phase::Stop ) {
            PACE__ASSERT( items_.empty() );
            if ( !executor.try_appoint( [this]() {
                   auto guard = utils::make_scope_fail(
                     [this]() noexcept { state_.store( Phase::Stop, std::memory_order_release ); } );
                   auto& screen         = console::Screen<Sink>::itself();
                   auto& ostream        = screen.ostream();
                   std::uint16_t height = 0; // used only in Region::Bottom or Region::Top
                   switch ( state_.load( std::memory_order_relaxed ) ) {
                   case Phase::Awake: {
                     const auto istty = screen.interactive();
                     screen.set_renderable( istty );
                     if ( istty ) {
                       if PACE__CXX17_CNSTXPR ( Zone == Region::Fixed || Zone == Region::Relative ) {
                         ostream << console::linestart;
                         if PACE__CXX17_CNSTXPR ( Zone == Region::Fixed )
                           ostream << console::savecursor;
                       } else if PACE__CXX17_CNSTXPR ( Zone == Region::Bottom || Zone == Region::Top ) {
                         height = screen.height();
                         (void)screen.vertical_alteration( height );
                       }
                     }
                     {
                       std::lock_guard<concurrent::SharedMutex> lock { res_mtx_ };
                       if PACE__CXX17_CNSTXPR ( Zone == Region::Bottom || Zone == Region::Top ) {
                         if ( istty ) {
                           ostream << console::savecursor;
                           // behaviors::Renderable and `append` together ensure that
                           // each progress bar is rendered at least once before its first `tick` returned,
                           // and since the Phase::Awake only appears when `DynamicLayout` is rendered for
                           // the first time, there cannot be any progress bars that need to be
                           // `retired` at that point. And the size of `items_` is the number of
                           // progress bars that need to be rendered.
                           if PACE__CXX17_CNSTXPR ( Zone == Region::Bottom )
                             ostream << io::when( height > items_.size(),
                                                  io::concat( io::cursornext_n( items_.size() ),
                                                              io::cursorup_n( items_.size() ),
                                                              console::savecursor,
                                                              io::setscroll( 1, height - items_.size() ) ) )
                                     << io::setline( height );
                           else
                             ostream << io::when( height > items_.size(),
                                                  io::concat( io::cursorup_n( items_.size() ),
                                                              io::cursordown_n( items_.size() ),
                                                              console::savecursor,
                                                              io::setscroll( items_.size() + 1, height ) ) )
                                     << console::firstline;
                         } else
                           screen.vertical_alteration( 0 );
                       }
                       do_schedule( ostream, istty, !istty && config::auto_style_off() );
                     }
                     if PACE__CXX17_CNSTXPR ( Zone == Region::Bottom || Zone == Region::Top )
                       ostream << io::when( istty, console::restorecursor );
                     ostream << io::flush;
                     auto expected = Phase::Awake;
                     state_.compare_exchange_strong( expected, Phase::Refresh, std::memory_order_relaxed );
                   } break;

                   case Phase::Refresh: {
                     const auto istty        = screen.renderable();
                     // used only in Region::Bottom or Region::Top
                     std::size_t active_rows = 0, new_rows = 0;
                     std::uint16_t old_height = 0; // used only in Region::Bottom or Region::Top
                     const auto num_newlines  = [&]() {
                       std::lock_guard<concurrent::SharedMutex> lock { res_mtx_ };
                       if ( istty ) {
                         if PACE__CXX17_CNSTXPR ( Zone == Region::Fixed || Zone == Region::Relative ) {
                           // In the Region::Bottom or Region::Top mode, when a new progress bar is added,
                           // we will first attempt to reclaim a row that is occupied by the Locus::Echo.
                           // Moreover, we do not want to lose too much historical output.
                           // Therefore, we only update the prefix's Locus::Echo to Locus::Offstage in the
                           // Region::Fixed or Region::Relative mode.
                           const auto essential =
                             std::find_if( items_.begin(), items_.end(), []( const Slot& slot ) noexcept {
                               // Although the prefix elements of `slot.aborted() == true` can skip
                               // the Locus::Echo and be marked as Locus::Offstage directly,
                               // the `do_render` state machine has this special handling code,
                               // so there is no need for us to intervene in advance.
                               return slot.stage == Locus::Backstage || slot.stage == Locus::Onstage;
                             } );
                           std::size_t removed_prefix = 0; // used only in Region::Bottom
                           std::for_each( items_.begin(), essential, [&]( Slot& slot ) noexcept {
                             if PACE__CXX17_CNSTXPR ( Zone == Region::Fixed )
                               removed_prefix += slot.stage == Locus::Echo;
                             slot.stage = Locus::Offstage;
                           } );
                           if PACE__CXX17_CNSTXPR ( Zone == Region::Fixed ) {
                             ostream << console::restorecursor
                                     << io::when( removed_prefix > 0,
                                                  io::concat( io::cursordown_n( removed_prefix ),
                                                              console::savecursor ) );
                           } else
                             ostream << io::prevline_n( std::count_if( items_.cbegin(),
                                                                       items_.cend(),
                                                                       []( const Slot& slot ) noexcept {
                                                                         return slot.stage == Locus::Onstage
                                                                             || slot.stage == Locus::Echo;
                                                                       } ) );
                         } else if PACE__CXX17_CNSTXPR ( Zone == Region::Bottom || Zone == Region::Top ) {
                           active_rows =
                             std::count_if( items_.cbegin(), items_.cend(), []( const Slot& slot ) noexcept {
                               return slot.stage != Locus::Offstage;
                             } );
                           // Based on the implementation of `append`,
                           // all newly added progress bars are continuously distributed at the end.
                           new_rows = utils::distance(
                             items_.crbegin(),
                             std::find_if( items_.crbegin(), items_.crend(), []( const Slot& slot ) noexcept {
                               return slot.stage != Locus::Backstage;
                             } ) );
                           height     = screen.height();
                           old_height = screen.vertical_alteration( height );
                           ostream << console::savecursor;
                           if PACE__CXX17_CNSTXPR ( Zone == Region::Bottom ) {
                             ostream << io::when(
                               height > active_rows,
                               io::concat( io::when( new_rows > 0,
                                                     io::concat( io::cursornext_n( new_rows ),
                                                                 io::cursorup_n( new_rows ),
                                                                 console::savecursor ) ),
                                           io::setscroll( height + 1 - active_rows, height ) ) )
                                     << io::setline( height );
                             // Reverse select the area, see the corresponding comments
                             // in @ref `StaticLayout::do_boot` for the reasons.
                           } else
                             // Since the terminal scrolls from bottom to top,
                             // most of the already output information is concentrated at the top.
                             // We don’t want a progress bar to create an inexplicable blank line
                             // between itself and the existing information when it `retire`
                             // (although overwriting existing information with a newly added progress bar
                             // is visually acceptable). Therefore, unlike Bottom mode,
                             // in Top mode we set the scroll region to the entire screen.
                             ostream << io::when( height > active_rows, console::resetscroll )
                                     << console::firstline;
                         }
                       } else if PACE__CXX17_CNSTXPR ( Zone == Region::Bottom || Zone == Region::Top )
                         (void)screen.vertical_alteration( 0 );

                       return do_schedule( ostream, istty, !istty && config::auto_style_off() );
                     }();

                     if PACE__CXX17_CNSTXPR ( Zone == Region::Bottom || Zone == Region::Top ) {
                       if ( istty ) {
                         PACE__ASSERT( active_rows > new_rows );
                         const auto canvas = active_rows - new_rows;
                         if PACE__CXX17_CNSTXPR ( Zone == Region::Bottom )
                           // `scrolldown_n` is used to handle a special case:
                           // when the number of line breaks output in this round is fewer than
                           // the number of line breaks output in the previous render,
                           // the progress bar will leave several blank lines between itself
                           // and the bottom of the terminal due to the effect of `console::deleteline`.
                           // We need to discard these blank lines so that the progress bar remains
                           // aligned with the bottom of the terminal.
                           ostream
                             << io::when( canvas > num_newlines, io::scrolldown_n( canvas - num_newlines ) )
                             << io::choice(
                                  height > num_newlines,
                                  io::concat( io::setscroll( 1, height - num_newlines ),
                                              console::restorecursor,
                                              io::when( height < old_height,
                                                        io::concat( io::cursordown_n( num_newlines ),
                                                                    io::cursorup_n( num_newlines ) ) ) ),
                                  console::restorecursor );
                         else
                           ostream << io::choice(
                             height > num_newlines,
                             io::concat( io::setscroll( num_newlines + 1, height ),
                                         console::restorecursor,
                                         io::when( height < old_height,
                                                   io::concat( io::cursorup_n( num_newlines ),
                                                               io::cursordown_n( num_newlines ) ) ) ),
                             console::restorecursor );
                       }
                     }
                     ostream << io::flush;
                   } break;

                   case Phase::Finish: {
                     const auto istty        = screen.renderable();
                     std::size_t active_rows = 0; // used only in Region::Bottom or Region::Top
                     if ( istty ) {
                       if PACE__CXX17_CNSTXPR ( Zone == Region::Fixed )
                         ostream << console::restorecursor;
                       else if PACE__CXX17_CNSTXPR ( Zone == Region::Relative )
                         ostream << io::prevline_n(
                           std::count_if( items_.cbegin(), items_.cend(), []( const Slot& slot ) noexcept {
                             return slot.stage == Locus::Onstage || slot.stage == Locus::Echo;
                           } ) );
                       else if PACE__CXX17_CNSTXPR ( Zone == Region::Bottom || Zone == Region::Top ) {
                         active_rows =
                           std::count_if( items_.cbegin(), items_.cend(), []( const Slot& slot ) noexcept {
                             return slot.stage != Locus::Offstage;
                           } );
                         height = screen.height();
                         ostream << console::savecursor;
                         if PACE__CXX17_CNSTXPR ( Zone == Region::Bottom ) {
                           ostream << io::when( height > active_rows,
                                                io::setscroll( height + 1 - active_rows, height ) )
                                   << io::setline( height );
                           // See the corresponding comments in @ref `StaticLayout::do_boot`.
                         } else
                           ostream << io::when( height > active_rows, console::resetscroll )
                                   << console::firstline;
                       }
                     }
                     const auto num_newlines = [&]() {
                       std::lock_guard<concurrent::SharedMutex> lock { res_mtx_ };
                       return do_schedule( ostream, istty, !istty && config::auto_style_off() );
                     }();
                     if PACE__CXX17_CNSTXPR ( Zone == Region::Bottom || Zone == Region::Top ) {
                       if PACE__CXX17_CNSTXPR ( Zone == Region::Bottom )
                         // In Phase::Finish, no new progress bars can exist,
                         // so `active_rows` is the number of rows output in the previous render pass.
                         ostream << io::when( num_newlines > 0 && active_rows > num_newlines,
                                              io::scrolldown_n( active_rows - num_newlines ) );
                       ostream << io::when( istty,
                                            io::concat( console::resetscroll, console::restorecursor ) );
                     }
                     ostream << io::flush;
                     state_.store( Phase::Stop, std::memory_order_release );
                   } break;

                   default: return;
                   }
                 } ) )
              PACE__UNLIKELY throw exception::StateError(
                charcodes::make_literal( "pace: another progress bar instance is already running" ) );

            auto guard = utils::make_scope_fail( [&]() noexcept {
              state_.store( Phase::Stop, std::memory_order_relaxed );
              executor.dismiss();
              items_.clear();
            } );
            items_.emplace_back( item );

            console::Screen<Sink>::itself().ostream() << io::reset;
            state_.store( Phase::Awake, std::memory_order_release );
            executor.template activate<Mode>();
            ++alive_cnt_;
          } else {
            {
              std::lock_guard<concurrent::SharedMutex> lock2 { res_mtx_ };
              eliminate();
              items_.emplace_back( item );
            }
            ++alive_cnt_;
            auto guard = utils::make_scope_fail( [this, item]() noexcept {
              --alive_cnt_;
              std::lock_guard<concurrent::SharedMutex> lock2 { res_mtx_ };
              const auto itr =
                std::find_if( items_.begin(), items_.end(), [item]( const Slot& slot ) noexcept {
                  return item == slot.target;
                } );
              // We cannot guarantee whether other threads have completed the rendering in advance
              // by calling the `tick` method of another progress bar during the exception throwing process.
              // Therefore, we can only consider the current object as being aborted
              // instead of directly deleting it.
              if ( itr != items_.end() )
                itr->target = nullptr;
            } );
            executor.template trigger<Mode>();
          }
        }
        void pop( const Indicator* item, bool forced = false )
        {
          auto& executor = render::Renderer<Sink>::itself();
          PACE__ASSERT( executor.empty() == false );
          PACE__ASSERT( height() > 0 );
          {
            std::lock_guard<std::mutex> lock1 { sched_mtx_ };
            if ( --alive_cnt_ == 0 ) {
              if ( forced )
                state_.store( Phase::Stop, std::memory_order_release );
              else {
                state_.store( Phase::Finish, std::memory_order_release );
                executor.execute();
              }
              {
                std::lock_guard<concurrent::SharedMutex> lock2 { res_mtx_ };
                items_.clear();
              }
              executor.dismiss_then( []() noexcept { console::Screen<Sink>::itself().ostream().reset(); } );
              return;
            }
            if ( !forced )
              executor.execute();
          }
          // In asynchronous rendering mode, there is a race condition here:
          // if thread A is stopping a bar object while another thread B attempts to call the `tick` method,
          // it's possible that B could succeed in reactivating before A releases `sched_mtx_`
          // and acquires `res_mtx_`.
          // This might happen because the rendering thread marks the corresponding item
          // in `items_` as nullptr, allowing B to reactivate successfully.
          // Then, A proceeds to the following code block and removes the just-activated progress bar.

          // However, this behavior represents an excessive number of `tick` calls beyond the quota
          // (because the `finish`/`retire`/`abort` call of A has not yet returned, the current task cycle has
          // not yet ended), which is not guaranteed to be thread-safe, so it does not need to be handled.
          std::lock_guard<concurrent::SharedMutex> lock { res_mtx_ };
          if ( forced ) {
            // When forced == false, a rendering operation will be executed first.
            // At this point, the rendering thread will automatically mark the bar
            // at the corresponding position as nullptr,
            // so there is no need to remove it repeatedly.
            const auto itr = std::find_if( items_.begin(), items_.end(), [item]( const Slot& slot ) noexcept {
              return item == slot.target;
            } );
            // Mark target as empty,
            // and then search for the first k invalid or destructed bars and remove them.
            if ( itr != items_.end() )
              itr->target = nullptr;
          }
          eliminate();
        }

        PACE__NODISCARD PACE__FORCEINLINE bool online() const noexcept
        { return state_.load( std::memory_order_relaxed ) != Phase::Stop; }
        PACE__NODISCARD PACE__FORCEINLINE std::size_t height() const noexcept
        {
          if ( state_.load( std::memory_order_acquire ) == Phase::Stop )
            return 0;
          concurrent::SharedLock<concurrent::SharedMutex> lock { res_mtx_ };
          const auto count = std::count_if( items_.cbegin(), items_.cend(), []( const Slot& slot ) noexcept {
            return slot.stage != Locus::Offstage;
          } );
          return count + ( count > 0 );
        }
      };
    } // namespace assets
  } // namespace details
} // namespace pace

#endif
