// Copyright 2024 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include "rtxui/component/default/tabs/tabs.hpp"

#include <charconv>
#include <iostream>

#include "rtxui/component/component_internal.hpp"
#include "rtxui/dom/element.hpp"
#include "rtxui/dom/text_element.hpp"

namespace rtxui {

void tabs::SelectTab(std::string index_str) {
  std::string_view s = index_str;
  while (!s.empty() && std::isspace(static_cast<unsigned char>(s.front()))) {
    s.remove_prefix(1);
  }
  while (!s.empty() && std::isspace(static_cast<unsigned char>(s.back()))) {
    s.remove_suffix(1);
  }
  if (s.empty()) {
    return;
  }
  size_t idx = 0;
  auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), idx);
  if (ec != std::errc() || ptr != s.data() + s.size()) {
    return;
  }
  auto panes = GetTabPanes();
  if (idx < panes.size()) {
    value = panes[idx].name;
    PropagateBinding("value", value);

    auto* root = Root();
    if (root && root->Attributes().count("onchange")) {
      std::string onchange_cb = root->Attributes().at("onchange");
      if (auto* comp = GetAttributeOwnerComponent(root)) {
        comp->RunCallback(onchange_cb);
      }
    }
  }
}

void tabs::InitReflection() {
  Bind(value);
  Bind(SelectTab);
  Component<tabs>::InitReflection();
}

std::string_view tabs::Setup() {
  return R"html(
    <div class="tabs-container" part="tabs-container">
      <div class="tabs-headers" part="tabs-headers">
        <slot.headers></slot.headers>
      </div>
      <div class="tabs-content" part="tabs-content">
        <slot></slot>
      </div>
    </div>
    <style>
      self {
        display: flex;
        flex-direction: column;
        width: 100%;
      }
      .tabs-headers {
        display: flex;
        flex-direction: row;
        /* An auto-width flex box shrinks to fit its items, which would stop
           the bottom rule right after the last tab instead of running the
           full width of the tab strip. */
        width: 100%;
        border-bottom: 1;
        border-style: solid;
        border-color: rgb(74, 85, 104);
        margin-bottom: 1;
      }
      .tab-header-btn {
        padding-left: 2;
        padding-right: 2;
        cursor: pointer;
        background-color: lighten(5%);
        transition: background-color 0.1s linear, color 0.1s linear;
      }
      .tab-header-btn:hover {
        background-color: lighten(12%);
      }
      .tab-header-btn:focus {
        background-color: lighten(20%);
      }
      .active-tab {
        background-color: rgb(59, 130, 246);
        color: white;
        font-weight: bold;
      }
      .tabs-content {
        display: block;
      }
      .active {
        display: block;
      }
      .inactive {
        display: none;
      }
    </style>
  )html";
}

std::vector<TabPaneInfo> tabs::GetTabPanes() {
  std::vector<TabPaneInfo> panes;
  auto* root = Root();
  if (!root) {
    return panes;
  }

  root->Visit([&](Element& el) {
    if (el.tag() == "tab-pane" || el.tag() == "tab_pane") {
      std::string name;
      if (el.Attributes().count("name")) {
        name = el.Attributes().at("name");
      }
      std::string label;
      if (el.Attributes().count("label")) {
        label = el.Attributes().at("label");
      }
      panes.push_back({name, label, &el});
    }
  });
  return panes;
}

bool tabs::OnEvent(Event event) {
  auto* root = Root();
  if (!root) {
    return false;
  }

  if (event.is<Event::Keyboard>()) {
    auto kb = event.get<Event::Keyboard>();
    if (kb.motion == Event::Keyboard::Motion::Pressed ||
        kb.motion == Event::Keyboard::Motion::Repeat) {
      auto headers_slot = Slot("headers");
      if (headers_slot) {
        for (size_t i = 0; i < headers_slot->ChildCount(); ++i) {
          auto* btn = headers_slot->ChildAt(i);
          if (btn && btn->focused()) {
            if (kb.special == Event::Keyboard::Special::Return ||
                (kb.special == Event::Keyboard::Special::None && kb.codepoint == 32)) {
              SelectTab(std::to_string(i));
              return true;
            }
          }
        }
      }
    }
  }

  return Component<tabs>::OnEvent(event);
}

bool tabs::Digest() {
  auto panes = GetTabPanes();
  if (value.empty() && !panes.empty()) {
    value = panes[0].name;
    PropagateBinding("value", value);
  }

  for (size_t i = 0; i < panes.size(); ++i) {
    auto* pane_el = panes[i].element;
    const bool active = panes[i].name == value;
    const std::vector<std::string> want_classes = {active ? "active"
                                                          : "inactive"};
    const std::string want_style =
        active ? "display: block;" : "display: none;";

    if (pane_el->classes != want_classes) {
      pane_el->classes = want_classes;
    }
    const std::string* style_attr = pane_el->GetAttribute("style");
    if (!style_attr || *style_attr != want_style) {
      pane_el->SetAttribute("style", want_style);
    }
  }

  auto headers_slot = Slot("headers");
  if (headers_slot) {
    std::vector<std::pair<std::string, std::string>> pane_ids;
    pane_ids.reserve(panes.size());
    for (auto& pane : panes) {
      pane_ids.emplace_back(pane.name, pane.label);
    }

    // Only recreate the header buttons when the set of panes actually
    // changed. Rebuilding them on every Digest() (even a value-only change
    // from switching tabs) would replace a keyboard-focused button with a
    // fresh, unfocused Element - silently swallowing the next Enter/Space
    // keypress if any unrelated Digest() runs in between. It also hands back
    // elements that no style pass has seen: Digest() runs after Render()
    // resolved styles, so a button built here keeps an empty base_style and
    // renders with no padding and no background until something else forces a
    // full re-render.
    //
    // Identify the panes by name and label, not by Element*. The host
    // component re-renders whenever its state changes -- which is exactly what
    // selecting a tab does -- and hands out fresh pane elements each time, so
    // an address comparison called every tab switch a change of pane set.
    if (pane_ids != last_pane_ids_) {
      headers_slot->RemoveChildren();
      for (size_t i = 0; i < panes.size(); ++i) {
        auto btn = Ref<Element>::New();
        btn->set_owner_component(this);
        btn->SetTag("div");
        btn->classes = {"tab-header-btn"};
        btn->SetAttribute("onclick", "SelectTab(" + std::to_string(i) + ")");
        btn->SetAttribute("tabindex", "0");

        auto text_el = Ref<TextElement>::New(panes[i].label);
        text_el->set_owner_component(this);
        btn->AddChild(text_el);
        headers_slot->AddChild(btn);
      }
      last_pane_ids_ = std::move(pane_ids);
    }

    // Update active-tab styling in place, regardless of whether the
    // buttons were just rebuilt - this is what needs to happen on every
    // value change, without touching element identity.
    for (size_t i = 0; i < panes.size() && i < headers_slot->ChildCount();
         ++i) {
      auto* btn = headers_slot->ChildAt(i);
      bool active = panes[i].name == value;
      std::vector<std::string> want = {"tab-header-btn"};
      if (active) {
        want.push_back("active-tab");
      }
      if (btn->classes != want) {
        btn->classes = std::move(want);
      }
      btn->SetAttribute("part", active ? "tab-header-btn active-tab"
                                        : "tab-header-btn");
    }
  }

  // The mutations above happen after Render() resolved styles, but the
  // caller re-resolves once Digest() returns, and mutating `classes` drops the
  // element's resolved-style memo on its own, so nothing more is needed here.
  return Component<tabs>::Digest();
}

namespace {
int RegisterThis = []() {
  RegisterGlobalComponent("tabs", []() { return Ref<tabs>::New(); });
  return 0;
}();
}  // namespace
}  // namespace rtxui
