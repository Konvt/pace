#ifndef PACE_TYPE_HASH
#define PACE_TYPE_HASH

#include <cstdint>
#include <type_traits>

namespace pace {
  namespace details {
    namespace traits {
      template<typename T>
      struct TypeHash32 {
      private:
        static constexpr std::uint32_t avalanche( std::uint32_t hash ) noexcept
        {
#if __cpp_constexpr >= 201304L
          hash ^= hash >> 16;
          hash *= 0x85EBCA6Bu;
          hash ^= hash >> 13;
          hash *= 0xC2B2AE35u;
          hash ^= hash >> 16;
          return hash;
#else
          return ( ( ( ( ( hash ^ ( hash >> 16 ) ) * 0x85EBCA6Bu )
                       ^ ( ( ( hash ^ ( hash >> 16 ) ) * 0x85EBCA6Bu ) >> 13 ) )
                     * 0xC2B2AE35u )
                   ^ ( ( ( ( hash ^ ( hash >> 16 ) ) * 0x85EBCA6Bu )
                         ^ ( ( ( hash ^ ( hash >> 16 ) ) * 0x85EBCA6Bu ) >> 13 ) )
                         * 0xC2B2AE35u
                       >> 16 ) );
#endif
        }
        static constexpr std::uint32_t fnv1a( const char* type_name,
                                              std::uint32_t hash = 2166136261u ) noexcept
        {
          return *type_name == '\0'
                 ? hash
                 : fnv1a( type_name + 1,
                          ( hash ^ static_cast<std::uint32_t>( static_cast<unsigned char>( *type_name ) ) )
                            * 16777619u );
        }

        template<typename U>
        static constexpr std::uint32_t hash() noexcept
        {
#if defined( _MSC_VER )
          return fnv1a( __FUNCSIG__ );
#else
          // require compiler extension
          return fnv1a( __PRETTY_FUNCTION__ );
#endif
        }

      public:
        static constexpr std::uint32_t value =
          hash<typename std::remove_cv<typename std::remove_reference<T>::type>::type>();
      };
    } // namespace traits
  } // namespace details
} // namespace pace

#endif
