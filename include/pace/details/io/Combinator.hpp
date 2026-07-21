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
          Values&&... values ) const&
        {
          return utils::apply(
            [&]( const typename std::remove_reference<Captures>::type&... caps ) {
              return Comb<traits::PassAs_t<Values>...> { caps..., { std::forward<Values>( values )... } };
            },
            capture );
        }
        template<typename... Values>
        PACE__NODISCARD PACE__FORCEINLINE PACE__CXX14_CNSTXPR Comb<traits::PassAs_t<Values>...> operator()(
          Values&&... values ) &&
        {
          return utils::apply(
            [&]( Captures&&... caps ) {
              return Comb<traits::PassAs_t<Values>...> { std::forward<Captures>( caps )...,
                                                         { std::forward<Values>( values )... } };
            },
            std::move( capture ) );
        }
      };

      //////////////////////////////////////////////////

      template<typename... Emissions>
      struct Concat {
        static_assert( sizeof...( Emissions ) > 0, "nothing to concat" );

        std::tuple<Emissions...> emission;

        PACE__FORCEINLINE friend io::CharPipeline& operator<<( io::CharPipeline& pipeline,
                                                               const Concat& self )
        {
          utils::apply(
            [&pipeline]( const typename std::remove_reference<Emissions>::type&... emis ) {
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
      PACE__NODISCARD PACE__FORCEINLINE PACE__CXX14_CNSTXPR Currying<Concat> concat() noexcept
      { return {}; }
      template<typename... Args>
      PACE__NODISCARD PACE__FORCEINLINE constexpr Concat<traits::PassAs_t<Args>...> concat( Args&&... args )
      { return { { std::forward<Args>( args )... } }; }

      //////////////////////////////////////////////////

      template<typename Separator, typename Emission, typename... Emissions>
      struct Join {
        Separator separator;
        std::tuple<Emission, Emissions...> emission;

        template<typename Stream>
        PACE__FORCEINLINE friend constexpr Stream& operator<<( Stream& stream, const Join& self )
        {
          std::apply(
            [&]( const auto& emit, const auto&... emis ) constexpr {
              stream << emit;
              (void)std::initializer_list<bool> { ( ( stream << self.separator << emis ), false )... };
            },
            self.emission );
          return stream;
        }
        template<typename Stream>
        PACE__FORCEINLINE friend constexpr Stream& operator<<( Stream& stream, Join&& self )
        {
          std::apply(
            [&]( Emission&& emit, Emissions&&... emis ) constexpr {
              stream << std::forward<Emission>( emit );
              (void)std::initializer_list<bool> {
                ( ( stream << self.separator << std::forward<Emissions>( emis ) ), false )...
              };
            },
            std::move( self.emission ) );
          return stream;
        }
      };

      template<typename Sep>
      struct Currying<Join, Sep> {
        std::tuple<Sep> capture;

        template<typename... Args>
        PACE__NODISCARD PACE__FORCEINLINE constexpr Join<Sep, traits::PassAs_t<Args>...> operator()(
          Args&&... args ) const&
        { return { std::get<0>( capture ), { std::forward<Args>( args )... } }; }
        template<typename... Args>
        PACE__NODISCARD PACE__FORCEINLINE constexpr Join<Sep, traits::PassAs_t<Args>...> operator()(
          Args&&... args ) &&
        { return { std::move( std::get<0>( capture ) ), { std::forward<Args>( args )... } }; }
      };

      template<typename Separator>
      PACE__NODISCARD PACE__FORCEINLINE constexpr Currying<Join, traits::PassAs_t<Separator>> join(
        Separator&& separator ) noexcept
      { return { { std::forward<Separator>( separator ) } }; }
      template<typename Separator, typename... Args>
      PACE__NODISCARD PACE__FORCEINLINE constexpr Join<Separator, traits::PassAs_t<Args>...> join(
        Separator&& separator,
        Args&&... args )
      { return { std::forward<Separator>( separator ), { std::forward<Args>( args )... } }; }

      //////////////////////////////////////////////////

      template<typename Emission>
      struct When {
        bool condition;
        Emission emission;

        PACE__FORCEINLINE friend CharPipeline& operator<<( CharPipeline& pipeline, const When& self )
        {
          if ( self.condition )
            pipeline << self.emission;
          return pipeline;
        }
        PACE__FORCEINLINE friend CharPipeline& operator<<( CharPipeline& pipeline, When&& self )
        {
          if ( self.condition )
            pipeline << std::move( self.emission );
          return pipeline;
        }
      };
      PACE__NODISCARD PACE__FORCEINLINE PACE__CXX14_CNSTXPR Currying<When, bool> when(
        bool condition ) noexcept
      { return { { condition } }; }
      template<typename Emission>
      PACE__NODISCARD PACE__FORCEINLINE constexpr When<traits::PassAs_t<Emission>> when( bool condition,
                                                                                         Emission&& emission )
      { return { condition, { std::forward<Emission>( emission ) } }; }

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

        PACE__FORCEINLINE friend CharPipeline& operator<<( CharPipeline& pipeline, const Repeat& self )
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
