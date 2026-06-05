// Copyright 2024 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include "rtxui/component/default/h1/h1.hpp"

namespace rtxui {

const std::string_view h1::view = R"html(
    <slot></slot>
    <style>
      self { 
        display: block; 
        font-weight: bold;
        text-decoration: underlined;
        margin-bottom: 1;
      }
    </style>
  )html";

const std::string_view h2::view = R"html(
    <slot></slot>
    <style>
      self { 
        display: block; 
        font-weight: bold;
        margin-bottom: 1;
      }
    </style>
  )html";

const std::string_view h3::view = R"html(
    <slot></slot>
    <style>
      self { 
        display: block; 
        font-weight: bold;
        margin-bottom: 1;
      }
    </style>
  )html";

const std::string_view h4::view = R"html(
    <slot></slot>
    <style>
      self { 
        display: block; 
        font-weight: bold;
      }
    </style>
  )html";

const std::string_view h5::view = R"html(
    <slot></slot>
    <style>
      self { 
        display: block; 
        font-weight: bold;
      }
    </style>
  )html";

const std::string_view h6::view = R"html(
    <slot></slot>
    <style>
      self { 
        display: block; 
        font-weight: bold;
      }
    </style>
  )html";

namespace {
int RegisterThis = []() {
  RegisterGlobalComponent("h1", []() { return Ref<h1>::New(); });
  RegisterGlobalComponent("h2", []() { return Ref<h2>::New(); });
  RegisterGlobalComponent("h3", []() { return Ref<h3>::New(); });
  RegisterGlobalComponent("h4", []() { return Ref<h4>::New(); });
  RegisterGlobalComponent("h5", []() { return Ref<h5>::New(); });
  RegisterGlobalComponent("h6", []() { return Ref<h6>::New(); });
  return 0;
}();
}  // namespace
}  // namespace rtxui
