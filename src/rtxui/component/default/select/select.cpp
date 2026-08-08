// Copyright 2024 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include "rtxui/component/default/select/select.hpp"

#include <iostream>
#include <algorithm>

#include "rtxui/dom/element.hpp"
#include "rtxui/dom/text_element.hpp"
#include "rtxui/component/component_internal.hpp"
#include "rtxui/component/default/option/option.hpp"

namespace rtxui {

void select::InitReflection() {
  Bind(value);
  Bind(disabled);
  Bind(selected_label);
  Bind(arrow_char);
  Bind(dropdown_class);
  Component<select>::InitReflection();
}

std::string_view select::Setup() {
  return R"html(
    <div class="select-btn" part="select-btn">
      <span class="select-label" part="select-label">{selected_label}</span>
      <span class="select-arrow" part="select-arrow">{arrow_char}</span>
    </div>
    <div class="dropdown-list {dropdown_class}" part="dropdown-list">
      <slot></slot>
    </div>
    <style>
      self {
        display: inline-flex;
        flex-direction: column;
        min-width: 15;
        position: relative;
      }
      self.open {
        display: flex;
        z-index: 1000;
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
      self:disabled .select-btn {
        cursor: default;
        opacity: 0.4;
      }
      .dropdown-list {
        display: flex;
        flex-direction: column;
        background-color: lighten(15%);
        position: absolute;
        top: 100%;
        left: 0;
        width: 100%;
        z-index: 1000;
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
      bool opt_disabled = false;
      if (auto* opt = dynamic_cast<option*>(
              const_cast<ComponentBase*>(el.component()))) {
        opt_disabled = opt->disabled;
      }
      opts.push_back({val, label, &el, opt_disabled});
    }
  });
  return opts;
}

bool select::OnEvent(Event event) {
  if (disabled) {
    return false;
  }
  if (Component<select>::OnEvent(event)) {
    return true;
  }

  auto* root = Root();
  if (!root) {
    return false;
  }

  bool changed = false;

  if (event.is<Event::Mouse>()) {
    auto mouse = event.get<Event::Mouse>();
    int click_x = mouse.x - 1;
    int click_y = mouse.y - 1;

    if (mouse.button == Event::Mouse::Button::Left &&
        mouse.motion == Event::Mouse::Motion::Pressed) {
      int abs_x = root->absolute_x();
      int abs_y = root->absolute_y();
      int layout_w = root->layout_width();
      int layout_h = root->layout_height();

      bool click_inside_select = (click_x >= abs_x && click_x < abs_x + layout_w &&
                                  click_y >= abs_y && click_y < abs_y + layout_h);
      bool click_inside_dropdown = false;
      if (is_open) {
        if (auto* dropdown_el = root->QuerySelector(".dropdown-list")) {
          int drop_x = dropdown_el->absolute_x();
          int drop_y = dropdown_el->absolute_y();
          int drop_w = dropdown_el->layout_width();
          int drop_h = dropdown_el->layout_height();
          if (click_x >= drop_x && click_x < drop_x + drop_w &&
              click_y >= drop_y && click_y < drop_y + drop_h) {
            click_inside_dropdown = true;
          }
        }
      }

      if (click_inside_select) {
        FocusExclusive(root);

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
      } else if (click_inside_dropdown) {
        // Click was inside the dropdown. Do nothing here to allow event propagation to the option components.
      } else {
        if (is_open) {
          is_open = false;
          changed = true;
        }
      }
    } else if (is_open && mouse.motion == Event::Mouse::Motion::Moved) {
      auto options = GetOptions();
      for (int i = 0; i < static_cast<int>(options.size()); ++i) {
        auto* opt_el = options[i].element;
        int opt_x = opt_el->absolute_x();
        int opt_y = opt_el->absolute_y();
        int opt_w = opt_el->layout_width();
        int opt_h = opt_el->layout_height();
        if (click_x >= opt_x && click_x < opt_x + opt_w &&
            click_y >= opt_y && click_y < opt_y + opt_h) {
          if (hovered_index != i) {
            hovered_index = i;
            changed = true;
          }
          break;
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
          if (kb.special == Event::Keyboard::Special::ArrowDown ||
              kb.special == Event::Keyboard::Special::ArrowUp) {
            if (!options.empty()) {
              int step =
                  (kb.special == Event::Keyboard::Special::ArrowDown) ? 1 : -1;
              int next = hovered_index;
              // Wraps around, skipping disabled options; if every option is
              // disabled this comes full circle back to hovered_index and
              // leaves it unchanged.
              for (size_t tries = 0; tries < options.size(); ++tries) {
                next = static_cast<int>(
                    (next + step + options.size()) % options.size());
                if (!options[next].disabled) {
                  break;
                }
              }
              if (next != hovered_index) {
                hovered_index = next;
                changed = true;
              }
            }
            return true;
          }
          if (kb.special == Event::Keyboard::Special::Return ||
              (kb.special == Event::Keyboard::Special::None &&
               kb.codepoint == 32)) {
            if (hovered_index >= 0 &&
                hovered_index < static_cast<int>(options.size())) {
              if (!options[hovered_index].disabled) {
                SelectOption(options[hovered_index].value);
              }
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
              if (curr_idx == -1) {
                // Nothing selected yet: land on the first enabled option,
                // regardless of arrow direction (matches the prior
                // always-picks-0 behavior when nothing was disabled).
                for (int i = 0; i < static_cast<int>(options.size()); ++i) {
                  if (!options[i].disabled) {
                    new_idx = i;
                    break;
                  }
                }
              } else {
                // Clamps at the ends rather than wrapping, skipping disabled
                // options; stops at the first edge it can't move past.
                int step =
                    (kb.special == Event::Keyboard::Special::ArrowDown) ? 1 : -1;
                int candidate = curr_idx;
                for (size_t tries = 0; tries < options.size(); ++tries) {
                  candidate += step;
                  if (candidate < 0 ||
                      candidate >= static_cast<int>(options.size())) {
                    break;
                  }
                  if (!options[candidate].disabled) {
                    new_idx = candidate;
                    break;
                  }
                }
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
  SyncDisabled(root, disabled);
  if (disabled) {
    is_open = false;
  }
  dropdown_class = is_open ? "open" : "closed";
  arrow_char = is_open ? "▴" : "▾";

  bool state_changed = false;
  if (is_open != last_is_open_) {
    last_is_open_ = is_open;
    state_changed = true;
  }
  if (hovered_index != last_hovered_index_) {
    last_hovered_index_ = hovered_index;
    state_changed = true;
  }
  if (value != last_value_) {
    last_value_ = value;
    state_changed = true;
  }

  if (root && state_changed) {
    auto& comp_cls = classes_;
    auto it_comp = std::find(comp_cls.begin(), comp_cls.end(), "open");
    if (is_open) {
      if (it_comp == comp_cls.end()) {
        comp_cls.push_back("open");
      }
    } else {
      if (it_comp != comp_cls.end()) {
        comp_cls.erase(it_comp);
      }
    }

    auto& root_cls = root->classes;
    auto it_root = std::find(root_cls.begin(), root_cls.end(), "open");
    if (is_open) {
      if (it_root == root_cls.end()) {
        root_cls.push_back("open");
      }
    } else {
      if (it_root != root_cls.end()) {
        root_cls.erase(it_root);
      }
    }
    root->Visit([](Element& el) { el.ClearResolvedStyles(); });

    auto options = GetOptions();
    selected_label = "Select...";
    for (size_t i = 0; i < options.size(); ++i) {
      auto& opt = options[i];
      auto& cls = opt.element->classes;
      auto it_sel = std::find(cls.begin(), cls.end(), "selected");
      if (it_sel != cls.end()) {
        cls.erase(it_sel);
      }
      auto it_hov = std::find(cls.begin(), cls.end(), "hovered");
      if (it_hov != cls.end()) {
        cls.erase(it_hov);
      }
      if (opt.value == value) {
        selected_label = opt.label;
        if (std::find(cls.begin(), cls.end(), "selected") == cls.end()) {
          cls.push_back("selected");
        }
      }
      if (is_open && static_cast<int>(i) == hovered_index) {
        if (std::find(cls.begin(), cls.end(), "hovered") == cls.end()) {
          cls.push_back("hovered");
        }
      }
      opt.element->Visit([](Element& el) { el.ClearResolvedStyles(); });
    }
    this->Render();
  }

  bool parent_digest = Component<select>::Digest();
  return state_changed || parent_digest;
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
