// Copyright 2024 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include "rtxui/component/default/hr/hr.hpp"

#include "rtxui/dom/element.hpp"
#include "rtxui/style/style.hpp"

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
  if (auto* root = Root()) {
    // The rule spans the element, so its length is only known once layout has
    // run. Before the first layout, fall back to the terminal width: it is an
    // upper bound for the final length, and `overflow: hidden` trims the
    // excess, so the very first frame already paints a full-width rule
    // instead of nothing.
    int layout_w = root->layout_width();
    if (layout_w <= 0) {
      layout_w = css::g_terminal_width;
    }
    std::string new_line;
    for (int i = 0; i < layout_w; ++i) {
      new_line += "─";
    }
    line_chars = new_line;
  }
  // Always run the base digest: it is what compares the bound state against
  // its snapshot and patches the DOM. Returning early on a `line_chars`
  // change would leave the new value unrendered until some later digest.
  return Component<hr>::Digest();
}

namespace {
int RegisterThis = []() {
  RegisterGlobalComponent("hr", []() { return Ref<hr>::New(); });
  return 0;
}();
}  // namespace

}  // namespace rtxui
