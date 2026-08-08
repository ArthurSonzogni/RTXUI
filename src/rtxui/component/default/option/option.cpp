// Copyright 2024 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include "rtxui/component/default/option/option.hpp"

#include "rtxui/component/component_internal.hpp"
#include "rtxui/component/default/select/select.hpp"
#include "rtxui/dom/element.hpp"

namespace rtxui {

void option::InitReflection() {
  Bind(value);
  Bind(disabled);
  Component<option>::InitReflection();
}

std::string_view option::Setup() {
  return R"html(<span class="option-container"><slot></slot></span><style>
    self {
      display: block;
      cursor: pointer;
      opacity: 0.85;
      padding-left: 1;
      padding-right: 1;
      transition: background-color 0.1s linear, opacity 0.1s linear, color 0.1s linear;
    }
    self:hover {
      background-color: rgb(59, 130, 246);
      color: white;
      opacity: 1.0;
    }
    .selected {
      opacity: 1.0;
      background-color: rgb(51, 65, 85);
      color: rgb(254, 240, 138);
      font-weight: bold;
    }
    .hovered {
      background-color: rgb(59, 130, 246);
      color: white;
      opacity: 1.0;
    }
    self:disabled {
      cursor: default;
      opacity: 0.35;
    }
  </style>)html";
}

bool option::OnEvent(Event event) {
  auto* root = Root();
  if (!root || disabled) {
    return false;
  }

  if (event.is<Event::Mouse>()) {
    auto mouse = event.get<Event::Mouse>();
    if (mouse.button == Event::Mouse::Button::Left &&
        mouse.motion == Event::Mouse::Motion::Pressed) {
      int click_x = mouse.x - 1;
      int click_y = mouse.y - 1;
      int abs_x = root->absolute_x();
      int abs_y = root->absolute_y();
      int layout_w = root->layout_width();
      int layout_h = root->layout_height();

      if (click_x >= abs_x && click_x < abs_x + layout_w && click_y >= abs_y &&
          click_y < abs_y + layout_h) {
        Element* parent_el = root->Parent();
        while (parent_el) {
          if (parent_el->tag() == "select" && parent_el->component()) {
            auto* select_comp =
                const_cast<ComponentBase*>(parent_el->component());
            auto* sel = static_cast<rtxui::select*>(select_comp);
            sel->SelectOption(value);
            return true;
          }
          parent_el = parent_el->Parent();
        }
      }
    }
  }
  return false;
}

bool option::Digest() {
  SyncDisabled(Root(), disabled);
  return Component<option>::Digest();
}

namespace {
int RegisterThis = []() {
  RegisterGlobalComponent("option", []() { return Ref<option>::New(); });
  return 0;
}();
}  // namespace

}  // namespace rtxui
