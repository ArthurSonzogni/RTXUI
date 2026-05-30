// Copyright 2024 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#ifndef RTXUI_COMPONENT_HPP_
#define RTXUI_COMPONENT_HPP_

#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#include "rtxui/internal/class_name.hpp"
#include "rtxui/internal/event.hpp"
#include "rtxui/internal/import.hpp"
#include "rtxui/internal/refcounted.hpp"

namespace rtxui {
class Element;
}  // namespace rtxui

namespace xml {
struct Node;
using Nodes = std::vector<Node>;
}  // namespace xml

namespace rtxui {

class ComponentBase : public RefCounted, public Bindings {
 public:
  ComponentBase();
  virtual ~ComponentBase();
  ComponentBase(const ComponentBase&) = delete;
  ComponentBase& operator=(const ComponentBase&) = delete;
  ComponentBase(ComponentBase&&) = delete;
  ComponentBase& operator=(ComponentBase&&) = delete;

  virtual std::string_view Setup();
  virtual std::string_view GetView() const = 0;
  virtual std::string_view Tag() const = 0;
  std::string_view Template();

  void Mount();
  void Render();
  virtual bool Digest() = 0;
  virtual void InitReflection();
  virtual bool OnEvent(Event event);

  Element* Root();
  Ref<Element> Slot(std::string_view name);
  void SetProperty(std::string_view name, std::string_view value);
  void PropagateBinding(std::string_view child_prop, std::string_view value);

  virtual std::string GetInterpolatedValue(std::string_view expression) = 0;

  struct BindingLink {
    std::string child_prop;
    ComponentBase* parent;
    std::string parent_prop;
  };

 protected:
  std::vector<BindingLink> two_way_bindings_;
  void Render(const xml::Node& node, Element* element, ComponentBase* source);
  std::string template_;
  std::string xml_string_;
  xml::Nodes xml_nodes_;
  Ref<Element> root_;
  std::map<std::string, Ref<Element>> slots_;
  std::set<Ref<ComponentBase>> children_;
  std::vector<Ref<ComponentBase>> old_children_;
  std::string id_;
  std::vector<std::string> classes_;

  // Simulated reflection registry.
  struct Entry {
    std::string name;
    std::function<std::string()> get_value;
    std::function<bool()> check_and_update;
    std::function<void(std::string_view)> set_value;
  };
  std::vector<Entry> entries_;
};

namespace reflection {
int ParseInt(std::string_view str);

template <typename T>
std::string to_string(const T& value) {
  if constexpr (std::is_convertible_v<T, std::string>) {
    return static_cast<std::string>(value);
  } else if constexpr (requires { std::to_string(value); }) {
    return std::to_string(value);
  } else {
    std::stringstream ss;
    ss << value;
    return ss.str();
  }
}

template <typename T>
void from_string(std::string_view str, T& value) {
  if constexpr (std::is_same_v<T, std::string>) {
    value = std::string(str);
  } else if constexpr (std::is_same_v<T, int>) {
    value = ParseInt(str);
  } else if constexpr (std::is_same_v<T, bool>) {
    value = (str == "true" || str == "1");
  } else {
    std::stringstream ss;
    ss << str;
    ss >> value;
  }
}
}  // namespace reflection

template <typename Derived>
class Component : public ComponentBase {
 public:
  using ComponentBase::Import;
  static std::string_view StaticTag() { return ClassName<Derived>(); }
  std::string_view Tag() const final { return StaticTag(); }

  std::string_view GetView() const override {
    if constexpr (requires { static_cast<const Derived*>(this)->view; }) {
      return static_cast<const Derived*>(this)->view;
    }
    return const_cast<Component<Derived>*>(this)->Setup();
  }

  std::string_view Setup() override { return ""; }

  bool Digest() override {
    bool changed = false;
    for (auto& entry : entries_) {
      if (entry.check_and_update && entry.check_and_update()) {
        changed = true;
      }
    }
    if (changed) {
      this->Render();
    }
    for (auto& child : children_) {
      if (child->Digest()) {
        changed = true;
      }
    }
    return changed;
  }

  std::string GetInterpolatedValue(std::string_view expression) override {
    std::string_view target = expression;
    if (target.starts_with("props.")) {
      target = target.substr(6);
    }
    for (const auto& entry : entries_) {
      if (entry.name == target) {
        return entry.get_value();
      }
    }
    return std::string(expression);
  }

  bool OnEvent(Event event) override {
    for (auto& child : children_) {
      if (child->OnEvent(event)) {
        return true;
      }
    }
    return false;
  }

  template <typename T>
  void Import(std::string name, T& ref) {
    RegisterState(name, &ref);
  }

  template <typename Ret>
  void Import(std::string name, Ret (Derived::*method)() const) {
    RegisterComputed(name, method);
  }

 protected:
  template <typename T>
  void RegisterState(std::string name, T* ptr) {
    std::string clean_name = name;
    if (clean_name.starts_with("props.")) {
      clean_name = clean_name.substr(6);
    }
    auto snapshot = std::make_shared<T>(*ptr);
    auto get_value = [ptr]() { return reflection::to_string(*ptr); };
    auto check_and_update = [ptr, snapshot]() mutable {
      if (*ptr != *snapshot) {
        *snapshot = *ptr;
        return true;
      }
      return false;
    };
    auto set_value = [ptr](std::string_view val) {
      reflection::from_string(val, *ptr);
    };
    entries_.push_back({std::move(clean_name), std::move(get_value),
                        std::move(check_and_update), std::move(set_value)});
  }

  template <typename Ret>
  void RegisterComputed(std::string name, Ret (Derived::*method)() const) {
    entries_.push_back({name,
                        [this, method]() {
                          return reflection::to_string(
                              (static_cast<const Derived*>(this)->*method)());
                        },
                        nullptr, nullptr});
  }
};

// Bind(x) registers a member variable for interpolation and snapshot checking.
#define Bind(x) this->Import(#x, this->x)

// BindComputed(x) registers a const member function for interpolation.
#define BindComputed(x) this->Import(#x, &std::decay_t<decltype(*this)>::x)

// Legacy compatibility macros
#define RTXUI_STATE(TYPE, NAME)              \
  TYPE NAME;                                 \
  int init_##NAME = [this]() {               \
    this->RegisterState(#NAME, &this->NAME); \
    return 0;                                \
  }()

#define RTXUI_COMPUTED(NAME)                                             \
  int init_##NAME = [this]() {                                           \
    this->RegisterComputed(#NAME, &std::decay_t<decltype(*this)>::NAME); \
    return 0;                                                            \
  }()

void RegisterGlobalComponent(std::string_view name, ComponentFactory factory);
ComponentFactory GetGlobalComponentFactory(std::string_view name);

}  // namespace rtxui

#endif  // RTXUI_COMPONENT_HPP_
