// Copyright 2024 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include "rtxui/component/default/tab_pane/tab_pane.hpp"

namespace rtxui {

const std::string_view tab_pane::view = R"html(
    <slot></slot>
  )html";
}  // namespace rtxui
