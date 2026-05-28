#ifndef DOM_ElEMENT_HPP_
#define DOM_ElEMENT_HPP_

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "rtxui/core/refcounted.hpp"
#include "rtxui/layout/style.hpp"

namespace rtxui {

class ComponentBase;

class Element : public RefCounted {
 public:
  Element();
  Element(const ComponentBase* component);
  virtual ~Element() = default;

  // Non-copyable, non-movable.
  Element(const Element&) = delete;
  Element& operator=(const Element&) = delete;
  Element(Element&&) = default;
  Element& operator=(Element&&) = default;

  void AddChild(Ref<Element> child);
  void RemoveChildren();
  void Visit(std::function<void(Element&)> f);

  // Hierarchical accessors.
  Element* Parent() { return parent_; }
  size_t ChildCount() const { return children_.size(); }
  Element* ChildAt(size_t index) { return children_[index].get(); }
  const std::vector<Ref<Element>>& children() const { return children_; }

  // Virtual tag.
  virtual std::string_view tag() const;
  void SetTag(std::string tag) { tag_ = std::move(tag); }

  // Attributes.
  void SetAttribute(std::string name, std::string value);
  const std::map<std::string, std::string>& Attributes() const {
    return attributes_;
  }

  // Common properties.
  std::string id;
  std::vector<std::string> classes;
  ComputedStyle style;

  // Rendering.
  virtual std::string Print(int depth = 0) const;

  // Query selector for testing.
  Element* QuerySelector(std::string_view selector);

  const ComponentBase* component() const { return component_; }

  bool is_slot() const { return is_slot_; }
  bool is_text() const { return is_text_; }

  int scroll_y() const { return scroll_y_; }
  void set_scroll_y(int y) { scroll_y_ = y; }
  int scroll_height() const { return scroll_height_; }
  void set_scroll_height(int h) { scroll_height_ = h; }

  int scroll_x() const { return scroll_x_; }
  void set_scroll_x(int x) { scroll_x_ = x; }
  int scroll_width() const { return scroll_width_; }
  void set_scroll_width(int w) { scroll_width_ = w; }

  bool focused() const { return focused_; }
  void set_focused(bool f) { focused_ = f; }

 protected:
   bool is_slot_ : 1 = false;
   bool is_text_ : 1 = false;
   int scroll_y_ = 0;
   int scroll_height_ = 0;
   int scroll_x_ = 0;
   int scroll_width_ = 0;
   bool focused_ = false;

  std::string tag_ = "div";
  std::map<std::string, std::string> attributes_;
  std::vector<Ref<Element>> children_;
  Element* parent_ = nullptr;
  const ComponentBase* component_ = nullptr;
};

}  // namespace rtxui

#endif  // DOM_ElEMENT_HPP_
