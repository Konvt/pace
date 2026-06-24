#ifndef PACE_INCREMENTAL
#define PACE_INCREMENTAL

#include "../../slice/IteratorSpan.hpp"
#include "../../slice/NumericSpan.hpp"
#include "../../slice/SizedSpan.hpp"
#include "../wrappers/MovableRef.hpp"

namespace pace {
  namespace slice {
    template<typename, typename>
    class TrackedSpan;
  }

  namespace details {
    namespace behaviors {
      template<typename Base, typename Derived>
      class Incremental : public Base {
        // Throws the exception::InvalidState if current object is active.
        PACE__FORCEINLINE void throw_if_active()
        {
          if ( static_cast<Derived*>( this )->active() )
            PACE__UNLIKELY throw exception::InvalidState(
              charcodes::make_literal( "pace: try to iterate using an active object" ) );
        }

      protected:
        std::atomic<std::uint64_t> task_cnt_ { 0 };
        std::uint64_t task_end_;

        Incremental() = default;
        Incremental( Incremental&& rhs ) noexcept : Base( std::move( rhs ) )
        { task_cnt_.store( 0, std::memory_order_relaxed ); }
        Incremental& operator=( Incremental&& rhs ) & noexcept
        {
          Base::operator=( std::move( rhs ) );
          return *this;
        }
        ~Incremental() = default;

      public:
        using Base::Base;

        // Get the progress of the task.
        PACE__NODISCARD std::uint64_t progress() const noexcept
        { return task_cnt_.load( std::memory_order_relaxed ); }

        PACE__FORCEINLINE void tick() & final
        {
          static_cast<Derived*>( this )->do_tick(
            [this]() noexcept { this->task_cnt_.fetch_add( 1, std::memory_order_relaxed ); } );
        }
        PACE__FORCEINLINE void tick( std::uint64_t next_step ) &
        {
          static_cast<Derived*>( this )->do_tick( [&]() noexcept {
            const auto task_cnt = this->task_cnt_.load( std::memory_order_relaxed );
            this->task_cnt_.fetch_add( task_cnt + next_step > this->task_end_ ? this->task_end_ - task_cnt
                                                                              : next_step,
                                       std::memory_order_relaxed );
          } );
        }
        /**
         * Set the iteration step of the progress bar to a specified percentage.
         * Ignore the call if the iteration count exceeds the given percentage.
         * If `percentage` is bigger than 100, it will be set to 100.
         *
         * @param percentage Value range: [0, 100].
         */
        PACE__FORCEINLINE void tick_to( std::uint8_t percentage ) &
        {
          static_cast<Derived*>( this )->do_tick( [&]() noexcept {
            auto updater = [this]( std::uint64_t target ) noexcept {
              auto current = this->task_cnt_.load( std::memory_order_relaxed );
              while ( !this->task_cnt_.compare_exchange_weak( current, target, std::memory_order_relaxed )
                      && target <= current ) {}
            };
            if ( percentage <= 100 ) {
              const auto target = static_cast<std::uint64_t>( this->task_end_ * percentage * 0.01 );
              PACE__ASSERT( target <= this->task_end_ );
              updater( target );
            } else
              updater( this->task_end_ );
          } );
        }

        /**
         * Visualize unidirectional traversal of a numeric interval defined by parameters.
         *
         * @return Return a range `[startpoint, endpoint)` that moves unidirectionally.
         */
        template<typename N>
        PACE__NODISCARD auto iterate( N startpoint, N endpoint, N step ) &
#ifdef __cpp_concepts
          -> slice::TrackedSpan<slice::NumericSpan<N>, wrappers::MovableRef<Derived>>
          requires std::is_arithmetic_v<N>
#else
          -> typename std::enable_if<
            std::is_arithmetic<N>::value,
            slice::TrackedSpan<slice::NumericSpan<N>, wrappers::MovableRef<Derived>>>::type
#endif
        { // default parameter will cause ambiguous overloads
          // so we have to write them all
          throw_if_active();
          return {
            { startpoint, endpoint, step },
            wrappers::mref( static_cast<Derived&>( *this ) )
          };
        }
        template<typename N, typename Proc>
        auto iterate( N startpoint, N endpoint, N step, Proc&& op )
#ifdef __cpp_concepts
          requires std::is_arithmetic_v<N>
#else
          -> typename std::enable_if<std::is_arithmetic<N>::value>::type
#endif
        {
          for ( N e : iterate( startpoint, endpoint, step ) )
            (void)op( e );
        }

        template<typename N>
        PACE__NODISCARD auto iterate( N endpoint, N step ) &
#ifdef __cpp_concepts
          -> slice::TrackedSpan<slice::NumericSpan<N>, wrappers::MovableRef<Derived>>
          requires std::is_floating_point_v<N>
#else
          -> typename std::enable_if<
            std::is_floating_point<N>::value,
            slice::TrackedSpan<slice::NumericSpan<N>, wrappers::MovableRef<Derived>>>::type
#endif
        {
          throw_if_active();
          return {
            { {}, endpoint, step },
            wrappers::mref( static_cast<Derived&>( *this ) )
          };
        }
        template<typename N, typename Proc>
        auto iterate( N endpoint, N step, Proc&& op )
#ifdef __cpp_concepts
          requires std::is_floating_point_v<N>
#else
          -> typename std::enable_if<std::is_floating_point<N>::value>::type
#endif
        {
          for ( N e : iterate( endpoint, step ) )
            (void)op( e );
        }

        // Only available for integer types.
        template<typename N>
        PACE__NODISCARD auto iterate( N startpoint, N endpoint ) &
#ifdef __cpp_concepts
          -> slice::TrackedSpan<slice::NumericSpan<N>, wrappers::MovableRef<Derived>>
          requires std::is_integral_v<N>
#else
          -> typename std::enable_if<
            std::is_integral<N>::value,
            slice::TrackedSpan<slice::NumericSpan<N>, wrappers::MovableRef<Derived>>>::type
#endif
        {
          throw_if_active();
          return {
            { startpoint, endpoint },
            wrappers::mref( static_cast<Derived&>( *this ) )
          };
        }
        template<typename N, typename Proc>
        auto iterate( N startpoint, N endpoint, Proc&& op )
#ifdef __cpp_concepts
          requires std::is_integral_v<N>
#else
          -> typename std::enable_if<std::is_integral<N>::value>::type
#endif
        {
          for ( N e : iterate( startpoint, endpoint ) )
            (void)op( e );
        }

        template<typename N>
        PACE__NODISCARD auto iterate( N endpoint ) &
#ifdef __cpp_concepts
          -> slice::TrackedSpan<slice::NumericSpan<N>, wrappers::MovableRef<Derived>>
          requires std::is_integral_v<N>
#else
          -> typename std::enable_if<
            std::is_integral<N>::value,
            slice::TrackedSpan<slice::NumericSpan<N>, wrappers::MovableRef<Derived>>>::type
#endif
        {
          throw_if_active();
          return { { endpoint }, wrappers::mref( static_cast<Derived&>( *this ) ) };
        }
        template<typename N, typename Proc>
        auto iterate( N endpoint, Proc&& op )
#ifdef __cpp_concepts
          requires std::is_integral_v<N>
#else
          -> typename std::enable_if<std::is_integral<N>::value>::type
#endif
        {
          for ( N e : iterate( endpoint ) )
            (void)op( e );
        }

        // Visualize unidirectional traversal of a iterator interval defined by parameters.
        template<typename Itr, typename Snt>
        PACE__NODISCARD auto iterate( Itr startpoint, Snt endpoint ) & noexcept(
          traits::all_of<std::is_nothrow_move_constructible<Itr>,
                         std::is_nothrow_move_constructible<Snt>>::value )
#ifdef __cpp_concepts
          -> slice::TrackedSpan<slice::IteratorSpan<Itr, Snt>, wrappers::MovableRef<Derived>>
          requires traits::is_sized_cursor<Itr, Snt>::value
#else
          -> typename std::enable_if<
            traits::is_sized_cursor<Itr, Snt>::value,
            slice::TrackedSpan<slice::IteratorSpan<Itr, Snt>, wrappers::MovableRef<Derived>>>::type
#endif
        {
          throw_if_active();
          return {
            { std::move( startpoint ), std::move( endpoint ) },
            wrappers::mref( static_cast<Derived&>( *this ) )
          };
        }
        template<typename Itr, typename Snt, typename Proc>
        auto iterate( Itr startpoint, Snt endpoint, Proc&& op )
#ifdef __cpp_concepts
          requires traits::is_sized_cursor<Itr, Snt>::value
#else
          -> typename std::enable_if<traits::is_sized_cursor<Itr, Snt>::value>::type
#endif
        {
          for ( auto&& e : iterate( std::move( startpoint ), std::move( endpoint ) ) )
            (void)op( std::forward<decltype( e )>( e ) );
        }

        // Visualize unidirectional traversal of a abstract range interval defined by `container`'s
        // slice.
        template<class R>
        PACE__NODISCARD auto iterate( R& container ) &
#ifdef __cpp_concepts
          -> slice::TrackedSpan<slice::SizedSpan<std::remove_reference_t<R>>, wrappers::MovableRef<Derived>>
          requires( traits::is_sized_range<std::remove_reference_t<R>>::value
                    && !std::ranges::view<std::remove_reference_t<R>> )
#else
          -> typename std::enable_if<
            traits::is_sized_range<typename std::remove_reference<R>::type>::value,
            slice::TrackedSpan<slice::SizedSpan<typename std::remove_reference<R>::type>,
                               wrappers::MovableRef<Derived>>>::type
#endif
        {
          throw_if_active();
          return { { container }, wrappers::mref( static_cast<Derived&>( *this ) ) };
        }
#ifdef __cpp_concepts
        template<class R>
          requires( traits::is_sized_range<R>::value && std::ranges::view<R> )
        PACE__NODISCARD slice::TrackedSpan<R, wrappers::MovableRef<Derived>> iterate( R view ) &
        {
          throw_if_active();
          return { std::move( view ), wrappers::mref( static_cast<Derived&>( *this ) ) };
        }
#endif
        template<class R, typename Proc>
        auto iterate( R&& range, Proc&& op )
#ifdef __cpp_concepts
          requires traits::is_sized_range<std::remove_reference_t<R>>::value
#else
          -> typename std::enable_if<
            traits::is_sized_range<typename std::remove_reference<R>::type>::value>::type
#endif
        {
          for ( auto&& e : iterate( std::forward<R>( range ) ) )
            (void)op( std::forward<decltype( e )>( e ) );
        }
      };
    } // namespace behaviors
  } // namespace details
} // namespace pace

#endif
