// Copyright 2024 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include "rtxui/component/default/ul/ul.hpp"

namespace rtxui {

const std::string_view ul::view = R"html(
    <slot></slot>
    <style>
      self { 
        display: block; 
        padding-left: 2;
      }
    </style>
  )html";
}  // namespace rtxui
