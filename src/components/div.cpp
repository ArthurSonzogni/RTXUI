#ifndef RTXUI_COMPONENT_DIV_HPP
#define RTXUI_COMPONENT_DIV_HPP

#include "component.hpp"

RTXUI_COMPONENT(div) {
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

#endif  // RTXUI_COMPONENT_DIV_HPP
