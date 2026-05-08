#ifndef PGBAR_MANAGED_BAR
#define PGBAR_MANAGED_BAR

#include "../prefabs/BasicBar.hpp"
#include <memory>

namespace pgbar {
  namespace _details {
    namespace prefabs {
      template<Channel, Policy, Region>
      class DynamicLayout;
    }

    namespace prefabs {
      template<typename C, Channel O, Policy M, Region A>
      class ManagedBar final : public BasicBar<C, O, M, A> {
        using Base    = BasicBar<C, O, M, A>;
        using Context = std::shared_ptr<prefabs::DynamicLayout<O, M, A>>;

        Context context_;

        PGBAR__FORCEINLINE void do_halt( bool forced ) noexcept final { context_->pop( this, forced ); }
        PGBAR__FORCEINLINE void do_boot() & final { context_->append( this ); }

      public:
        ManagedBar( Context context, C&& config ) noexcept
          : Base( std::move( config ) ), context_ { std::move( context ) }
        {}
        ManagedBar( Context context, BasicBar<C, O, M, A>&& bar ) noexcept
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
    } // namespace prefabs
  } // namespace _details
} // namespace pgbar

#endif
