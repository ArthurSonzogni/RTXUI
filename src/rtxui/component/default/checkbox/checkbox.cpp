// Copyright 2024 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include "rtxui/component/default/checkbox/checkbox.hpp"

#include "rtxui/component/component_internal.hpp"
#include "rtxui/dom/element.hpp"

namespace rtxui {

void checkbox::InitReflection() {
  Bind(checked);
  Bind(disabled);
  Bind(checked_char);
  Component<checkbox>::InitReflection();
}

std::string_view checkbox::Setup() {
  return R"html(
    <span class="checkmark" part="checkmark">{checked_char}</span>
    <slot></slot>
    <style>
      self {
        display: inline-flex;
        flex-direction: row;
        white-space: nowrap;
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
      self:disabled {
        cursor: default;
        opacity: 0.4;
      }
      .checkmark {
        margin-right: 1;
      }
    </style>
  )html";
}

bool checkbox::OnEvent(Event event) {
  auto* root = Root();
  if (!root || disabled) {
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
        FocusExclusive(root);
        trigger = true;
      }
    }
  }

  if (event.is<Event::Keyboard>()) {
    auto kb = event.get<Event::Keyboard>();
    if ((kb.motion == Event::Keyboard::Motion::Pressed ||
         kb.motion == Event::Keyboard::Motion::Repeat) &&
        root->focused()) {
      if ((kb.special == Event::Keyboard::Special::None &&
           kb.codepoint == 32) ||  // Space
          kb.special == Event::Keyboard::Special::Return) {
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
  SyncDisabled(Root(), disabled);
  SyncChecked(Root(), checked);
  return Component<checkbox>::Digest();
}

namespace {
int RegisterThis = []() {
  RegisterGlobalComponent("checkbox", []() { return Ref<checkbox>::New(); });
  return 0;
}();
}  // namespace

}  // namespace rtxui
