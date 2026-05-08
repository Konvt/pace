#ifndef PGBAR_REVERSIBLE
#define PGBAR_REVERSIBLE

#include "../../config/Provider.hpp"
#include "../concurrent/SharedLock.hpp"
#include "../concurrent/SharedMutex.hpp"
#include "../wrappers/OptionPacket.hpp"
#include <mutex>

namespace pgbar {
  namespace option {
    // A wrapper that stores the flag of direction.
    struct Reversed : PGBAR__DERIVING_OPTION1( Reversed, bool, _enable );
  }

  namespace details {
    namespace aspects {
      template<typename Base, typename Derived>
      class Reversible : public Base {
        friend PGBAR__FORCEINLINE PGBAR__CXX20_CNSTXPR void unpack( Reversible& self,
                                                                    option::Reversed&& val ) noexcept
        {
          self.reversed_ = val.value();
        }

      protected:
        bool reversed_;

        template<typename... Options>
        PGBAR__CXX14_CNSTXPR Reversible( traits::TypeSet<Options...> tag ) : Base( tag )
        {
          using OptionSet = traits::TypeSet<Options...>;
          if PGBAR__CXX17_CNSTXPR ( !traits::TpContain<OptionSet, option::Reversed>::value )
            unpack( *this, config::provide_for<Derived, option::Reversed>() );
        }

        constexpr Reversible() = default;
        PGBAR__SPECIAL_MEMBERS_CX( Reversible, PGBAR__CXX14_CNSTXPR );

      public:
#define PGBAR__METHOD( ReturnType )                                \
  std::lock_guard<concurrent::SharedMutex> lock { this->rw_mtx_ }; \
  unpack( *this, option::Reversed( flag ) );                       \
  return static_cast<ReturnType>( *this )

        Derived& reverse( bool flag ) & noexcept
        {
          PGBAR__METHOD( Derived& );
        }
        Derived&& reverse( bool flag ) && noexcept
        {
          PGBAR__METHOD( Derived&& );
        }

#undef PGBAR__METHOD

        PGBAR__NODISCARD bool reverse() const noexcept
        {
          concurrent::SharedLock<concurrent::SharedMutex> lock { this->rw_mtx_ };
          return reversed_;
        }

        PGBAR__CXX20_CNSTXPR void swap( Reversible& other ) noexcept
        {
          std::swap( reversed_, other.reversed_ );
          Base::swap( other );
        }
      };

    } // namespace aspects

    PGBAR__OPTION_REGISTER( aspects::Reversible, option::Reversed );
  } // namespace details
} // namespace pgbar

#endif
