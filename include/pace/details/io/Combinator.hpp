#ifndef PACE_IO_COMBINATOR
#define PACE_IO_COMBINATOR

#include "../traits/Util.hpp"
#include "../wrappers/Emission.hpp"

namespace pace {
  namespace details {
    namespace io {
      template<template<typename...> class Comb, typename... Captures>
      struct Currying {
        std::tuple<Captures...> capture;

        template<typename... Values>
        PACE__FORCEINLINE constexpr Comb<traits::PassParam_t<Values>...> operator()( Values&&... vals ) const&
        {
          return utils::apply(
            [&]( const typename std::remove_reference<Captures>::type&... attrs ) {
              return Comb<traits::PassParam_t<Values>...> { attrs..., { std::forward<Values>( vals )... } };
            },
            capture );
        }
        template<typename... Values>
        PACE__FORCEINLINE PACE__CXX14_CNSTXPR Comb<traits::PassParam_t<Values>...> operator()(
          Values&&... vals ) &&
        {
          return utils::apply(
            [&]( Captures&&... attrs ) {
              return Comb<traits::PassParam_t<Values>...> { std::forward<Captures>( attrs )...,
                                                            { std::forward<Values>( vals )... } };
            },
            std::move( capture ) );
        }
      };

      template<typename... Values>
      struct When {
        static_assert( sizeof...( Values ) > 0, "nothing to skip" );

        bool cond;
        wrappers::Emission<Values...> emission;

        friend PACE__FORCEINLINE CharPipeline& operator<<( CharPipeline& pipeline, const When& self )
        {
          if ( self.cond )
            pipeline << self.emission;
          return pipeline;
        }
      };
      PACE__NODISCARD PACE__FORCEINLINE PACE__CXX14_CNSTXPR Currying<When, bool> when( bool cond ) noexcept
      { return { { cond } }; }
      template<typename... Args>
      PACE__NODISCARD PACE__FORCEINLINE constexpr When<traits::PassParam_t<Args>...> when( bool cond,
                                                                                           Args&&... args )
      { return { cond, { std::forward<Args>( args )... } }; }

      template<typename... Values>
      struct Repeat {
        static_assert( sizeof...( Values ) > 0, "nothing to repeat" );

        std::size_t times;
        wrappers::Emission<Values...> emission;
      };
      // Due to the redefinition, we have to write those operator<< externally.
      template<typename Val>
      PACE__FORCEINLINE auto operator<<( CharPipeline& pipeline, const Repeat<Val>& self ) ->
        typename std::enable_if<
          std::is_same<decltype( pipeline.append( std::declval<Val>(), std::declval<std::size_t>() ) ),
                       CharPipeline&>::value,
          CharPipeline&>::type
      { return pipeline.append( std::get<0>( self.emission.value ), self.times ); }
      template<typename Val>
      PACE__FORCEINLINE auto operator<<( CharPipeline& pipeline, const Repeat<Val>& self ) ->
        typename std::enable_if<
          std::is_same<decltype( pipeline.apply( std::declval<Val>(), std::declval<std::size_t>() ) ),
                       CharPipeline&>::value,
          CharPipeline&>::type
      { return pipeline.apply( std::get<0>( self.emission.value ), self.times ); }
      template<typename... Vals>
      PACE__FORCEINLINE CharPipeline& operator<<( CharPipeline& pipeline, const Repeat<Vals...>& self )
      {
        for ( std::size_t i = 0; i < self.times; ++i )
          pipeline << self.emission;
        return pipeline;
      }
      PACE__NODISCARD PACE__FORCEINLINE PACE__CXX14_CNSTXPR Currying<Repeat, std::size_t> repeat(
        std::size_t times ) noexcept
      { return { times }; }
      template<typename... Args>
      PACE__NODISCARD PACE__FORCEINLINE constexpr Repeat<traits::PassParam_t<Args>...> repeat(
        std::size_t times,
        Args&&... args )
      { return { times, { std::forward<Args>( args )... } }; }
    } // namespace io
  } // namespace details
} // namespace pace

#endif
