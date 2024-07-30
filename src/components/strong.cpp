#ifndef RTXUI_COMPONENT_STRONG_HPP
#define RTXUI_COMPONENT_STRONG_HPP

#include "component.hpp"

RTXUI_COMPONENT(strong) {
  return R"(
    <template>
      <slot/>
    </template>

    <style>
      strong {
        font-weight: bold;
      }
    </style>
  )";
}

#endif  // RTXUI_COMPONENT_STRONG_HPP
