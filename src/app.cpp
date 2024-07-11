// Copyright 2024 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include "app.hpp"

namespace rtxui {

void App::Register(const std::string_view label,  //
                   std::function<void(Element&)> f) {
  components_[std::string(label)] = f;
}

void App::Render(const std::string_view label) {
  root_element_ = Ref<Element>();
  root_node_ = Ref<Node>();

  components_[std::string(label)](*root_element_);
}

}  // namespace rtxui
