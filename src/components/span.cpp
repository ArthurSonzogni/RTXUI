#ifndef RTXUI_COMPONENT_SPAN_HPP
#define RTXUI_COMPONENT_SPAN_HPP

#include "component.hpp"

RTXUI_COMPONENT(span) {
  return R"(
    <template>
      <slot/>
    </template>

    <style>
      template {
        display-inside: flow;
        display-outside: inline;
      }
    </style>
  )";
}

#endif  // RTXUI_COMPONENT_SPAN_HPP
