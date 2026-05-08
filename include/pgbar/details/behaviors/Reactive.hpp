#ifndef PGBAR_REACTIVE
#define PGBAR_REACTIVE

#include "../utils/Backport.hpp"
#include "../wrappers/UniqueFunction.hpp"
#include <mutex>

namespace pgbar {
  namespace details {
    namespace behaviors {
      template<typename Base, typename Derived>
      class Reactive : public Base {
        union Callback {
          wrappers::UniqueFunction<void()> on_;
          wrappers::UniqueFunction<void( Derived& )> on_self_;
          bool nil_ = false;

          constexpr Callback() = default;
          PGBAR__CXX20_CNSTXPR ~Callback() noexcept {}
        } hook_;
        enum class Tag : std::uint8_t { Nil, Nullary, Unary } tag_ = Tag::Nil;

        PGBAR__CXX20_CNSTXPR void destroy() noexcept
        {
          switch ( tag_ ) {
          case Tag::Nullary: utils::destroy_at( hook_.on_ ); break;
          case Tag::Unary:   utils::destroy_at( hook_.on_self_ ); break;

          case Tag::Nil: PGBAR__FALLTHROUGH;
          default:       break;
          }
          hook_.nil_ = false;
          tag_       = Tag::Nil;
        }

        PGBAR__CXX20_CNSTXPR void move_to( Reactive& other ) & noexcept
        {
          other.destroy();
          switch ( tag_ ) {
          case Tag::Nullary: {
            utils::construct_at( &other.hook_.on_, std::move( hook_.on_ ) );
          } break;
          case Tag::Unary: {
            utils::construct_at( &other.hook_.on_self_, std::move( hook_.on_self_ ) );
          } break;

          case Tag::Nil: PGBAR__FALLTHROUGH;
          default:       break;
          }
          other.tag_ = tag_;
          destroy();
        }

      protected:
        PGBAR__FORCEINLINE void react() &
        {
          switch ( tag_ ) {
          case Tag::Nullary: hook_.on_(); break;
          case Tag::Unary:   hook_.on_self_( static_cast<Derived&>( *this ) ); break;

          case Tag::Nil: PGBAR__FALLTHROUGH;
          default:       break;
          }
        }

        constexpr Reactive() = default;
        PGBAR__CXX20_CNSTXPR Reactive( Reactive&& rhs )
          noexcept( std::is_nothrow_move_constructible<Base>::value )
          : Base( std::move( rhs ) )
        {
          rhs.move_to( *this );
        }
        PGBAR__CXX20_CNSTXPR Reactive& operator=( Reactive&& rhs ) & noexcept(
          std::is_nothrow_move_assignable<Base>::value )
        { // The thread insecurity here is deliberately designed.
          // Because for a move-only type, transferring ownership simultaneously
          // in multiple locations should not occur.
          Base::operator=( std::move( rhs ) );
          rhs.move_to( *this );
          return *this;
        }
        PGBAR__CXX23_CNSTXPR ~Reactive() noexcept { destroy(); }

      public:
        using Base::Base;

#define PGBAR__METHOD( UnionMem, TagVal, ReturnType )                       \
  std::lock_guard<std::mutex> lock { static_cast<Derived*>( this )->mtx_ }; \
  utils::construct_at( &UnionMem, std::forward<F>( fn ) );                  \
  tag_ = Tag::TagVal;                                                       \
  return static_cast<ReturnType>( *this )

        template<typename F>
        auto action( F&& fn ) & noexcept(
          std::is_nothrow_constructible<wrappers::UniqueFunction<void()>, F>::value )
#ifdef __cpp_concepts
          -> decltype( auto )
          requires( !std::is_null_pointer_v<std::decay_t<F>>
                    && std::is_constructible_v<wrappers::UniqueFunction<void()>, F &&> )
#else
          -> typename std::enable_if<
            traits::AllOf<traits::Not<std::is_same<typename std::decay<F>::type, std::nullptr_t>>,
                          std::is_constructible<wrappers::UniqueFunction<void()>, F&&>>::value,
            Derived&>::type
#endif
        {
          PGBAR__METHOD( hook_.on_, Nullary, Derived& );
        }
        template<typename F>
        auto action( F&& fn ) && noexcept(
          std::is_nothrow_constructible<wrappers::UniqueFunction<void()>, F>::value )
#ifdef __cpp_concepts
          -> decltype( auto )
          requires( !std::is_null_pointer_v<std::decay_t<F>>
                    && std::is_constructible_v<wrappers::UniqueFunction<void()>, F &&> )
#else
          -> typename std::enable_if<
            traits::AllOf<traits::Not<std::is_same<typename std::decay<F>::type, std::nullptr_t>>,
                          std::is_constructible<wrappers::UniqueFunction<void()>, F&&>>::value,
            Derived&&>::type
#endif
        {
          PGBAR__METHOD( hook_.on_, Nullary, Derived&& );
        }
        template<typename F>
        auto action( F&& fn ) & noexcept(
          std::is_nothrow_constructible<wrappers::UniqueFunction<void( Derived& )>, F>::value )
#ifdef __cpp_concepts
          -> decltype( auto )
          requires( !std::is_null_pointer_v<std::decay_t<F>>
                    && std::is_constructible_v<wrappers::UniqueFunction<void( Derived& )>, F &&> )
#else
          -> typename std::enable_if<
            traits::AllOf<traits::Not<std::is_same<typename std::decay<F>::type, std::nullptr_t>>,
                          std::is_constructible<wrappers::UniqueFunction<void( Derived& )>, F&&>>::value,
            Derived&>::type
#endif
        {
          PGBAR__METHOD( hook_.on_self_, Unary, Derived& );
        }
        template<typename F>
        auto action( F&& fn ) && noexcept(
          std::is_nothrow_constructible<wrappers::UniqueFunction<void( Derived& )>, F>::value )
#ifdef __cpp_concepts
          -> decltype( auto )
          requires( !std::is_null_pointer_v<std::decay_t<F>>
                    && std::is_constructible_v<wrappers::UniqueFunction<void( Derived& )>, F &&> )
#else
          -> typename std::enable_if<
            traits::AllOf<traits::Not<std::is_same<typename std::decay<F>::type, std::nullptr_t>>,
                          std::is_constructible<wrappers::UniqueFunction<void( Derived& )>, F&&>>::value,
            Derived&&>::type
#endif
        {
          PGBAR__METHOD( hook_.on_self_, Unary, Derived&& );
        }

#undef PGBAR__METHOD
#define PGBAR__METHOD( ReturnType )                                         \
  std::lock_guard<std::mutex> lock { static_cast<Derived*>( this )->mtx_ }; \
  destroy();                                                                \
  return static_cast<ReturnType>( *this )

        Derived& action() & noexcept
        {
          PGBAR__METHOD( Derived& );
        }
        Derived&& action() && noexcept
        {
          PGBAR__METHOD( Derived&& );
        }

#undef PGBAR__METHOD

        template<typename F>
        friend PGBAR__FORCEINLINE auto operator|=( Reactive& bar, F&& fn )
#ifdef __cpp_concepts
          -> decltype( auto )
          requires( std::is_constructible_v<wrappers::UniqueFunction<void()>, F &&>
                    || std::is_constructible_v<wrappers::UniqueFunction<void( Derived& )>, F &&> )
#else
          -> typename std::enable_if<traits::AllOf<
            traits::Not<std::is_same<typename std::decay<F>::type, std::nullptr_t>>,
            traits::AnyOf<std::is_constructible<wrappers::UniqueFunction<void()>, F&&>,
                          std::is_constructible<wrappers::UniqueFunction<void( Derived& )>, F&&>>>::value>::
            type
#endif
        {
          bar.action( std::forward<F>( fn ) );
        }
        template<typename F>
        friend PGBAR__FORCEINLINE auto operator|( Reactive& bar, F&& fn )
#ifdef __cpp_concepts
          -> decltype( auto )
          requires( std::is_constructible_v<wrappers::UniqueFunction<void()>, F &&>
                    || std::is_constructible_v<wrappers::UniqueFunction<void( Derived& )>, F &&> )
#else
          -> typename std::enable_if<
            traits::AllOf<
              traits::AnyOf<std::is_constructible<wrappers::UniqueFunction<void()>, F&&>,
                            std::is_constructible<wrappers::UniqueFunction<void( Derived& )>, F&&>>>::value,
            Derived&>::type
#endif
        {
          return bar.action( std::forward<F>( fn ) );
        }
        template<typename F>
        friend PGBAR__FORCEINLINE auto operator|( Reactive&& bar, F&& fn )
#ifdef __cpp_concepts
          -> decltype( auto )
          requires( std::is_constructible_v<wrappers::UniqueFunction<void()>, F &&>
                    || std::is_constructible_v<wrappers::UniqueFunction<void( Derived& )>, F &&> )
#else
          -> typename std::enable_if<
            traits::AllOf<
              traits::AnyOf<std::is_constructible<wrappers::UniqueFunction<void()>, F&&>,
                            std::is_constructible<wrappers::UniqueFunction<void( Derived& )>, F&&>>>::value,
            Derived&&>::type
#endif
        {
          return std::move( bar.action( std::forward<F>( fn ) ) );
        }

        friend PGBAR__FORCEINLINE void operator|=( Reactive& bar, std::nullptr_t ) noexcept
        {
          bar.action();
        }
        friend PGBAR__FORCEINLINE Derived& operator|( Reactive& bar, std::nullptr_t ) noexcept
        {
          return bar.action();
        }
        friend PGBAR__FORCEINLINE Derived&& operator|( Reactive&& bar, std::nullptr_t ) noexcept
        {
          return std::move( bar.action() );
        }

        void swap( Reactive& other ) noexcept
        {
          Base::swap( other );
          switch ( tag_ ) {
          case Tag::Nullary:
            if ( other.tag_ == Tag::Nullary )
              hook_.on_.swap( other.hook_.on_ );
            else {
              wrappers::UniqueFunction<void()> tmp { std::move( hook_.on_ ) };
              other.move_to( *this );
              utils::construct_at( &other.hook_.on_, std::move( tmp ) );
              other.tag_ = Tag::Nullary;
            }
            break;

          case Tag::Unary:
            if ( other.tag_ == Tag::Unary )
              hook_.on_self_.swap( other.hook_.on_self_ );
            else {
              wrappers::UniqueFunction<void( Derived& )> tmp { std::move( hook_.on_self_ ) };
              other.move_to( *this );
              utils::construct_at( &other.hook_.on_self_, std::move( tmp ) );
              other.tag_ = Tag::Unary;
            }
            break;

          case Tag::Nil: PGBAR__FALLTHROUGH;
          default:
            if ( other.tag_ != Tag::Nil )
              other.move_to( *this );
            break;
          }
        }
      };
    } // namespace behaviors
  } // namespace details
} // namespace pgbar

#endif
