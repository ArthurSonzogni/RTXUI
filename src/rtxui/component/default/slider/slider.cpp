// Copyright 2024 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include "rtxui/component/default/slider/slider.hpp"

#include <algorithm>
#include <cmath>

#include "rtxui/dom/element.hpp"
#include "rtxui/component/component_internal.hpp"

namespace rtxui {

void slider::InitReflection() {
  Bind(value);
  Bind(min);
  Bind(max);
  Bind(step);
  Bind(width);
  Bind(direction);
  Bind(track_left);
  Bind(thumb_char);
  Bind(track_right);
  Bind(container_class);
  Component<slider>::InitReflection();
}

std::string_view slider::Setup() {
  return R"html(
    <span class="{container_class}" part="slider-container">
      <span class="track-left" part="track-left">{track_left}</span><span class="thumb" part="thumb">{thumb_char}</span><span class="track-right" part="track-right">{track_right}</span>
    </span>
    <style>
      self {
        display: inline-block;
        cursor: pointer;
        padding: 0 1;
        transition: background-color 0.1s linear;
        white-space: pre;
        flex-shrink: 0;
      }
      .slider-container {
        display: flex;
        align-items: center;
        justify-content: center;
      }
      .slider-container.vertical {
        flex-direction: column-reverse;
        width: 1;
      }
      .slider-container.horizontal {
        flex-direction: row;
      }
      .track-left, .track-right, .thumb {
        flex-shrink: 0;
      }

      self:hover {
        background-color: lighten(10%);
      }
      self:focus {
        background-color: lighten(25%);
      }
      .track-left {
        opacity: 1.0;
      }
      .track-right {
        opacity: 0.35;
      }
      .thumb {
        opacity: 1.0;
      }
    </style>
  )html";
}

bool slider::OnEvent(Event event) {
  auto* root = Root();
  if (!root) {
    return false;
  }

  bool is_vertical = (direction == "vertical");
  bool value_changed = false;
  bool is_captured = (GetMouseCapturer() == this);
  bool was_captured = is_captured;
  // step is bindable and freely settable via the step="" attribute with no
  // validation; a step of 0 (or negative) would divide/modulo by zero below
  // and crash the app (SIGFPE). Clamp locally rather than overwriting the
  // app-bound `step` member.
  int safe_step = std::max(1, step);

  if (event.is<Event::Mouse>()) {
    auto mouse = event.get<Event::Mouse>();
    if (mouse.button == Event::Mouse::Button::Left) {
      if (mouse.motion == Event::Mouse::Motion::Pressed) {
        int click_x = mouse.x - 1;
        int click_y = mouse.y - 1;
        int abs_x = root->absolute_x();
        int abs_y = root->absolute_y();
        
        int track_x = abs_x + 1;
        int track_y = abs_y;
        int track_w = is_vertical ? 1 : std::max(2, width);
        int track_h = is_vertical ? std::max(2, width) : 1;

        if (click_x >= track_x && click_x < track_x + track_w &&
            click_y >= track_y && click_y < track_y + track_h) {
          FocusExclusive(root);

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
        int click_y = mouse.y - 1;
        int abs_x = root->absolute_x();
        int abs_y = root->absolute_y();
        int track_x = abs_x + 1;
        int track_y = abs_y;
        int track_size = std::max(2, width);
        
        int pos = 0;
        if (is_vertical) {
          pos = std::clamp(track_y + track_size - 1 - click_y, 0, track_size - 1);
        } else {
          pos = std::clamp(click_x - track_x, 0, track_size - 1);
        }

        // Map pos to [min, max]
        double pct = static_cast<double>(pos) / (track_size - 1);
        int raw_val = min + static_cast<int>(std::round(pct * (max - min)));

        // Snap to nearest step
        int remainder = (raw_val - min) % safe_step;
        int new_val = raw_val;
        if (remainder < safe_step / 2.0) {
          new_val = raw_val - remainder;
        } else {
          new_val = raw_val + (safe_step - remainder);
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
        bool is_primary_axis = false;
        
        if (is_vertical) {
          if (kb.special == Event::Keyboard::Special::ArrowDown) {
            delta = -safe_step;
            is_primary_axis = true;
          } else if (kb.special == Event::Keyboard::Special::ArrowUp) {
            delta = safe_step;
            is_primary_axis = true;
          }
        } else {
          if (kb.special == Event::Keyboard::Special::ArrowLeft) {
            delta = -safe_step;
            is_primary_axis = true;
          } else if (kb.special == Event::Keyboard::Special::ArrowRight) {
            delta = safe_step;
            is_primary_axis = true;
          }
        }

        if (is_primary_axis) {
          int new_val = std::clamp(value + delta, min, max);
          if (new_val != value) {
            value = new_val;
            value_changed = true;
            PropagateBinding("value", std::to_string(value));
            // Trigger onchange
            if (root->Attributes().count("onchange")) {
                std::string onchange_cb = root->Attributes().at("onchange");
                if (auto* comp = GetAttributeOwnerComponent(root)) {
                    comp->RunCallback(onchange_cb);
                }
            }
            return true;
          }
          // If we are at the boundary and pressing in that direction, 
          // we don't return true, allowing spatial navigation to take over.
        }
      }
    }
  }

  if (value_changed) {
    PropagateBinding("value", std::to_string(value));

    // Run onchange callback if present
    if (root->Attributes().count("onchange")) {
      std::string onchange_cb = root->Attributes().at("onchange");
      if (auto* comp = GetAttributeOwnerComponent(root)) {
          comp->RunCallback(onchange_cb);
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
  int track_size = std::max(2, width);
  int range = max - min;
  int pos = 0;
  if (range > 0) {
    pos = static_cast<int>(
        std::round(static_cast<double>(value - min) / range * (track_size - 1)));
  }
  pos = std::clamp(pos, 0, track_size - 1);

  bool is_vertical = (direction == "vertical");
  container_class = is_vertical ? "slider-container vertical" : "slider-container horizontal";
  std::string char_sym = is_vertical ? "│" : "─";

  track_left = "";
  for (int i = 0; i < pos; ++i) {
    track_left += char_sym;
    if (is_vertical && i < pos - 1) track_left += "\n";
  }
  
  thumb_char = "●";
  
  track_right = "";
  for (int i = pos + 1; i < track_size; ++i) {
    track_right += char_sym;
    if (is_vertical && i < track_size - 1) track_right += "\n";
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
