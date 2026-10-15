// Copyright 2024 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#ifndef RTXUI_COMPONENT_HPP_
#define RTXUI_COMPONENT_HPP_

#include <concepts>
#include <cstddef>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <ranges>
#include <rtxui/bind.hpp>  // IWYU pragma: export
#include <rtxui/rtxui_export.hpp>
#include <source_location>
#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#include "rtxui/element.hpp"
#include "rtxui/event.hpp"
#include "rtxui/field_sink.hpp"  // IWYU pragma: export
#include "rtxui/internal/binding.hpp"
#include "rtxui/internal/class_name.hpp"
#include "rtxui/refcounted.hpp"

namespace css {
struct Ruleset;
using StyleSheet = std::vector<Ruleset>;
}  // namespace css

namespace xml {
struct Node;
}  // namespace xml

namespace rtxui {

class ComponentBase;
class Element;
struct CategorizedRules;
template <typename Derived>
class Component;

namespace internal {
struct LocalScope;
struct SlotFilter;
}  // namespace internal

/// Creates a component, for a tag a template writes. See Import().
using ComponentFactory = std::function<Ref<ComponentBase>()>;

/// What every component is. Applications derive from Component<Derived>, which
/// implements the part that depends on the derived type.
class RTXUI_EXPORT ComponentBase : public RefCounted {
 public:
  ComponentBase();
  ~ComponentBase() override;
  ComponentBase(const ComponentBase&) = delete;
  ComponentBase& operator=(const ComponentBase&) = delete;
  ComponentBase(ComponentBase&&) = delete;
  ComponentBase& operator=(ComponentBase&&) = delete;

  /// The template: the derived class's `view` member or Setup() method.
  virtual std::string_view GetView() const = 0;
  /// The class name, which is the tag a template writes to create one.
  virtual std::string_view Tag() const = 0;

  // Reactivity ---------------------------------------------------------------
  /// Registers the component's bindings. Called once, before the first
  /// render; override it to Bind() or Import() there instead of in the
  /// constructor.
  virtual void InitReflection();

  /// Compares bound state with its last snapshot, re-renders what changed and
  /// recurses into children. Screen calls it every frame; returns whether
  /// anything changed. An override doing more each frame calls this one.
  virtual bool Digest();

  /// Pushes `value` to the parent state bound to this component's `child_prop`
  /// attribute, for a component exposing a two-way binding.
  void PropagateBinding(std::string_view child_prop, std::string_view value);

  // Imports ------------------------------------------------------------------
  /// Makes `callback` callable from the template as `name`: `onclick="name"`.
  void Import(std::string_view name, std::function<void()> callback);
  /// The same, for a callback taking the value the template passes.
  void Import(std::string_view name, std::function<void(std::string)> callback);
  /// Makes `<name>` in the template create a component with `factory`.
  void Import(std::string_view name, ComponentFactory factory);
  /// Makes `<T>` in the template, T's class name, create a T.
  template <typename T>
    requires std::derived_from<T, ComponentBase>
  void Import() {
    Import(T::StaticTag(), ComponentFactory([] { return Ref<T>::New(); }));
  }
  /// Makes `<alias>` in the template create a T.
  template <typename T>
    requires std::derived_from<T, ComponentBase>
  void Import(std::string_view alias) {
    ImportAlias(ClassName<T>(), alias,
                ComponentFactory([] { return Ref<T>::New(); }));
  }

  // Input --------------------------------------------------------------------
  /// Handles an event no focused element consumed. Return true to stop it.
  /// The default offers it to the components this one's template wrote.
  virtual bool OnEvent(Event event);
  /// Routes every mouse event to this component until ReleaseMouse().
  void CaptureMouse();
  void ReleaseMouse();

  /// Asks the terminal to set the system clipboard to `text` (OSC 52), on the
  /// next frame.
  void SetClipboard(std::string_view text);

  // DOM ----------------------------------------------------------------------
  /// The root of this component's rendered DOM, or a null handle before the
  /// first render. See ElementHandle.
  ElementHandle RootElement() const;

  /// The first element in this component's DOM matching `selector` (`#id`,
  /// `.class` or a tag), or a null handle.
  ElementHandle QueryElement(std::string_view selector) const;

  /// Finds the component rendered at `selector` (`#id`, `.class` or a tag),
  /// or nullptr.
  ComponentBase* QueryComponent(std::string_view selector);

  // Hot reload ---------------------------------------------------------------
  void EnableHotReload(
      std::string_view view_var_name = "view",
      std::source_location location = std::source_location::current());
  void EnableHotReload(std::string_view view_var_name,
                       std::string_view filepath);
  void HotReload(std::string_view new_template);

 private:
  // Everything below is the library's own: it reaches it through
  // ComponentInternals (component_internal.hpp), and Component<Derived>
  // through the bindings it registers.
  friend class ScreenImpl;
  friend struct ComponentInternals;
  template <typename Derived>
  friend class Component;

  void ImportAlias(std::string_view class_name,
                   std::string_view alias,
                   ComponentFactory factory);
  bool RunCallback(std::string_view name, std::string_view arg = "");

  /// The tag a template wrote to create this component: what selectors and
  /// QuerySelector() match it by. Tag() -- the class name -- for a component
  /// no template created, such as the application's own.
  std::string_view WrittenTag() const;

  /// Parses the template and builds the DOM. Screen does this itself.
  void Mount();
  /// Rebuilds the DOM from the template and the current state.
  void Render();
  /// The rendered root element.
  Element* Root() const;
  void ResolveTargetStyles();
  void ResolveTargetStyles(double current_time_ms);

  std::string_view Template();

  /// Whether any element under this component still needs a base style pass.
  /// Cheap: a tree walk and a hash per element, no rule matching. Also drops
  /// any resolved-style memo that no longer matches its element's classes.
  bool StylesNeedResolve();

  /// Recomputes `base_style` for the elements that need it, then seeds
  /// `target_style`/`style` from those. Returns immediately when nothing is
  /// stale, so it is safe to call once per frame.
  ///
  /// Render() ends with this, and drawing begins with it, so a component that
  /// mutates its own DOM after rendering -- a Digest() override that re-tags
  /// elements, say -- does not need to ask for anything.
  void ResolveStyles();

  /// While one is alive, Render() leaves style resolution to the end of the
  /// outermost scope, which resolves each re-rendered subtree once: a
  /// component re-rendered inside another one's Render() or Digest() would
  /// otherwise resolve its subtree, only for the enclosing one to do it again.
  class RTXUI_EXPORT StyleResolutionScope {
   public:
    /// `owner` is the component whose Render() or Digest() opens the scope.
    explicit StyleResolutionScope(ComponentBase* owner);
    ~StyleResolutionScope();
    StyleResolutionScope(const StyleResolutionScope&) = delete;
    StyleResolutionScope& operator=(const StyleResolutionScope&) = delete;
  };

  const css::StyleSheet* stylesheet() const;
  const CategorizedRules* categorized_rules() const;
  bool HasAnyPseudoClasses() const;
  static ComponentBase* GetMouseCapturer();

  /// The clipboard write SetClipboard() requested, for Screen to flush.
  static std::optional<std::string> TakePendingClipboardWrite();

  Ref<Element> Slot(std::string_view name);
  const std::map<std::string, Ref<Element>, std::less<>>& slots() const;
  void SetProperty(std::string_view name, std::string_view value);

  void Render(const xml::Node& node,
              Element* element,
              ComponentBase* source,
              const internal::LocalScope* scope = nullptr);
  void RenderReconcile(const xml::Node& node,
                       Element* element,
                       ComponentBase* source,
                       const internal::LocalScope* scope,
                       size_t& child_idx,
                       bool preserve_newlines = false,
                       const internal::SlotFilter* filter = nullptr,
                       const std::string* item_key = nullptr);
  /// The part of RenderReconcile() that renders a `<for>`.
  void RenderLoop(const xml::Node& node,
                  Element* element,
                  ComponentBase* source,
                  const internal::LocalScope* scope,
                  size_t& child_idx,
                  bool preserve_newlines,
                  const internal::SlotFilter* filter);

  /// The value of the bound name `expression`, as text; reports a diagnostic
  /// and returns the expression itself when there is none.
  std::string GetInterpolatedValue(std::string_view expression);

  void MarkSlotsRecreated(bool recreated);
  // Whether a component this one's template wrote re-rendered on its own and
  // left a slot only this one can fill.
  bool ConsumesRecreatedSlots() const;
  /// Whether a virtual loop's scroll container now shows items outside what
  /// the loop rendered, so that it has to render again.
  bool VirtualWindowsStale();

  void AddEntry(internal::ValueBinding entry);
  void AddRange(internal::RangeBinding entry);
  /// The bound name `name`, or nullptr.
  const internal::ValueBinding* FindEntry(std::string_view name) const;
  /// Compares every bound value with its snapshot, and takes a new one.
  /// Whether any changed.
  bool UpdateBindings();
  /// Digests the components this one's template wrote. Whether any changed.
  bool DigestChildren();
  /// Offers `event` to the components this one's template wrote, until one
  /// handles it.
  bool ChildrenOnEvent(Event event);

  /// Everything a component holds, defined inside the library
  /// (component_internal.hpp) so that it is neither visible to a component
  /// nor part of its layout.
  struct Data;
  std::unique_ptr<Data> data_;
};

/// The base of every application component:
///
///   class Counter : public Component<Counter> {
///    public:
///     int count = 0;
///     std::string_view view = R"(<button onclick="Add">{count}</button>)";
///     void Add() { count++; }
///     Counter() { Bind(count); Bind(Add); }
///   };
template <typename Derived>
class Component : public ComponentBase {
 public:
  using ComponentBase::Import;
  static std::string_view StaticTag() { return ClassName<Derived>(); }
  std::string_view Tag() const final { return StaticTag(); }

  // The template is either a `view` member or the result of a Setup() method,
  // both found at compile time. Both must be public.
  std::string_view GetView() const override {
    constexpr bool kHasView = requires(const Derived& d) { d.view; };
    constexpr bool kHasSetup = requires(Derived& d) { d.Setup(); };
    static_assert(!(kHasView && kHasSetup),
                  "Define either a `view` member or a Setup() method, not "
                  "both: Setup() would never be called.");
    if constexpr (kHasView) {
      return static_cast<const Derived*>(this)->view;
    } else if constexpr (kHasSetup) {
      static_assert(std::is_same_v<decltype(std::declval<Derived&>().Setup()),
                                   std::string_view>,
                    "Setup() must return std::string_view.");
      return const_cast<Derived*>(static_cast<const Derived*>(this))->Setup();
    } else {
      return "";
    }
  }

  // What Bind() and BindCollection() call.

  // Member Pointer Binding (Variables or Computed Properties)
  template <typename T, typename C, typename... Args>
  void Import(std::string name, T C::* member, Args&&... args) {
    if constexpr (std::is_member_function_pointer_v<T C::*>) {
      if constexpr (requires { (std::declval<const Derived>().*member)(); }) {
        // Const member function -> Computed property
        AddEntry({name,
                  [this, member]() {
                    return internal::to_string(
                        (static_cast<const Derived*>(this)->*member)());
                  },
                  nullptr, nullptr});
      } else if constexpr (requires {
                             (std::declval<Derived>().*member)(std::string{});
                           }) {
        ComponentBase::Import(
            name,
            std::function<void(std::string)>([this, member](std::string s) {
              (static_cast<Derived*>(this)->*member)(s);
            }));
      } else {
        // Non-const member function -> Event handler
        ComponentBase::Import(name, std::function<void()>([this, member]() {
                                (static_cast<Derived*>(this)->*member)();
                              }));
      }
    } else {
      // Member variable pointer
      auto& ref = static_cast<Derived*>(this)->*member;
      using V = std::remove_reference_t<decltype(ref)>;
      if constexpr (std::ranges::range<V> && !std::is_same_v<V, std::string>) {
        RegisterCollection(name, &ref, std::forward<Args>(args)...);
      } else {
        RegisterState(name, &ref);
      }
    }
  }

  // Generic Binding (Event Handlers, Collections, or State References).
  // State and collections are bound by address, so they must be lvalues: a
  // temporary would be destroyed at the end of the call, leaving a dangling
  // binding behind.
  template <typename T, typename... Args>
    requires(!std::is_member_pointer_v<std::decay_t<T>> &&
             (std::is_lvalue_reference_v<T> ||
              std::is_pointer_v<std::decay_t<T>> ||
              std::is_invocable_v<std::decay_t<T>> ||
              std::is_invocable_v<std::decay_t<T>, std::string>))
  void Import(std::string name, T&& item, Args&&... args) {
    using U = std::decay_t<T>;
    if constexpr (std::is_invocable_v<U> ||
                  std::is_invocable_v<U, std::string>) {
      if constexpr (std::is_invocable_v<U, std::string>) {
        ComponentBase::Import(
            name, std::function<void(std::string)>(std::forward<T>(item)));
      } else if constexpr (std::is_convertible_v<U, ComponentFactory>) {
        ComponentBase::Import(
            name, static_cast<ComponentFactory>(std::forward<T>(item)));
      } else {
        ComponentBase::Import(name,
                              std::function<void()>(std::forward<T>(item)));
      }
    } else if constexpr (std::is_pointer_v<U>) {
      using V = std::remove_pointer_t<U>;
      if constexpr (std::ranges::range<V> && !std::is_same_v<V, std::string>) {
        RegisterCollection(name, item, std::forward<Args>(args)...);
      } else {
        RegisterState(name, item);
      }
    } else {
      if constexpr (std::ranges::range<U> && !std::is_same_v<U, std::string>) {
        RegisterCollection(name, &item, std::forward<Args>(args)...);
      } else {
        RegisterState(name, &item);
      }
    }
  }

 private:
  static std::string WithoutPropsPrefix(std::string name) {
    if (name.starts_with("props.")) {
      name = name.substr(6);
    }
    return name;
  }

  template <typename Container>
  void RegisterCollection(std::string name, const Container* ptr) {
    AddRange({WithoutPropsPrefix(std::move(name)),
              std::make_shared<internal::TypeErasedRangeImpl<Container>>(ptr)});
  }

  // The binding's own fields: Bind(books, [](const Book&, FieldSink&) {...}).
  template <typename Container>
  void RegisterCollection(
      std::string name,
      const Container* ptr,
      std::function<void(const std::ranges::range_value_t<Container>&,
                         FieldSink&)> fields) {
    AddRange({WithoutPropsPrefix(std::move(name)),
              std::make_shared<internal::TypeErasedRangeImpl<Container>>(
                  ptr, std::move(fields))});
  }

  // The same, from a const member function: Bind(books, &App::BookFields).
  template <typename Container, typename C>
    requires std::derived_from<Derived, C>
  void RegisterCollection(
      std::string name,
      const Container* ptr,
      void (C::*fields)(const std::ranges::range_value_t<Container>&,
                        FieldSink&) const) {
    RegisterCollection(
        std::move(name), ptr,
        std::function<void(const std::ranges::range_value_t<Container>&,
                           FieldSink&)>(
            [this, fields](const std::ranges::range_value_t<Container>& item,
                           FieldSink& out) {
              (static_cast<const Derived*>(this)->*fields)(item, out);
            }));
  }

  template <typename T>
  void RegisterState(std::string name, T* ptr) {
    auto snapshot = std::make_shared<T>(*ptr);
    auto get_value = [ptr]() { return internal::to_string(*ptr); };
    auto check_and_update = [ptr, snapshot]() mutable {
      if (*ptr != *snapshot) {
        *snapshot = *ptr;
        return true;
      }
      return false;
    };
    auto set_value = [ptr](std::string_view val) {
      if constexpr (requires(std::stringstream ss) { ss >> *ptr; }) {
        internal::from_string(val, *ptr);
      }
    };
    AddEntry({WithoutPropsPrefix(std::move(name)), std::move(get_value),
              std::move(check_and_update), std::move(set_value)});
  }
};

}  // namespace rtxui

#endif  // RTXUI_COMPONENT_HPP_
