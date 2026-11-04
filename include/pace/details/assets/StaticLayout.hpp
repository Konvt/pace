#ifndef PACE_STATIC_LAYOUT
#define PACE_STATIC_LAYOUT

#include "../assets/PackagedBar.hpp"
#include "../io/OStream.hpp"
#include "../utils/Backport.hpp"
#include "../utils/ScopeGuard.hpp"
#include "../utils/Util.hpp"
#include <initializer_list>
#include <mutex>
#include <tuple>

namespace pace {
  namespace details {
    namespace assets {
      template<Channel, Policy, Region, typename, typename...>
      class StaticLayout;
      template<Channel Sink, Policy Mode, Region Zone, std::size_t... Tags, typename... Configs>
      class StaticLayout<Sink, Mode, Zone, traits::IndexSequence<Tags...>, Configs...> final
        : public assets::PackagedBar<Configs, Sink, Mode, Zone, Tags>... {
        static_assert( sizeof...( Tags ) == sizeof...( Configs ), "unexpected type mismatch" );
        static_assert( sizeof...( Configs ) > 0, "the number of progress bars cannot be zero" );

        template<typename Config>
        struct FetchElement {
        private:
          static_assert( traits::is_config<Config>::value, "cannot fetch an invalid type" );

          template<typename Result, typename... Rests>
          struct Helper {
            static_assert( !std::is_void<Result>::value, "cannot fetch a non-existent type" );
            using type = Result;
          };
          template<std::size_t Tag1, std::size_t Tag2, typename... Rests>
          struct Helper<assets::PackagedBar<Config, Sink, Mode, Zone, Tag1>,
                        assets::PackagedBar<Config, Sink, Mode, Zone, Tag2>,
                        Rests...> {
            static_assert( sizeof...( Rests ) != sizeof...( Rests ),
                           "the target type must appear exactly once" );
          };
          template<std::size_t Tag, typename... Rests>
          struct Helper<void, assets::PackagedBar<Config, Sink, Mode, Zone, Tag>, Rests...>
            : Helper<assets::PackagedBar<Config, Sink, Mode, Zone, Tag>, Rests...> {};
          template<typename First, typename... Rests>
          struct Helper<void, First, Rests...> : Helper<void, Rests...> {};

        public:
          using type = typename Helper<void, assets::PackagedBar<Configs, Sink, Mode, Zone, Tags>...>::type;
        };
        template<typename C, Channel S, Policy M, Region Z>
        struct FetchElement<prefab::BasicBar<C, S, M, Z>> : FetchElement<C> {
          static_assert( S == Sink && M == Mode && Z == Zone, "try to fetch a non-existent type" );
        };

        template<typename T>
        using FetchElement_t = typename FetchElement<T>::type;
        template<std::size_t Pos>
        using ElementAt_t = traits::TypeAt_t<Pos, assets::PackagedBar<Configs, Sink, Mode, Zone, Tags>...>;

        std::size_t alive_cnt_ = 0;
        mutable concurrent::SharedMutex res_mtx_;
        mutable std::mutex sched_mtx_;

        enum class Phase : std::uint8_t { Stop, Awake, Refresh, Finish };
        std::atomic<Phase> state_ { Phase::Stop };

        enum class Locus : std::uint8_t { Offstage, Onstage, Echo };
        std::array<Locus, sizeof...( Configs )> stages_;

        /// @return Indicates whether the current rendering needs to occupy a separate line.
        template<std::size_t Pos>
        PACE__NODISCARD bool do_render( io::CharPipeline& pipeline,
                                        bool istty,
                                        bool style_off,
                                        bool hide_done ) &
        {
          PACE__ASSERT( online() );
          switch ( stages_[Pos] ) {
          case Locus::Echo: {
            if ( !istty || hide_done )
              stages_[Pos] = Locus::Offstage;
            if ( !this->ElementAt_t<Pos>::active() ) {
              if ( istty )
                return true;
              break;
            }
          }
            goto Resurrection; // NOLINT(cppcoreguidelines-avoid-goto): avoid repeating the active() check
          case Locus::Offstage: {
            if ( !this->ElementAt_t<Pos>::active() )
              break;
            // The newly added progress bar needs to obtain a screen line,
            // and the progress bar under the Locus::Echo no longer outputs anything except for newline.
            // We must reclaim it, even if this might go against the original intention of the Locus::Echo.
            if ( istty && !hide_done ) {
              if PACE__CXX17_CNSTXPR ( Zone == Region::Bottom ) {
                const auto ritr = std::find( stages_.rbegin() + Pos + 1, stages_.rend(), Locus::Echo );
                if ( ritr != stages_.rend() )
                  *ritr = Locus::Offstage;
              } else {
                const auto itr = std::find( stages_.begin() + Pos + 1, stages_.end(), Locus::Echo );
                if ( itr != stages_.end() )
                  *itr = Locus::Offstage;
              }
            }
Resurrection:
            stages_[Pos] = Locus::Onstage;
          }
            PACE__FALLTHROUGH;
          case Locus::Onstage: {
            const auto painted = this->ElementAt_t<Pos>::draw( pipeline, style_off );

            if ( istty && painted )
              pipeline << console::linewipe;
            if ( this->ElementAt_t<Pos>::active() )
              return true;
            /**
             * Here are the scenarios where a newline character is output:
             * 1. If the output stream is bound to a terminal and the completed progress bar does not need
             *    to be hidden, it should be output when stages_[Pos] is equal to Phase::Offstage.
             * 2. If the output stream is bound to a terminal and the completed progress bar needs to be
             *    hidden, it only be output when Pos-th is still active.
             * 3. If the output stream is not bound to a terminal,
             *    it should be output whenever Pos-th has just been rendered.
             */
            if ( !istty || !hide_done ) {
              stages_[Pos] = istty ? Locus::Echo : Locus::Offstage;
              return true;
            }
            stages_[Pos] = Locus::Offstage;
            pipeline << console::linestart << console::linewipe;
          } break;

          default: utils::unreachable();
          }

          return false;
        }
        PACE__FORCEINLINE std::size_t do_schedule( io::CharPipeline& pipeline, bool istty, bool hide_done ) &
        {
          const auto style_off    = !istty && config::auto_style_off();
          std::size_t active_rows = 0;
          if ( istty && hide_done )
            // read only, lock free
            active_rows = std::count( stages_.cbegin(), stages_.cend(), Locus::Onstage );

          std::size_t num_newlines = 0;
          if ( Zone != Region::Bottom || !istty ) {
            std::lock_guard<concurrent::SharedMutex> lock { res_mtx_ };
            (void)std::initializer_list<bool> { ( do_render<Tags>( pipeline, istty, style_off, hide_done )
                                                    ? ( ++num_newlines, pipeline << console::nextline, false )
                                                    : false )... };
          } else {
            std::lock_guard<concurrent::SharedMutex> lock { res_mtx_ };
            // In Bottom mode, we have a fixed lower boundary;
            // rendering from top to bottom would make it difficult to predict
            // the actual number of rows required, so we need to render in reverse.
            (void)std::initializer_list<bool> { (
              do_render<sizeof...( Configs ) - Tags - 1>( pipeline, istty, style_off, hide_done )
                ? ( ++num_newlines, pipeline << console::prevline, false )
                : false )... };
          }

          if ( istty && hide_done && active_rows > num_newlines ) {
            // If multiple bars complete simultaneously in a single rendering cycle,
            // and the number of newly added or updated bars is insufficient to exceed
            // the number of disappearing bars,
            // some terminal lines may fail to be properly erased due to rendering logic issues;
            // here we supplement the difference and erase them accordingly.
            const auto vanish = active_rows - num_newlines;
            if PACE__CXX17_CNSTXPR ( Zone != Region::Bottom )
              pipeline << io::repeat( vanish, io::concat( console::linewipe, console::nextline ) )
                       << io::cursorup( vanish - 1 );
            else
              pipeline << io::repeat( vanish, io::concat( console::linewipe, console::prevline ) )
                       << io::cursordown( vanish - 1 );
          }
          return num_newlines;
        }

        void do_halt( bool forced ) final
        { // This virtual function is invoked only via the vtable,
          // hence the default arguments from the base class declaration are always used.
          // Any default arguments provided in the derived class are ignored.
          PACE__ASSERT( online() );
          auto& executor = render::Renderer<Sink>::itself();
          PACE__ASSERT( executor.empty() == false );

          std::lock_guard<std::mutex> lock { sched_mtx_ };
          PACE__ASSERT( alive_cnt_ > 0 );
          if ( --alive_cnt_ == 0 ) {
            if ( forced )
              state_.store( Phase::Stop, std::memory_order_release );
            else {
              state_.store( Phase::Finish, std::memory_order_release );
              executor.execute();
            }
            executor.dismiss_then( []() noexcept { io::OStream<Sink>::itself().reset(); } );
          } else if ( !forced )
            executor.execute();
        }
        void do_boot() & final
        {
          auto& executor = render::Renderer<Sink>::itself();
          std::lock_guard<std::mutex> lock { sched_mtx_ };
          if ( state_.load( std::memory_order_relaxed ) == Phase::Stop ) {
            if ( !executor.try_appoint( [this]() {
                   auto guard = utils::make_scope_fail(
                     [this]() noexcept { state_.store( Phase::Stop, std::memory_order_release ); } );
                   auto& ostream        = io::OStream<Sink>::itself();
                   const auto istty     = console::Screen<Sink>::renderable();
                   const auto hide_done = config::hide_completed();
                   switch ( state_.load( std::memory_order_acquire ) ) {
                   case Phase::Awake: {
                     if ( istty ) {
                       if PACE__CXX17_CNSTXPR ( Zone == Region::Fixed || Zone == Region::Relative ) {
                         ostream << console::linestart;
                         if PACE__CXX17_CNSTXPR ( Zone == Region::Fixed )
                           ostream << console::savecursor;
                       } else if PACE__CXX17_CNSTXPR ( Zone == Region::Bottom || Zone == Region::Top ) {
                         ostream << console::savecursor;
                         auto& screen      = console::Screen<Sink>::itself();
                         const auto height = screen.height();
                         (void)screen.vertical_alteration( height );
                         if ( height > sizeof...( Configs ) ) {
                           if PACE__CXX17_CNSTXPR ( Zone == Region::Bottom )
                             ostream << io::nextline_n( sizeof...( Configs ) )
                                     << io::setscroll( 1, height - sizeof...( Configs ) )
                                     << console::restorecursor << io::cursordown( sizeof...( Configs ) )
                                     << io::cursorup( sizeof...( Configs ) ) << console::savecursor;
                           else
                             ostream << io::setscroll( sizeof...( Configs ) + 1, height )
                                     << console::restorecursor << io::cursorup( sizeof...( Configs ) )
                                     << io::cursordown( sizeof...( Configs ) ) << console::savecursor;
                         }
                         if PACE__CXX17_CNSTXPR ( Zone == Region::Bottom )
                           ostream << io::setline( height );
                         else
                           ostream << console::firstline;
                       }
                     } else
                       console::Screen<Sink>::itself().vertical_alteration( 0 );

                     // Since the Renderer ensures that only one thread is performing rendering at any time,
                     // all access to stages_ always occurs within a same thread,
                     // so we do not need to use a mutex to protect it.
                     std::fill( stages_.begin(), stages_.end(), Locus::Offstage );
                     do_schedule( ostream, istty, hide_done );

                     if PACE__CXX17_CNSTXPR ( Zone == Region::Bottom || Zone == Region::Top )
                       if ( istty )
                         ostream << console::restorecursor;

                     auto expected = Phase::Awake;
                     state_.compare_exchange_strong( expected, Phase::Refresh, std::memory_order_release );
                     ostream << io::flush;
                   } break;

                   case Phase::Refresh: {
                     if ( istty ) {
                       if PACE__CXX17_CNSTXPR ( Zone == Region::Fixed )
                         ostream << console::restorecursor;
                       else if PACE__CXX17_CNSTXPR ( Zone == Region::Relative )
                         ostream << io::prevline_n(
                           stages_.size() - std::count( stages_.cbegin(), stages_.cend(), Locus::Offstage ) );
                       else if PACE__CXX17_CNSTXPR ( Zone == Region::Bottom || Zone == Region::Top ) {
                         ostream << console::savecursor;
                         auto& screen          = console::Screen<Sink>::itself();
                         const auto height     = screen.height();
                         const auto old_height = screen.vertical_alteration( height );
                         if ( height != old_height && height > sizeof...( Configs ) ) {
                           if PACE__CXX17_CNSTXPR ( Zone == Region::Bottom )
                             ostream << io::setscroll( 1, height - sizeof...( Configs ) )
                                     << io::when( height < old_height,
                                                  io::concat( console::restorecursor,
                                                              io::cursordown( sizeof...( Configs ) ),
                                                              io::cursorup( sizeof...( Configs ) ),
                                                              console::savecursor ) );
                           else
                             ostream << io::setscroll( sizeof...( Configs ) + 1, height )
                                     << io::when( height < old_height,
                                                  io::concat( console::restorecursor,
                                                              io::cursorup( sizeof...( Configs ) ),
                                                              io::cursordown( sizeof...( Configs ) ),
                                                              console::savecursor ) );
                         }
                         if PACE__CXX17_CNSTXPR ( Zone == Region::Bottom )
                           ostream << io::setline( height );
                         else
                           ostream << console::firstline;
                       }
                     } else if PACE__CXX17_CNSTXPR ( Zone == Region::Bottom || Zone == Region::Top )
                       (void)console::Screen<Sink>::itself().vertical_alteration( 0 );

                     do_schedule( ostream, istty, hide_done );

                     if PACE__CXX17_CNSTXPR ( Zone == Region::Bottom || Zone == Region::Top )
                       if ( istty )
                         ostream << console::restorecursor;
                     ostream << io::flush;
                   } break;

                   case Phase::Finish: {
                     if ( istty ) {
                       if PACE__CXX17_CNSTXPR ( Zone == Region::Fixed )
                         ostream << console::restorecursor;
                       else if PACE__CXX17_CNSTXPR ( Zone == Region::Relative )
                         ostream << io::prevline_n(
                           stages_.size() - std::count( stages_.cbegin(), stages_.cend(), Locus::Offstage ) );
                       else if PACE__CXX17_CNSTXPR ( Zone == Region::Bottom )
                         ostream << console::savecursor << io::setline( console::Screen<Sink>::height() );
                       else if PACE__CXX17_CNSTXPR ( Zone == Region::Top )
                         ostream << console::savecursor << console::firstline;
                     }

                     const auto num_newlines = do_schedule( ostream, istty, hide_done );

                     if ( istty ) {
                       if PACE__CXX17_CNSTXPR ( Zone == Region::Bottom || Zone == Region::Top )
                         ostream << console::resetscroll << console::restorecursor;
                       else if ( hide_done && num_newlines > 0 )
                         // Since rendering always adds an extra line break, this cancels it out.
                         ostream << console::prevline;
                     }
                     ostream << io::flush;
                     state_.store( Phase::Stop, std::memory_order_release );
                   } break;

                   default: break;
                   }
                 } ) )
              PACE__UNLIKELY throw exception::StateError(
                charcodes::make_literal( "pace: another progress bar instance is already running" ) );

            io::OStream<Sink>::itself() << io::reset;
            state_.store( Phase::Awake, std::memory_order_relaxed );

            auto guard = utils::make_scope_fail( [&]() noexcept {
              state_.store( Phase::Stop, std::memory_order_release );
              executor.dismiss();
            } );
            executor.template activate<Mode>();
          } else {
            PACE__ASSERT( state_ != Phase::Finish );
            executor.template trigger<Mode>();
          }
          PACE__ASSERT( alive_cnt_ <= sizeof...( Configs ) );
          ++alive_cnt_;
        }

        template<typename Tuple, std::size_t... Is>
        StaticLayout( Tuple&& tup, traits::IndexSequence<Is...> )
          noexcept( std::tuple_size<typename std::decay<Tuple>::type>::value == sizeof...( Configs ) )
          : ElementAt_t<Is>( utils::pick_or<Is, ElementAt_t<Is>>( std::forward<Tuple>( tup ) ) )...
        {
          static_assert( std::tuple_size<typename std::decay<Tuple>::type>::value <= sizeof...( Is ),
                         "unexpected tuple size mismatch" );
        }

      public:
        template<std::size_t... Is, typename... Cs, Channel S, Policy M, Region Z>
        StaticLayout( const assets::PackagedBar<Cs, S, M, Z, Is>&... ) = delete;

        // SFINAE is used here to prevent infinite recursive matching of errors.
        template<typename Cfg,
                 typename... Cfgs,
                 typename = typename std::enable_if<traits::TpStartsWith<
                   traits::TypeList<typename std::decay<Cfg>::type, typename std::decay<Cfgs>::type...>,
                   Configs...>::value>::type>
        StaticLayout( Cfg&& cfg, Cfgs&&... cfgs ) noexcept( sizeof...( Cfgs ) + 1 == sizeof...( Configs ) )
          : StaticLayout( std::forward_as_tuple( std::forward<Cfg>( cfg ), std::forward<Cfgs>( cfgs )... ),
                          traits::MakeIndexSequence<sizeof...( Cfgs ) + 1>() )
        {}
        template<typename... Cfgs,
                 typename = typename std::enable_if<
                   traits::TpStartsWith<traits::TypeList<Cfgs...>, Configs...>::value>::type>
        StaticLayout( prefab::BasicBar<Cfgs, Sink, Mode, Zone>&&... bars )
          noexcept( sizeof...( Cfgs ) == sizeof...( Configs ) )
          : StaticLayout( std::forward_as_tuple( std::move( bars )... ),
                          traits::MakeIndexSequence<sizeof...( Cfgs )>() )
        {}
        StaticLayout( const StaticLayout& )            = delete;
        StaticLayout& operator=( const StaticLayout& ) = delete;
        StaticLayout( StaticLayout&& rhs ) noexcept
          : assets::PackagedBar<Configs, Sink, Mode, Zone, Tags>( std::move( rhs ) )...
        { PACE__ASSERT( rhs.online() == false ); }
        StaticLayout& operator=( StaticLayout&& rhs ) & noexcept
        { // The thread insecurity here is deliberately designed.
          // After all, for a type where a base class reference can be exposed,
          // we cannot apply the lock within the type to the outside.
          PACE__TRUST( this != &rhs );
          PACE__ASSERT( online() == false );
          PACE__ASSERT( rhs.online() == false );
          (void)std::initializer_list<bool> {
            ( assets::PackagedBar<Configs, Sink, Mode, Zone, Tags>::operator=( std::move( rhs ) ), false )...
          };
          return *this;
        }
        ~StaticLayout() noexcept
        { kill(); }

        void shut()
        {
          // We cannot lock `sched_mtx_` here, because it will reverse the locking order
          // between `sched_mtx_` and the mutex held by the bar object itself.
          (void)std::initializer_list<bool> { ( this->ElementAt_t<Tags>::reset(), false )... };
          PACE__ASSERT( online() == false );
        }
        void kill() noexcept
        {
          (void)std::initializer_list<bool> { ( this->ElementAt_t<Tags>::abort(), false )... };
          PACE__ASSERT( online() == false );
        }

        PACE__NODISCARD PACE__FORCEINLINE bool online() const noexcept
        { return state_.load( std::memory_order_relaxed ) != Phase::Stop; }
        PACE__NODISCARD PACE__FORCEINLINE std::size_t height() const noexcept
        {
          if ( state_.load( std::memory_order_relaxed ) == Phase::Stop )
            return 0;
          concurrent::SharedLock<concurrent::SharedMutex> lock { res_mtx_ };
          std::size_t count = 0;
          (void)std::initializer_list<bool> {
            ( count += ( stages_[Tags] != Locus::Offstage || this->ElementAt_t<Tags>::active() ), false )...
          }; // Avoid returning 1 during the critical moment right after activating.
          return count + ( count > 0 );
        }

        void swap( StaticLayout& other ) noexcept
        { // The thread insecurity here is deliberately designed.
          // The reason can be found in the move assignment.
          PACE__TRUST( this != &other );
          PACE__ASSERT( online() == false );
          PACE__ASSERT( other.online() == false );
          (void)std::initializer_list<bool> {
            ( this->ElementAt_t<Tags>::swap( static_cast<ElementAt_t<Tags>&>( other ) ), false )...
          };
        }

        template<std::size_t Pos>
        PACE__FORCEINLINE PACE__CXX14_CNSTXPR ElementAt_t<Pos>& at() & noexcept
        { return static_cast<ElementAt_t<Pos>&>( *this ); }
        template<std::size_t Pos>
        PACE__FORCEINLINE constexpr const ElementAt_t<Pos>& at() const& noexcept
        { return static_cast<const ElementAt_t<Pos>&>( *this ); }
        template<std::size_t Pos>
        PACE__FORCEINLINE PACE__CXX14_CNSTXPR ElementAt_t<Pos>& at() && noexcept
        { return std::move( at<Pos>() ); }

        template<typename T>
        PACE__FORCEINLINE PACE__CXX14_CNSTXPR FetchElement_t<T>& get() & noexcept
        { return static_cast<FetchElement_t<T>&>( *this ); }
        template<typename T>
        PACE__FORCEINLINE constexpr const FetchElement_t<T>& get() const& noexcept
        { return static_cast<const FetchElement_t<T>&>( *this ); }
        template<typename T>
        PACE__FORCEINLINE PACE__CXX14_CNSTXPR FetchElement_t<T>&& get() && noexcept
        { return static_cast<FetchElement_t<T>&&>( *this ); }
      };
    } // namespace assets
  } // namespace details
} // namespace pace

#endif
