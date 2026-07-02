#ifndef PACE_MANAGED_BAR
#define PACE_MANAGED_BAR

#include "../../prefab/BasicBar.hpp"
#include <memory>

namespace pace {
  namespace details {
    namespace assets {
      template<Channel, Policy, Region>
      class DynamicLayout;

      template<typename C, Channel S, Policy M, Region Z>
      class ManagedBar final : public prefab::BasicBar<C, S, M, Z> {
        using Base    = prefab::BasicBar<C, S, M, Z>;
        using Context = std::shared_ptr<DynamicLayout<S, M, Z>>;

        Context context_;

        PACE__FORCEINLINE void do_halt( bool forced ) noexcept final { context_->pop( this, forced ); }
        PACE__FORCEINLINE void do_boot() & final { context_->append( this ); }

      public:
        ManagedBar( Context context, C&& config ) noexcept
          : Base( std::move( config ) ), context_ { std::move( context ) }
        {}
        ManagedBar( Context context, prefab::BasicBar<C, S, M, Z>&& bar ) noexcept
          : Base( std::move( bar ) ), context_ { std::move( context ) }
        {}
        template<typename... Args>
        ManagedBar( Context context, Args&&... args )
          : Base( std::forward<Args>( args )... ), context_ { std::move( context ) }
        {}

        // This thing is always wrapped by `std::unique_ptr` under normal circumstances,
        // so there is no need to add move semantics support for it;
        // otherwise, additional null checks would be required in the methods.
        ManagedBar( const ManagedBar& )            = delete;
        ManagedBar& operator=( const ManagedBar& ) = delete;

        /**
         * The object model of C++ requires that derived classes be destructed first.
         * When the derived class is destructed and the base class destructor attempts to call `abort`,
         * the internal virtual function `do_halt` will point to a non-existent derived class.

         * Therefore, here it is necessary to explicitly re-call the base class's `abort`
         * to shut down any possible running state.

         * After calling `abort`, the object state will switch to Stop,
         * and further calls to `abort` will have no effect.
         * So even if the base class destructor calls `abort` again, it is safe.
         */
        virtual ~ManagedBar() noexcept { this->abort(); }
      };
    } // namespace assets
  } // namespace details
} // namespace pace

#endif
