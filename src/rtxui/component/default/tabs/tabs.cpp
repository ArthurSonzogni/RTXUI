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
  BindCollection("headers", &headers_, [](const TabHeader& header) {
    return std::make_shared<ManualStructVisitor>(
        std::map<std::string, std::string, std::less<>>{
            {"label", header.label},
            {"active_class", header.active_class},
            // `part` mirrors the class list, so a host can still reach the
            // selected header with `tabs::part(active-tab)`.
            {"part", header.active_class.empty()
                         ? std::string("tab-header-btn")
                         : "tab-header-btn " + header.active_class},
        });
  });
  Component<tabs>::InitReflection();
}

std::string_view tabs::Setup() {
  return R"html(
    <div class="tabs-container" part="tabs-container">
      <div class="tabs-headers" part="tabs-headers">
        <for each="{headers}" as="header">
          <div class="tab-header-btn {header.active_class}"
               part="{header.part}"
               tabindex="0"
               onclick="SelectTab({$index})">{header.label}</div>
        </for>
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

// The header buttons the template's <for> produced, in pane order.
std::vector<Element*> tabs::HeaderButtons() {
  std::vector<Element*> buttons;
  if (Element* root = Root()) {
    root->Visit([&](Element& element) {
      for (const std::string& name : element.classes) {
        if (name == "tab-header-btn") {
          buttons.push_back(&element);
        }
      }
    });
  }
  return buttons;
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
      std::vector<Element*> buttons = HeaderButtons();
      {
        for (size_t i = 0; i < buttons.size(); ++i) {
          auto* btn = buttons[i];
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

  // Publish the header strip as data and let the template's <for> turn it
  // into buttons. Reconciliation then owns their identity -- it reuses them
  // across a value change rather than rebuilding, which is what the
  // hand-rolled version needed a guard for -- and their styles are resolved
  // by the normal pass instead of being assigned after it had already run.
  headers_.clear();
  headers_.reserve(panes.size());
  for (const TabPaneInfo& pane : panes) {
    headers_.push_back({pane.label, pane.name == value ? "active-tab" : ""});
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
