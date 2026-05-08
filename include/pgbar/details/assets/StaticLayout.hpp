#ifndef PGBAR_STATIC_LAYOUT
#define PGBAR_STATIC_LAYOUT

#include "../../prefab/BasicBar.hpp"
#include "../io/OStream.hpp"
#include "../wrappers/TuplePacket.hpp"
#include <initializer_list>
#include <tuple>

namespace pgbar {
  namespace details {
    namespace assets {
      template<typename Seq, typename... Bars>
      class StaticLayout;
      template<types::Size... Tags, Channel Outlet, Policy Mode, Region Area, typename... Configs>
      class StaticLayout<traits::IndexSeq<Tags...>, prefab::BasicBar<Configs, Outlet, Mode, Area>...> final
        : public wrappers::TuplePacket<prefab::BasicBar<Configs, Outlet, Mode, Area>, Tags>... {
        static_assert( sizeof...( Tags ) == sizeof...( Configs ), "unexpected type mismatch" );
        static_assert( sizeof...( Configs ) > 0, "the number of progress bars cannot be zero" );

        template<types::Size Pos>
        using ElementAt_t =
          traits::TypeAt_t<Pos,
                           wrappers::TuplePacket<prefab::BasicBar<Configs, Outlet, Mode, Area>, Tags>...>;

        std::atomic<types::Size> alive_cnt_;
        mutable std::mutex sched_mtx_;
        // The std::bitset isn't TriviallyCopyable, so we cannot use std::atomic<std::bitset>.
        mutable concurrent::SharedMutex res_mtx_;

        enum class Phase : std::uint8_t { Stop, Awake, Refresh };
        std::atomic<Phase> state_;

        enum class Locus : std::uint8_t { Offstage, Onstage, Echo };
        // Bitmask indicating which bars produced output in the current render pass.
        std::array<Locus, sizeof...( Configs )> stages_;

        template<types::Size Pos>
        PGBAR__FORCEINLINE PGBAR__CXX14_CNSTXPR typename std::enable_if<( Pos >= sizeof...( Configs ) )>::type
          do_render( bool, bool ) &
        {}
        template<types::Size Pos = 0>
        typename std::enable_if<( Pos < sizeof...( Configs ) )>::type do_render( bool istty,
                                                                                 bool hide_done ) &
        {
          PGBAR__ASSERT( online() );
          auto& ostream = io::OStream<Outlet>::itself();
          switch ( stages_[Pos] ) {
          case Locus::Echo: {
            if ( !istty || hide_done )
              stages_[Pos] = Locus::Offstage;
            else if ( !at<Pos>().active() ) {
              ostream << console::escodes::nextline;
              break;
            }
          }
            PGBAR__FALLTHROUGH;
          case Locus::Offstage: {
            if ( !at<Pos>().active() )
              break;
            /**
             * The newly added progress bar needs to obtain a screen line,
             * and the progress bar under the Locus::Echo no longer outputs anything except for newline.
             * We must reclaim it, even if this might go against the original intention of the Locus::Echo.
             */
            if ( istty && !hide_done && stages_[Pos] != Locus::Echo ) {
              auto itr = std::find( stages_.begin(), stages_.end(), Locus::Echo );
              if ( itr != stages_.end() )
                *itr = Locus::Offstage;
            }
            stages_[Pos] = Locus::Onstage;
          }
            PGBAR__FALLTHROUGH;
          case Locus::Onstage: {
            if ( istty )
              ostream << console::escodes::linewipe;

            draw_content( at<Pos>() );

            if ( !at<Pos>().active() ) {
              /**
               * Here are the scenarios where a newline character is output:
               * 1. If the output stream is bound to a terminal and the completed progress bar does not need
               *    to be hidden, it should be output when stages_[Pos] is equal to Step::Spare.
               * 2. If the output stream is bound to a terminal and the completed progress bar needs to be
               *    hidden, it only be output when Pos-th is still active.
               * 3. If the output stream is not bound to a terminal,
               *    it should be output whenever Pos-th has just been rendered.
               */
              if ( istty && hide_done )
                ostream << console::escodes::linestart;
              else
                ostream << console::escodes::nextline;
              if ( istty && !hide_done )
                stages_[Pos] = Locus::Echo;
              else
                stages_[Pos] = Locus::Offstage;
            } else
              ostream << console::escodes::nextline;
            if ( istty && hide_done )
              ostream << console::escodes::linewipe;
          } break;

          default: utils::unreachable();
          }

          return do_render<Pos + 1>( istty, hide_done ); // tail recursive
        }

        void do_halt( bool forced ) noexcept final
        { // This virtual function is invoked only via the vtable,
          // hence the default arguments from the base class declaration are always used.
          // Any default arguments provided in the derived class are ignored.
          if ( online() ) {
            auto& executor = render::Renderer<Outlet>::itself();
            PGBAR__ASSERT( executor.empty() == false );
            std::lock_guard<std::mutex> lock { sched_mtx_ };
            if ( !forced )
              executor.template trigger<Mode>();
            if ( alive_cnt_.fetch_sub( 1, std::memory_order_acq_rel ) == 1 ) {
              state_.store( Phase::Stop, std::memory_order_release );
              executor.dismiss_then( []() noexcept { io::OStream<Outlet>::itself().release(); } );
            }
          }
        }
        void do_boot() & final
        {
          std::lock_guard<std::mutex> lock { sched_mtx_ };
          auto& executor = render::Renderer<Outlet>::itself();
          if ( state_.load( std::memory_order_acquire ) == Phase::Stop ) {
            if ( !executor.try_appoint( [this]() {
                   auto& ostream    = io::OStream<Outlet>::itself();
                   const auto istty = console::TermContext<Outlet>::itself().connected();
                   switch ( state_.load( std::memory_order_acquire ) ) {
                   case Phase::Awake: {
                     if PGBAR__CXX17_CNSTXPR ( Area == Region::Fixed )
                       if ( istty )
                         ostream << console::escodes::savecursor;
                     {
                       std::lock_guard<concurrent::SharedMutex> lock { res_mtx_ };
                       std::fill( stages_.begin(), stages_.end(), Locus::Offstage );
                       do_render( console::TermContext<Outlet>::itself().connected(),
                                  config::hide_completed() );
                     }
                     ostream << io::flush;

                     auto expected = Phase::Awake;
                     state_.compare_exchange_strong( expected, Phase::Refresh, std::memory_order_release );
                   } break;
                   case Phase::Refresh: {
                     {
                       std::lock_guard<concurrent::SharedMutex> lock { res_mtx_ };
                       if ( istty ) {
                         if PGBAR__CXX17_CNSTXPR ( Area == Region::Fixed )
                           ostream << console::escodes::resetcursor;
                         else
                           ostream
                             .append( console::escodes::prevline,
                                      std::count_if(
                                        stages_.cbegin(),
                                        stages_.cend(),
                                        []( Locus stage ) noexcept { return stage != Locus::Offstage; } ) )
                             .append( console::escodes::linestart );
                       }
                       do_render( console::TermContext<Outlet>::itself().connected(),
                                  config::hide_completed() );
                     }
                     ostream << io::flush;
                   } break;
                   default: break;
                   }
                 } ) )
              PGBAR__UNLIKELY throw exception::InvalidState(
                charcodes::make_literal( "pgbar: another progress bar instance is already running" ) );

            io::OStream<Outlet>::itself() << io::release;
            state_.store( Phase::Awake, std::memory_order_release );

            auto guard = utils::make_scope_fail( [&]() noexcept {
              state_.store( Phase::Stop, std::memory_order_release );
              executor.dismiss();
            } );
            executor.template activate<Mode>();
          } else
            executor.template trigger<Mode>();
          alive_cnt_.fetch_add( 1, std::memory_order_release );
          PGBAR__ASSERT( alive_cnt_ <= sizeof...( Configs ) );
        }

        template<typename Tuple, types::Size... Is>
        StaticLayout( Tuple&& tup, const traits::IndexSeq<Is...>& )
          noexcept( std::tuple_size<typename std::decay<Tuple>::type>::value == sizeof...( Configs ) )
          : ElementAt_t<Is>( utils::pick_or<Is, ElementAt_t<Is>>( std::forward<Tuple>( tup ) ) )...
          , alive_cnt_ { 0 }
          , sched_mtx_ {}
          , res_mtx_ {}
          , state_ { Phase::Stop }
        {
          static_assert( std::tuple_size<typename std::decay<Tuple>::type>::value <= sizeof...( Is ),
                         "unexpected tuple size mismatch" );
        }

      public:
        template<types::Size... Is, typename... Cs, Channel O, Policy M, Region A>
        StaticLayout( const wrappers::TuplePacket<prefab::BasicBar<Cs, O, M, A>, Is>&... ) = delete;

        // SFINAE is used here to prevent infinite recursive matching of errors.
        template<typename Cfg,
                 typename... Cfgs,
                 typename = typename std::enable_if<traits::TpStartsWith<
                   traits::TypeList<typename std::decay<Cfg>::type, typename std::decay<Cfgs>::type...>,
                   Configs...>::value>::type>
        StaticLayout( Cfg&& cfg, Cfgs&&... cfgs ) noexcept( sizeof...( Cfgs ) + 1 == sizeof...( Configs ) )
          : StaticLayout( std::forward_as_tuple( std::forward<Cfg>( cfg ), std::forward<Cfgs>( cfgs )... ),
                          traits::MakeIndexSeq<sizeof...( Cfgs ) + 1>() )
        {}
        template<typename... Cfgs,
                 typename = typename std::enable_if<
                   traits::TpStartsWith<traits::TypeList<Cfgs...>, Configs...>::value>::type>
        StaticLayout( prefab::BasicBar<Cfgs, Outlet, Mode, Area>&&... bars )
          noexcept( sizeof...( Cfgs ) == sizeof...( Configs ) )
          : StaticLayout( std::forward_as_tuple( std::move( bars )... ),
                          traits::MakeIndexSeq<sizeof...( Cfgs )>() )
        {}
        StaticLayout( const StaticLayout& )            = delete;
        StaticLayout& operator=( const StaticLayout& ) = delete;
        StaticLayout( StaticLayout&& rhs ) noexcept
          : wrappers::TuplePacket<prefab::BasicBar<Configs, Outlet, Mode, Area>, Tags>( std::move( rhs ) )...
          , alive_cnt_ { 0 }
          , state_ { Phase::Stop }
        {
          PGBAR__ASSERT( rhs.online() == false );
        }
        StaticLayout& operator=( StaticLayout&& rhs ) & noexcept
        { // The thread insecurity here is deliberately designed.
          // After all, for a type where a base class reference can be exposed,
          // we cannot apply the lock within the type to the outside.
          PGBAR__TRUST( this != &rhs );
          PGBAR__ASSERT( online() == false );
          PGBAR__ASSERT( rhs.online() == false );
          (void)std::initializer_list<bool> { (
            wrappers::TuplePacket<prefab::BasicBar<Configs, Outlet, Mode, Area>, Tags>::operator=(
              std::move( rhs ) ),
            false )... };
          return *this;
        }
        ~StaticLayout() noexcept { kill(); }

        void shut()
        {
          if ( online() && !details::render::Renderer<Outlet>::itself().empty() )
            (void)std::initializer_list<bool> { ( this->ElementAt_t<Tags>::reset(), false )... };
          PGBAR__ASSERT( alive_cnt_ == 0 );
          PGBAR__ASSERT( online() == false );
        }
        void kill() noexcept
        {
          if ( online() && !details::render::Renderer<Outlet>::itself().empty() )
            (void)std::initializer_list<bool> { ( this->ElementAt_t<Tags>::abort(), false )... };
          PGBAR__ASSERT( alive_cnt_ == 0 );
          PGBAR__ASSERT( online() == false );
        }
        PGBAR__NODISCARD PGBAR__FORCEINLINE bool online() const noexcept
        {
          return state_.load( std::memory_order_acquire ) != Phase::Stop;
        }
        PGBAR__NODISCARD PGBAR__FORCEINLINE types::Size online_count() const noexcept
        {
          concurrent::SharedLock<concurrent::SharedMutex> lock { res_mtx_ };
          return std::count_if( stages_.cbegin(), stages_.cend(), []( Locus stage ) noexcept {
            return stage == Locus::Onstage || stage == Locus::Offstage;
          } );
        }

        void swap( StaticLayout& other ) noexcept
        { // The thread insecurity here is deliberately designed.
          // The reason can be found in the move assignment.
          PGBAR__TRUST( this != &other );
          PGBAR__ASSERT( online() == false );
          PGBAR__ASSERT( lhs.online() == false );
          (void)std::initializer_list<bool> {
            ( this->ElementAt_t<Tags>::swap( static_cast<ElementAt_t<Tags>&>( other ) ), false )...
          };
        }

        template<types::Size Pos>
        PGBAR__FORCEINLINE ElementAt_t<Pos>& at() & noexcept
        {
          return static_cast<ElementAt_t<Pos>&>( *this );
        }
        template<types::Size Pos>
        PGBAR__FORCEINLINE const ElementAt_t<Pos>& at() const& noexcept
        {
          return static_cast<const ElementAt_t<Pos>&>( *this );
        }
        template<types::Size Pos>
        PGBAR__FORCEINLINE ElementAt_t<Pos>& at() && noexcept
        {
          return std::move( at<Pos>() );
        }
      };
    } // namespace assets
  } // namespace details
} // namespace pgbar

#endif
