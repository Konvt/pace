#ifndef PACE_BUILDER
#define PACE_BUILDER

#include "Assembler.hpp"

namespace pace {
  namespace details {
    namespace render {
      template<typename Config>
      struct Builder;
      template<template<typename...> class... Facades>
      struct Builder<prefab::BasicConfig<Facades...>> final
        : public Assembler<prefab::BasicConfig<Facades...>> {
      private:
        using Config = prefab::BasicConfig<Facades...>;
        using Base   = Assembler<Config>;
        static_assert( traits::AllOf<traits::is_instance_of<Config, aspects::Prefix>,
                                     traits::is_instance_of<Config, aspects::Postfix>,
                                     traits::is_instance_of<Config, aspects::Segment>>::value,
                       "the config type must contains the Prefix, Postfix and Segment" );

      public:
        using Base::Base;
        io::CharPipeline& build( io::CharPipeline& pipeline, Parameter params ) const
        {
          concurrent::SharedLock<concurrent::SharedMutex> lock { this->rw_mtx_ };
          this->font_effect( pipeline, params.style_off_ );

          if ( !this->prefix_.empty() || !this->postfix_.empty() || this->projection_.any() ) {
            pipeline << this->with_dye( this->info_col_, params.style_off_ ) << this->l_border_;
          }
          this->traits::BaseOf_t<typename Config::Layout, aspects::Prefix>::build( pipeline, params );

          this->template render_each<Facades...>( pipeline, params );

          this->traits::BaseOf_t<typename Config::Layout, aspects::Postfix>::build( pipeline, params );
          if ( !this->prefix_.empty() || !this->postfix_.empty() || this->projection_.any() ) {
            pipeline << this->clear_then_dye( this->info_col_, params.style_off_ ) << this->r_border_;
          }

          return this->reset_style( pipeline, params.style_off_ );
        }
      };
    } // namespace render
  } // namespace details
} // namespace pace

#endif
