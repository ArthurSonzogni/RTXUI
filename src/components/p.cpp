#ifndef RTXUI_COMPONENT_P_HPP
#define RTXUI_COMPONENT_P_HPP

#include "component.hpp"

RTXUI_COMPONENT(p) {
  return R"(
    <template>
      <slot/>
    </template>

    <style>
      template {
        display-inside: flow;
        display-outside: block;
      }
    </style>
  )";
}

#endif  // RTXUI_COMPONENT_P_HPP
