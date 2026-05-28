#include "dom/element.hpp"

#include "component/component.hpp"

namespace rtxui {

Element::Element() {}
Element::Element(const ComponentBase* component) : component_(component) {}

void Element::AddChild(Ref<Element> child) {
  assert(child->parent_ == nullptr);
  child->parent_ = this;
  children_.push_back(std::move(child));
}

void Element::RemoveChildren() {
  for (auto& child : children_) {
    child->parent_ = nullptr;
  }
  children_.clear();
}

void Element::Visit(std::function<void(Element&)> f) {
  f(*this);
  for (auto& child : children_) {
    child->Visit(f);
  }
}

void Element::SetAttribute(std::string name, std::string value) {
  attributes_[std::move(name)] = std::move(value);
}

std::string Element::Print(int depth) const {
  std::string out;
  out += std::string(depth, ' ') + "<" + std::string(tag()) + "";
  if (!id.empty()) {
    out += " id=\"" + id + "\"";
  }
  if (!classes.empty()) {
    out += " class=\"";
    for (size_t i = 0; i < classes.size(); ++i) {
      if (i > 0) out += " ";
      out += classes[i];
    }
    out += "\"";
  }
  for (const auto& [name, value] : attributes_) {
    if (name == "id" || name == "class") continue;
    out += " " + name + "=\"" + value + "\"";
  }
  out += ">\n";
  for (const auto& child : children_) {
    out += child->Print(depth + 2);
  }
  out += std::string(depth, ' ') + "</" + std::string(tag()) + ">\n";
  return out;
}

std::string_view Element::tag() const {
  if (component_) return component_->Tag();
  return tag_;
}

}  // namespace rtxui
