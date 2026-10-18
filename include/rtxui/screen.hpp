// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#ifndef RTXUI_SCREEN_HPP_
#define RTXUI_SCREEN_HPP_

#include <memory>
#include <rtxui/rtxui_export.hpp>
#include <string>
#include <string_view>

#include "rtxui/color.hpp"
#include "rtxui/component.hpp"
#include "rtxui/event.hpp"

namespace rtxui {

class ScreenImpl;
class TerminalDevice;

class RTXUI_EXPORT Screen {
 public:
  explicit Screen(Ref<ComponentBase> component);
  ~Screen();
  Screen(const Screen&) = delete;
  Screen& operator=(const Screen&) = delete;

  /// Runs the event loop. Returns after Exit(), or when an Escape or Ctrl+C
  /// that no component handled arrives, or when the input closes.
  void Loop();

  /// Makes Loop() return once the event being handled is done. Call it from
  /// the UI thread, typically from a callback; from another thread, post it
  /// with PostTask().
  void Exit();

  /// Draws the application where the cursor is, on the terminal's main
  /// screen, rather than on the alternate screen over the whole terminal.
  /// This is the default: SetInline(false) gives the application the whole
  /// terminal instead, as a full-screen program such as an editor takes it.
  ///
  /// The application is then as tall as its content, as a web page is, up to
  /// the terminal's height: `height: 100%` still makes it fill the terminal.
  /// The shell's lines stay above it. When Loop() returns, the last frame
  /// drawn stays there too, with the shell going on below it: an application
  /// that wants to leave nothing behind draws nothing before it exits.
  ///
  /// Call it before Loop().
  void SetInline(bool enabled);
  bool is_inline() const;

  /// Writes `text` above the application, where it stays: inline, as the
  /// lines the shell scrolls away; on the alternate screen, once Loop()
  /// returns. A line break ends it unless it already ends with one.
  ///
  /// Call it from the UI thread; from another one, post it with PostTask().
  void Print(std::string_view text);
  /// Writes what `component` draws, as wide as the terminal and as tall as
  /// it is, as Print(text) writes text.
  void Print(Ref<ComponentBase> component);

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
  /// Anything that needs to blend against it -- a semi-transparent overlay, a
  /// reversed border cell -- uses the color the terminal reports when asked
  /// (OSC 11), and approximates in a terminal that does not answer. Naming it
  /// here makes all of that exact everywhere, at the price of no longer
  /// inheriting the terminal's background.
  ///
  /// Text no style gives a color to is likewise drawn in the terminal's own
  /// text color (OSC 10), which reads on its background, light or dark. Once
  /// the app names a background, that text is white instead.
  void SetBackgroundColor(Color color);
  Color background_color() const;

  /// The last frame drawn, as plain text: one line per row, without colors or
  /// styles, trailing spaces removed.
  std::string Text() const;

 private:
  friend struct ScreenInternals;
  Screen(Ref<ComponentBase> component, std::shared_ptr<TerminalDevice> device);
  std::unique_ptr<ScreenImpl> impl_;
};

}  // namespace rtxui

#endif  // RTXUI_SCREEN_HPP_
