// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include "rtxui/component/default/label/label.hpp"

#include "rtxui/component/component_internal.hpp"
#include "rtxui/dom/element.hpp"

namespace rtxui {

const std::string_view label::view = R"html(
    <span><slot></slot></span>
    <style>
      self {
        display: inline-flex;
        cursor: pointer;
      }
    </style>
  )html";

bool label::OnEvent(Event event) {
  auto* root = Root();
  if (!root) {
    return false;
  }

  // Handle click on label
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
        // Find target element to focus
        Element* target_el = nullptr;

        // Option 1: "for" attribute
        if (root->Attributes().count("for")) {
          std::string for_id = root->Attributes().at("for");
          Element* owner_root = root;
          while (owner_root->Parent()) {
            owner_root = owner_root->Parent();
          }
          owner_root->Visit([&](Element& el) {
            if (el.id == for_id) {
              target_el = &el;
            }
          });
        }

        // Option 2: Nested input/focusable element
        if (!target_el) {
          root->Visit([&](Element& el) {
            if (&el != root) {
              std::string_view tag = el.tag();
              if (tag == "input" || tag == "textarea" || tag == "checkbox" ||
                  tag == "radio" || tag == "slider" || tag == "button" ||
                  tag == "select") {
                target_el = &el;
              }
            }
          });
        }

        // If we found a target, focus it and trigger it (e.g. click simulation)
        if (target_el) {
          // Unfocus other elements first
          Element* owner_root = root;
          while (owner_root->Parent()) {
            owner_root = owner_root->Parent();
          }
          owner_root->Visit([](Element& el) { el.set_focused(false); });

          target_el->set_focused(true);

          // Trigger a click event on target if it supports click (like
          // checkbox/radio/button)
          if (auto* comp = const_cast<ComponentBase*>(target_el->component())) {
            // Trigger checkbox check change or button action by simulating
            // spacebar keyboard press
            Event dummy_click = Event::Keyboard::From(' ');
            comp->OnEvent(dummy_click);
            if (auto* parent =
                    const_cast<ComponentBase*>(target_el->owner_component())) {
              parent->Digest();
            }
          }
          return true;
        }
      }
    }
  }

  return Component<label>::OnEvent(event);
}

namespace {
int RegisterThis = []() {
  RegisterGlobalComponent("label", []() { return Ref<label>::New(); });
  return 0;
}();
}  // namespace
}  // namespace rtxui
