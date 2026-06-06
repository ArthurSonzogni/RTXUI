// Copyright 2024 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include "rtxui/component/default/input/input.hpp"

namespace rtxui {

void input::InitReflection() {
  Bind(value);
  Bind(left_text);
  Bind(cursor_char);
  Bind(right_text);
  Bind(cursor_class);
  Component<input>::InitReflection();
}

std::string_view input::Setup() {
  return R"html(
    <span>{left_text}</span><span class="{cursor_class}">{cursor_char}</span><span>{right_text}</span>
    <style>
      self {
        display: inline flex;
        flex-direction: row;
        width: 20;
        padding-left: 1;
        padding-right: 1;
        overflow-x: scroll;
        scrollbar-width: none;
        white-space: nowrap;
        background-color: #1e293b;
        border: solid;
        border-color: #334155;
        transition: all 0.1s linear;
      }
      self:hover {
        background-color: #334155;
      }
      self:focus {
        background-color: #0c4a6e;
        border-color: #38bdf8;
        color: #fff;
      }
      .cursor {
        background-color: transparent;
      }
      .cursor-focused {
        background-color: #fff;
        color: #000;
      }
    </style>
  )html";
}

bool input::OnEvent(Event event) {
  return OnEventShared(this, event, false);
}

bool input::Digest() {
  DigestShared(this);
  bool changed = Component<input>::Digest();
  if (changed) {
    KeepCursorVisible(Root(), false);
  }
  return changed;
}

namespace {
int RegisterThis = []() {
  RegisterGlobalComponent("input", []() { return Ref<input>::New(); });
  return 0;
}();
}  // namespace

}  // namespace rtxui
