// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include "rtxui/testing/timeline.hpp"

#include <utility>

#include "rtxui/base/clock.hpp"

namespace rtxui::testing {

namespace {
// Not zero: some code treats a time of 0 as "never".
constexpr double kStart = 1000.0;
double g_now = kStart;
double Now() {
  return g_now;
}
}  // namespace

Timeline::Timeline(Ref<ComponentBase> app, int width, int height)
    : device_(std::make_shared<MockTerminalDevice>()) {
  g_now = kStart;
  time::SetCustomClock(&Now);
  device_->TriggerResize(width, height);
  screen_ = std::make_unique<Screen>(std::move(app), device_);
  screen_->Draw();
}

Timeline::~Timeline() {
  screen_.reset();
  time::SetCustomClock(nullptr);
}

std::string Timeline::Advance(double ms) {
  g_now += ms;
  screen_->Step();
  // Only the text is read back; the escape sequences would pile up.
  device_->ClearOutput();
  return Text();
}

std::vector<std::string> Timeline::Record(double duration_ms, double frame_ms) {
  std::vector<std::string> frames = {Text()};
  for (double elapsed = 0; elapsed < duration_ms; elapsed += frame_ms) {
    std::string frame = Advance(frame_ms);
    if (frame != frames.back()) {
      frames.push_back(std::move(frame));
    }
  }
  return frames;
}

void Timeline::Input(std::string_view bytes) {
  device_->PushInput(bytes);
  Advance(0);
}

void Timeline::Click(int x, int y) {
  const std::string column = std::to_string(x + 1);
  const std::string row = std::to_string(y + 1);
  Input("\x1b[<0;" + column + ";" + row + "M" + "\x1b[<0;" + column + ";" +
        row + "m");
}

std::string Timeline::Text() const {
  return screen_->Text();
}

double Timeline::Elapsed() const {
  return g_now - kStart;
}

}  // namespace rtxui::testing
