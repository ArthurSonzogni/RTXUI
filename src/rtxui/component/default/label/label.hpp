// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#ifndef RTXUI_COMPONENT_DEFAULT_LABEL_HPP_
#define RTXUI_COMPONENT_DEFAULT_LABEL_HPP_

#include <string_view>

#include "rtxui/internal/component.hpp"

namespace rtxui {

class label : public Component<label> {
 public:
  static const std::string_view view;

  bool OnEvent(Event event) override;
};

}  // namespace rtxui

#endif  // RTXUI_COMPONENT_DEFAULT_LABEL_HPP_
