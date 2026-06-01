// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#ifndef RTXUI_TERMINAL_SCREEN_HPP_
#define RTXUI_TERMINAL_SCREEN_HPP_

#include <memory>

#include "rtxui/internal/component.hpp"
#include "rtxui/internal/event.hpp"

namespace rtxui {

class ScreenImpl;
class TerminalDevice;

class Screen {
 public:
  explicit Screen(Ref<ComponentBase> component,
                  std::shared_ptr<TerminalDevice> device = nullptr);
  ~Screen();

  // Run the event loop (blocks until Escape or Ctrl+C is pressed)
  void Loop();

  // Run one step of the event loop
  void Step();

  // Dispatch a single event directly to the component tree
  void Dispatch(Event event);

  // Render and draw the component to the terminal
  void Draw();

  // Enable or disable smooth scrolling animations
  void SetSmoothScrollEnabled(bool enabled);
  bool smooth_scroll_enabled() const;

 private:
  std::unique_ptr<ScreenImpl> impl_;
};

}  // namespace rtxui

#endif  // RTXUI_TERMINAL_SCREEN_HPP_
