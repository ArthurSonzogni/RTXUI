// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#ifndef RTXUI_HEADLESS_HPP_
#define RTXUI_HEADLESS_HPP_

#include <memory>
#include <rtxui/rtxui_export.hpp>
#include <string>
#include <string_view>

#include "rtxui/component.hpp"

namespace rtxui {

class Screen;
class HeadlessTerminalDevice;

/// Runs an application without a terminal: at a fixed size, fed input as
/// bytes, read back as text. For tests, and for checking an interface from a
/// script or an agent.
///
/// Any program can also run this way without code changes: with the
/// environment variable RTXUI_HEADLESS=<width>x<height>, Screen::Loop() feeds
/// stdin to the application as input, prints the final screen as text to
/// stdout and returns.
///
///   printf '\t\r' | RTXUI_HEADLESS=80x24 ./my_app
class RTXUI_EXPORT HeadlessScreen {
 public:
  HeadlessScreen(Ref<ComponentBase> app, int width, int height);
  ~HeadlessScreen();
  HeadlessScreen(const HeadlessScreen&) = delete;
  HeadlessScreen& operator=(const HeadlessScreen&) = delete;

  /// Processes `bytes` as a terminal would send them: text is typed, and
  /// "\t", "\r", "\x7f" (Backspace), "\x1b[A" (Up), ... are keys. Returns
  /// once all of it is handled and transitions it started have finished.
  void Input(std::string_view bytes);

  /// Left-clicks the cell at column `x`, row `y`, both 0-based.
  void Click(int x, int y);

  /// Changes the size, as resizing the terminal would.
  void Resize(int width, int height);

  /// The screen as plain text: one line per row, without colors or styles,
  /// trailing spaces removed.
  std::string Text() const;

 private:
  std::shared_ptr<HeadlessTerminalDevice> device_;
  std::unique_ptr<Screen> screen_;
};

/// Renders `app` once at `width` x `height` and returns the screen as text,
/// as HeadlessScreen::Text() does.
RTXUI_EXPORT std::string RenderToString(Ref<ComponentBase> app,
                                        int width,
                                        int height);

}  // namespace rtxui

#endif  // RTXUI_HEADLESS_HPP_
