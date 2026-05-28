#include "rtxui/dom/element.hpp"

#include <algorithm>
#include "rtxui/component/component.hpp"

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
  if (name == "id") {
    id = value;
  } else if (name == "class") {
    classes.clear();
    size_t start = 0;
    while (true) {
      size_t pos = value.find(' ', start);
      if (pos == std::string::npos) {
        auto sub = value.substr(start);
        if (!sub.empty()) {
          classes.push_back(std::string(sub));
        }
        break;
      }
      auto sub = value.substr(start, pos - start);
      if (!sub.empty()) {
        classes.push_back(std::string(sub));
      }
      start = pos + 1;
    }
  }
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

Element* Element::QuerySelector(std::string_view selector) {
  if (selector.empty()) {
    return nullptr;
  }
  if (selector[0] == '#') {
    std::string_view target_id = selector.substr(1);
    if (id == target_id) {
      return this;
    }
  } else if (selector[0] == '.') {
    std::string_view target_class = selector.substr(1);
    if (std::find(classes.begin(), classes.end(), target_class) != classes.end()) {
      return this;
    }
  } else {
    if (tag() == selector) {
      return this;
    }
  }
  for (const auto& child : children_) {
    if (auto* found = child->QuerySelector(selector)) {
      return found;
    }
  }
  return nullptr;
}

}  // namespace rtxui
