// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include "rtxui/component/default/semantic/semantic.hpp"

namespace rtxui {

const std::string_view header::view = R"html(
    <slot></slot>
    <style>
      self { display: block; }
    </style>
  )html";

const std::string_view footer::view = R"html(
    <slot></slot>
    <style>
      self { display: block; }
    </style>
  )html";

const std::string_view main::view = R"html(
    <slot></slot>
    <style>
      self { display: block; }
    </style>
  )html";

const std::string_view nav::view = R"html(
    <slot></slot>
    <style>
      self { display: block; }
    </style>
  )html";

const std::string_view aside::view = R"html(
    <slot></slot>
    <style>
      self { display: block; }
    </style>
  )html";

const std::string_view section::view = R"html(
    <slot></slot>
    <style>
      self { display: block; }
    </style>
  )html";

const std::string_view article::view = R"html(
    <slot></slot>
    <style>
      self { display: block; }
    </style>
  )html";
}  // namespace rtxui
