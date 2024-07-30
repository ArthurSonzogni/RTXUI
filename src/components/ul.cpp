#ifndef RTXUI_COMPONENT_UL_HPP
#define RTXUI_COMPONENT_UL_HPP

#include "component.hpp"

RTXUI_COMPONENT(ul) {
  return R"(
    <template>
      <slot/>
    </template>

    <style>
      template {
        list-style-type: disc;
        list-style-position: outside;
      }
    </style>
  )";
}

#endif  // RTXUI_COMPONENT_UL_HPP
