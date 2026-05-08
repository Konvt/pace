#ifndef PGBAR_ASSEMBLER
#define PGBAR_ASSEMBLER

#include "../prefabs/BasicConfig.hpp"

namespace pgbar {
  namespace _details {
    namespace render {
      template<typename Config>
      struct Assembler;
      template<template<typename...> class... Facades>
      struct Assembler<prefabs::BasicConfig<Facades...>> : public prefabs::BasicConfig<Facades...> {
      private:
        using Base = prefabs::BasicConfig<Facades...>;

      protected:
        template<template<typename...> class... Compnts>
        PGBAR__NODISCARD PGBAR__FORCEINLINE bool any_more() const noexcept
        {
#if PGBAR__CXX17
          return ( this->projection_.test( traits::IndexIn<Compnts, Facades...>::value ) || ... );
#else
          bool existance = false;
          (void)std::initializer_list<bool> { (
            ( existance |= this->projection_.test( traits::IndexIn<Compnts, Facades...>::value ) ),
            false )... };
          return existance;
#endif
        }

        template<template<typename...> class... Elements>
        typename std::enable_if<sizeof...( Elements ) == 0>::type render_each(
          io::CharPipeline&,
          const Parameter& ) const noexcept
        {}
        template<template<typename...> class Element, template<typename...> class... Elements>
        void render_each( io::CharPipeline& pipeline, const Parameter& params ) const noexcept
        {
          // Before the first element and the last element, we do not set a divider.
          if ( this->projection_.test( traits::IndexIn<Element, Facades...>::value ) ) {
            pipeline << this->reset_then_style( this->info_col_, params.style_off_ );
            this->traits::BaseOf_t<typename Base::Layout, Element>::build( pipeline, params );
            if ( any_more<Elements...>() )
              pipeline << this->reset_then_style( this->info_col_, params.style_off_ ) << this->divider_;
          }
          render_each<Elements...>( pipeline, params );
        }

      public:
        using Base::Base;
        constexpr Assembler( const Base& config ) noexcept( std::is_nothrow_copy_constructible<Base>::value )
          : Base( config )
        {}
        constexpr Assembler( Base&& config ) noexcept : Base( std::move( config ) ) {}
      };
    } // namespace render
  } // namespace _details
} // namespace pgbar

#endif
