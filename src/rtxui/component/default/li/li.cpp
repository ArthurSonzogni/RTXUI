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
// Parses a trimmed integer HTML attribute, returning `fallback` if the
// attribute is absent or not a valid integer.
int ParseIntAttribute(Element* el, const char* name, int fallback) {
  if (!el) {
    return fallback;
  }
  auto* attr = el->GetAttribute(name);
  if (!attr) {
    return fallback;
  }
  std::string_view s = *attr;
  while (!s.empty() && std::isspace(static_cast<unsigned char>(s.front()))) {
    s.remove_prefix(1);
  }
  while (!s.empty() && std::isspace(static_cast<unsigned char>(s.back()))) {
    s.remove_suffix(1);
  }
  int value = fallback;
  auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), value);
  if (ec != std::errc() || ptr != s.data() + s.size()) {
    return fallback;
  }
  return value;
}

// Visits every <li> whose closest list ancestor is `immediate_list`, in
// document order. Stops early (returning false from the visitor) once
// satisfied.
void ForEachListItem(Element* immediate_list,
                     const std::function<bool(Element*)>& visit) {
  bool stop = false;
  std::function<void(Element*)> traverse = [&](Element* el) {
    if (stop || !el) {
      return;
    }
    if (el->tag() == "li") {
      Element* curr = el->Parent();
      Element* closest_list = nullptr;
      while (curr) {
        if (curr->tag() == "ul" || curr->tag() == "ol") {
          closest_list = curr;
          break;
        }
        curr = curr->Parent();
      }
      if (closest_list == immediate_list && !visit(el)) {
        stop = true;
        return;
      }
    }
    for (auto& child : el->children()) {
      traverse(child.get());
      if (stop) {
        return;
      }
    }
  };
  traverse(immediate_list);
}

// The HTML start="" attribute on <ol>: the number the first item counts
// from (default 1, or the item count if reversed). Negative values are
// valid per spec.
int GetListStart(Element* list, bool reversed) {
  if (!list || list->tag() != "ol") {
    return 1;
  }
  if (list->GetAttribute("start")) {
    return ParseIntAttribute(list, "start", 1);
  }
  if (!reversed) {
    return 1;
  }
  int count = 0;
  ForEachListItem(list, [&](Element*) {
    count++;
    return true;
  });
  return count;
}

// The HTML boolean reversed attribute on <ol>: items count down instead
// of up.
bool GetListReversed(Element* list) {
  return list && list->tag() == "ol" && list->GetAttribute("reversed");
}

// Resolves li_root's ordinal number within immediate_list, honoring each
// preceding sibling <li>'s own HTML value="" override (which also shifts
// the count for every item after it, per spec), immediate_list's start,
// and its counting direction (1, or -1 when reversed).
int GetListItemNumber(Element* li_root,
                      Element* immediate_list,
                      int start,
                      int step) {
  int current = start - step;
  int result = start;
  ForEachListItem(immediate_list, [&](Element* el) {
    current = ParseIntAttribute(el, "value", current + step);
    if (el == li_root) {
      result = current;
      return false;
    }
    return true;
  });
  return result;
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
        type = (immediate_list->tag() == "ul") ? ListStyleType::Disc
                                               : ListStyleType::Decimal;
      }
    }

    // 3. Resolve marker string
    std::string new_marker;
    if (type == ListStyleType::None) {
      new_marker = "";
    } else if (type == ListStyleType::Decimal) {
      int number = 1;
      if (immediate_list) {
        bool reversed = GetListReversed(immediate_list);
        int start = GetListStart(immediate_list, reversed);
        number =
            GetListItemNumber(root, immediate_list, start, reversed ? -1 : 1);
      }
      new_marker = std::to_string(number) + ". ";
    } else {
      // Unordered list types (Disc, Circle, Square)
      bool has_explicit_type =
          root->style.list_style_type.has_value() ||
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
