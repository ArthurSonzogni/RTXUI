// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include "rtxui/component/default/i/i.hpp"

namespace rtxui {

const std::string_view i::view = R"html(
    <slot></slot>
    <style>
      self {
        display: inline;
        font-style: italic;
      }
    </style>
  )html";

const std::string_view em::view = R"html(
    <slot></slot>
    <style>
      self {
        display: inline;
        font-style: italic;
      }
    </style>
  )html";
}  // namespace rtxui
