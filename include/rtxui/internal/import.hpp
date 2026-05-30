// Copyright 2025 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.

#ifndef RTXUI_COMPONENT_IMPORT_HPP_
#define RTXUI_COMPONENT_IMPORT_HPP_

#include <functional>
#include <print>
#include <string>
#include <unordered_map>

#include "rtxui/internal/class_name.hpp"
#include "rtxui/internal/refcounted.hpp"

namespace rtxui {

class ComponentBase;

using ComponentFactory = std::function<Ref<ComponentBase>()>;
using ComponentImportMap = std::unordered_map<std::string, ComponentFactory>;

using Callback = std::function<void()>;
using CallbackImportMap = std::unordered_map<std::string, Callback>;

/// Bindings is a structure that allows you to import components and callbacks
/// into a component.
class Bindings {
 protected:
  CallbackImportMap callbacks_;
  ComponentImportMap imports_;

 public:
  /// Invoke an imported callback by name.
  bool RunCallback(std::string_view name);

  /// Bind a callback into the template.
  ///
  /// **Example:**
  /// ```cpp
  /// auto callback = [=] { count->Value(count->Value() + 1); };
  /// Import("callback", callback);
  /// ```
  void Import(std::string_view name, std::function<void()> callback);

  /// Import a component using a custom factory function.
  void Import(std::string_view name, ComponentFactory factory);

  /// Import a component into the template.
  /// We require the component to be a subclass of `ComponentBase`.
  ///
  /// ***Example:**
  /// ```cpp
  /// Import<MyComponent>();
  /// ```
  template <typename T>
    requires std::derived_from<T, ComponentBase>
  void Import() {
    std::string name(T::StaticTag());
    if (imports_.count(name)) {
      std::println("Error: Component '{}' is already imported.", name);
      std::exit(1);
    }

    imports_[name] = []() { return Ref<ComponentBase>(new T()); };
  }

  /// Import a component with an alias name.
  /// This allows you to use a different name for the component in the template.
  /// **Example:**
  /// ```cpp
  /// Import<MyComponent>("AliasName");
  /// ```
  template <typename T>
    requires std::derived_from<T, ComponentBase>
  void Import(std::string_view alias) {
    if (imports_.count(std::string(alias))) {
      std::println(
          "Error: Can't import '{}' as '{}`, since it is already imported.",
          ClassName<T>(), alias);
      std::exit(1);
    }

    imports_[std::string(alias)] = [] { return Ref<T>::New(); };
  }
};

}  // namespace rtxui

#endif  // RTXUI_COMPONENT_IMPORT_HPP_
