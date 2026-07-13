#ifndef PACE_STATIC_LAYOUT
#define PACE_STATIC_LAYOUT

#include "../assets/PackagedBar.hpp"
#include "../io/OStream.hpp"
#include "../utils/Util.hpp"
#include <initializer_list>
#include <tuple>

namespace pace {
  namespace details {
    namespace assets {
      template<Channel, Policy, Region, typename, typename...>
      class StaticLayout;
      template<Channel Sink, Policy Mode, Region Zone, types::Size... Tags, typename... Configs>
      class StaticLayout<Sink, Mode, Zone, traits::IndexSequence<Tags...>, Configs...> final
        : public assets::PackagedBar<Configs, Sink, Mode, Zone, Tags>... {
        static_assert( sizeof...( Tags ) == sizeof...( Configs ), "unexpected type mismatch" );
        static_assert( sizeof...( Configs ) > 0, "the number of progress bars cannot be zero" );

        template<types::Size Pos>
        using ElementAt_t = traits::TypeAt_t<Pos, assets::PackagedBar<Configs, Sink, Mode, Zone, Tags>...>;

        std::atomic<types::Size> alive_cnt_;
        mutable std::mutex mtx_;

        enum class Phase : std::uint8_t { Stop, Awake, Refresh };
        std::atomic<Phase> state_ { Phase::Stop };

        enum class Locus : std::uint8_t { Offstage, Onstage, Echo };
        // Bitmask indicating which bars produced output in the current render pass.
        std::array<Locus, sizeof...( Configs )> stages_;

        template<types::Size Pos, typename... Args>
        PACE__FORCEINLINE PACE__CXX14_CNSTXPR typename std::enable_if<( Pos >= sizeof...( Configs ) )>::type
          do_render( Args&&... ) &
        {}
        template<types::Size Pos = 0>
        typename std::enable_if<( Pos < sizeof...( Configs ) )>::type do_render( io::CharPipeline& pipeline,
                                                                                 bool istty,
                                                                                 bool style_off,
                                                                                 bool hide_done ) &
        {
          PACE__ASSERT( online() );
          switch ( stages_[Pos] ) {
          case Locus::Echo: {
            if ( !istty || hide_done )
              stages_[Pos] = Locus::Offstage;
            else if ( !at<Pos>().active() ) {
              pipeline << console::nextline;
              break;
            }
          }
            PACE__FALLTHROUGH;
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
            PACE__FALLTHROUGH;
          case Locus::Onstage: {
            at<Pos>().draw( pipeline, style_off );

            if ( istty )
              pipeline << console::linewipe;
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
                pipeline << console::linestart;
              else
                pipeline << console::nextline;
              if ( istty && !hide_done )
                stages_[Pos] = Locus::Echo;
              else
                stages_[Pos] = Locus::Offstage;
            } else
              pipeline << console::nextline;
            if ( istty && hide_done )
              pipeline << console::linewipe;
          } break;

          default: utils::unreachable();
          }

          return do_render<Pos + 1>( pipeline, istty, style_off, hide_done ); // tail recursive
        }

        void do_halt( bool forced ) noexcept final
        { // This virtual function is invoked only via the vtable,
          // hence the default arguments from the base class declaration are always used.
          // Any default arguments provided in the derived class are ignored.
          PACE__ASSERT( online() );
          PACE__ASSERT( alive_cnt_ > 0 );
          auto& executor = render::Renderer<Sink>::itself();
          PACE__ASSERT( executor.empty() == false );
          if ( !forced )
            executor.template trigger<Mode>();

          if ( alive_cnt_.fetch_sub( 1, std::memory_order_relaxed ) > 1 )
            return;
          std::lock_guard<std::mutex> lock { mtx_ };
          if ( alive_cnt_.load( std::memory_order_relaxed ) == 0 ) {
            state_.store( Phase::Stop, std::memory_order_relaxed );
            executor.dismiss_then( []() noexcept { io::OStream<Sink>::itself().release(); } );
          }
        }
        void do_boot() & final
        {
          auto& executor = render::Renderer<Sink>::itself();
          std::lock_guard<std::mutex> lock { mtx_ };
          if ( state_.load( std::memory_order_relaxed ) == Phase::Stop ) {
            if ( !executor.try_appoint( [this]() {
                   auto& ostream        = io::OStream<Sink>::itself();
                   const auto istty     = console::TermContext<Sink>::itself().connected();
                   const auto style_off = !istty && config::auto_style_off();
                   switch ( state_.load( std::memory_order_relaxed ) ) {
                   case Phase::Awake: {
                     if PACE__CXX17_CNSTXPR ( Zone == Region::Fixed )
                       if ( istty )
                         ostream << console::savecursor;
                     // Since the Renderer ensures that only one thread is performing rendering at any time,
                     // all access to stages_ always occurs within a same thread,
                     // so we do not need to use a mutex to protect it.
                     std::fill( stages_.begin(), stages_.end(), Locus::Offstage );
                     do_render( ostream, istty, style_off, config::hide_completed() );
                     ostream << io::flush;

                     auto expected = Phase::Awake;
                     state_.compare_exchange_strong( expected, Phase::Refresh, std::memory_order_relaxed );
                   } break;
                   case Phase::Refresh: {
                     if ( istty ) {
                       if PACE__CXX17_CNSTXPR ( Zone == Region::Fixed )
                         ostream << console::resetcursor;
                       else
                         ostream
                           .apply( console::prevline,
                                   std::count_if(
                                     stages_.cbegin(),
                                     stages_.cend(),
                                     []( Locus stage ) noexcept { return stage != Locus::Offstage; } ) )
                           .apply( console::linestart );
                     }
                     do_render( ostream, istty, style_off, config::hide_completed() );
                     ostream << io::flush;
                   } break;
                   default: break;
                   }
                 } ) )
              PACE__UNLIKELY throw exception::InvalidState(
                charcodes::make_literal( "pace: another progress bar instance is already running" ) );

            (void)console::TermContext<Sink>::itself().detect();
            io::OStream<Sink>::itself() << io::release;
            state_.store( Phase::Awake, std::memory_order_relaxed );

            auto guard2 = utils::make_scope_fail( [&]() noexcept {
              state_.store( Phase::Stop, std::memory_order_relaxed );
              executor.dismiss();
            } );
            executor.template activate<Mode>();
          } else if PACE__CXX17_CNSTXPR ( Mode == Policy::Sync )
            executor.template commit<Mode>();
          else
            executor.template trigger<Mode>();
          alive_cnt_.fetch_add( 1, std::memory_order_relaxed );
          PACE__ASSERT( alive_cnt_ <= sizeof...( Configs ) );
        }

        template<typename Tuple, types::Size... Is>
        StaticLayout( Tuple&& tup, traits::IndexSequence<Is...> )
          noexcept( std::tuple_size<typename std::decay<Tuple>::type>::value == sizeof...( Configs ) )
          : ElementAt_t<Is>( utils::pick_or<Is, ElementAt_t<Is>>( std::forward<Tuple>( tup ) ) )...
        {
          static_assert( std::tuple_size<typename std::decay<Tuple>::type>::value <= sizeof...( Is ),
                         "unexpected tuple size mismatch" );
        }

      public:
        template<types::Size... Is, typename... Cs, Channel S, Policy M, Region Z>
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
        ~StaticLayout() noexcept { kill(); }

        void shut()
        {
          if ( online() && !details::render::Renderer<Sink>::itself().empty() )
            (void)std::initializer_list<bool> { ( this->ElementAt_t<Tags>::reset(), false )... };
          PACE__ASSERT( alive_cnt_ == 0 );
          PACE__ASSERT( online() == false );
        }
        void kill() noexcept
        {
          if ( online() && !details::render::Renderer<Sink>::itself().empty() )
            (void)std::initializer_list<bool> { ( this->ElementAt_t<Tags>::abort(), false )... };
          PACE__ASSERT( alive_cnt_ == 0 );
          PACE__ASSERT( online() == false );
        }

        PACE__NODISCARD PACE__FORCEINLINE bool online() const noexcept
        { return state_.load( std::memory_order_relaxed ) != Phase::Stop; }
        PACE__NODISCARD PACE__FORCEINLINE types::Size online_count() const noexcept
        { return alive_cnt_.load( std::memory_order_relaxed ); }

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

        template<types::Size Pos>
        PACE__FORCEINLINE PACE__CXX14_CNSTXPR ElementAt_t<Pos>& at() & noexcept
        { return static_cast<ElementAt_t<Pos>&>( *this ); }
        template<types::Size Pos>
        PACE__FORCEINLINE PACE__CXX14_CNSTXPR const ElementAt_t<Pos>& at() const& noexcept
        { return static_cast<const ElementAt_t<Pos>&>( *this ); }
        template<types::Size Pos>
        PACE__FORCEINLINE PACE__CXX14_CNSTXPR ElementAt_t<Pos>& at() && noexcept
        { return std::move( at<Pos>() ); }
      };
    } // namespace assets
  } // namespace details
} // namespace pace

#endif
