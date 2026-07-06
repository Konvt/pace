#ifndef PACE_REVERSIBLE
#define PACE_REVERSIBLE

#include "../../config/Provider.hpp"
#include "../concurrent/SharedLock.hpp"
#include "../concurrent/SharedMutex.hpp"
#include "../wrappers/OptionPacket.hpp"
#include <mutex>

namespace pace {
  namespace option {
    // A wrapper that stores the flag of direction.
    struct Reversed : PACE__DERIVING_OPTION2( Reversed, bool, _enable );
  }

  namespace details {
    namespace aspects {
      template<typename Base, typename Derived>
      class Reversible : public Base {
        friend PACE__FORCEINLINE PACE__CXX20_CNSTXPR void unpack( Reversible& self,
                                                                  option::Reversed&& val ) noexcept
        { self.reversed_ = val.value(); }

      protected:
        bool reversed_;

        template<typename... Options>
        PACE__CXX14_CNSTXPR Reversible( traits::TypeSet<Options...> tag ) : Base( tag )
        {
          using OptionSet = traits::TypeSet<Options...>;
          if PACE__CXX17_CNSTXPR ( !traits::TpContains<OptionSet, option::Reversed>::value )
            unpack( *this, config::provide_for<Derived, option::Reversed>() );
        }

        constexpr Reversible() = default;
        PACE__SPECIAL_MEMBERS_CX( Reversible, PACE__CXX14_CNSTXPR );

      public:
#define PACE__METHOD( ReturnType )                                 \
  std::lock_guard<concurrent::SharedMutex> lock { this->rw_mtx_ }; \
  unpack( *this, option::Reversed( flag ) );                       \
  return static_cast<ReturnType>( *this )

        Derived& reverse( bool flag ) & noexcept
        { PACE__METHOD( Derived& ); }
        Derived&& reverse( bool flag ) && noexcept
        { PACE__METHOD( Derived&& ); }

#undef PACE__METHOD

        PACE__NODISCARD bool reverse() const noexcept
        {
          concurrent::SharedLock<concurrent::SharedMutex> lock { this->rw_mtx_ };
          return reversed_;
        }

        PACE__CXX20_CNSTXPR void swap( Reversible& other ) noexcept
        {
          std::swap( reversed_, other.reversed_ );
          Base::swap( other );
        }
      };

    } // namespace aspects

    PACE__OPTION_REGISTER( aspects::Reversible, option::Reversed );
  } // namespace details
} // namespace pace

#endif
