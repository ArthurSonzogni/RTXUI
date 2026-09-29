// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include <algorithm>
#include <rtxui/element.hpp>
#include <string>
#include <utility>
#include <vector>

#include "rtxui/dom/element.hpp"

namespace rtxui {

namespace {

// The children as written in the template: a <slot> is replaced by what was
// projected into it, and text nodes are left out.
void CollectChildren(Element* element, std::vector<Element*>& out) {
  for (const Ref<Element>& child : element->children()) {
    if (child->is_slot()) {
      CollectChildren(child.get(), out);
    } else if (!child->is_text()) {
      out.push_back(child.get());
    }
  }
}

}  // namespace

ElementHandle::ElementHandle() = default;
ElementHandle::ElementHandle(Element* element) {
  if (element) {
    target_ = element->HandleTarget();
  }
}
ElementHandle::ElementHandle(const ElementHandle& other) = default;
ElementHandle::ElementHandle(ElementHandle&& other) noexcept
    : target_(std::move(other.target_)) {}
ElementHandle& ElementHandle::operator=(const ElementHandle& other) = default;
ElementHandle& ElementHandle::operator=(ElementHandle&& other) noexcept {
  target_ = std::move(other.target_);
  return *this;
}
ElementHandle::~ElementHandle() = default;

Element* ElementHandle::get() const {
  return target_ ? target_->element : nullptr;
}

ElementHandle::operator bool() const {
  return get() != nullptr;
}

ElementHandle ElementHandle::QuerySelector(std::string_view selector) const {
  Element* element = get();
  if (!element) {
    return {};
  }
  return ElementHandle(element->QuerySelector(selector));
}

ElementHandle ElementHandle::Parent() const {
  Element* element = get();
  if (!element) {
    return {};
  }
  Element* parent = element->Parent();
  while (parent && parent->is_slot()) {
    parent = parent->Parent();
  }
  return ElementHandle(parent);
}

std::size_t ElementHandle::ChildCount() const {
  Element* element = get();
  if (!element) {
    return 0;
  }
  std::vector<Element*> children;
  CollectChildren(element, children);
  return children.size();
}

ElementHandle ElementHandle::ChildAt(std::size_t index) const {
  Element* element = get();
  if (!element) {
    return {};
  }
  std::vector<Element*> children;
  CollectChildren(element, children);
  if (index >= children.size()) {
    return {};
  }
  return ElementHandle(children[index]);
}

std::string ElementHandle::tag() const {
  Element* element = get();
  if (!element) {
    return {};
  }
  return std::string(element->tag());
}

std::optional<std::string> ElementHandle::GetAttribute(
    std::string_view name) const {
  Element* element = get();
  if (!element) {
    return std::nullopt;
  }
  const std::string* value = element->GetAttribute(std::string(name));
  if (!value) {
    return std::nullopt;
  }
  return *value;
}

int ElementHandle::scroll_x() const {
  Element* element = get();
  return element ? element->scroll_x() : 0;
}

int ElementHandle::scroll_y() const {
  Element* element = get();
  return element ? element->scroll_y() : 0;
}

void ElementHandle::SetScrollX(int x, bool smooth) {
  if (Element* element = get()) {
    element->set_scroll_x(std::max(0, x), smooth);
  }
}

void ElementHandle::SetScrollY(int y, bool smooth) {
  if (Element* element = get()) {
    element->set_scroll_y(std::max(0, y), smooth);
  }
}

}  // namespace rtxui
