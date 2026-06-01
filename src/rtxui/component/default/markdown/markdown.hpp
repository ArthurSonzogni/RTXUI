// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#ifndef RTXUI_COMPONENT_DEFAULT_MARKDOWN_HPP_
#define RTXUI_COMPONENT_DEFAULT_MARKDOWN_HPP_

#include <string>
#include <string_view>

#include "rtxui/internal/component.hpp"

namespace rtxui {

class markdown : public Component<markdown> {
 public:
  std::string content;
  std::string stylesheet;

  void InitReflection() override;
  std::string_view Setup() override;
  std::string_view GetView() const override;
  bool Digest() override;

 private:
  mutable std::string generated_html_;
};

}  // namespace rtxui

#endif  // RTXUI_COMPONENT_DEFAULT_MARKDOWN_HPP_
