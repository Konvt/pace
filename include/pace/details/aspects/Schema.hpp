#ifndef PACE_SCHEMA
#define PACE_SCHEMA

#include "../concurrent/SharedMutex.hpp"
#include "../traits/TypeSet.hpp"

namespace pace {
  namespace details {
    namespace aspects {
      /**
       * The top type of each configuration type.
       * It will be injected into the C3 calculation result as a top-level non-template base class.
       */
      class Schema {
      protected:
        mutable concurrent::SharedMutex rw_mtx_;

        template<typename... Args>
        Schema( traits::TypeSet<Args...> ) noexcept
        {}

        Schema() = default;
        Schema( const Schema& ) noexcept {}
        Schema( Schema&& ) noexcept {}

        Schema& operator=( const Schema& ) & noexcept { return *this; }
        Schema& operator=( Schema&& ) & noexcept { return *this; }
        ~Schema() = default;
      };
    } // namespace aspects
  } // namespace details
} // namespace pace

#endif
