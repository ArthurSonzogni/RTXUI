// Copyright 2024 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include "rtxui/component/default/slider/slider.hpp"

#include <algorithm>
#include <cmath>

#include "rtxui/dom/element.hpp"

namespace rtxui {

void slider::InitReflection() {
  Bind(value);
  Bind(min);
  Bind(max);
  Bind(step);
  Bind(width);
  Bind(track_left);
  Bind(thumb_char);
  Bind(track_right);
  Bind(focus_class);
  Component<slider>::InitReflection();
}

std::string_view slider::Setup() {
  return R"html(<span class="{focus_class}"><span class="track-left">{track_left}</span><span class="thumb">{thumb_char}</span><span class="track-right">{track_right}</span></span><style>
      self {
        display: inline-block;
        cursor: pointer;
      }
      .focused {
        background-color: #333;
        color: #fff;
      }
      .track-left {
        color: #38bdf8;
      }
      .track-right {
        color: #555;
      }
      .thumb {
        font-weight: bold;
        color: #38bdf8;
      }
    </style>)html";
}

bool slider::OnEvent(Event event) {
  auto* root = Root();
  if (!root) {
    return false;
  }

  bool value_changed = false;
  bool is_captured = (GetMouseCapturer() == this);
  bool was_captured = is_captured;

  if (event.is<Event::Mouse>()) {
    auto mouse = event.get<Event::Mouse>();
    if (mouse.button == Event::Mouse::Button::Left) {
      if (mouse.motion == Event::Mouse::Motion::Pressed) {
        int click_x = mouse.x - 1;
        int click_y = mouse.y - 1;
        int abs_x = root->absolute_x();
        int abs_y = root->absolute_y();
        // Use the `width` attribute for bounds check: Root() is an inline <span>
        // whose layout_width_ is 0. The drag code already uses max(2, width).
        int track_w = std::max(2, width);

        if (click_x >= abs_x && click_x < abs_x + track_w &&
            click_y >= abs_y && click_y <= abs_y) {
          // Focus this element
          if (root->Parent()) {
            Element* root_el = root;
            while (root_el->Parent()) {
              root_el = root_el->Parent();
            }
            root_el->Visit([](Element& el) { el.set_focused(false); });
          }
          root->set_focused(true);

          CaptureMouse();
          is_captured = true;
          was_captured = true;
        }
      }
    }

    if (is_captured) {
      if (mouse.motion == Event::Mouse::Motion::Moved ||
          mouse.motion == Event::Mouse::Motion::Pressed ||
          mouse.motion == Event::Mouse::Motion::Released) {
        int click_x = mouse.x - 1;
        int abs_x = root->absolute_x();
        int track_w = std::max(2, width);
        int pos = std::clamp(click_x - abs_x, 0, track_w - 1);

        // Map pos to [min, max]
        double pct = static_cast<double>(pos) / (track_w - 1);
        int raw_val = min + static_cast<int>(std::round(pct * (max - min)));

        // Snap to nearest step
        int remainder = (raw_val - min) % step;
        int new_val = raw_val;
        if (remainder < step / 2.0) {
          new_val = raw_val - remainder;
        } else {
          new_val = raw_val + (step - remainder);
        }
        new_val = std::clamp(new_val, min, max);

        if (new_val != value) {
          value = new_val;
          value_changed = true;
        }

        if (mouse.motion == Event::Mouse::Motion::Released) {
          ReleaseMouse();
        }
      }
    }
  }

  if (event.is<Event::Keyboard>()) {
    auto kb = event.get<Event::Keyboard>();
    if (kb.motion == Event::Keyboard::Motion::Pressed ||
        kb.motion == Event::Keyboard::Motion::Repeat) {
      if (root->focused()) {
        int delta = 0;
        if (kb.special == Event::Keyboard::Special::ArrowLeft ||
            kb.special == Event::Keyboard::Special::ArrowDown) {
          delta = -step;
        } else if (kb.special == Event::Keyboard::Special::ArrowRight ||
                   kb.special == Event::Keyboard::Special::ArrowUp) {
          delta = step;
        }

        if (delta != 0) {
          int new_val = std::clamp(value + delta, min, max);
          if (new_val != value) {
            value = new_val;
            value_changed = true;
          }
        }
      }
    }
  }

  if (value_changed) {
    PropagateBinding("value", std::to_string(value));

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
              parent_comp = const_cast<ComponentBase*>(parent_el->component());
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

  if (was_captured) {
    return true;
  }

  return false;
}

bool slider::Digest() {
  auto* root = Root();
  bool is_focused = root ? root->focused() : false;
  focus_class = is_focused ? "focused" : "";

  int track_w = std::max(2, width);
  int range = max - min;
  int pos = 0;
  if (range > 0) {
    pos = static_cast<int>(
        std::round(static_cast<double>(value - min) / range * (track_w - 1)));
  }
  pos = std::clamp(pos, 0, track_w - 1);

  track_left = "";
  for (int i = 0; i < pos; ++i) {
    track_left += "─";
  }
  thumb_char = "●";
  track_right = "";
  for (int i = pos + 1; i < track_w; ++i) {
    track_right += "─";
  }

  return Component<slider>::Digest();
}

namespace {
int RegisterThis = []() {
  RegisterGlobalComponent("slider", []() { return Ref<slider>::New(); });
  return 0;
}();
}  // namespace

}  // namespace rtxui
