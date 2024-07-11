// Copyright 2024 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#ifndef APP_HPP_
#define APP_HPP_

#include <functional>
#include <map>
#include <string_view>

#include "dom/node.hpp"
#include "element.hpp"

namespace rtxui {

// An App is a collection of components.
class App {
 public:
  App() = default;

  // Registers a component with the given label.
  void Register(const std::string_view label, std::function<void(Element&)> f);

  void Render(const std::string_view label);

 private:
  std::map<std::string, std::function<void(Element&)>> components_;

  Ref<Element> root_element_ = nullptr;
  Ref<Node> root_node_ = nullptr;
};

}  // namespace rtxui

#endif  // APP_HPP_
