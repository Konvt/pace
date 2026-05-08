#ifndef PGBAR_TEMPLATE_LIST
#define PGBAR_TEMPLATE_LIST

namespace pgbar {
  namespace details {
    namespace traits {
      // A lightweight tuple type that stores multiple template class types.
      template<template<typename...> class... Ts>
      struct TemplateList {};
    } // namespace traits
  } // namespace details
} // namespace pgbar

#endif
