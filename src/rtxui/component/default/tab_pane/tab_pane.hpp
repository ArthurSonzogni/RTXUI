// Copyright 2024 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#ifndef RTXUI_COMPONENT_DEFAULT_TAB_PANE_HPP_
#define RTXUI_COMPONENT_DEFAULT_TAB_PANE_HPP_

#include <string_view>

#include "rtxui/internal/component.hpp"

namespace rtxui {

class tab_pane : public Component<tab_pane> {
 public:
  static const std::string_view view;
};

}  // namespace rtxui

#endif  // RTXUI_COMPONENT_DEFAULT_TAB_PANE_HPP_
