// Copyright 2024 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include "rtxui/component/default/select/select.hpp"

#include <iostream>

#include "rtxui/dom/element.hpp"
#include "rtxui/dom/text_element.hpp"
#include "rtxui/component/component_internal.hpp"

namespace rtxui {

void select::InitReflection() {
  Bind(value);
  Bind(selected_label);
  Bind(arrow_char);
  Bind(dropdown_class);
  Component<select>::InitReflection();
}

std::string_view select::Setup() {
  return R"html(
    <div class="select-btn">
      <span class="select-label">{selected_label}</span>
      <span class="select-arrow">{arrow_char}</span>
    </div>
    <div class="dropdown-list {dropdown_class}">
      <slot></slot>
    </div>
    <style>
      self {
        display: inline-flex;
        flex-direction: column;
        min-width: 15;
      }
      .select-btn {
        display: flex;
        flex-direction: row;
        justify-content: space-between;
        background-color: lighten(7%);
        opacity: 0.8;
        padding-left: 1;
        padding-right: 1;
        cursor: pointer;
        transition: background-color 0.1s linear, opacity 0.1s linear, color 0.1s linear;
      }
      self:hover .select-btn {
        background-color: lighten(14%);
        opacity: 0.9;
      }
      self:focus .select-btn {
        background-color: lighten(24%);
        opacity: 1.0;
      }
      .dropdown-list {
        display: flex;
        flex-direction: column;
        background-color: lighten(15%);
        position: absolute;
        width: 100%;
        margin-top: 1;
        z-index: 10;
      }
      .closed {
        display: none;
      }
    </style>
  )html";
}

std::vector<OptionInfo> select::GetOptions() {
  std::vector<OptionInfo> opts;
  auto* root = Root();
  if (!root) {
    return opts;
  }

  root->Visit([&](Element& el) {
    if (el.tag() == "option") {
      std::string val;
      if (el.Attributes().count("value")) {
        val = el.Attributes().at("value");
      }
      std::string label;
      el.Visit([&](Element& child) {
        if (child.is_text()) {
          label += static_cast<TextElement&>(child).text();
        }
      });
      opts.push_back({val, label, &el});
    }
  });
  return opts;
}

bool select::OnEvent(Event event) {
  auto* root = Root();
  if (!root) {
    return false;
  }

  bool changed = false;

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

        is_open = !is_open;
        if (is_open) {
          auto options = GetOptions();
          hovered_index = 0;
          for (int i = 0; i < static_cast<int>(options.size()); ++i) {
            if (options[i].value == value) {
              hovered_index = i;
              break;
            }
          }
        }
        changed = true;
      } else {
        if (is_open) {
          is_open = false;
          changed = true;
        }
      }
    }
  }

  if (event.is<Event::Keyboard>()) {
    auto kb = event.get<Event::Keyboard>();
    if (kb.motion == Event::Keyboard::Motion::Pressed ||
        kb.motion == Event::Keyboard::Motion::Repeat) {
      if (root->focused()) {
        auto options = GetOptions();
        if (is_open) {
          if (kb.special == Event::Keyboard::Special::ArrowDown) {
            if (!options.empty()) {
              hovered_index = (hovered_index + 1) % options.size();
              changed = true;
            }
            return true;
          }
          if (kb.special == Event::Keyboard::Special::ArrowUp) {
            if (!options.empty()) {
              hovered_index =
                  (hovered_index - 1 + options.size()) % options.size();
              changed = true;
            }
            return true;
          }
          if (kb.special == Event::Keyboard::Special::Return ||
              (kb.special == Event::Keyboard::Special::None &&
               kb.codepoint == 32)) {
            if (hovered_index >= 0 &&
                hovered_index < static_cast<int>(options.size())) {
              SelectOption(options[hovered_index].value);
            } else {
              is_open = false;
            }
            changed = true;
            return true;
          }
          if (kb.special == Event::Keyboard::Special::Escape) {
            is_open = false;
            changed = true;
            return true;
          }
        } else {
          if (kb.special == Event::Keyboard::Special::ArrowDown ||
              kb.special == Event::Keyboard::Special::ArrowUp) {
            if (!options.empty()) {
              int curr_idx = -1;
              for (int i = 0; i < static_cast<int>(options.size()); ++i) {
                if (options[i].value == value) {
                  curr_idx = i;
                  break;
                }
              }
              int new_idx = curr_idx;
              if (kb.special == Event::Keyboard::Special::ArrowDown) {
                new_idx = (curr_idx == -1)
                              ? 0
                              : std::min(static_cast<int>(options.size() - 1),
                                         curr_idx + 1);
              } else {
                new_idx = (curr_idx == -1) ? 0 : std::max(0, curr_idx - 1);
              }
              if (new_idx != curr_idx) {
                SelectOption(options[new_idx].value);
                changed = true;
                return true;
              }
            }
          }
          if (kb.special == Event::Keyboard::Special::Return ||
              (kb.special == Event::Keyboard::Special::None &&
               kb.codepoint == 32)) {
            is_open = true;
            if (!options.empty()) {
              hovered_index = 0;
              for (int i = 0; i < static_cast<int>(options.size()); ++i) {
                if (options[i].value == value) {
                  hovered_index = i;
                  break;
                }
              }
            }
            changed = true;
            return true;
          }
        }
      }
    }
  }

  return changed;
}

bool select::Digest() {
  auto* root = Root();
  dropdown_class = is_open ? "open" : "closed";
  arrow_char = is_open ? "▴" : "▾";

  if (root) {
    auto options = GetOptions();
    selected_label = "Select...";
    for (size_t i = 0; i < options.size(); ++i) {
      auto& opt = options[i];
      opt.element->classes.clear();
      if (opt.value == value) {
        selected_label = opt.label;
        opt.element->classes.push_back("selected");
      }
      if (is_open && static_cast<int>(i) == hovered_index) {
        opt.element->classes.push_back("hovered");
      }
    }
  }

  return Component<select>::Digest();
}

void select::SelectOption(std::string_view opt_val) {
  value = std::string(opt_val);
  PropagateBinding("value", value);
  is_open = false;

  auto* root = Root();
  if (root && root->Attributes().count("onchange")) {
    std::string onchange_cb = root->Attributes().at("onchange");
    if (auto* comp = GetAttributeOwnerComponent(root)) {
        comp->RunCallback(onchange_cb);
    }
  }
}

namespace {
int RegisterThis = []() {
  RegisterGlobalComponent("select", []() { return Ref<select>::New(); });
  return 0;
}();
}  // namespace

}  // namespace rtxui
