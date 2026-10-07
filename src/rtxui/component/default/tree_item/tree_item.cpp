// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include "rtxui/component/default/tree_item/tree_item.hpp"

#include "rtxui/dom/element.hpp"

namespace rtxui {

void tree_item::SetOpen(bool value) {
  if (open == value) {
    return;
  }
  open = value;
  PropagateBinding("open", open ? "true" : "false");
}

void tree_item::Toggle() {
  if (HasChildren()) {
    SetOpen(!open);
  }
}

void tree_item::InitReflection() {
  Bind(open);
  Bind(label);
  Bind(arrow);
  Bind(children_class);
  Bind(row_action);
  Bind(Toggle);
  Component<tree_item>::InitReflection();
}

std::string_view tree_item::Setup() {
  return R"html(
    <div class="tree-row" part="row" tabindex="0" onclick="{row_action}">
      <span class="tree-arrow" part="arrow" onclick="Toggle">{arrow}</span>
      <span class="tree-label" part="label">{label}</span>
    </div>
    <div class="tree-children {children_class}" part="children">
      <slot></slot>
    </div>
    <style>
      self {
        display: block;
      }
      .tree-row {
        display: flex;
        cursor: pointer;
      }
      .tree-row:hover {
        background-color: lighten(10%);
      }
      .tree-row:focus {
        background-color: lighten(20%);
      }
      .tree-arrow {
        width: 2;
      }
      .tree-children {
        display: block;
        padding-left: 2;
      }
      .closed {
        display: none;
      }
    </style>
  )html";
}

bool tree_item::HasChildren() {
  const Ref<Element> slot = Slot("");
  if (!slot) {
    return false;
  }
  for (const auto& child : slot->children()) {
    if (!child->is_text()) {
      return true;
    }
  }
  return false;
}

bool tree_item::OnEvent(Event event) {
  // Children first: a nested item's row handles keys meant for it.
  if (Component<tree_item>::OnEvent(event)) {
    return true;
  }
  Element* root = Root();
  if (!root || !event.is<Event::Keyboard>()) {
    return false;
  }
  const auto kb = event.get<Event::Keyboard>();
  if (kb.motion != Event::Keyboard::Motion::Pressed &&
      kb.motion != Event::Keyboard::Motion::Repeat) {
    return false;
  }
  Element* row = root->QuerySelector(".tree-row");
  if (!row || !row->focused() || !HasChildren()) {
    return false;
  }
  // Right opens and Left closes a branch, as in a file explorer. Once there
  // is nothing to open or close, the keys fall through to spatial navigation.
  if (kb.special == Event::Keyboard::Special::ArrowRight && !open) {
    SetOpen(true);
    return true;
  }
  if (kb.special == Event::Keyboard::Special::ArrowLeft && open) {
    SetOpen(false);
    return true;
  }
  return false;
}

bool tree_item::Digest() {
  const bool branch = HasChildren();
  arrow = !branch ? " " : open ? "▾" : "▸";
  children_class = open ? "open" : "closed";
  const bool has_own_onclick = root_ && (root_->GetAttribute("onclick") ||
                                         root_->GetAttribute("@click") ||
                                         root_->GetAttribute("@click.left"));
  row_action = (branch && !has_own_onclick) ? "Toggle" : "";
  return Component<tree_item>::Digest();
}

}  // namespace rtxui
