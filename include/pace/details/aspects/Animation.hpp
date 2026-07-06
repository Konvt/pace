#ifndef PACE_ANIMATION
#define PACE_ANIMATION

#include "../../config/Provider.hpp"
#include "../concurrent/SharedMutex.hpp"
#include "../wrappers/OptionPacket.hpp"
#include <mutex>

namespace pace {
  namespace option {
    /**
     * A wrapper that stores the rate factor for animation frame transitions.
     *
     * Controls the speed of per-frame animation updates:
     *
     * - Positive values accelerate the transition (higher -> faster).
     *
     * - Negative values decelerate the transition (lower -> slower).
     *
     * - Zero freezes the animation completely.
     *
     * The effective range is between -128 (slowest) and 127 (fastest).
     */
    struct Shift : PACE__DERIVING_OPTION2( Shift, std::int8_t, _shift_factor );
  }

  namespace details {
    namespace aspects {
      template<typename Base, typename Derived>
      class Animation : public Base {
        friend PACE__FORCEINLINE PACE__CXX14_CNSTXPR void unpack( Animation& self,
                                                                  option::Shift&& val ) noexcept
        { self.shift_factor_ = val.value() < 0 ? ( 1.0 / ( -val.value() ) ) : val.value(); }

      protected:
        types::Float shift_factor_;

        template<typename... Options>
        PACE__CXX14_CNSTXPR Animation( traits::TypeSet<Options...> tag ) : Base( tag )
        {
          using OptionSet = traits::TypeSet<Options...>;
          if PACE__CXX17_CNSTXPR ( !traits::TpContains<OptionSet, option::Shift>::value )
            unpack( *this, config::provide_for<Derived, option::Shift>() );
        }

        PACE__CXX20_CNSTXPR Animation() = default;
        PACE__SPECIAL_MEMBERS_CX( Animation, PACE__CXX20_CNSTXPR );

      public:
#define PACE__METHOD( ReturnType )                                 \
  std::lock_guard<concurrent::SharedMutex> lock { this->rw_mtx_ }; \
  unpack( *this, option::Shift( _shift_factor ) );                 \
  return static_cast<ReturnType>( *this )

        /**
         * Set the rate factor of the animation with negative value slowing down the switch per frame
         * and positive value speeding it up.
         *
         * The maximum and minimum of the rate factor is between -128 and 127.
         *
         * If the value is zero, freeze the animation.
         */
        Derived& shift( std::int8_t _shift_factor ) & noexcept
        { PACE__METHOD( Derived& ); }
        Derived&& shift( std::int8_t _shift_factor ) && noexcept
        { PACE__METHOD( Derived&& ); }

#undef PACE__METHOD

        PACE__CXX20_CNSTXPR void swap( Animation& other ) noexcept
        {
          using std::swap;
          swap( shift_factor_, other.shift_factor_ );
          Base::swap( other );
        }
      };
    } // namespace aspects

    PACE__OPTION_REGISTER( aspects::Animation, option::Shift );
  } // namespace details
} // namespace pace

#endif
