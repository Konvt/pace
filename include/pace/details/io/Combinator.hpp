#ifndef PACE_IO_COMBINATOR
#define PACE_IO_COMBINATOR

#include "../traits/Util.hpp"
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
            [&]( traits::AsConst_t<Captures>... caps ) {
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

      struct Nop {
        PACE__FORCEINLINE friend constexpr CharPipeline& operator<<( CharPipeline& pipeline, const Nop& )
        { return pipeline; }
      };
      PACE__NODISCARD PACE__FORCEINLINE constexpr Nop nop() noexcept
      { return {}; }

      //////////////////////////////////////////////////

      template<typename Predicate, typename Positive, typename Negative, typename... Args>
      struct Choice {
        // The tuple supports EBO, thus we put them together.
        std::tuple<Predicate, Positive, Negative, Args...> value;

        PACE__FORCEINLINE friend PACE__CXX23_CNSTXPR CharPipeline& operator<<( CharPipeline& pipeline,
                                                                               const Choice& self )
        {
          return utils::apply(
            [&pipeline]( traits::AsConst_t<Predicate> pred,
                         traits::AsConst_t<Positive> pos,
                         traits::AsConst_t<Negative> neg,
                         traits::AsConst_t<Args>... args ) {
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
            [&]( traits::AsConst_t<Pred> pred, traits::AsConst_t<Args>... args ) {
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
      PACE__NODISCARD PACE__FORCEINLINE constexpr Currying<Choice, Predicate> choice(
        Predicate&& predicate ) noexcept
      { return { { std::forward<Predicate>( predicate ) } }; }
      template<typename Predicate, typename... Args>
      PACE__NODISCARD PACE__FORCEINLINE constexpr typename std::enable_if<
        traits::AnyOf<
          traits::is_invocable_r<bool, Predicate, Args...>,
          traits::is_invocable_r<bool, traits::AsConst_t<Predicate>, traits::AsConst_t<Args>...>>::value,
        Currying<Choice, Predicate, void, void, Args...>>::type
        choice( Predicate&& predicate, Args&&... args ) noexcept
      {
        return {
          { std::forward<Predicate>( predicate ), std::forward<Args>( args )... }
        };
      }
      template<typename Predicate, typename Positive, typename Negative, typename... Args>
      PACE__NODISCARD PACE__FORCEINLINE constexpr typename std::enable_if<
        traits::AnyOf<
          traits::is_invocable_r<bool, Predicate, Args...>,
          traits::is_invocable_r<bool, traits::AsConst_t<Predicate>, traits::AsConst_t<Args>...>>::value,
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
            [&positive]( traits::AsConst_t<Pred> pred,
                         traits::AsConst_t<Neg> neg,
                         traits::AsConst_t<Args>... args ) {
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
        Predicate&& predicate ) noexcept
      {
        return {
          { std::forward<Predicate>( predicate ), {} }
        };
      }
      template<typename Predicate, typename... Args>
      PACE__NODISCARD PACE__FORCEINLINE constexpr typename std::enable_if<
        traits::AnyOf<
          traits::is_invocable_r<bool, Predicate, Args...>,
          traits::is_invocable_r<bool, traits::AsConst_t<Predicate>, traits::AsConst_t<Args>...>>::value,
        Currying<Choice, Predicate, void, Nop, Args...>>::type
        when( Predicate&& predicate, Args&&... args ) noexcept
      {
        return {
          { std::forward<Predicate>( predicate ), {}, std::forward<Args>( args )... }
        };
      }
      template<typename Predicate, typename Emission, typename... Args>
      PACE__NODISCARD PACE__FORCEINLINE constexpr typename std::enable_if<
        traits::AnyOf<
          traits::is_invocable_r<bool, Predicate, Args...>,
          traits::is_invocable_r<bool, traits::AsConst_t<Predicate>, traits::AsConst_t<Args>...>>::value,
        Choice<Predicate, Emission, Nop, Args...>>::type
        when( Predicate&& predicate, Emission&& emission, Args&&... args )
      {
        return {
          { std::forward<Predicate>( predicate ),
           std::forward<Emission>( emission ),
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
            [&negative]( traits::AsConst_t<Pred> pred,
                         traits::AsConst_t<Pos> pos,
                         traits::AsConst_t<Args>... args ) {
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
        Predicate&& predicate ) noexcept
      {
        return {
          { std::forward<Predicate>( predicate ), {} }
        };
      }
      template<typename Predicate, typename... Args>
      PACE__NODISCARD PACE__FORCEINLINE constexpr typename std::enable_if<
        traits::AnyOf<
          traits::is_invocable_r<bool, Predicate, Args...>,
          traits::is_invocable_r<bool, traits::AsConst_t<Predicate>, traits::AsConst_t<Args>...>>::value,
        Currying<Choice, Predicate, Nop, void, Args...>>::type
        unless( Predicate&& predicate, Args&&... args ) noexcept
      {
        return {
          { std::forward<Predicate>( predicate ), {}, std::forward<Args>( args )... }
        };
      }
      template<typename Predicate, typename Emission, typename... Args>
      PACE__NODISCARD PACE__FORCEINLINE constexpr typename std::enable_if<
        traits::AnyOf<
          traits::is_invocable_r<bool, Predicate, Args...>,
          traits::is_invocable_r<bool, traits::AsConst_t<Predicate>, traits::AsConst_t<Args>...>>::value,
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

      template<typename Emission>
      struct Repeat {
      private:
        template<typename T, typename = void>
        struct is_directly_emitting : std::false_type {};
        template<typename T>
        struct is_directly_emitting<T,
                                    typename std::enable_if<std::is_same<
                                      decltype( std::declval<CharPipeline&>()
                                                  .append( std::declval<T>(), std::declval<std::size_t>() ) ),
                                      CharPipeline&>::value>::type> : std::true_type {};

        template<typename T>
        PACE__FORCEINLINE void emit( std::true_type, CharPipeline& pipeline, const T& val ) const&
        { pipeline.append( val, std::get<0>( value ) ); }
        template<typename T>
        PACE__FORCEINLINE void emit( std::false_type, CharPipeline& pipeline, const T& val ) const&
        {
          for ( std::size_t i = 0; i < std::get<0>( value ); ++i )
            pipeline << val;
        }
        template<typename T>
        PACE__FORCEINLINE void emit( std::true_type, CharPipeline& pipeline, T&& val ) &&
        { pipeline.append( val, std::get<0>( value ) ); }
        template<typename T>
        PACE__FORCEINLINE void emit( std::false_type, CharPipeline& pipeline, T&& val ) &&
        {
          for ( std::size_t i = 0; i < std::get<0>( value ); ++i )
            pipeline << val;
        }

      public:
        std::tuple<std::size_t, Emission> value;

        PACE__FORCEINLINE friend CharPipeline& operator<<( CharPipeline& pipeline, const Repeat& self )
        {
          self.emit( is_directly_emitting<Emission>(), pipeline, std::get<1>( self.value ) );
          return pipeline;
        }
        PACE__FORCEINLINE friend CharPipeline& operator<<( CharPipeline& pipeline, Repeat&& self )
        {
          std::move( self ).emit( is_directly_emitting<Emission>(),
                                  pipeline,
                                  std::forward<Emission>( std::get<1>( self.value ) ) );
          return pipeline;
        }
      };
      PACE__NODISCARD PACE__FORCEINLINE PACE__CXX14_CNSTXPR Currying<Repeat, std::size_t> repeat(
        std::size_t times ) noexcept
      { return { { times } }; }
      template<typename Emission>
      PACE__NODISCARD PACE__FORCEINLINE constexpr Repeat<Emission> repeat( std::size_t times,
                                                                           Emission&& emission )
      {
        return {
          { times, std::forward<Emission>( emission ) }
        };
      }

      //////////////////////////////////////////////////

      template<typename... Emissions>
      struct Concat {
        static_assert( sizeof...( Emissions ) > 0, "nothing to concat" );

        std::tuple<Emissions...> emission;

        PACE__FORCEINLINE friend io::CharPipeline& operator<<( io::CharPipeline& pipeline,
                                                               const Concat& self )
        {
          utils::apply(
            [&pipeline]( traits::AsConst_t<Emissions>... emis ) {
              (void)std::initializer_list<bool> { ( pipeline << emis, false )... };
            },
            self.emission );
          return pipeline;
        }
        PACE__FORCEINLINE friend io::CharPipeline& operator<<( io::CharPipeline& pipeline, Concat&& self )
        {
          utils::apply(
            [&pipeline]( Emissions&&... emis ) {
              (void)std::initializer_list<bool> { ( pipeline << std::forward<Emissions>( emis ), false )... };
            },
            std::move( self.emission ) );
          return pipeline;
        }
      };
      template<>
      struct Concat<> : public Nop {
        std::tuple<> emission;
      };
      template<typename... Args>
      PACE__NODISCARD PACE__FORCEINLINE constexpr Concat<Args...> concat( Args&&... args )
      { return { { std::forward<Args>( args )... } }; }

      //////////////////////////////////////////////////

      template<typename Separator, typename Emission, typename... Emissions>
      struct Join {
        std::tuple<Separator, Emission, Emissions...> value;

        template<typename CharPipeline>
        PACE__FORCEINLINE friend PACE__CXX14_CNSTXPR CharPipeline& operator<<( CharPipeline& pipeline,
                                                                               const Join& self )
        {
          utils::apply(
            [&pipeline]( traits::AsConst_t<Separator> sep,
                         traits::AsConst_t<Emission> emi,
                         traits::AsConst_t<Emissions>... emis ) {
              pipeline << emi;
              (void)std::initializer_list<bool> { ( ( pipeline << sep << emis ), false )... };
            },
            self.value );
          return pipeline;
        }
        template<typename CharPipeline>
        PACE__FORCEINLINE friend PACE__CXX14_CNSTXPR CharPipeline& operator<<( CharPipeline& pipeline,
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
      PACE__NODISCARD PACE__FORCEINLINE constexpr Currying<Join, Separator> join(
        Separator&& separator ) noexcept
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
    } // namespace io
  } // namespace details
} // namespace pace

#endif
