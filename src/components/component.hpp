// Copyright 2024 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#ifndef RTXUI_COMPONENT_HPP_
#define RTXUI_COMPONENT_HPP_

#include <functional>
#include <map>
#include <set>
#include <source_location>
#include <string>
#include <string_view>

#include "cell/cell.hpp"
#include "core/refcounted.hpp"
#include "dom/element.hpp"
#include "register.hpp"
#include "xml/xml.hpp"

namespace rtxui {

class Component : public RefCounted {
 public:
  virtual std::string Setup() = 0;
  virtual Ref<Component> New() const = 0;
  virtual std::string_view Tag() const = 0;
  virtual const std::vector<std::string>& Namespaces() = 0;

  // Bind a name in the template to a cell.
  void Bind(std::string_view name, Ref<Cell> value);

  template <typename T>
  auto State(T&& value) {
    return Ref<TypedCell<T>>(std::forward<T>(value));
  }

  template <typename T>
  auto Computed(std::function<T()> f) {
    return Ref<ComputedTypedCell<T>>(std::move(f));
  }

  // Create a watcher that will execute the given function whenever the content
  // of the function f() changes. It returns a function that can be called to
  // stop the watcher.
  auto Watch(std::function<void()> f) -> std::function<void()> {
    auto watcher = Ref<WatcherCaptureCell>::New(std::move(f));
    auto watcher_cell = Ref<Cell>(watcher);
    auto out = [=, this] { watchers_.erase(watcher_cell); };
    watchers_.insert(watcher.get());
    return out;
  }

  // Create a watcher that will execute the given function whenever the content
  // of `cell` changes. It returns a function that can be called to stop the
  // watcher.
  auto Watch(Ref<Cell> cell, std::function<void()> f) {
    auto watcher = Ref<WatcherCell>::New(std::move(f));
    auto watcher_cell = Ref<Cell>(watcher);
    watcher->DependsOn(cell.get());
    watchers_.insert(watcher.get());
    return [=, this] { watchers_.erase(watcher_cell); };
  }

  void Mount();
  void Render();
  Element* Root() { return root_.get(); }

  Element* DefaultSlot();
  Element* Slot(std::string_view name);

 private:
  void Render(const xml::Node& node, Element* element);

  std::string xml_string_;
  xml::Nodes xml_nodes_;

  std::map<std::string, Ref<Cell>> bindings_;
  std::set<Ref<Cell>> watchers_;
  Ref<Element> root_;

  Ref<Element> default_slot_;
  std::map<std::string, Ref<Element>> slots_;

  std::set<Ref<Component>> children_;
};

}  // namespace rtxui

#define RTXUI_COMPONENT(name)                                     \
  struct RTXUI_##name : public rtxui::Component {                 \
    inline std::string Setup() final;                             \
    ::rtxui::Ref<Component> New() const final {                   \
      return ::rtxui::Ref<RTXUI_##name>::New();                   \
    }                                                             \
    static std::source_location SourceLocation() {                \
      return std::source_location::current();                     \
    }                                                             \
    std::string_view Tag() const final {                          \
      return #name;                                               \
    }                                                             \
    const std::vector<std::string>& Namespaces() final {          \
      static std::vector<std::string> namespaces_ =               \
          ::rtxui::Register::ComputeNamespaces(SourceLocation()); \
      return namespaces_;                                         \
    }                                                             \
  };                                                              \
  ::rtxui::Register rtxui_reg_##name(new RTXUI_##name());         \
  inline std::string RTXUI_##name::Setup()

#define RTXUI_BIND(x) Bind(#x, x)

#endif  // RTXUI_COMPONENT_HPP_
