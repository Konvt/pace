#ifndef PACE_TEMPLATE_LIST
#define PACE_TEMPLATE_LIST

namespace pace {
  namespace details {
    namespace traits {
      // A lightweight tuple type that stores multiple template class types.
      template<template<typename...> class... Ts>
      struct TemplateList {};
    } // namespace traits
  } // namespace details
} // namespace pace

#endif
