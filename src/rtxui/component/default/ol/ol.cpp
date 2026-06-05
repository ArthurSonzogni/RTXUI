// Copyright 2024 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include "rtxui/component/default/ol/ol.hpp"

namespace rtxui {

const std::string_view ol::view = R"html(
    <slot></slot>
    <style>
      self { 
        display: block; 
        padding-left: 3;
      }
    </style>
  )html";

namespace {
int RegisterThis = []() {
  RegisterGlobalComponent("ol", []() { return Ref<ol>::New(); });
  return 0;
}();
}  // namespace
}  // namespace rtxui
