// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#ifndef RTXUI_SCREEN_HPP_
#define RTXUI_SCREEN_HPP_

#include <memory>
#include <rtxui/rtxui_export.hpp>
#include <string>

#include "rtxui/color.hpp"
#include "rtxui/component.hpp"
#include "rtxui/event.hpp"

namespace rtxui {

class ScreenImpl;
class TerminalDevice;

class RTXUI_EXPORT Screen {
 public:
  explicit Screen(Ref<ComponentBase> component,
                  std::shared_ptr<TerminalDevice> device = nullptr);
  ~Screen();

  /// Runs the event loop. Returns after Exit(), or when an Escape or Ctrl+C
  /// that no component handled arrives, or when the input closes.
  void Loop();

  /// Makes Loop() return once the event being handled is done. Call it from
  /// the UI thread, typically from a callback; from another thread, post it
  /// with PostTask().
  void Exit();

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

  /// The last frame drawn, as plain text: one line per row, without colors or
  /// styles, trailing spaces removed.
  std::string Text() const;

 private:
  friend class HeadlessScreen;
  std::unique_ptr<ScreenImpl> impl_;
};

}  // namespace rtxui

#endif  // RTXUI_SCREEN_HPP_
