// Copyright 2024 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#pragma once

#include <source_location>
#include <string_view>
#include <vector>

namespace rtxui {
class Component;

struct Register {
  explicit Register(Component* component);

  static std::vector<std::string> ComputeNamespaces(std::source_location);
  static const Component* Get(
      std::string_view name,
      const std::vector<std::string>& namespaces =
          ComputeNamespaces(std::source_location::current()));

  // Debug
  static std::string Print();
};

}  // namespace rtxui
