#ifndef PACE_DYNAMIC_LAYOUT
#define PACE_DYNAMIC_LAYOUT

#include "../io/OStream.hpp"
#include "ManagedBar.hpp"
#include <deque>

namespace pace {
  namespace details {
    namespace assets {
      template<Channel Outlet, Policy Mode, Region Area>
      class DynamicLayout final {
        enum Locus : std::uint8_t { Offstage, Onstage, Echo };
        struct Slot final {
        private:
          template<typename Derived>
          static void render( Indicator* item )
          {
            static_assert( traits::AllOf<std::is_base_of<Indicator, Derived>, traits::is_bar<Derived>>::value,
                           "Derived must inherit from Indicator" );
            PACE__TRUST( item != nullptr );
            draw_content( static_cast<Derived&>( *item ) );
          }

        public:
          void ( *render_ )( Indicator* );
          Indicator* target_;
          // Only the active bar will be constructed as a slot, hence, the default value is Onstage.
          Locus stage_ { Locus::Onstage };

          template<typename Config>
          Slot( assets::ManagedBar<Config, Outlet, Mode, Area>* item ) noexcept
            : render_ { render<prefab::BasicBar<Config, Outlet, Mode, Area>> }, target_ { item }
          {}
        };

        std::deque<Slot> items_;
        /**
         * If Area is equal to Region::Fixed,
           the variable represents the number of lines that need to be discarded;
         * If Area is equal to Region::Relative,
           the variable represents the number of nextlines output last time.
         */
        std::uint64_t num_modified_lines_ = 0;
        mutable concurrent::SharedMutex res_mtx_;
        mutable std::mutex sched_mtx_;

        enum class Phase : std::uint8_t { Stop, Awake, Refresh };
        std::atomic<Phase> state_ = { Phase::Stop };

        void do_render() &
        {
          auto& ostream        = io::OStream<Outlet>::itself();
          const auto istty     = console::TermContext<Outlet>::itself().connected();
          const auto hide_done = config::hide_completed();

          for ( types::Size i = 0; i < items_.size(); ++i ) {
            switch ( items_[i].stage_ ) {
            case Locus::Echo: {
              if ( istty && !hide_done ) {
                ostream << console::nextline;
                if PACE__CXX17_CNSTXPR ( Area == Region::Relative )
                  ++num_modified_lines_;
              } else
                items_[i].stage_ = Locus::Offstage;
            } break;

            case Locus::Onstage: {
              if ( items_[i].target_ == nullptr ) {
                // being aborted
                if ( istty && !hide_done ) {
                  ostream << console::nextline;
                  if PACE__CXX17_CNSTXPR ( Area == Region::Relative )
                    ++num_modified_lines_;
                  items_[i].stage_ = Locus::Echo;
                } else
                  items_[i].stage_ = Locus::Offstage;
                break;
              } else if ( istty )
                ostream << console::linewipe;

              ( *items_[i].render_ )( items_[i].target_ );

              /**
               * When Area is equal to Region::Fixed, the row discard policy is as follows:
               * After eliminating the first k consecutive stopped items in the render queue item_
               * (via the eliminate method), the starting area for rendering is moved down by k rows.
               * Therefore, at this point,
               * all remaining items that have not been removed should trigger a line break for rendering.

               * When Area is equal to Region::Relative, the row discard policy is as follows:
               * During the rendering process,
               * count the number of consecutive line breaks output starting from the first rendered item,
               * denoted as n.
               * In the next round of rendering, move up by n rows.
               * Therefore, at this point,
               * it is necessary to track which items in the render queue have been rendered
               * and whether any items have been rendered in the current round of rendering.

               * If the output stream is not bound to a terminal, there is no line discard policy;
               * all rendered items will trigger a newline character to be rendered.
               */
              if ( !items_[i].target_->active() ) {
                // Mark it as nullptr to prevent it from being found during append (re-insert).
                std::tie( items_[i].target_, items_[i].stage_ ) = std::make_pair( nullptr, Locus::Offstage );
                if ( istty && hide_done )
                  ostream << console::linestart;
                else {
                  ostream << console::nextline;
                  if PACE__CXX17_CNSTXPR ( Area == Region::Relative )
                    ++num_modified_lines_;
                  if ( istty && !hide_done )
                    items_[i].stage_ = Locus::Echo;
                }
              } else {
                ostream << console::nextline;
                if PACE__CXX17_CNSTXPR ( Area == Region::Relative )
                  ++num_modified_lines_;
              }
              if ( istty && hide_done )
                ostream << console::linewipe;
            } break;

            default: break;
            }
          }
        }

        void eliminate() noexcept
        {
          // Search for the first k stopped progress bars and remove them.
          auto itr = std::find_if( items_.cbegin(), items_.cend(), []( const Slot& slot ) noexcept {
            return slot.stage_ == Locus::Onstage && slot.target_ != nullptr;
          } );
          if PACE__CXX17_CNSTXPR ( Area == Region::Fixed )
            num_modified_lines_ += utils::distance( items_.cbegin(), itr );
          else if PACE__CXX17_CNSTXPR ( Area == Region::Relative ) {
            const auto num_discarded = static_cast<std::uint64_t>(
              std::count_if( items_.cbegin(), itr, []( const Slot& slot ) noexcept {
                return slot.stage_ != Locus::Offstage;
              } ) );
            PACE__TRUST( num_modified_lines_ >= num_discarded );
            num_modified_lines_ -= num_discarded;
          }
          items_.erase( items_.cbegin(), itr );
        }

        template<bool Forced>
        void do_shut() noexcept( Forced )
        {
          std::lock_guard<std::mutex> lock1 { sched_mtx_ };
          std::lock_guard<concurrent::SharedMutex> lock2 { res_mtx_ };
          if ( state_.load( std::memory_order_acquire ) != Phase::Stop ) {
            for ( types::Size i = 0; i < items_.size(); ++i ) {
              if ( items_[i].stage_ == Locus::Onstage ) {
                if PACE__CXX17_CNSTXPR ( Forced )
                  items_[i].target_->abort();
                else
                  items_[i].target_->reset();
              }
            }
            render::Renderer<Outlet>::itself().dismiss();
          }
          state_.store( Phase::Stop, std::memory_order_release );
          items_.clear();
        }

      public:
        DynamicLayout()                                  = default;
        DynamicLayout( const DynamicLayout& )            = delete;
        DynamicLayout& operator=( const DynamicLayout& ) = delete;
        ~DynamicLayout() noexcept { kill(); }

        void shut() { do_shut<false>(); }
        void kill() noexcept { do_shut<true>(); }

        template<typename C>
        void append( assets::ManagedBar<C, Outlet, Mode, Area>* item ) & noexcept( false )
        {
          std::lock_guard<std::mutex> lock1 { sched_mtx_ };
          auto& executor = render::Renderer<Outlet>::itself();
          concurrent::SharedLock<concurrent::SharedMutex> lock2 { res_mtx_ };
          if ( items_.empty() ) {
            lock2.unlock();
            if ( !executor.try_appoint( [this]() {
                   auto& ostream        = io::OStream<Outlet>::itself();
                   const auto istty     = console::TermContext<Outlet>::itself().connected();
                   const auto hide_done = config::hide_completed();
                   switch ( state_.load( std::memory_order_acquire ) ) {
                   case Phase::Awake: {
                     if PACE__CXX17_CNSTXPR ( Area == Region::Fixed )
                       if ( istty )
                         ostream << console::savecursor;
                     {
                       concurrent::SharedLock<concurrent::SharedMutex> lock { res_mtx_ };
                       do_render();
                     }
                     ostream << io::flush;
                     auto expected = Phase::Awake;
                     state_.compare_exchange_strong( expected, Phase::Refresh, std::memory_order_release );
                   } break;
                   case Phase::Refresh: {
                     {
                       concurrent::SharedLock<concurrent::SharedMutex> lock { res_mtx_ };
                       if ( istty ) {
                         if PACE__CXX17_CNSTXPR ( Area == Region::Fixed ) {
                           ostream << console::resetcursor;
                           if ( !hide_done && num_modified_lines_ > 0 ) {
                             ostream.append( console::nextline, num_modified_lines_ )
                               .append( console::savecursor );
                             num_modified_lines_ = 0;
                           }
                         } else {
                           ostream.append( console::prevline, num_modified_lines_ )
                             .append( console::linestart );
                           num_modified_lines_ = 0;
                         }
                       }
                       do_render();
                     }
                     ostream << io::flush;
                   } break;
                   default: return;
                   }
                 } ) )
              PACE__UNLIKELY throw exception::InvalidState(
                charcodes::make_literal( "pace: another progress bar instance is already running" ) );

            io::OStream<Outlet>::itself() << io::release;
            num_modified_lines_ = 0;
            state_.store( Phase::Awake, std::memory_order_release );

            auto guard = utils::make_scope_fail( [this]() noexcept {
              std::lock_guard<concurrent::SharedMutex> lock2 { res_mtx_ };
              items_.clear();
              state_.store( Phase::Stop, std::memory_order_release );
            } );
            items_.emplace_back( item );
            executor.template activate<Mode>();
          } else {
            eliminate();
            items_.emplace_back( item );
            lock2.unlock();
            executor.template trigger<Mode>();
          }
        }
        void pop( const Indicator* item, bool forced = false ) noexcept
        {
          auto& executor = render::Renderer<Outlet>::itself();
          PACE__ASSERT( executor.empty() == false );
          std::lock_guard<std::mutex> lock1 { sched_mtx_ };
          PACE__ASSERT( online_count() != 0 );
          if ( !forced )
            executor.template trigger<Mode>();

          bool suspend_flag = false;
          {
            std::lock_guard<concurrent::SharedMutex> lock2 { res_mtx_ };
            const auto itr = std::find_if( items_.begin(), items_.end(), [item]( const Slot& slot ) noexcept {
              return item == slot.target_;
            } );
            // Mark target_ as empty,
            // and then search for the first k invalid or destructed progress bars and remove them.
            if ( itr != items_.end() )
              itr->target_ = nullptr;
            eliminate();
            suspend_flag = items_.empty();
          }

          if ( suspend_flag ) {
            state_.store( Phase::Stop, std::memory_order_release );
            executor.dismiss_then( []() noexcept { io::OStream<Outlet>::itself().release(); } );
          }
        }

        PACE__NODISCARD PACE__FORCEINLINE types::Size online_count() const noexcept
        {
          concurrent::SharedLock<concurrent::SharedMutex> lock { res_mtx_ };
          return items_.size();
        }
      };
    } // namespace assets
  } // namespace details
} // namespace pace

#endif
