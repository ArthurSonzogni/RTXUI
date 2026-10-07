// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#ifndef RTXUI_COMPONENT_DEFAULT_TOAST_HPP_
#define RTXUI_COMPONENT_DEFAULT_TOAST_HPP_

#include <string>
#include <string_view>

#include "rtxui/component.hpp"

namespace rtxui {

// A short notification in a corner of the screen, over everything else, which
// closes itself after `duration` milliseconds (never, when 0) or when clicked.
// `open` is two-way bound, so the application's flag follows it closing.
//
// It slides in from its side of the screen, and slides back out when it
// closes: it stays on screen, as part `closing`, until that animation ends.
class toast : public Component<toast> {
 public:
  bool open = false;
  int duration = 3000;
  std::string placement = "bottom-right";
  std::string toast_class = "closed";
  std::string toast_part = "toast";

  void Close();
  void AnimationEnded();

  void InitReflection() override;
  std::string_view Setup();
  bool Digest() override;

 private:
  // Hides the toast once it has closed, its closing animation done.
  void FinishClosing();

  bool was_open_ = false;
  // Closed, but still on screen while its closing animation plays.
  bool closing_ = false;
  // Bumped each time the toast opens, so that the timer of an earlier
  // opening cannot close a later one early.
  int generation_ = 0;
};

}  // namespace rtxui

#endif  // RTXUI_COMPONENT_DEFAULT_TOAST_HPP_
