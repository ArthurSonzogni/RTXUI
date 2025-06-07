// Copyright 2024 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#ifndef RTXUI_COMPONENT_HPP_
#define RTXUI_COMPONENT_HPP_

#include <functional>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <string_view>
#include <unordered_map>
#include <variant>
#include <vector>

#include "cell/cell.hpp"
#include "core/refcounted.hpp"
#include "dom/element.hpp"
#include "components/import.hpp"
#include "xml/xml.hpp"
#include "reflection/class_name.hpp"

namespace rtxui {

class ComponentBase : public RefCounted, public Bindings {
 public:
  virtual ~ComponentBase() = default;

  /// Define a component.
  ///
  /// @return the template of the component, which is a string containing
  /// HTML-like syntax.
  virtual std::string_view RunSetup() = 0;

  /// This is used by `Import` to avoid users to have to specify the name of a
  /// class.
  virtual std::string_view Tag() const = 0;

  std::string_view Template();

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

  Ref<Element> Slot(std::string_view name);

 private:
  void Render(const xml::Node& node, Element* element, ComponentBase* source);

  std::string template_;

  std::string xml_string_;
  xml::Nodes xml_nodes_;

  std::set<Ref<Cell>> watchers_;
  Ref<Element> root_;

  std::map<std::string, Ref<Element>> slots_;

  std::set<Ref<ComponentBase>> children_;
};

template <typename Derived>
class Component : public ComponentBase {
 public:
  static std::string_view StaticTag() { return ClassName<Derived>(); }
  std::string_view Tag() const final { return StaticTag(); }
  std::string_view RunSetup() final {
    if constexpr (requires { Derived::kTemplate; }) {
      // The derived class has a static member named kTemplate ?
      return Derived::kTemplate;
    } else if constexpr (requires { static_cast<Derived*>(this)->Setup(); }) {
      // Either the derived class implements Setup(), or it is a static
      // string_view named kTemplate.
      return static_cast<Derived*>(this)->Setup();
    } else {
      static_assert(false,
                    "Component must implement Setup() or have a static "
                    "kTemplate member.");
    }
  }
};

}  // namespace rtxui

/// A macro turning:
/// ```cpp
/// RTXUI_COMPONENT(Hello) {4
///   return R"html(
///   <div>
///   Hello
///   </div>
///   )html";
/// }
/// ```
///
/// Into:
/// ```cpp
/// class Hello : public rtxui::Component<Hello> {
///  public:
///   std::string_view Setup() {
///     return R"html(
///       <div>
///         Hello
///       </div>
///     )html";
///   }
/// };
/// ```
#define RTXUI_COMPONENT(T)               \
  class T : public rtxui::Component<T> { \
   public:                               \
    std::string_view Setup();            \
  };                                     \
  std::string_view T::Setup()

#endif  // RTXUI_COMPONENT_HPP_
