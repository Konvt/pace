#ifndef PGBAR_CAPACITY
#define PGBAR_CAPACITY

#include "../concurrent/SharedLock.hpp"
#include "../concurrent/SharedMutex.hpp"
#include "../wrappers/OptionPacket.hpp"
#include <mutex>

namespace pgbar {
  namespace option {
    // A wrapper that stores the number of quota.
    struct Quota : PGBAR__DERIVING_OPTION1( Quota, std::uint64_t, _num_quota );
  }

  namespace _details {
    namespace aspects {
      template<typename Base, typename Derived>
      class Capacity : public Base {
        friend PGBAR__FORCEINLINE PGBAR__CXX20_CNSTXPR void unpack( Capacity& self,
                                                                    option::Quota&& val ) noexcept
        {
          self.task_quota_ = val.value();
        }

      protected:
        std::uint64_t task_quota_;

        template<typename... Options>
        PGBAR__CXX14_CNSTXPR Capacity( traits::TypeSet<Options...> tag ) : Base( tag )
        {
          using OptionSet = traits::TypeSet<Options...>;
          if PGBAR__CXX17_CNSTXPR ( !traits::TpContain<OptionSet, option::Quota>::value )
            unpack( *this, utils::provide_for<Derived, option::Quota>() );
        }

        constexpr Capacity() = default;
        PGBAR__SPECIAL_MEMBERS_CX( Capacity, PGBAR__CXX14_CNSTXPR );

      public:
#define PGBAR__METHOD( ReturnType )                                \
  std::lock_guard<concurrent::SharedMutex> lock { this->rw_mtx_ }; \
  unpack( *this, option::Quota( _quota ) );                        \
  return static_cast<ReturnType>( *this )

        // Set the number of quota, passing in zero is no exception.
        Derived& quota( std::uint64_t _quota ) & noexcept
        {
          PGBAR__METHOD( Derived& );
        }
        Derived&& quota( std::uint64_t _quota ) && noexcept
        {
          PGBAR__METHOD( Derived&& );
        }
#undef PGBAR__METHOD

        // Get the current number of quota.
        PGBAR__NODISCARD std::uint64_t quota() const noexcept
        {
          concurrent::SharedLock<concurrent::SharedMutex> lock { this->rw_mtx_ };
          return task_quota_;
        }

        PGBAR__CXX14_CNSTXPR void swap( Capacity& other ) noexcept
        {
          std::swap( task_quota_, other.task_quota_ );
          Base::swap( other );
        }
      };

    } // namespace aspects

    PGBAR__OPTION_REGISTER( aspects::Capacity, option::Quota );
  } // namespace _details
} // namespace pgbar

#endif
