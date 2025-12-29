#ifndef DOM_ElEMENT_HPP_
#define DOM_ElEMENT_HPP_

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "core/refcounted.hpp"
#include "layout/style.hpp"

namespace rtxui {

class ComponentBase;

// A generic HTML element
class Element : public RefCounted {
 public:
  Element() = default;
  explicit Element(const ComponentBase* component) : component_(component) {}

  // Disallow copy and assign.
  Element(const Element&) = delete;
  Element& operator=(const Element&) = delete;
  Element(Element&&) = default;
  Element& operator=(Element&&) = default;

  void AddChild(Ref<Element> child);
  void Visit(std::function<void(Element&)> f);

  // Hierarchical accessors.
  Element* Parent() { return parent_; }
  size_t ChildCount() const { return children_.size(); }
  Element* ChildAt(int index) { return children_.at(index).get(); }
  const std::vector<Ref<Element>>& children() const { return children_; }

  // Element info.
  bool is_slot() const { return is_slot_; }
  bool is_text() const { return is_text_; }
  std::string_view tag() const;

  // Debugging.
  std::string Print() const { return Print(0); }
  virtual std::string Print(int depth) const;

  ComputedStyle style;

 protected:
  bool is_slot_ : 1 = false;
  bool is_text_ : 1 = false;

  std::vector<Ref<Element>> children_;
  Element* parent_ = nullptr;
  const ComponentBase* component_ = nullptr;
};

}  // namespace rtxui

#endif  // DOM_ElEMENT_HPP_
