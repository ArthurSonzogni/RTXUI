// Copyright 2025 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.

#ifndef RTXUI_COMPONENT_IMPORT_HPP_
#define RTXUI_COMPONENT_IMPORT_HPP_

#include <functional>
#include <map>
#include <string>
#include <string_view>

#include "rtxui/internal/class_name.hpp"
#include "rtxui/internal/refcounted.hpp"
#include <rtxui/rtxui_export.hpp>

namespace rtxui {

class ComponentBase;

// Defined out of line so that this header does not need <print>: libstdc++
// only shipped it in GCC 14, and requiring that of every consumer for an
// error message is a poor trade. Both terminate the process.
[[noreturn]] RTXUI_EXPORT void ReportDuplicateImport(std::string_view name);
[[noreturn]] RTXUI_EXPORT void ReportDuplicateImportAlias(
    std::string_view class_name, std::string_view alias);

using ComponentFactory = std::function<Ref<ComponentBase>()>;
using ComponentImportMap = std::map<std::string, ComponentFactory, std::less<>>;

using Callback = std::function<void()>;
using ParameterizedCallback = std::function<void(std::string)>;
using CallbackImportMap = std::map<std::string, Callback, std::less<>>;
using ParameterizedCallbackImportMap =
    std::map<std::string, ParameterizedCallback, std::less<>>;

/// Bindings is a structure that allows you to import components and callbacks
/// into a component.
class RTXUI_EXPORT Bindings {
 protected:
  CallbackImportMap callbacks_;
  ParameterizedCallbackImportMap parameterized_callbacks_;
  ComponentImportMap imports_;

 public:
  /// Invoke an imported callback by name, optionally with an argument.
  bool RunCallback(std::string_view name, std::string_view arg = "");

  /// Bind a callback into the template.
  ///
  /// **Example:**
  /// ```cpp
  /// auto callback = [=] { count->Value(count->Value() + 1); };
  /// Import("callback", callback);
  /// ```
  void Import(std::string_view name, std::function<void()> callback);

  /// Bind a parameterized callback into the template.
  void Import(std::string_view name, std::function<void(std::string)> callback);

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
      ReportDuplicateImport(name);
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
    if (imports_.count(alias)) {
      ReportDuplicateImportAlias(ClassName<T>(), alias);
    }

    imports_[std::string(alias)] = [] { return Ref<T>::New(); };
  }
};

}  // namespace rtxui

#endif  // RTXUI_COMPONENT_IMPORT_HPP_
