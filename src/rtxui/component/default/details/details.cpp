// Copyright 2024 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include "rtxui/component/default/details/details.hpp"

#include "rtxui/component/component_internal.hpp"
#include "rtxui/dom/element.hpp"
#include "rtxui/dom/text_element.hpp"

namespace rtxui {

void details::Toggle() {
  open = !open;
  PropagateBinding("open", open ? "true" : "false");
}

void details::InitReflection() {
  Bind(open);
  Bind(arrow_char);
  Bind(content_class);
  Bind(no_summary);
  Bind(Toggle);
  Component<details>::InitReflection();
}

std::string_view details::Setup() {
  return R"html(
    <div class="details-container" part="details-container">
      <div class="summary-line" part="summary-line" tabindex="0" onclick="Toggle()">
        <span class="arrow" part="arrow">{arrow_char}</span>
        <slot.summary select="summary"></slot.summary>
        <if condition="{no_summary}"><span>Details</span></if>
      </div>
      <div class="details-content {content_class}" part="details-content">
        <slot></slot>
      </div>
    </div>
    <style>
      self {
        display: block;
      }
      .summary-line {
        display: flex;
        flex-direction: row;
        cursor: pointer;
        font-weight: bold;
      }
      .summary-line:hover {
        background-color: lighten(10%);
      }
      .summary-line:focus {
        background-color: lighten(20%);
      }
      .arrow {
        margin-right: 1;
      }
      .details-content {
        display: block;
        padding-left: 2;
      }
      .closed {
        display: none;
      }
    </style>
  )html";
}

bool details::OnEvent(Event event) {
  auto* root = Root();
  if (!root) {
    return false;
  }

  if (event.is<Event::Keyboard>()) {
    auto kb = event.get<Event::Keyboard>();
    if (kb.motion == Event::Keyboard::Motion::Pressed ||
        kb.motion == Event::Keyboard::Motion::Repeat) {
      auto* summary_btn = root->QuerySelector(".summary-line");
      if (summary_btn && summary_btn->focused()) {
        if (kb.special == Event::Keyboard::Special::Return ||
            (kb.special == Event::Keyboard::Special::None &&
             kb.codepoint == 32)) {
          Toggle();
          return true;
        }
      }
    }
  }

  return Component<details>::OnEvent(event);
}

bool details::Digest() {
  arrow_char = open ? "\u25bc" : "\u25b6";
  content_class = open ? "open" : "closed";

  // The <summary> is routed into the summary slot by `select`; only the
  // fallback label is left to decide.
  auto summary_slot = Slot("summary");
  no_summary = !summary_slot || summary_slot->ChildCount() == 0;

  return Component<details>::Digest();
}

namespace {
int RegisterThis = []() {
  RegisterGlobalComponent("details", []() { return Ref<details>::New(); });
  return 0;
}();
}  // namespace
}  // namespace rtxui
