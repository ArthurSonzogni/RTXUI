// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#ifndef RTXUI_TERMINAL_SCREEN_HPP_
#define RTXUI_TERMINAL_SCREEN_HPP_

#include <memory>
#include <string>

#include "rtxui/internal/component.hpp"
#include "rtxui/internal/event.hpp"

class TerminalInputParser;

namespace rtxui {
class TerminalDevice;
struct PhysicalFragment;
} // namespace rtxui

namespace rtxui {

struct PhysicalFragment;

class Screen {
 public:
   explicit Screen(Ref<ComponentBase> component, std::shared_ptr<TerminalDevice> device = nullptr);
   ~Screen();

   // Run the event loop (blocks until Escape or Ctrl+C is pressed)
   void Loop();

   // Run one step of the event loop
   void Step();

   // Dispatch a single event directly to the component tree
   void Dispatch(Event event);

   // Render and draw the component to the terminal
   void Draw();

  private:
   void UpdateSize();
   void DigestAndDraw();
   void HandleEvent(const Event& event);

   Ref<ComponentBase> component_;
   int width_ = 80;
   int height_ = 24;
   int last_height_ = 0;
   bool has_drawn_ = false;
   bool running_ = true;
   std::shared_ptr<PhysicalFragment> root_fragment_;
   std::shared_ptr<TerminalDevice> device_;
   std::unique_ptr<TerminalInputParser> parser_;
   Element* focused_element_ = nullptr;

   // RAII raw terminal controller
   struct RawTerminal {
     TerminalDevice* device_ = nullptr;
     explicit RawTerminal(TerminalDevice* device);
     ~RawTerminal();
   };
};

} // namespace rtxui

#endif // RTXUI_TERMINAL_SCREEN_HPP_
