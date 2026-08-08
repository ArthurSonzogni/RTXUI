// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include "rtxui/component/default/blockquote/blockquote.hpp"

namespace rtxui {

const std::string_view blockquote::view = R"html(
    <slot></slot>
    <style>
      self {
        display: block;
        margin-top: 1;
        margin-bottom: 1;
        padding-left: 1;
        border-left: 1;
        border-style: solid;
        border-color: #555;
      }
    </style>
  )html";

namespace {
int RegisterThis = []() {
  RegisterGlobalComponent("blockquote", []() { return Ref<blockquote>::New(); });
  return 0;
}();
}  // namespace
}  // namespace rtxui
