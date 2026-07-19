#ifndef PACE_OPTION_PACKET
#define PACE_OPTION_PACKET

#include "../traits/TemplateSet.hpp"
#include "../traits/TypeSet.hpp"
#include <type_traits>
#include <utility>

namespace pace {
  namespace details {
    namespace wrappers {
      template<typename T>
      struct OptionPacket {
        static_assert( std::is_default_constructible<T>::value,
                       "the type T should be default constructible to generate it implicitly" );

      protected:
        T data_;

        constexpr OptionPacket() = default;
        constexpr OptionPacket( T&& data ) noexcept( std::is_nothrow_move_constructible<T>::value )
          : data_ { std::move( data ) }
        {}

        constexpr OptionPacket( const OptionPacket& )                        = default;
        PACE__CXX14_CNSTXPR OptionPacket& operator=( const OptionPacket& ) & = default;
        constexpr OptionPacket( OptionPacket&& )                             = default;
        PACE__CXX14_CNSTXPR OptionPacket& operator=( OptionPacket&& ) &      = default;
        // Intentional non-virtual destructors.
        PACE__CXX20_CNSTXPR ~OptionPacket()                                  = default;

      public:
        PACE__CXX14_CNSTXPR T& value() & noexcept { return data_; }
        PACE__CXX14_CNSTXPR const T& value() const& noexcept { return data_; }
        PACE__CXX14_CNSTXPR T&& value() && noexcept { return std::move( data_ ); }

        PACE__CXX20_CNSTXPR void swap( T& other ) noexcept
        {
          PACE__TRUST( this != &other );
          using std::swap;
          swap( data_, other.data_ );
        }
        friend PACE__CXX20_CNSTXPR void swap( T& a, T& b ) noexcept { a.swap( b ); }
      };
    } // namespace wrappers

    namespace traits {
      template<template<typename...> class Component>
      struct OptionOf : Identity<TypeSet<>> {};
      template<template<typename...> class Component>
      using OptionOf_t = typename OptionOf<Component>::type;

#define PACE__OPTION_REGISTER( Component, ... )     \
  template<>                                        \
  struct pace::details::traits::OptionOf<Component> \
    : pace::details::traits::Identity<pace::details::traits::TypeSet<__VA_ARGS__>> {}

      // Resolves and links option declarations into a list.
      template<typename ComponentList>
      struct OptionLinker;
      template<typename ComponentList>
      using OptionLinker_t = typename OptionLinker<ComponentList>::type;

      template<template<typename...> class... Components>
      struct OptionLinker<TemplateSet<Components...>> : Merge<TypeSet<>, OptionOf_t<Components>...> {};
    } // namespace traits
  } // namespace details
} // namespace pace

#define PACE__DERIVING_OPTION1( StructName, ValueType, ParamType, ParamName )      \
  pace::details::wrappers::OptionPacket<ValueType>                                 \
  {                                                                                \
  public:                                                                          \
    PACE__CXX20_CNSTXPR StructName() = default;                                    \
    PACE__CXX20_CNSTXPR StructName( ParamType ParamName ) noexcept                 \
      : pace::details::wrappers::OptionPacket<ValueType>( std::move( ParamName ) ) \
    {}                                                                             \
  }

#define PACE__DERIVING_OPTION2( StructName, ValueType, ParamName ) \
  PACE__DERIVING_OPTION1( StructName, ValueType, ValueType, ParamName )

#ifdef __cpp_lib_char8_t
# define PACE__DERIVING_OPTION3( StructName, ValueType, ParamName )                              \
   pace::details::wrappers::OptionPacket<ValueType>                                              \
   {                                                                                             \
   public:                                                                                       \
     PACE__CXX20_CNSTXPR StructName() = default;                                                 \
     PACE__CXX20_CNSTXPR StructName( std::string ParamName )                                     \
       : pace::details::wrappers::OptionPacket<ValueType>( ValueType( std::move( ParamName ) ) ) \
     {}                                                                                          \
     PACE__CXX20_CNSTXPR StructName( pace::details::charcodes::U8StringView ParamName )          \
       : pace::details::wrappers::OptionPacket<ValueType>( ValueType( std::move( ParamName ) ) ) \
     {}                                                                                          \
   }
#else
# define PACE__DERIVING_OPTION3( StructName, ValueType, ParamName )                              \
   pace::details::wrappers::OptionPacket<ValueType>                                              \
   {                                                                                             \
     static_assert( std::is_default_constructible<ValueType>::value,                             \
                    "the value should be default constructible" );                               \
                                                                                                 \
   public:                                                                                       \
     PACE__CXX20_CNSTXPR StructName() = default;                                                 \
     PACE__CXX20_CNSTXPR StructName( std::string ParamName )                                     \
       : pace::details::wrappers::OptionPacket<ValueType>( ValueType( std::move( ParamName ) ) ) \
     {}                                                                                          \
   }
#endif

#endif
