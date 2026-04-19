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
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <variant>
#include <vector>

#include "cell/cell.hpp"
#include "component/import.hpp"
#include "core/refcounted.hpp"
#include "dom/element.hpp"
#include "reflection/class_name.hpp"
#include "xml/xml.hpp"

namespace rtxui {

class ComponentBase : public RefCounted, public Bindings {
 public:
  virtual ~ComponentBase() = default;

  virtual std::string_view Setup() = 0;
  virtual std::string_view Tag() const = 0;

  std::string_view Template();

  void Mount();
  void Render();
  Element* Root() { return root_.get(); }

  Ref<Element> Slot(std::string_view name);

  // The Digest cycle: Detects changes in plain members and triggers updates.
  virtual void Digest() = 0;
  virtual void InitReflection() {}

 protected:
  void Render(const xml::Node& node, Element* element, ComponentBase* source);

  std::string template_;
  std::string xml_string_;
  xml::Nodes xml_nodes_;

  std::set<Ref<Cell>> watchers_;
  Ref<Element> root_;
  std::map<std::string, Ref<Element>> slots_;
  std::set<Ref<ComponentBase>> children_;

  std::string id_;
  std::vector<std::string> classes_;
};

/// Component<Derived> provides Transparent Reactivity.
/// 
/// Since the Bloomberg fork's reflection traits are experimental and unstable,
/// we provide a hybrid approach:
/// - Data members are registered using RTXUI_REFLECT.
/// - Methods can be discovered automatically or registered.
template <typename Derived>
class Component : public ComponentBase {
 public:
  Component() {}

  void InitReflection() override {
    // Discovery logic would go here. 
    // In this PoC, we assume the user has used the RTXUI_REFLECT macro
    // which populates the bindings.
  }

  static std::string_view StaticTag() { return ClassName<Derived>(); }
  std::string_view Tag() const final { return StaticTag(); }

  void Digest() final {
    // In a fully reflected version, this compares state against a snapshot.
    // For this PoC, we trigger a re-render.
    this->Render();
  }
};

// --- SIMULATED REFLECTION MACRO ---
#define RTXUI_REFLECT(TYPE, NAME) \
  TYPE NAME = [this]() { \
    this->Import(#NAME, rtxui::Ref<rtxui::ComputedTypedCell<TYPE>>::New([this]() { return this->NAME; })); \
    return TYPE(); \
  }(); \
  static_assert(true)

}  // namespace rtxui

#endif  // RTXUI_COMPONENT_HPP_
