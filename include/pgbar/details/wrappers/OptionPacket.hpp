#ifndef PGBAR_OPTION_PACKET
#define PGBAR_OPTION_PACKET

#include "../traits/TemplateSet.hpp"
#include "../traits/TypeSet.hpp"
#include <type_traits>
#include <utility>

namespace pgbar {
  namespace config {
    template<typename Config, typename Option>
    struct ProvideFor {
      static_assert( std::is_default_constructible<Option>::value,
                     "the provided parameters cannot be constructed by default" );
      static constexpr Option provide() noexcept( std::is_nothrow_default_constructible<Option>::value )
      {
        return Option();
      }
    };

#if PGBAR__CXX14
    template<typename Config, typename Option>
    PGBAR__CXX17_INLINE constexpr const auto ProvideFor_v = ProvideFor<Config, Option>::provide;
#endif

#define PGBAR__PROVIDE_FOR( Config, Option, Defaults )                                   \
  template<>                                                                             \
  struct pgbar::config::ProvideFor<Config, Option> {                                     \
    static Option provide()                                                              \
      noexcept( std::is_nothrow_constructible<Option, decltype( ( Defaults ) )>::value ) \
    {                                                                                    \
      return Option( Defaults );                                                         \
    }                                                                                    \
  }
  } // namespace config

  namespace _details {
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

        constexpr OptionPacket( OptionPacket&& )                         = default;
        PGBAR__CXX14_CNSTXPR OptionPacket& operator=( OptionPacket&& ) & = default;
        // Intentional non-virtual destructors.
        PGBAR__CXX20_CNSTXPR ~OptionPacket()                             = default;

      public:
        PGBAR__CXX14_CNSTXPR T& value() & noexcept { return data_; }
        PGBAR__CXX14_CNSTXPR const T& value() const& noexcept { return data_; }
        PGBAR__CXX14_CNSTXPR T&& value() && noexcept { return std::move( data_ ); }

        PGBAR__CXX20_CNSTXPR void swap( T& other ) noexcept
        {
          PGBAR__TRUST( this != &other );
          using std::swap;
          swap( data_, other.data_ );
        }
        friend PGBAR__CXX20_CNSTXPR void swap( T& a, T& b ) noexcept { a.swap( b ); }
      };
    } // namespace wrappers

    namespace traits {
      template<template<typename...> class Component>
      struct OptionOf {
        using type = TypeSet<>;
      };
      template<template<typename...> class Component>
      using OptionOf_t = typename OptionOf<Component>::type;

#define PGBAR__OPTION_REGISTER( Component, ... )                \
  template<>                                                    \
  struct pgbar::_details::traits::OptionOf<Component> {         \
    using type = pgbar::_details::traits::TypeSet<__VA_ARGS__>; \
  }

      // Resolves and links option declarations into a list.
      template<typename ComponentList>
      struct OptionLinker;
      template<typename ComponentList>
      using OptionLinker_t = typename OptionLinker<ComponentList>::type;

      template<template<typename...> class... Components>
      struct OptionLinker<TemplateSet<Components...>> : Merge<TypeSet<>, OptionOf_t<Components>...> {};
    } // namespace traits

    namespace utils {
      template<typename Config, typename Option>
      constexpr Option provide_for()
#if PGBAR__CXX14
        noexcept( noexcept( config::ProvideFor_v<Config, Option>() ) )
      {
        static_assert(
          std::is_constructible<Option, decltype( config::ProvideFor_v<Config, Option>() )>::value,
          "the ProvideFor_v specialization must be an invocable object" );
        return config::ProvideFor_v<Config, Option>();
      }
#else
        noexcept( noexcept( config::ProvideFor<Config, Option>::provide() ) )
      {
        static_assert(
          std::is_constructible<Option, decltype( config::ProvideFor<Config, Option>::provide() )>::value,
          "the ProvideFor_v::provide must be an invocable object" );
        return config::ProvideFor<Config, Option>::provide();
      }
#endif
    } // namespace utils
  } // namespace _details
} // namespace pgbar

#define PGBAR__DERIVING_OPTION1( StructName, ValueType, ParamName )           \
  _details::wrappers::OptionPacket<ValueType>                                 \
  {                                                                           \
  public:                                                                     \
    StructName() = default;                                                   \
    StructName( ValueType ParamName ) noexcept                                \
      : _details::wrappers::OptionPacket<ValueType>( std::move( ParamName ) ) \
    {}                                                                        \
  }

#ifdef __cpp_lib_char8_t
# define PGBAR__DERIVING_OPTION2( StructName, ValueType, ParamName )                        \
   _details::wrappers::OptionPacket<ValueType>                                              \
   {                                                                                        \
   public:                                                                                  \
     StructName() = default;                                                                \
     StructName( _details::types::String ParamName )                                        \
       : _details::wrappers::OptionPacket<ValueType>( ValueType( std::move( ParamName ) ) ) \
     {}                                                                                     \
     StructName( _details::types::LitU8 ParamName )                                         \
       : _details::wrappers::OptionPacket<ValueType>( ValueType( std::move( ParamName ) ) ) \
     {}                                                                                     \
   }
#else
# define PGBAR__DERIVING_OPTION2( StructName, ValueType, ParamName )                        \
   _details::wrappers::OptionPacket<ValueType>                                              \
   {                                                                                        \
     static_assert( std::is_default_constructible<ValueType>::value,                        \
                    "the value should be default constructible" );                          \
                                                                                            \
   public:                                                                                  \
     StructName() = default;                                                                \
     StructName( _details::types::String ParamName )                                        \
       : _details::wrappers::OptionPacket<ValueType>( ValueType( std::move( ParamName ) ) ) \
     {}                                                                                     \
   }
#endif

#endif
