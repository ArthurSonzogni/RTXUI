// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include "rtxui/component/default/a/a.hpp"

#include "rtxui/paint/paint.hpp"

namespace rtxui {

// Painting turns the text of a link into a terminal hyperlink, and only looks
// for one while a link exists.
a::a() {
  CountLinkElements(1);
}

a::~a() {
  CountLinkElements(-1);
}

const std::string_view a::view = R"html(
    <slot></slot>
    <style>
      self {
        display: inline;
        text-decoration: underline;
        color: #3b82f6;
        cursor: pointer;
      }
      self:focus {
        background-color: rgb(37, 99, 235);
        color: white;
      }
    </style>
  )html";
}  // namespace rtxui
