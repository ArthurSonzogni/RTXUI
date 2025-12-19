#ifndef RTXUI_COMPONENTS_HPP
#define RTXUI_COMPONENTS_HPP

#include "component/component.hpp"

namespace rtxui {

RTXUI_COMPONENT(div) {
  return R"html(
    <style>
      self {
        display: block flow;
      }
    </style>
    <slot></slot>
  )html";
}

RTXUI_COMPONENT(span) {
  return R"html(
    <style>
      self {
        display: inline flow;
      }
    </style>
    <slot></slot>
  )html";
}

RTXUI_COMPONENT(p) {
  return R"html(
    <style>
      self {
        display: block flow;
      }
    </style>
    <slot></slot>
  )html";
}

RTXUI_COMPONENT(strong) {
  return R"html(
    <style>
      self {
        decoration: bold;
      }
    </style>
    <slot></slot>
  )html";
}

RTXUI_COMPONENT(ul) {
  return R"html(
    <style>
      self {
        display: block flow;
        list-style-position: outside;
        list-style-type: disc;
      }
    </style>
    <slot></slot>
  )html";
}

RTXUI_COMPONENT(ol) {
  return R"html(
    <style>
      self {
        display: block flow;
        list-style-position: outside;
        list-style-type: decimal;
      }
    </style>
    <slot></slot>
  )html";
}

RTXUI_COMPONENT(li) {
  return R"html(
    <style>
      self {
        display: list-item flow;
      }
    </style>
    <slot></slot>
  )html";
}

RTXUI_COMPONENT(button) {
  // `on_click` is a Computed
  //auto on_click = Import<std::function<void()>>("onclick");

  //auto listen_click = AddEventListener("click", on_click);

  return R"html(
    <style>
      self {
        display: inline flow;
        background-color: blue;
        color: white;
        padding: 5px;
        border-radius: 3px;
      }
    </style>
    <slot></slot>
  )html";
}

}  // namespace rtxui

#endif  // RTXUI_COMPONENTS_HPP
