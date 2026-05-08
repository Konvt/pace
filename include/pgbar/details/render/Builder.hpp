#ifndef PGBAR_BUILDER
#define PGBAR_BUILDER

#include "Assembler.hpp"

namespace pgbar {
  namespace _details {
    namespace render {
      template<typename Config>
      struct Builder;
      template<template<typename...> class... Facades>
      struct Builder<prefabs::BasicConfig<Facades...>> final
        : public Assembler<prefabs::BasicConfig<Facades...>> {
      private:
        using Config = prefabs::BasicConfig<Facades...>;
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
          if ( !this->prefix_.empty() || !this->postfix_.empty() || this->projection_.any() ) {
            pipeline << this->with_style( this->info_col_, params.style_off_ ) << this->l_border_;
          }
          this->traits::BaseOf_t<typename Config::Layout, aspects::Prefix>::build( pipeline, params );

          this->template render_each<Facades...>( pipeline, params );

          this->traits::BaseOf_t<typename Config::Layout, aspects::Postfix>::build( pipeline, params );
          if ( !this->prefix_.empty() || !this->postfix_.empty() || this->projection_.any() ) {
            pipeline << this->reset_then_style( this->info_col_, params.style_off_ ) << this->r_border_;
          }
          pipeline << this->with_reset( params.style_off_ );
          return pipeline;
        }
      };
    } // namespace render
  } // namespace _details
} // namespace pgbar

#endif
