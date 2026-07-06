#ifndef PACE_ASSEMBLER
#define PACE_ASSEMBLER

#include "../../prefab/BasicConfig.hpp"

namespace pace {
  namespace details {
    namespace render {
      template<typename Config>
      struct Assembler;
      template<template<typename...> class... Facades>
      struct Assembler<prefab::BasicConfig<Facades...>> : public prefab::BasicConfig<Facades...> {
      private:
        using Base = prefab::BasicConfig<Facades...>;

      protected:
        template<template<typename...> class... Compnts>
        PACE__NODISCARD PACE__FORCEINLINE bool any_more() const noexcept
        {
#if PACE__CXX17
          return ( this->projection_.test( traits::IndexIn<Compnts, Facades...>::value ) || ... );
#else
          bool existance = false;
          (void)std::initializer_list<bool> { (
            ( existance |= this->projection_.test( traits::IndexIn<Compnts, Facades...>::value ) ),
            false )... };
          return existance;
#endif
        }

        template<template<typename...> class... Es>
        typename std::enable_if<( sizeof...( Es ) == 0 )>::type render_each( io::CharPipeline&,
                                                                             const Parameter& ) const noexcept
        {}
        template<template<typename...> class Element, template<typename...> class... Elements>
        void render_each( io::CharPipeline& pipeline, const Parameter& params ) const
        {
          // Before the first element and the last element, we do not set a divider.
          if ( this->projection_.test( traits::IndexIn<Element, Facades...>::value ) ) {
            pipeline << this->clear_then_dye(
              console::Dualcolor( this->info_forecolor_, this->info_backcolor_ ),
              params.style_off_ );
            this->traits::BaseOf_t<typename Base::layout, Element>::build( pipeline, params );
            if ( any_more<Elements...>() )
              pipeline << this->clear_then_dye(
                console::Dualcolor( this->info_forecolor_, this->info_backcolor_ ),
                params.style_off_ )
                       << this->divider_;
          }
          render_each<Elements...>( pipeline, params );
        }

      public:
        using Base::Base;
        constexpr Assembler( const Base& config ) noexcept( std::is_nothrow_copy_constructible<Base>::value )
          : Base( config )
        {}
        constexpr Assembler( Base&& config ) noexcept : Base( std::move( config ) ) {}

        std::uint64_t fixed_width() const noexcept override
        {
          details::concurrent::SharedLock<details::concurrent::SharedMutex> lock { this->rw_mtx_ };
          details::types::Size num_enabled = 0;
          std::uint64_t width              = 0;
          (void)std::initializer_list<bool> { (
            num_enabled += this->projection_.test( details::traits::IndexIn<Facades, Facades...>::value ),
            width += ( this->projection_.test( details::traits::IndexIn<Facades, Facades...>::value )
                         ? details::traits::BaseOf_t<typename Base::layout, Facades>::fixed_length()
                         : 0 ),
            false )... };
          // Before the first element and the last element, we do not set a divider.
          return width
               + details::traits::BaseOf_t<typename Base::layout, details::aspects::Prefix>::fixed_length()
               + details::traits::BaseOf_t<typename Base::layout, details::aspects::Postfix>::fixed_length()
               + details::traits::BaseOf_t<typename Base::layout, details::aspects::Segment>::fixed_length(
                   num_enabled );
        }
      };
    } // namespace render
  } // namespace details
} // namespace pace

#endif
