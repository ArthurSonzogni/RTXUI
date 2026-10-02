// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#ifndef RTXUI_ELEMENT_HPP_
#define RTXUI_ELEMENT_HPP_

#include <cstddef>
#include <optional>
#include <rtxui/rtxui_export.hpp>
#include <string>
#include <string_view>

#include "rtxui/refcounted.hpp"

namespace rtxui {

class Element;
struct ElementHandleTarget;

/// A handle to a DOM element, obtained from ComponentBase::RootElement() or
/// ComponentBase::QueryElement().
///
/// The element itself is internal: its layout, style and animation state change
/// between releases. The handle is a single pointer and every method is
/// out-of-line, so none of that reaches the ABI.
///
/// A handle may be null: a query that matched nothing, or an element a
/// re-render has since destroyed -- the handle does not keep it alive. Every
/// method on a null handle returns an empty value and setters do nothing, so
/// calls can be chained without checking.
///
/// The tree it exposes is the one written in templates: `<slot>` wrappers are
/// transparent and text nodes are not children.
class RTXUI_EXPORT ElementHandle {
 public:
  ElementHandle();
  explicit ElementHandle(Element* element);
  ElementHandle(const ElementHandle& other);
  ElementHandle(ElementHandle&& other) noexcept;
  ElementHandle& operator=(const ElementHandle& other);
  ElementHandle& operator=(ElementHandle&& other) noexcept;
  ~ElementHandle();

  explicit operator bool() const;

  // Tree -------------------------------------------------------------------
  /// The first descendant matching `selector` (`#id`, `.class` or a tag).
  ElementHandle QuerySelector(std::string_view selector) const;
  ElementHandle Parent() const;
  std::size_t ChildCount() const;
  ElementHandle ChildAt(std::size_t index) const;

  // Content ----------------------------------------------------------------
  std::string tag() const;
  std::optional<std::string> GetAttribute(std::string_view name) const;

  // Scroll -----------------------------------------------------------------
  /// Negative offsets become 0; offsets past the end are clamped by the next
  /// layout.
  int scroll_x() const;
  int scroll_y() const;
  void SetScrollX(int x, bool smooth = false);
  void SetScrollY(int y, bool smooth = false);

 private:
  Element* get() const;

  Ref<ElementHandleTarget> target_;
};

}  // namespace rtxui

#endif  // RTXUI_ELEMENT_HPP_
