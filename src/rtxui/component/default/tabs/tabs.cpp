// Copyright 2024 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include "rtxui/component/default/tabs/tabs.hpp"

#include <iostream>

#include "rtxui/component/component_internal.hpp"
#include "rtxui/dom/element.hpp"
#include "rtxui/dom/text_element.hpp"

namespace rtxui {

void tabs::SelectTab(std::string index_str) {
  size_t idx = std::stoull(index_str);
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
    <div class="tabs-container">
      <div class="tabs-headers">
        <slot.headers></slot.headers>
      </div>
      <div class="tabs-content">
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
        border-bottom: solid;
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
    pane_el->classes.clear();
    if (panes[i].name == value) {
      pane_el->classes.push_back("active");
    } else {
      pane_el->classes.push_back("inactive");
    }
  }

  auto headers_slot = Slot("headers");
  if (headers_slot) {
    headers_slot->RemoveChildren();
    for (size_t i = 0; i < panes.size(); ++i) {
      auto btn = Ref<Element>::New();
      btn->set_owner_component(this);
      btn->SetTag("div");
      btn->classes = {"tab-header-btn"};
      if (panes[i].name == value) {
        btn->classes.push_back("active-tab");
      }
      btn->SetAttribute("onclick", "SelectTab(" + std::to_string(i) + ")");
      btn->SetAttribute("tabindex", "0");

      auto text_el = Ref<TextElement>::New(panes[i].label);
      text_el->set_owner_component(this);
      btn->AddChild(text_el);
      text_el->set_parent(btn.get());

      headers_slot->AddChild(btn);
      btn->set_parent(headers_slot.get());
    }
  }

  return Component<tabs>::Digest();
}

namespace {
int RegisterThis = []() {
  RegisterGlobalComponent("tabs", []() { return Ref<tabs>::New(); });
  return 0;
}();
}  // namespace
}  // namespace rtxui
