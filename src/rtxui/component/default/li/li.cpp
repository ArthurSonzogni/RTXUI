// Copyright 2024 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include "rtxui/component/default/li/li.hpp"

#include <cctype>
#include <charconv>
#include <functional>
#include <string>

#include "rtxui/dom/element.hpp"

namespace rtxui {

namespace {
// The HTML start="" attribute on <ol>: the number the first item counts
// from (default 1). Negative values are valid per spec.
int GetListStart(Element* list) {
  if (!list || list->tag() != "ol") {
    return 1;
  }
  auto* start_attr = list->GetAttribute("start");
  if (!start_attr) {
    return 1;
  }
  std::string_view s = *start_attr;
  while (!s.empty() && std::isspace(static_cast<unsigned char>(s.front()))) {
    s.remove_prefix(1);
  }
  while (!s.empty() && std::isspace(static_cast<unsigned char>(s.back()))) {
    s.remove_suffix(1);
  }
  int value = 1;
  auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), value);
  if (ec != std::errc() || ptr != s.data() + s.size()) {
    return 1;
  }
  return value;
}

int GetListItemIndex(Element* li_root, Element* immediate_list) {
  int index = 0;
  bool found = false;

  std::function<void(Element*)> traverse = [&](Element* el) {
    if (found || !el) return;

    if (el->tag() == "li") {
      // Check if this 'li' element's closest list parent is immediate_list
      Element* curr = el->Parent();
      Element* closest_list = nullptr;
      while (curr) {
        if (curr->tag() == "ul" || curr->tag() == "ol") {
          closest_list = curr;
          break;
        }
        curr = curr->Parent();
      }
      if (closest_list == immediate_list) {
        index++;
        if (el == li_root) {
          found = true;
          return;
        }
      }
    }

    for (auto& child : el->children()) {
      traverse(child.get());
      if (found) return;
    }
  };

  traverse(immediate_list);
  return index;
}
}  // namespace

void li::InitReflection() {
  Bind(marker);
  Component<li>::InitReflection();
}

std::string_view li::Setup() {
  return R"html(
    <span>{marker}</span><slot></slot>
    <style>
      self {
        display: block;
      }
    </style>
  )html";
}

bool li::Digest() {
  auto* root = Root();
  if (root) {
    // 1. Find closest list parent and list depth
    Element* immediate_list = nullptr;
    int list_depth = 0;
    Element* curr = root->Parent();
    while (curr) {
      if (curr->tag() == "ul" || curr->tag() == "ol") {
        if (!immediate_list) {
          immediate_list = curr;
        }
        list_depth++;
      }
      curr = curr->Parent();
    }

    // 2. Resolve list style type
    ListStyleType type = ListStyleType::Disc;
    if (root->style.list_style_type) {
      type = *root->style.list_style_type;
    } else if (immediate_list) {
      if (immediate_list->style.list_style_type) {
        type = *immediate_list->style.list_style_type;
      } else {
        type = (immediate_list->tag() == "ul") ? ListStyleType::Disc : ListStyleType::Decimal;
      }
    }

    // 3. Resolve marker string
    std::string new_marker;
    if (type == ListStyleType::None) {
      new_marker = "";
    } else if (type == ListStyleType::Decimal) {
      int index = 1;
      int start = 1;
      if (immediate_list) {
        index = GetListItemIndex(root, immediate_list);
        start = GetListStart(immediate_list);
      }
      new_marker = std::to_string(index - 1 + start) + ". ";
    } else {
      // Unordered list types (Disc, Circle, Square)
      bool has_explicit_type = root->style.list_style_type.has_value() ||
                               (immediate_list && immediate_list->style.list_style_type.has_value());
      if (!has_explicit_type) {
        // Fallback for nesting depth if no explicit style was specified
        if (list_depth == 2) {
          new_marker = "○ ";
        } else if (list_depth >= 3) {
          new_marker = "■ ";
        } else {
          new_marker = "• ";
        }
      } else {
        // Explicit list style type was specified
        if (type == ListStyleType::Circle) {
          new_marker = "○ ";
        } else if (type == ListStyleType::Square) {
          new_marker = "■ ";
        } else {
          new_marker = "• ";
        }
      }
    }

    if (marker != new_marker) {
      marker = new_marker;
    }
  }
  return Component<li>::Digest();
}

namespace {
int RegisterThis = []() {
  RegisterGlobalComponent("li", []() { return Ref<li>::New(); });
  return 0;
}();
}  // namespace
}  // namespace rtxui
