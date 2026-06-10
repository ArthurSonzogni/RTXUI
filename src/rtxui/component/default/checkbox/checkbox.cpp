// Copyright 2024 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include "rtxui/component/default/checkbox/checkbox.hpp"

#include "rtxui/dom/element.hpp"
#include "rtxui/component/component_internal.hpp"

namespace rtxui {

void checkbox::InitReflection() {
  Bind(checked);
  Bind(checked_char);
  Component<checkbox>::InitReflection();
}

std::string_view checkbox::Setup() {
  return R"html(
    <span><span class="checkmark">{checked_char}</span> <slot></slot></span>
    <style>
      self {
        display: inline-block;
        cursor: pointer;
        padding-left: 1;
        padding-right: 1;
        transition: background-color 0.1s linear;
      }
      self:hover {
        background-color: lighten(10%);
      }
      self:focus {
        background-color: lighten(25%);
      }
      self:active {
        background-color: lighten(35%);
      }
      .checkmark {
      }
    </style>
  )html";
}

bool checkbox::OnEvent(Event event) {
  auto* root = Root();
  if (!root) {
    return false;
  }

  bool trigger = false;

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
        // Focus this element
        if (root->Parent()) {
          Element* root_el = root;
          while (root_el->Parent()) {
            root_el = root_el->Parent();
          }
          root_el->Visit([](Element& el) { el.set_focused(false); });
        }
        root->set_focused(true);
        trigger = true;
      }
    }
  }

  if (event.is<Event::Keyboard>()) {
    auto kb = event.get<Event::Keyboard>();
    if ((kb.motion == Event::Keyboard::Motion::Pressed ||
         kb.motion == Event::Keyboard::Motion::Repeat) &&
        root->focused()) {
      if (kb.special == Event::Keyboard::Special::None &&
          kb.codepoint == 32) {  // Space
        trigger = true;
      }
    }
  }

  if (trigger) {
    checked = !checked;
    PropagateBinding("checked", checked ? "true" : "false");

    // Run onchange callback if present
    if (root->Attributes().count("onchange")) {
      std::string onchange_cb = root->Attributes().at("onchange");
      if (auto* comp = GetAttributeOwnerComponent(root)) {
          comp->RunCallback(onchange_cb);
      }
    }
    return true;
  }

  return false;
}

bool checkbox::Digest() {
  checked_char = checked ? "☑" : "☐";
  return Component<checkbox>::Digest();
}

namespace {
int RegisterThis = []() {
  RegisterGlobalComponent("checkbox", []() { return Ref<checkbox>::New(); });
  return 0;
}();
}  // namespace

}  // namespace rtxui
