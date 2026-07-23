#ifndef PACE_IO_MONOID
#define PACE_IO_MONOID

#include "../utils/Backport.hpp"
#include "CharPipeline.hpp"

namespace pace {
  namespace details {
    namespace io {
      template<template<typename...> class Comb, typename... Captures>
      struct Currying {
        std::tuple<Captures...> capture;

        template<typename... Args>
        PACE__NODISCARD PACE__FORCEINLINE constexpr Comb<Args...> operator()( Args&&... args ) const&
        {
          return utils::apply(
            [&]( const Captures&... caps ) {
              return Comb<Args...> {
                { caps..., std::forward<Args>( args )... }
              };
            },
            capture );
        }
        template<typename... Args>
        PACE__NODISCARD PACE__FORCEINLINE PACE__CXX14_CNSTXPR Comb<Args...> operator()( Args&&... args ) &&
        {
          return utils::apply(
            [&]( Captures&&... caps ) {
              return Comb<Args...> {
                { std::forward<Captures>( caps )..., std::forward<Args>( args )... }
              };
            },
            std::move( capture ) );
        }
      };

      //////////////////////////////////////////////////

      template<typename... Emissions>
      struct Concat {
        static_assert( sizeof...( Emissions ) > 0, "nothing to concat" );

        std::tuple<Emissions...> emission;

        PACE__FORCEINLINE friend PACE__CXX23_CNSTXPR io::CharPipeline& operator<<( io::CharPipeline& pipeline,
                                                                                   const Concat& self )
        {
          utils::apply(
            [&pipeline]( const Emissions&... emis ) {
              (void)std::initializer_list<bool> { ( pipeline << emis, false )... };
            },
            self.emission );
          return pipeline;
        }
        PACE__FORCEINLINE friend PACE__CXX23_CNSTXPR io::CharPipeline& operator<<( io::CharPipeline& pipeline,
                                                                                   Concat&& self )
        {
          utils::apply(
            [&pipeline]( Emissions&&... emis ) {
              (void)std::initializer_list<bool> { ( pipeline << std::forward<Emissions>( emis ), false )... };
            },
            std::move( self.emission ) );
          return pipeline;
        }
      };
      template<typename... Args>
      PACE__NODISCARD PACE__FORCEINLINE constexpr Concat<Args...> concat( Args&&... args )
      { return { { std::forward<Args>( args )... } }; }

      //////////////////////////////////////////////////

      template<>
      struct Concat<> {
        std::tuple<> emission;

        PACE__FORCEINLINE friend PACE__CXX23_CNSTXPR CharPipeline& operator<<( CharPipeline& pipeline,
                                                                               const Concat& )
        { return pipeline; }
      };
      using Nop = Concat<>;
      PACE__NODISCARD PACE__FORCEINLINE constexpr Nop nop() noexcept
      { return {}; }

      //////////////////////////////////////////////////

      template<typename Separator, typename Emission, typename... Emissions>
      struct Join {
        // The tuple supports EBO, thus we put them together.
        std::tuple<Separator, Emission, Emissions...> value;

        template<typename CharPipeline>
        PACE__FORCEINLINE friend PACE__CXX23_CNSTXPR CharPipeline& operator<<( CharPipeline& pipeline,
                                                                               const Join& self )
        {
          utils::apply(
            [&pipeline]( const Separator& sep, const Emission& emi, const Emissions&... emis ) {
              pipeline << emi;
              (void)std::initializer_list<bool> { ( ( pipeline << sep << emis ), false )... };
            },
            self.value );
          return pipeline;
        }
        template<typename CharPipeline>
        PACE__FORCEINLINE friend PACE__CXX23_CNSTXPR CharPipeline& operator<<( CharPipeline& pipeline,
                                                                               Join&& self )
        {
          utils::apply(
            [&pipeline]( Separator&& sep, Emission&& emi, Emissions&&... emis ) {
              pipeline << std::forward<Emission>( emi );
              (void)std::initializer_list<bool> { ( ( pipeline << sep << std::forward<Emissions>( emis ) ),
                                                    false )... };
            },
            std::move( self.value ) );
          return pipeline;
        }
      };

      template<typename Sep>
      struct Currying<Join, Sep> {
        std::tuple<Sep> capture;

        template<typename... Emis>
        PACE__NODISCARD PACE__FORCEINLINE constexpr Join<Sep, Emis...> operator()( Emis&&... emis ) const&
        {
          return {
            { std::get<0>( capture ), std::forward<Emis>( emis )... }
          };
        }
        template<typename... Emis>
        PACE__NODISCARD PACE__FORCEINLINE PACE__CXX14_CNSTXPR Join<Sep, Emis...> operator()(
          Emis&&... emis ) &&
        {
          return {
            { std::move( std::get<0>( capture ) ), std::forward<Emis>( emis )... }
          };
        }
      };

      template<typename Separator>
      PACE__NODISCARD PACE__FORCEINLINE constexpr Currying<Join, Separator> join( Separator&& separator )
      { return { { std::forward<Separator>( separator ) } }; }
      template<typename Separator, typename... Emissions>
      PACE__NODISCARD PACE__FORCEINLINE constexpr Join<Separator, Emissions...> join(
        Separator&& separator,
        Emissions&&... emissions )
      {
        return {
          { std::forward<Separator>( separator ), std::forward<Emissions>( emissions )... }
        };
      }

      //////////////////////////////////////////////////

      template<typename Predicate, typename Positive, typename Negative, typename... Args>
      struct Choice {
        std::tuple<Predicate, Positive, Negative, Args...> value;

        PACE__FORCEINLINE friend PACE__CXX23_CNSTXPR CharPipeline& operator<<( CharPipeline& pipeline,
                                                                               const Choice& self )
        {
          return utils::apply(
            [&pipeline]( const Predicate& pred,
                         const Positive& pos,
                         const Negative& neg,
                         const Args&... args ) {
              if ( utils::invoke( pred, args... ) )
                pipeline << pos;
              else
                pipeline << neg;
              return pipeline;
            },
            self.value );
        }
        PACE__FORCEINLINE friend PACE__CXX23_CNSTXPR CharPipeline& operator<<( CharPipeline& pipeline,
                                                                               Choice&& self )
        {
          utils::apply(
            [&pipeline]( Predicate&& pred, Positive&& pos, Negative&& neg, Args&&... args ) {
              if ( utils::invoke( std::forward<Predicate>( pred ), std::forward<Args>( args )... ) )
                pipeline << std::forward<Positive>( pos );
              else
                pipeline << std::forward<Negative>( neg );
            },
            std::move( self.value ) );
          return pipeline;
        }
      };

      template<typename Pred>
      struct Currying<Choice, Pred> {
        std::tuple<Pred> capture;

        template<typename Positive, typename Negative, typename... Args>
        PACE__NODISCARD PACE__FORCEINLINE constexpr Choice<Pred, Positive, Negative, Args...>
          operator()( Positive&& positive, Negative&& negative, Args&&... args ) const&
        {
          return { std::get<0>( capture ),
                   std::forward<Positive>( positive ),
                   std::forward<Negative>( negative ),
                   { std::forward<Args>( args )... } };
        }
        template<typename Positive, typename Negative, typename... Args>
        PACE__NODISCARD PACE__FORCEINLINE PACE__CXX14_CNSTXPR Choice<Pred, Positive, Negative, Args...>
          operator()( Positive&& positive, Negative&& negative, Args&&... args ) &&
        {
          return { std::forward<Pred>( std::get<0>( capture ) ),
                   std::forward<Positive>( positive ),
                   std::forward<Negative>( negative ),
                   { std::forward<Args>( args )... } };
        }
      };
      template<typename Pred, typename... Args>
      struct Currying<Choice, Pred, void, void, Args...> {
        // void means unspecified
        std::tuple<Pred, Args...> capture;

        template<typename Positive, typename Negative>
        PACE__NODISCARD PACE__FORCEINLINE constexpr Choice<Pred, Positive, Negative> operator()(
          Positive&& positive,
          Negative&& negative ) const&
        {
          return utils::apply(
            [&]( const Pred& pred, const Args&... args ) {
              return Choice<Pred, Positive, Negative> {
                { pred, std::forward<Positive>( positive ), std::forward<Negative>( negative ), args... }
              };
            },
            capture );
        }
        template<typename Positive, typename Negative>
        PACE__NODISCARD PACE__FORCEINLINE PACE__CXX14_CNSTXPR Choice<Pred, Positive, Negative> operator()(
          Positive&& positive,
          Negative&& negative ) &&
        {
          return utils::apply(
            [&]( Pred&& pred, Args&&... args ) {
              return Choice<Pred, Positive, Negative> {
                { std::forward<Pred>( pred ),
                 std::forward<Positive>( positive ),
                 std::forward<Negative>( negative ),
                 std::forward<Args>( args )... }
              };
            },
            std::move( capture ) );
        }
      };

      template<typename Predicate>
      PACE__NODISCARD PACE__FORCEINLINE constexpr Currying<Choice, Predicate> choice( Predicate&& predicate )
      { return { { std::forward<Predicate>( predicate ) } }; }
      template<typename Predicate, typename... Args>
      PACE__NODISCARD PACE__FORCEINLINE constexpr
        typename std::enable_if<traits::is_invocable_r<bool, Predicate, Args...>::value,
                                Currying<Choice, Predicate, void, void, Args...>>::type
        choice( Predicate&& predicate, Args&&... args )
      {
        return {
          { std::forward<Predicate>( predicate ), std::forward<Args>( args )... }
        };
      }
      template<typename Predicate, typename Positive, typename Negative, typename... Args>
      PACE__NODISCARD PACE__FORCEINLINE constexpr
        typename std::enable_if<traits::is_invocable_r<bool, Predicate, Args...>::value,
                                Choice<Predicate, Positive, Negative, Args...>>::type
        choice( Predicate&& predicate, Positive&& positive, Negative&& negative, Args&&... args )
      {
        return {
          { std::forward<Predicate>( predicate ),
           std::forward<Positive>( positive ),
           std::forward<Negative>( negative ),
           std::forward<Args>( args )... }
        };
      }

      //////////////////////////////////////////////////

      template<typename Pred, typename Neg>
      struct Currying<Choice, Pred, void, Neg> {
        std::tuple<Pred, Neg> capture;

        template<typename Positive, typename... Args>
        PACE__NODISCARD PACE__FORCEINLINE constexpr Choice<Pred, Positive, Neg, Args...> operator()(
          Positive&& positive,
          Args&&... args ) const&
        {
          return {
            { std::get<0>( capture ),
             std::forward<Positive>( positive ),
             std::get<1>( capture ),
             std::forward<Args>( args )... }
          };
        }
        template<typename Positive, typename... Args>
        PACE__NODISCARD PACE__FORCEINLINE PACE__CXX14_CNSTXPR Choice<Pred, Positive, Neg, Args...> operator()(
          Positive&& positive,
          Args&&... args ) &&
        {
          return {
            { std::forward<Pred>( std::get<0>( capture ) ),
             std::forward<Positive>( positive ),
             std::forward<Neg>( std::get<1>( capture ) ),
             std::forward<Args>( args )... }
          };
        }
      };
      template<typename Pred, typename Neg, typename... Args>
      struct Currying<Choice, Pred, void, Neg, Args...> {
        std::tuple<Pred, Neg, Args...> capture;

        template<typename Positive>
        PACE__NODISCARD PACE__FORCEINLINE constexpr Choice<Pred, Positive, Neg, Args...> operator()(
          Positive&& positive ) const&
        {
          return utils::apply(
            [&positive]( const Pred& pred, const Neg& neg, const Args&... args ) {
              return Choice<Pred, Positive, Neg, Args...> {
                { pred, std::forward<Positive>( positive ), neg, args... }
              };
            },
            capture );
        }
        template<typename Positive>
        PACE__NODISCARD PACE__FORCEINLINE PACE__CXX14_CNSTXPR Choice<Pred, Positive, Neg, Args...> operator()(
          Positive&& positive ) &&
        {
          return utils::apply(
            [&positive]( Pred&& pred, Neg&& neg, Args&&... args ) {
              return Choice<Pred, Positive, Neg, Args...> {
                { std::forward<Pred>( pred ),
                 std::forward<Positive>( positive ),
                 std::forward<Neg>( neg ),
                 std::forward<Args>( args )... }
              };
            },
            std::move( capture ) );
        }
      };

      template<typename Predicate>
      PACE__NODISCARD PACE__FORCEINLINE constexpr Currying<Choice, Predicate, void, Nop> when(
        Predicate&& predicate )
      {
        return {
          { std::forward<Predicate>( predicate ), {} }
        };
      }
      template<typename Predicate, typename... Args>
      PACE__NODISCARD PACE__FORCEINLINE constexpr
        typename std::enable_if<traits::is_invocable_r<bool, Predicate, Args...>::value,
                                Currying<Choice, Predicate, void, Nop, Args...>>::type
        when( Predicate&& predicate, Args&&... args )
      {
        return {
          { std::forward<Predicate>( predicate ), {}, std::forward<Args>( args )... }
        };
      }
      template<typename Predicate, typename Emission, typename... Args>
      PACE__NODISCARD PACE__FORCEINLINE constexpr
        typename std::enable_if<traits::is_invocable_r<bool, Predicate, Args...>::value,
                                Choice<Predicate, Emission, Nop, Args...>>::type
        when( Predicate&& predicate, Emission&& emission, Args&&... args )
      {
        return {
          { std::forward<Predicate>( predicate ),
           std::forward<Emission>( emission ),
           {},
           std::forward<Args>( args )... }
        };
      }
      PACE__NODISCARD PACE__FORCEINLINE PACE__CXX14_CNSTXPR Currying<Choice, utils::Identity, void, Nop, bool>
        when( bool condition ) noexcept
      {
        return {
          { {}, {}, condition }
        };
      }
      template<typename Emission>
      PACE__NODISCARD PACE__FORCEINLINE constexpr Choice<utils::Identity, Emission, Nop, bool> when(
        bool condition,
        Emission&& emission )
      {
        return {
          { {}, std::forward<Emission>( emission ), {}, condition }
        };
      }

      //////////////////////////////////////////////////

      template<typename Pred, typename Pos>
      struct Currying<Choice, Pred, Pos, void> {
        std::tuple<Pred, Pos> capture;

        template<typename Negative, typename... Args>
        PACE__NODISCARD PACE__FORCEINLINE constexpr Choice<Pred, Pos, Negative, Args...> operator()(
          Negative&& negative,
          Args&&... args ) const&
        {
          return {
            { std::get<0>( capture ),
             std::get<1>( capture ),
             std::forward<Negative>( negative ),
             std::forward<Args>( args )... }
          };
        }
        template<typename Negative, typename... Args>
        PACE__NODISCARD PACE__FORCEINLINE PACE__CXX14_CNSTXPR Choice<Pred, Pos, Negative, Args...> operator()(
          Negative&& negative,
          Args&&... args ) &&
        {
          return {
            { std::forward<Pred>( std::get<0>( capture ) ),
             std::forward<Pos>( std::get<1>( capture ) ),
             std::forward<Negative>( negative ),
             std::forward<Args>( args )... }
          };
        }
      };
      template<typename Pred, typename Pos, typename... Args>
      struct Currying<Choice, Pred, Pos, void, Args...> {
        std::tuple<Pred, Pos, Args...> capture;

        template<typename Negative>
        PACE__NODISCARD PACE__FORCEINLINE constexpr Choice<Pred, Pos, Negative, Args...> operator()(
          Negative&& negative ) const&
        {
          return utils::apply(
            [&negative]( const Pred& pred, const Pos& pos, const Args&... args ) {
              return Choice<Pred, Pos, Negative, Args...> {
                { pred, pos, std::forward<Negative>( negative ), args... }
              };
            },
            capture );
        }
        template<typename Negative>
        PACE__NODISCARD PACE__FORCEINLINE PACE__CXX14_CNSTXPR Choice<Pred, Pos, Negative, Args...> operator()(
          Negative&& negative ) &&
        {
          return utils::apply(
            [&negative]( Pred&& pred, Pos&& pos, Args&&... args ) {
              return Choice<Pred, Pos, Negative, Args...> {
                { std::forward<Pred>( pred ),
                 std::forward<Pos>( pos ),
                 std::forward<Negative>( negative ),
                 std::forward<Args>( args )... }
              };
            },
            std::move( capture ) );
        }
      };

      template<typename Predicate>
      PACE__NODISCARD PACE__FORCEINLINE constexpr Currying<Choice, Predicate, Nop, void> unless(
        Predicate&& predicate )
      {
        return {
          { std::forward<Predicate>( predicate ), {} }
        };
      }
      template<typename Predicate, typename... Args>
      PACE__NODISCARD PACE__FORCEINLINE constexpr
        typename std::enable_if<traits::is_invocable_r<bool, Predicate, Args...>::value,
                                Currying<Choice, Predicate, Nop, void, Args...>>::type
        unless( Predicate&& predicate, Args&&... args )
      {
        return {
          { std::forward<Predicate>( predicate ), {}, std::forward<Args>( args )... }
        };
      }
      template<typename Predicate, typename Emission, typename... Args>
      PACE__NODISCARD PACE__FORCEINLINE constexpr
        typename std::enable_if<traits::is_invocable_r<bool, Predicate, Args...>::value,
                                Choice<Predicate, Nop, Emission, Args...>>::type
        unless( Predicate&& predicate, Emission&& emission, Args&&... args )
      {
        return {
          { std::forward<Predicate>( predicate ),
           {},
           std::forward<Emission>( emission ),
           std::forward<Args>( args )... }
        };
      }
      PACE__NODISCARD PACE__FORCEINLINE PACE__CXX14_CNSTXPR Currying<Choice, utils::Identity, Nop, void, bool>
        unless( bool condition ) noexcept
      {
        return {
          { {}, {}, condition }
        };
      }
      template<typename Emission>
      PACE__NODISCARD PACE__FORCEINLINE constexpr Choice<utils::Identity, Nop, Emission, bool> unless(
        bool condition,
        Emission&& emission )
      {
        return {
          { {}, {}, std::forward<Emission>( emission ), condition }
        };
      }

      //////////////////////////////////////////////////

      template<typename Predicate, typename Emission, typename... Args>
      struct Until {
        std::tuple<Predicate, Emission, Args...> value;

        PACE__FORCEINLINE friend PACE__CXX23_CNSTXPR CharPipeline& operator<<( CharPipeline& pipeline,
                                                                               const Until& self )
        {
          utils::apply(
            [&pipeline]( const Predicate& pred, const Emission& emi, const Args&... args ) {
              while ( utils::invoke( pred, args... ) )
                pipeline << emi;
            },
            self.value );
          return pipeline;
        }
        PACE__FORCEINLINE friend PACE__CXX23_CNSTXPR CharPipeline& operator<<( CharPipeline& pipeline,
                                                                               Until&& self )
        {
          utils::apply(
            [&pipeline]( Predicate&& pred, Emission&& emi, Args&&... args ) {
              while ( utils::invoke( std::forward<Predicate>( pred ), std::forward<Args>( args )... ) )
                pipeline << emi;
            },
            std::move( self.value ) );
          return pipeline;
        }
      };

      template<typename Pred>
      struct Currying<Until, Pred> {
        std::tuple<Pred> capture;

        template<typename Emission, typename... Args>
        PACE__NODISCARD PACE__FORCEINLINE constexpr Until<Pred, Emission, Args...> operator()(
          Emission&& emission,
          Args&&... args ) const&
        {
          return {
            { std::get<0>( capture ), std::forward<Emission>( emission ), std::forward<Args>( args )... }
          };
        }
        template<typename Emission, typename... Args>
        PACE__NODISCARD PACE__FORCEINLINE PACE__CXX14_CNSTXPR Until<Pred, Emission, Args...> operator()(
          Emission&& emission,
          Args&&... args ) &&
        {
          return {
            { std::forward<Pred>( std::get<0>( capture ) ),
             std::forward<Emission>( emission ),
             std::forward<Args>( args )... }
          };
        }
      };
      template<typename Pred, typename... Args>
      struct Currying<Until, Pred, Args...> {
        std::tuple<Pred, Args...> capture;

        template<typename Emission>
        PACE__NODISCARD PACE__FORCEINLINE constexpr Until<Pred, Emission, Args...> operator()(
          Emission&& emission ) const&
        {
          return utils::apply(
            [&emission]( const Pred& pred, const Args&... args ) {
              return Until<Pred, Emission, Args...> {
                { pred, std::forward<Emission>( emission ), args... }
              };
            },
            capture );
        }
        template<typename Emission>
        PACE__NODISCARD PACE__FORCEINLINE PACE__CXX14_CNSTXPR Until<Pred, Emission, Args...> operator()(
          Emission&& emission ) &&
        {
          return utils::apply(
            [&emission]( Pred&& pred, Args&&... args ) {
              return Until<Pred, Emission, Args...> {
                { std::forward<Pred>( pred ),
                 std::forward<Emission>( emission ),
                 std::forward<Args>( args )... }
              };
            },
            std::move( capture ) );
        }
      };

      template<typename Predicate>
      PACE__NODISCARD PACE__FORCEINLINE constexpr Currying<Until, Predicate> until( Predicate&& predicate )
      { return { { std::forward<Predicate>( predicate ) } }; }
      template<typename Predicate, typename... Args>
      PACE__NODISCARD PACE__FORCEINLINE constexpr
        typename std::enable_if<traits::is_invocable_r<bool, Predicate, Args...>::value,
                                Currying<Until, Predicate, Args...>>::type
        until( Predicate&& predicate, Args&&... args )
      {
        return {
          { std::forward<Predicate>( predicate ), std::forward<Args>( args )... }
        };
      }
      template<typename Predicate, typename Emission, typename... Args>
      PACE__NODISCARD PACE__FORCEINLINE constexpr
        typename std::enable_if<traits::is_invocable_r<bool, Predicate, Args...>::value,
                                Until<Predicate, Emission, Args...>>::type
        until( Predicate&& predicate, Emission&& emission, Args&&... args )
      {
        return {
          { std::forward<Predicate>( predicate ),
           std::forward<Emission>( emission ),
           std::forward<Args>( args )... }
        };
      }

      struct _count_guard {
        std::size_t iteration = 0;

        PACE__NODISCARD PACE__FORCEINLINE bool operator()( std::size_t times ) noexcept
        {
          if ( iteration++ < times )
            return true;
          iteration = 0;
          return false;
        }
      };
      PACE__NODISCARD PACE__FORCEINLINE PACE__CXX14_CNSTXPR Currying<Until, _count_guard, std::size_t> repeat(
        std::size_t times ) noexcept
      {
        return {
          { {}, times }
        };
      }
      template<typename Emission>
      PACE__NODISCARD PACE__FORCEINLINE constexpr auto repeat( std::size_t times, Emission&& emission )
        -> decltype( repeat( times )( std::forward<Emission>( emission ) ) )
      { return repeat( times )( std::forward<Emission>( emission ) ); }
    } // namespace io
  } // namespace details
} // namespace pace

#endif
