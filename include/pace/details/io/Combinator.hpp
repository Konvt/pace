#ifndef PACE_IO_COMBINATOR
#define PACE_IO_COMBINATOR

#include "../traits/Util.hpp"
#include "CharPipeline.hpp"

namespace pace {
  namespace details {
    namespace io {
      template<template<typename...> class Comb, typename... Captures>
      struct Currying {
        std::tuple<Captures...> capture;

        template<typename... Values>
        PACE__NODISCARD PACE__FORCEINLINE constexpr Comb<traits::PassAs_t<Values>...> operator()(
          Values&&... vals ) const&
        {
          return utils::apply(
            [&]( const typename std::remove_reference<Captures>::type&... caps ) {
              return Comb<traits::PassAs_t<Values>...> { caps..., { std::forward<Values>( vals )... } };
            },
            capture );
        }
        template<typename... Values>
        PACE__NODISCARD PACE__FORCEINLINE PACE__CXX14_CNSTXPR Comb<traits::PassAs_t<Values>...> operator()(
          Values&&... vals ) &&
        {
          return utils::apply(
            [&]( Captures&&... caps ) {
              return Comb<traits::PassAs_t<Values>...> { std::forward<Captures>( caps )...,
                                                         { std::forward<Values>( vals )... } };
            },
            std::move( capture ) );
        }
      };

      template<typename... Emissions>
      struct Join {
        static_assert( sizeof...( Emissions ) > 0, "nothing to join" );

        std::tuple<Emissions...> value;

        friend PACE__FORCEINLINE io::CharPipeline& operator<<( io::CharPipeline& pipeline, const Join& self )
        {
          utils::apply(
            [&pipeline]( const typename std::remove_reference<Emissions>::type&... emis ) {
              (void)std::initializer_list<bool> { ( pipeline << emis, false )... };
            },
            self.value );
          return pipeline;
        }
        friend PACE__FORCEINLINE io::CharPipeline& operator<<( io::CharPipeline& pipeline, Join&& self )
        {
          utils::apply(
            [&pipeline]( Emissions&&... emis ) {
              (void)std::initializer_list<bool> { ( pipeline << std::forward<Emissions>( emis ), false )... };
            },
            std::move( self.value ) );
          return pipeline;
        }
      };
      PACE__NODISCARD PACE__FORCEINLINE PACE__CXX14_CNSTXPR Currying<Join> join() noexcept
      { return {}; }
      template<typename... Args>
      PACE__NODISCARD PACE__FORCEINLINE constexpr Join<traits::PassAs_t<Args>...> join( Args&&... args )
      { return { { std::forward<Args>( args )... } }; }

      template<typename Emission>
      struct When {
        bool cond;
        Emission emission;

        friend PACE__FORCEINLINE CharPipeline& operator<<( CharPipeline& pipeline, const When& self )
        {
          if ( self.cond )
            pipeline << self.emission;
          return pipeline;
        }
        friend PACE__FORCEINLINE CharPipeline& operator<<( CharPipeline& pipeline, When&& self )
        {
          if ( self.cond )
            pipeline << std::move( self.emission );
          return pipeline;
        }
      };
      PACE__NODISCARD PACE__FORCEINLINE PACE__CXX14_CNSTXPR Currying<When, bool> when( bool cond ) noexcept
      { return { { cond } }; }
      template<typename Emission>
      PACE__NODISCARD PACE__FORCEINLINE constexpr When<traits::PassAs_t<Emission>> when( bool cond,
                                                                                         Emission&& emission )
      { return { cond, { std::forward<Emission>( emission ) } }; }

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
        PACE__FORCEINLINE void emit( std::true_type, CharPipeline& pipeline, const T& val ) const
        { pipeline.append( val, times ); }
        template<typename T>
        PACE__FORCEINLINE void emit( std::false_type, CharPipeline& pipeline, const T& val ) const
        {
          for ( std::size_t i = 0; i < times; ++i )
            pipeline << val;
        }

      public:
        std::size_t times;
        Emission emission;

        friend PACE__FORCEINLINE CharPipeline& operator<<( CharPipeline& pipeline, const Repeat& self )
        {
          self.emit( is_directly_emitting<Emission>(), pipeline, self.emission );
          return pipeline;
        }
      };
      PACE__NODISCARD PACE__FORCEINLINE PACE__CXX14_CNSTXPR Currying<Repeat, std::size_t> repeat(
        std::size_t times ) noexcept
      { return { times }; }
      template<typename Emission>
      PACE__NODISCARD PACE__FORCEINLINE constexpr Repeat<traits::PassAs_t<Emission>> repeat(
        std::size_t times,
        Emission&& emission )
      { return { times, { std::forward<Emission>( emission ) } }; }
    } // namespace io
  } // namespace details
} // namespace pace

#endif
