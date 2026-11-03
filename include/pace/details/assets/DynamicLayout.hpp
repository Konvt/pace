#ifndef PACE_DYNAMIC_LAYOUT
#define PACE_DYNAMIC_LAYOUT

#include "../console/Screen.hpp"
#include "../io/OStream.hpp"
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
          PACE__NODISCARD static bool draw( Indicator* item, io::CharPipeline& pipeline, bool style_off )
          {
            static_assert( traits::AllOf<std::is_base_of<Indicator, Derived>, traits::is_bar<Derived>>::value,
                           "Derived must inherit from Indicator" );
            PACE__TRUST( item != nullptr );
            return static_cast<Derived*>( item )->draw( pipeline, style_off );
          }

        public:
          bool ( *render )( Indicator*, io::CharPipeline&, bool );
          Indicator* target;
          // Only the active bar will be constructed as a slot, hence, the default value is Backstage.
          Locus stage { Locus::Backstage };

          template<typename Config>
          Slot( assets::ManagedBar<Config, Sink, Mode, Zone>* item ) noexcept
            : render { draw<assets::ManagedBar<Config, Sink, Mode, Zone>> }, target { item }
          {}

          PACE__NODISCARD bool aborted() const noexcept { return target == nullptr; }
        };

        std::vector<Slot> items_;
        std::size_t alive_cnt_ = 0;
        // For details on selecting lock types, see the comments in the `do_shut`.
        mutable concurrent::SharedMutex res_mtx_;
        mutable std::mutex sched_mtx_;

        enum class Phase : std::uint8_t { Stop, Awake, Refresh, Finish };
        std::atomic<Phase> state_ { Phase::Stop };

        // If Zone == Region::Fixed,
        //   it indicates the number of discarded lines.
        // If Zone == Region::Relative,
        //   it indicates the number of LF outputs in the last rendering.
        // If Zone == Region::Bottom or Zone == Region::Top,
        //   it indicates the maximum number of LF outputs in all the rendering.
        std::size_t line_variation_ = 0;

        /// @return Indicates whether the current rendering needs to occupy a separate line.
        PACE__NODISCARD bool do_render( std::size_t pos,
                                        io::CharPipeline& pipeline,
                                        bool istty,
                                        bool style_off,
                                        bool hide_done )
        {
          PACE__ASSERT( pos < items_.size() );
          auto& item = items_[pos];
          switch ( item.stage ) {
          case Locus::Echo: {
            if ( istty && !hide_done )
              return true;
            item.stage = Locus::Offstage;
          } break;

          case Locus::Backstage: {
            // There is a potential race condition:
            // in the asynchronous (with Policy::Signal) mode,
            // if a progress bar object `bar` is aborted after it has been activated
            // but before the asynchronous thread has rendered it,
            // and assuming there are other progress bars still present in `DynamicLayout`
            // (i.e., the termination condition for `do_halt` is not met),
            // and this particular bar does not belong to a contiguous terminated prefix
            // (see `eliminate` method), then later when the asynchronous thread accesses the slot
            // of this bar — which has not yet been removed — it may result in dereferencing a null pointer.

            // Therefore, we require that **any bar object must ensure it is rendered at least once**
            // after being activated (see the `do_tick` implementation in Renderable.hpp).
            PACE__ASSERT( item.aborted() == false );
            if PACE__CXX17_CNSTXPR ( Zone == Region::Bottom || Zone == Region::Top ) {
              // We don't want the terminal space occupied by this layout to expand
              // indefinitely, so without activating `hide_completed`, we prefer to retain
              // completed progress bars in a limited manner and reuse those terminal lines
              // when necessary, just like StaticLayout does.
              const auto phantom =
                std::find_if( items_.rbegin() + pos + 1, items_.rend(), []( const Slot& slot ) {
                  return slot.stage == Locus::Echo;
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
                if ( istty && !hide_done ) {
                  item.stage = Locus::Echo;
                  return true;
                }
                item.stage = Locus::Offstage;
                break;
              }

Portrayal:
            const auto painted = ( *item.render )( item.target, pipeline, style_off );

            if ( istty && painted )
              // If the progress bar is aborted, then there will be a critical condition here:
              // the status of the progress bar itself is marked as Phase::Stop,
              // but the rendering end is still unaware of this.
              // At this point, we need to use the bool returned by the `item.render`
              // to determine whether the rendering has occurred,
              // in order to prevent accidental erasure of the remaining characters of the bar.
              pipeline << console::linewipe;
            if ( item.target->active() )
              return true;
            // Mark it as nullptr to prevent it from being found during `pop`.
            item.target = nullptr;
            if ( !istty || !hide_done ) {
              item.stage = istty ? Locus::Echo : Locus::Offstage;
              return true;
            }
            item.stage = Locus::Offstage;
            pipeline << console::linestart << console::linewipe;
          } break;

          default: break;
          }
          return false;
        }
        /// @return The number of LF outputs in this rendering.
        std::size_t do_schedule( io::CharPipeline& pipeline, bool istty, bool style_off, bool hide_done ) &
        {
          std::size_t active_rows = 0;
          if ( istty && hide_done )
            active_rows = std::count_if( items_.cbegin(), items_.cend(), []( const Slot& slot ) noexcept {
              return slot.stage == Locus::Onstage;
            } );

          std::size_t num_newlines = 0;
          if ( Zone != Region::Bottom || !istty )
            for ( std::size_t i = 0; i < items_.size(); ++i ) {
              if ( do_render( i, pipeline, istty, style_off, hide_done ) ) {
                pipeline << console::nextline;
                ++num_newlines;
              }
            }
          else
            for ( std::size_t i = items_.size(); i-- > 0; ) {
              if ( do_render( i, pipeline, istty, style_off, hide_done ) ) {
                pipeline << console::prevline;
                ++num_newlines;
              }
            }

          if ( istty && hide_done && active_rows > num_newlines ) {
            const auto vanish = active_rows - num_newlines;
            if PACE__CXX17_CNSTXPR ( Zone != Region::Bottom )
              pipeline << io::repeat( vanish, io::concat( console::linewipe, console::nextline ) )
                       << io::cursorup( vanish );
            else
              pipeline << io::repeat( vanish, io::concat( console::linewipe, console::prevline ) )
                       << io::cursordown( vanish );
          }
          return num_newlines;
        }

        PACE__FORCEINLINE void eliminate() noexcept
        {
          // Search for the first k stopped progress bars and remove them.
          auto itr = std::find_if( items_.cbegin(), items_.cend(), []( const Slot& slot ) noexcept {
            return slot.stage == Locus::Onstage && !slot.aborted();
          } );
          if PACE__CXX17_CNSTXPR ( Zone == Region::Fixed || Zone == Region::Relative ) {
            // When `hide_completed` is activated,
            // `do_render` automatically changes all Phase::Echo to Phase::Offstage,
            // at which point these prefixes no longer participate in line breaks;
            // thus, we only need to count the non-Offstage elements to track the number of line breaks
            // from the previous render output.
            const auto occupied_lines = std::count_if( items_.cbegin(), itr, []( const Slot& slot ) noexcept {
              return slot.stage != Locus::Offstage;
            } );
            if PACE__CXX17_CNSTXPR ( Zone == Region::Fixed )
              line_variation_ += occupied_lines;
            else
              line_variation_ -= occupied_lines;
          }
          items_.erase( items_.cbegin(), itr );
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
                   auto& ostream        = io::OStream<Sink>::itself();
                   const auto istty     = console::Screen<Sink>::renderable();
                   const auto style_off = !istty && config::auto_style_off();
                   const auto hide_done = config::hide_completed();
                   std::uint16_t height = 0; // used only in Region::Bottom or Region::Top

                   switch ( state_.load( std::memory_order_relaxed ) ) {
                   case Phase::Awake: {
                     if ( istty ) {
                       if PACE__CXX17_CNSTXPR ( Zone == Region::Fixed || Zone == Region::Relative ) {
                         ostream << console::linestart;
                         if PACE__CXX17_CNSTXPR ( Zone == Region::Fixed )
                           ostream << console::savecursor;
                       } else if PACE__CXX17_CNSTXPR ( Zone == Region::Bottom || Zone == Region::Top ) {
                         auto& screen = console::Screen<Sink>::itself();
                         height       = screen.height();
                         (void)screen.vertical_alteration( height );
                       }
                     }
                     {
                       std::lock_guard<concurrent::SharedMutex> lock { res_mtx_ };
                       if PACE__CXX17_CNSTXPR ( Zone == Region::Bottom || Zone == Region::Top ) {
                         if ( istty ) {
                           ostream << console::savecursor;
                           if ( height > items_.size() ) {
                             if PACE__CXX17_CNSTXPR ( Zone == Region::Bottom )
                               ostream << io::nextline_n( items_.size() )
                                       << io::setscroll( 1, height - items_.size() ) << console::restorecursor
                                       << io::cursordown( items_.size() ) << io::cursorup( items_.size() )
                                       << console::savecursor;
                             else
                               ostream << io::setscroll( items_.size() + 1, height ) << console::restorecursor
                                       << io::cursorup( items_.size() ) << io::cursordown( items_.size() )
                                       << console::savecursor;
                           }
                           if PACE__CXX17_CNSTXPR ( Zone == Region::Bottom )
                             ostream << io::setline( height );
                           else
                             ostream << console::firstline;
                         } else
                           console::Screen<Sink>::itself().vertical_alteration( 0 );
                       }
                       const auto num_newlines = do_schedule( ostream, istty, style_off, hide_done );
                       if PACE__CXX17_CNSTXPR ( Zone != Region::Fixed )
                         line_variation_ = num_newlines;
                       else
                         line_variation_ = 0;
                     }
                     if PACE__CXX17_CNSTXPR ( Zone == Region::Bottom || Zone == Region::Top )
                       if ( istty )
                         ostream << console::restorecursor;
                     ostream << io::flush;
                     auto expected = Phase::Awake;
                     state_.compare_exchange_strong( expected, Phase::Refresh, std::memory_order_relaxed );
                   } break;

                   case Phase::Refresh: {
                     std::uint16_t old_height = 0; // used only in Region::Bottom or Region::Top
                     {
                       std::lock_guard<concurrent::SharedMutex> lock { res_mtx_ };
                       if ( istty ) {
                         if PACE__CXX17_CNSTXPR ( Zone == Region::Fixed )
                           ostream << console::restorecursor
                                   << io::when( line_variation_ > 0,
                                                io::concat( console::restorecursor,
                                                            io::cursordown( line_variation_ ),
                                                            console::savecursor ) );
                         else if PACE__CXX17_CNSTXPR ( Zone == Region::Relative )
                           ostream << io::prevline_n( line_variation_ );
                         else if PACE__CXX17_CNSTXPR ( Zone == Region::Bottom || Zone == Region::Top ) {
                           auto& screen = console::Screen<Sink>::itself();
                           height       = screen.height();
                           old_height   = screen.vertical_alteration( height );
                           ostream << console::savecursor;
                           if PACE__CXX17_CNSTXPR ( Zone == Region::Bottom )
                             ostream << io::setline( height );
                           else
                             ostream << console::firstline;
                         }
                       } else if PACE__CXX17_CNSTXPR ( Zone == Region::Bottom || Zone == Region::Top )
                         (void)console::Screen<Sink>::itself().vertical_alteration( 0 );

                       const auto num_newlines = do_schedule( ostream, istty, style_off, hide_done );

                       if PACE__CXX17_CNSTXPR ( Zone == Region::Fixed )
                         line_variation_ = 0;
                       if PACE__CXX17_CNSTXPR ( Zone == Region::Relative )
                         line_variation_ = num_newlines;
                       else if PACE__CXX17_CNSTXPR ( Zone == Region::Bottom || Zone == Region::Top ) {
                         if ( istty ) {
                           if ( height > num_newlines
                                && ( line_variation_ != num_newlines || height != old_height ) ) {
                             if PACE__CXX17_CNSTXPR ( Zone == Region::Bottom )
                               ostream << io::setscroll( 1, height - num_newlines ) << console::restorecursor
                                       << io::when( line_variation_ < num_newlines || height < old_height,
                                                    io::concat( io::cursordown( num_newlines ),
                                                                io::cursorup( num_newlines ) ) );
                             else
                               ostream << io::setscroll( num_newlines + 1, height ) << console::restorecursor
                                       << io::when( line_variation_ < num_newlines || height < old_height,
                                                    io::concat( io::cursorup( num_newlines ),
                                                                io::cursordown( num_newlines ) ) );
                           } else
                             ostream << console::restorecursor;
                         }
                         if ( hide_done )
                           line_variation_ = num_newlines;
                         else
                           line_variation_ = (std::max)( line_variation_, num_newlines );
                       }
                     }
                     ostream << io::flush;
                   } break;

                   case Phase::Finish: {
                     if ( istty ) {
                       if PACE__CXX17_CNSTXPR ( Zone == Region::Fixed )
                         ostream << console::restorecursor;
                       else if PACE__CXX17_CNSTXPR ( Zone == Region::Relative )
                         ostream << io::prevline_n( line_variation_ );
                       else if PACE__CXX17_CNSTXPR ( Zone == Region::Bottom )
                         ostream << console::savecursor << io::setline( console::Screen<Sink>::height() );
                       else if PACE__CXX17_CNSTXPR ( Zone == Region::Top )
                         ostream << console::savecursor << console::firstline;
                     }
                     {
                       std::lock_guard<concurrent::SharedMutex> lock { res_mtx_ };
                       do_schedule( ostream, istty, style_off, hide_done );
                     }
                     if PACE__CXX17_CNSTXPR ( Zone == Region::Bottom || Zone == Region::Top )
                       if ( istty )
                         ostream << console::resetscroll << console::restorecursor;
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

            io::OStream<Sink>::itself() << io::reset;
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
            if PACE__CXX17_CNSTXPR ( Mode == Policy::Sync ) {
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
            } else {
              lock1.unlock();
              executor.template trigger<Mode>(); // noexcept
            }
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
              executor.dismiss_then( []() noexcept { io::OStream<Sink>::itself().reset(); } );
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
          // (because the `reset`/`abort` call of A has not yet returned, the current task cycle has not yet
          // ended), which is not guaranteed to be thread-safe, so it does not need to be handled.
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
