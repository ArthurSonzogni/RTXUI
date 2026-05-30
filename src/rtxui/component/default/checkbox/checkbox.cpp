// Copyright 2024 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include "rtxui/component/default/checkbox/checkbox.hpp"
#include "rtxui/dom/element.hpp"

namespace rtxui {

void checkbox::InitReflection() {
  Bind(checked);
  Bind(checked_char);
  Bind(focus_class);
  Component<checkbox>::InitReflection();
}

std::string_view checkbox::Setup() {
  return R"html(<span class="{focus_class}">[<span class="checkmark">{checked_char}</span>] <slot></slot></span><style>
      self {
        display: inline-block;
        cursor: pointer;
      }
      .focused {
        background-color: #333;
        color: #fff;
      }
      .checkmark {
        font-weight: bold;
        color: #38bdf8;
      }
    </style>)html";
}

bool checkbox::OnEvent(Event event) {
  auto* root = Root();
  if (event.is<Event::Mouse>()) {
    auto mouse = event.get<Event::Mouse>();
    if (mouse.button == Event::Mouse::Button::Left &&
        mouse.motion == Event::Mouse::Motion::Pressed) {
      if (!root) {
        return false;
      }
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

        checked = !checked;
        PropagateBinding("checked", checked ? "true" : "false");

        // Run onchange callback if present
        if (root->Attributes().count("onchange")) {
          std::string onchange_cb = root->Attributes().at("onchange");
          Element* parent_el = root->Parent();
          ComponentBase* parent_comp = nullptr;
          while (parent_el) {
            if (parent_el->component()) {
              parent_comp = const_cast<ComponentBase*>(parent_el->component());
              break;
            }
            parent_el = parent_el->Parent();
          }
          while (parent_comp) {
            if (parent_comp->RunCallback(onchange_cb)) {
              break;
            }
            if (parent_comp->Root()) {
              parent_el = parent_comp->Root()->Parent();
              parent_comp = nullptr;
              while (parent_el) {
                if (parent_el->component()) {
                  parent_comp =
                      const_cast<ComponentBase*>(parent_el->component());
                  break;
                }
                parent_el = parent_el->Parent();
              }
            } else {
              break;
            }
          }
        }

        return true;
      }
    }
  }

  if (event.is<Event::Keyboard>()) {
    auto kb = event.get<Event::Keyboard>();
    if (kb.motion == Event::Keyboard::Motion::Pressed ||
        kb.motion == Event::Keyboard::Motion::Repeat) {
      if (!root || !root->focused()) {
        return false;
      }

      if (kb.special == Event::Keyboard::Special::None &&
          kb.codepoint == 32) {  // Space
        checked = !checked;
        PropagateBinding("checked", checked ? "true" : "false");

        // Run onchange callback if present
        if (root->Attributes().count("onchange")) {
          std::string onchange_cb = root->Attributes().at("onchange");
          Element* parent_el = root->Parent();
          ComponentBase* parent_comp = nullptr;
          while (parent_el) {
            if (parent_el->component()) {
              parent_comp = const_cast<ComponentBase*>(parent_el->component());
              break;
            }
            parent_el = parent_el->Parent();
          }
          while (parent_comp) {
            if (parent_comp->RunCallback(onchange_cb)) {
              break;
            }
            if (parent_comp->Root()) {
              parent_el = parent_comp->Root()->Parent();
              parent_comp = nullptr;
              while (parent_el) {
                if (parent_el->component()) {
                  parent_comp =
                      const_cast<ComponentBase*>(parent_el->component());
                  break;
                }
                parent_el = parent_el->Parent();
              }
            } else {
              break;
            }
          }
        }

        return true;
      }
    }
  }

  return false;
}

bool checkbox::Digest() {
  auto* root = Root();
  bool is_focused = root ? root->focused() : false;
  focus_class = is_focused ? "focused" : "";
  checked_char = checked ? "x" : " ";
  return Component<checkbox>::Digest();
}

namespace {
int RegisterThis = []() {
  RegisterGlobalComponent("checkbox", []() { return Ref<checkbox>::New(); });
  return 0;
}();
}  // namespace

}  // namespace rtxui
