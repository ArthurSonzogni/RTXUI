// Copyright 2024 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#ifndef RTXUI_COMPONENT_DEFAULT_PROGRESS_HPP_
#define RTXUI_COMPONENT_DEFAULT_PROGRESS_HPP_

#include <string>
#include <string_view>

#include "rtxui/internal/component.hpp"

namespace rtxui {

class progress : public Component<progress> {
 public:
  double value = 0;
  double max = 100;
  int width = 20;

  // Render bindings
  std::string filled_track;
  std::string empty_track;

  void InitReflection() override;
  std::string_view Setup() override;
  bool Digest() override;
};

}  // namespace rtxui

#endif  // RTXUI_COMPONENT_DEFAULT_PROGRESS_HPP_
