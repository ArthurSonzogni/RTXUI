// Copyright 2024 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include "rtxui/component/default/textarea/textarea.hpp"

namespace rtxui {

void textarea::InitReflection() {
  Bind(value);
  Bind(disabled);
  Bind(readonly);
  Bind(placeholder);
  Bind(placeholder_text);
  Bind(selection_start);
  Bind(left_text);
  Bind(cursor_char);
  Bind(right_text);
  Bind(cursor_class);
  Bind(left_unselected);
  Bind(left_selected);
  Bind(right_selected);
  Bind(right_unselected);
  Bind(selection_class_left);
  Bind(selection_class_right);
  Component<textarea>::InitReflection();
}

std::string_view textarea::Setup() {
  return R"html(
    <span>{left_unselected}</span><span class="{selection_class_left}">{left_selected}</span><span class="{cursor_class}">{cursor_char}</span><span class="{selection_class_right}">{right_selected}</span><span>{right_unselected}</span><span class="placeholder">{placeholder_text}</span>
    <style>
      self {
        display: block;
        width: 40;
        height: 5;
        white-space: pre-wrap;
        padding-left: 1;
        padding-right: 1;
        overflow-y: scroll;
        scrollbar-width: none;
        background-color: rgb(40, 40, 40);
        opacity: 0.8;
        transition: background-color 0.1s linear, opacity 0.1s linear, color 0.1s linear;
      }
      /* Fixed absolute colors, not lighten(): self:hover and self:focus can
         both match at once (e.g. clicking focuses the textarea while the
         mouse is still over it, so it's also hovered), and since both
         rules apply to the same style in cascade order, lighten() would
         compound -- self:focus lightening the already-lightened
         self:hover result -- producing a much brighter, washed-out color
         instead of a stable focus look. */
      self:hover {
        background-color: rgb(75, 75, 75);
        opacity: 0.9;
      }
      self:focus {
        background-color: rgb(101, 101, 101);
        opacity: 1.0;
      }
      .cursor {
        background-color: transparent;
      }
      .cursor-focused {
        background-color: transparent;
      }
      /* An absolute color pair (not derived from self's background) so the
         selection highlight stays readable no matter what color an app
         gives the textarea: self's background-color is a separate element,
         so it can't be referenced here (background-color doesn't inherit,
         and nested-component style resolution finishes before an outer
         app's overrides are known anyway). */
      .selection {
        background-color: rgb(38, 79, 120);
        color: white;
      }
      /* Fixed absolute color, not lighten()/dim, for the same reason the
         selection colors are fixed above. */
      .placeholder {
        color: rgb(150, 150, 150);
      }
      /* Listed after :hover/:focus so it wins the cascade for a
         disabled-and-hovered field (mouse hover isn't gated on
         interactivity, unlike focus, which OnEventShared/DigestShared
         never let a disabled field acquire). */
      self:disabled {
        background-color: rgb(40, 40, 40);
        opacity: 0.4;
      }
    </style>
  )html";
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
