// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include "rtxui/component/default/s/s.hpp"

namespace rtxui {

const std::string_view s::view = R"html(
    <slot></slot>
    <style>
      self { 
        display: inline; 
        text-decoration: line-through;
      }
    </style>
  )html";

const std::string_view strike::view = R"html(
    <slot></slot>
    <style>
      self { 
        display: inline; 
        text-decoration: line-through;
      }
    </style>
  )html";

const std::string_view del::view = R"html(
    <slot></slot>
    <style>
      self { 
        display: inline; 
        text-decoration: line-through;
      }
    </style>
  )html";

namespace {
int RegisterThis = []() {
  RegisterGlobalComponent("s", []() { return Ref<s>::New(); });
  RegisterGlobalComponent("strike", []() { return Ref<strike>::New(); });
  RegisterGlobalComponent("del", []() { return Ref<del>::New(); });
  return 0;
}();
}  // namespace
}  // namespace rtxui
