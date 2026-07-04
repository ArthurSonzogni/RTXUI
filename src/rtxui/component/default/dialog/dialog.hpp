// Copyright 2024 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#ifndef RTXUI_COMPONENT_DEFAULT_DIALOG_HPP_
#define RTXUI_COMPONENT_DEFAULT_DIALOG_HPP_

#include <string>
#include <string_view>

#include "rtxui/internal/component.hpp"

namespace rtxui {

class dialog : public Component<dialog> {
 public:
  bool open = false;
  std::string title = "Dialog";
  std::string overlay_class = "closed";

  void InitReflection() override;
  std::string_view Setup() override;
  bool Digest() override;
  bool OnEvent(Event event) override;
};

}  // namespace rtxui

#endif  // RTXUI_COMPONENT_DEFAULT_DIALOG_HPP_
