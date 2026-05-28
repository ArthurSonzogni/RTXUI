// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#ifndef RTXUI_TERMINAL_SCREEN_HPP_
#define RTXUI_TERMINAL_SCREEN_HPP_

#include <termios.h>
#include <signal.h>
#include <memory>
#include <string>

#include "component/component.hpp"
#include "terminal/terminal_device.hpp"
#include "terminal/event.hpp"

namespace rtxui {

struct PhysicalFragment;

class Screen {
 public:
  explicit Screen(Ref<ComponentBase> component, std::shared_ptr<TerminalDevice> device = nullptr);
  ~Screen();

  // Run the event loop (blocks until Escape or Ctrl+C is pressed)
  void Loop();

  // Render and draw the component to the terminal
  void Draw();

 private:
  void UpdateSize();
  void DigestAndDraw();

  Ref<ComponentBase> component_;
  int width_ = 80;
  int height_ = 24;
  int last_height_ = 0;
  bool has_drawn_ = false;
  std::shared_ptr<PhysicalFragment> root_fragment_;
  std::shared_ptr<TerminalDevice> device_;

  // RAII raw terminal controller
  struct RawTerminal {
    TerminalDevice* device_ = nullptr;
    explicit RawTerminal(TerminalDevice* device);
    ~RawTerminal();
  };
};

} // namespace rtxui

#endif // RTXUI_TERMINAL_SCREEN_HPP_
