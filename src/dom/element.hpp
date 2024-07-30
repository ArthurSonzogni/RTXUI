#ifndef DOM_ElEMENT_HPP_
#define DOM_ElEMENT_HPP_

#include <functional>
#include <string>
#include <vector>

#include "core/refcounted.hpp"

namespace rtxui {

class Component;

// A generic HTML element
class Element : public RefCounted {
 public:
  Element() = default;
  explicit Element(const Component* component) : component_(component) {}

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

  std::string_view tag() const;

  virtual std::string Print(int depth = 0) const;

 protected:
  std::vector<Ref<Element>> children_;
  Element* parent_ = nullptr;
  const Component* component_ = nullptr;
};

}  // namespace rtxui

#endif  // DOM_ElEMENT_HPP_
