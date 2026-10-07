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
class toast : public Component<toast> {
 public:
  bool open = false;
  int duration = 3000;
  std::string placement = "bottom-right";
  std::string toast_class = "closed";

  void Close();

  void InitReflection() override;
  std::string_view Setup();
  bool Digest() override;

 private:
  bool was_open_ = false;
  // Bumped each time the toast opens, so that the timer of an earlier
  // opening cannot close a later one early.
  int generation_ = 0;
};

}  // namespace rtxui

#endif  // RTXUI_COMPONENT_DEFAULT_TOAST_HPP_
