// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#ifndef RTXUI_TESTING_TIMELINE_HPP_
#define RTXUI_TESTING_TIMELINE_HPP_

#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "rtxui/component.hpp"
#include "rtxui/screen.hpp"
#include "rtxui/terminal/terminal_device.hpp"

namespace rtxui::testing {

// Runs an application on a clock that moves only when the test moves it, so
// what animations, transitions, smooth scrolling and delayed tasks draw can be
// checked frame by frame, at exact times. One at a time: it replaces the
// process-wide clock until it is destroyed.
class Timeline {
 public:
  Timeline(Ref<ComponentBase> app, int width, int height);
  ~Timeline();
  Timeline(const Timeline&) = delete;
  Timeline& operator=(const Timeline&) = delete;

  // Moves time forward by `ms`, then runs one frame: due tasks, animations,
  // then the draw. Returns the screen as text.
  std::string Advance(double ms);

  // Advances `frame_ms` at a time until `duration_ms` have passed, and
  // returns each frame that differs from the one before it.
  std::vector<std::string> Record(double duration_ms, double frame_ms = 16);

  // Feeds terminal input, and runs one frame without moving time.
  void Input(std::string_view bytes);
  // A left click on the cell at column `x`, row `y`, from 0.
  void Click(int x, int y);

  // The screen as text, one line per row, trailing spaces trimmed.
  std::string Text() const;
  // Milliseconds since the timeline started.
  double Elapsed() const;

 private:
  std::shared_ptr<MockTerminalDevice> device_;
  std::unique_ptr<Screen> screen_;
};

}  // namespace rtxui::testing

#endif  // RTXUI_TESTING_TIMELINE_HPP_
