// Copyright 2024 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include "rtxui/component/default/hr/hr.hpp"

#include "rtxui/dom/element.hpp"

namespace rtxui {

void hr::InitReflection() {
  Bind(line_chars);
  Component<hr>::InitReflection();
}

std::string_view hr::Setup() {
  return R"html(<span class="hr-span">{line_chars}</span><style>
    self {
      display: block;
      margin-top: 1;
      margin-bottom: 1;
      overflow: hidden;
      white-space: nowrap;
    }
    .hr-span {
      color: #555;
    }
  </style>)html";
}

bool hr::Digest() {
  auto* root = Root();
  if (root) {
    int layout_w = root->layout_width();
    if (layout_w <= 0) {
      layout_w = 80;
    }
    std::string new_line;
    for (int i = 0; i < layout_w; ++i) {
      new_line += "─";
    }
    if (line_chars != new_line) {
      line_chars = new_line;
      return true;
    }
  }
  return Component<hr>::Digest();
}

namespace {
int RegisterThis = []() {
  RegisterGlobalComponent("hr", []() { return Ref<hr>::New(); });
  return 0;
}();
}  // namespace

}  // namespace rtxui
