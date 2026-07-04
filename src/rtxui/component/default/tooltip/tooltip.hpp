// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#ifndef RTXUI_COMPONENT_DEFAULT_TOOLTIP_HPP_
#define RTXUI_COMPONENT_DEFAULT_TOOLTIP_HPP_

#include <string>
#include <string_view>

#include "rtxui/internal/component.hpp"

namespace rtxui {

class tooltip : public Component<tooltip> {
 public:
  std::string content = "";
  std::string placement = "top";
  std::string tooltip_class = "hidden";

  void InitReflection() override;
  std::string_view Setup() override;
  bool Digest() override;
};

}  // namespace rtxui

#endif  // RTXUI_COMPONENT_DEFAULT_TOOLTIP_HPP_
