#ifndef PACE_CAPACITY
#define PACE_CAPACITY

#include "../../config/Provider.hpp"
#include "../concurrent/SharedLock.hpp"
#include "../concurrent/SharedMutex.hpp"
#include "../wrappers/OptionPacket.hpp"
#include <mutex>

namespace pace {
  namespace option {
    // A wrapper that stores the number of quota.
    struct Quota : PACE__DERIVING_OPTION1( Quota, std::uint64_t, _num_quota );
  }

  namespace details {
    namespace aspects {
      template<typename Base, typename Derived>
      class Capacity : public Base {
        friend PACE__FORCEINLINE PACE__CXX20_CNSTXPR void unpack( Capacity& self,
                                                                  option::Quota&& val ) noexcept
        {
          self.task_quota_ = val.value();
        }

      protected:
        std::uint64_t task_quota_;

        template<typename... Options>
        PACE__CXX14_CNSTXPR Capacity( traits::TypeSet<Options...> tag ) : Base( tag )
        {
          using OptionSet = traits::TypeSet<Options...>;
          if PACE__CXX17_CNSTXPR ( !traits::TpContain<OptionSet, option::Quota>::value )
            unpack( *this, config::provide_for<Derived, option::Quota>() );
        }

        constexpr Capacity() = default;
        PACE__SPECIAL_MEMBERS_CX( Capacity, PACE__CXX14_CNSTXPR );

      public:
#define PACE__METHOD( ReturnType )                                 \
  std::lock_guard<concurrent::SharedMutex> lock { this->rw_mtx_ }; \
  unpack( *this, option::Quota( _quota ) );                        \
  return static_cast<ReturnType>( *this )

        // Set the number of quota, passing in zero is no exception.
        Derived& quota( std::uint64_t _quota ) & noexcept
        {
          PACE__METHOD( Derived& );
        }
        Derived&& quota( std::uint64_t _quota ) && noexcept
        {
          PACE__METHOD( Derived&& );
        }
#undef PACE__METHOD

        // Get the current number of quota.
        PACE__NODISCARD std::uint64_t quota() const noexcept
        {
          concurrent::SharedLock<concurrent::SharedMutex> lock { this->rw_mtx_ };
          return task_quota_;
        }

        PACE__CXX14_CNSTXPR void swap( Capacity& other ) noexcept
        {
          std::swap( task_quota_, other.task_quota_ );
          Base::swap( other );
        }
      };

    } // namespace aspects

    PACE__OPTION_REGISTER( aspects::Capacity, option::Quota );
  } // namespace details
} // namespace pace

#endif
