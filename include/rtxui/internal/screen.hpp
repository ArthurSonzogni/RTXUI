// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#ifndef RTXUI_TERMINAL_SCREEN_HPP_
#define RTXUI_TERMINAL_SCREEN_HPP_

#include <memory>
#include <rtxui/rtxui_export.hpp>

#include "rtxui/color.hpp"
#include "rtxui/internal/component.hpp"
#include "rtxui/internal/event.hpp"

namespace rtxui {

class ScreenImpl;
class TerminalDevice;

class RTXUI_EXPORT Screen {
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

  /// Declares what is behind the interface, which is what every
  /// partially-transparent color composites against.
  ///
  /// The default is transparent: the terminal's own background shows through
  /// wherever nothing paints over it, which is what lets an app sit in a
  /// themed or translucent terminal instead of stamping a rectangle onto it.
  /// The cost is that the color is unknown, so anything that needs to blend
  /// against it -- a semi-transparent overlay, a reversed border cell -- has to
  /// approximate. Naming it here makes all of that exact, at the price of no
  /// longer inheriting the terminal's background.
  void SetBackgroundColor(Color color);
  Color background_color() const;

 private:
  std::unique_ptr<ScreenImpl> impl_;
};

}  // namespace rtxui

#endif  // RTXUI_TERMINAL_SCREEN_HPP_
