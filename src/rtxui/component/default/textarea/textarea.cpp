// Copyright 2024 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include "rtxui/component/default/textarea/textarea.hpp"

namespace rtxui {

void textarea::InitReflection() {
  Bind(value);
  Bind(left_text);
  Bind(cursor_char);
  Bind(right_text);
  Bind(cursor_class);
  Component<textarea>::InitReflection();
}

std::string_view textarea::Setup() {
  return R"html(<span>{left_text}</span><span class="{cursor_class}">{cursor_char}</span><span>{right_text}</span><style>
      self {
        display: block;
        width: 40;
        height: 5;
        padding-left: 1;
        padding-right: 1;
        overflow-y: scroll;
        scrollbar-width: none;
      }
      .cursor {
        background-color: transparent;
      }
      .cursor-focused {
        background-color: white;
        color: black;
      }
    </style>)html";
}

bool textarea::OnEvent(Event event) {
  return OnEventShared(this, event, true);
}

bool textarea::Digest() {
  DigestShared(this);
  bool changed = Component<textarea>::Digest();
  if (changed) {
    KeepCursorVisible(Root(), true);
  }
  return changed;
}

namespace {
int RegisterThis = []() {
  RegisterGlobalComponent("textarea", []() { return Ref<textarea>::New(); });
  return 0;
}();
}  // namespace

}  // namespace rtxui
