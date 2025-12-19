#include "dom/element.hpp"

#include "component/component.hpp"

namespace rtxui {

void Element::AddChild(Ref<Element> child) {
  assert(child->parent_ == nullptr);
  child->parent_ = this;
  children_.push_back(std::move(child));
}

void Element::Visit(std::function<void(Element&)> f) {
  f(*this);
  for (auto& child : children_) {
    child->Visit(f);
  }
}

std::string_view Element::tag() const {
  return component_ ? component_->Tag() : "";
}

std::string Element::Print(int depth) const {
  if (tag().empty()) {
    std::string out;
    for (const auto& child : children_) {
      out += child->Print(depth);
    }
    return out;
  }

  // Self-closing tag.
  if (children_.empty()) {
    return std::string(depth, ' ') + "<" + std::string(tag()) + "/>\n";
  }

  std::string out;
  out += std::string(depth, ' ') + "<" + std::string(tag()) + ">\n";
  for (const auto& child : children_) {
    out += child->Print(depth + 2);
  }
  out += std::string(depth, ' ') + "</" + std::string(tag()) + ">\n";
  return out;
}

}  // namespace rtxui
