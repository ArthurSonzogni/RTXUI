// Copyright 2024 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#ifndef RTXUI_COMPONENT_HPP_
#define RTXUI_COMPONENT_HPP_

#include <charconv>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <ranges>
#include <source_location>
#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>
#if defined(RTXUI_HAS_REFLECTION)
#include <meta>
#endif

#include <rtxui/diagnostic.hpp>
#include <rtxui/rtxui_export.hpp>

#include "rtxui/element.hpp"
#include "rtxui/event.hpp"
#include "rtxui/internal/class_name.hpp"
#include "rtxui/internal/import.hpp"
#include "rtxui/refcounted.hpp"

namespace css {
struct Ruleset;
using StyleSheet = std::vector<Ruleset>;
}  // namespace css

namespace rtxui {
class Element;
}  // namespace rtxui

namespace xml {
struct Node;
using Nodes = std::vector<Node>;
}  // namespace xml

namespace rtxui {
struct CategorizedRules;

class RTXUI_EXPORT StructVisitor {
 public:
  virtual ~StructVisitor() = default;
  virtual std::string GetFieldValue(std::string_view field_name) const = 0;
};

class RTXUI_EXPORT TypeErasedRange {
 public:
  virtual ~TypeErasedRange() = default;
  virtual size_t Size() const = 0;
  virtual std::string GetItemString(size_t index) const = 0;
  virtual std::string_view GetItemStringView(
      size_t index,
      std::string& fallback_storage) const = 0;
  virtual std::shared_ptr<StructVisitor> GetItemVisitor(size_t index) const = 0;
  virtual bool CheckAndUpdate() = 0;
};

struct LocalScope {
  const LocalScope* parent = nullptr;
  std::string_view name;
  std::variant<std::string_view, std::shared_ptr<StructVisitor>> value;

  const std::variant<std::string_view, std::shared_ptr<StructVisitor>>* Get(
      std::string_view var_name) const {
    if (name == var_name) {
      return &value;
    }
    if (parent) {
      return parent->Get(var_name);
    }
    return nullptr;
  }
};

class RTXUI_EXPORT ComponentBase : public RefCounted, public Bindings {
 public:
  ComponentBase();
  virtual ~ComponentBase();
  ComponentBase(const ComponentBase&) = delete;
  ComponentBase& operator=(const ComponentBase&) = delete;
  ComponentBase(ComponentBase&&) = delete;
  ComponentBase& operator=(ComponentBase&&) = delete;

  virtual std::string_view GetView() const = 0;
  virtual std::string_view Tag() const = 0;

  // Reactivity ---------------------------------------------------------------
  /// Registers the component's bindings. Called once, before the first
  /// render; override it to Bind() or Import() there instead of in the
  /// constructor.
  virtual void InitReflection();

  /// Compares bound state with its last snapshot, re-renders what changed and
  /// recurses into children. Screen calls it every frame; returns whether
  /// anything changed.
  virtual bool Digest() = 0;

  /// Pushes `value` to the parent state bound to this component's `child_prop`
  /// attribute, for a component exposing a two-way binding.
  void PropagateBinding(std::string_view child_prop, std::string_view value);

  // Input --------------------------------------------------------------------
  /// Handles an event no focused element consumed. Return true to stop it.
  virtual bool OnEvent(Event event);
  void CaptureMouse();
  void ReleaseMouse();

  /// Asks the terminal to set the system clipboard to `text` (OSC 52), on the
  /// next frame.
  void SetClipboard(std::string_view text);

  // DOM ----------------------------------------------------------------------
  /// The root of this component's rendered DOM, or a null handle before
  /// Mount(). See ElementHandle.
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

  // Driving a component without a Screen, for tests --------------------------
  /// Parses the template and builds the DOM. Screen does this itself.
  void Mount();
  /// Rebuilds the DOM from the template and the current state.
  void Render();
  /// The rendered root element. Element is internal to the library.
  Element* Root() const;
  void ResolveTargetStyles();
  void ResolveTargetStyles(double current_time_ms);

 protected:
  // The library's own access to what follows; see component_internal.hpp.
  friend class ScreenImpl;
  friend struct ComponentInternals;

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
  const CategorizedRules* categorized_rules() const {
    return categorized_rules_.get();
  }
  bool HasAnyPseudoClasses() const;
  static ComponentBase* GetMouseCapturer();

  /// The clipboard write SetClipboard() requested, for Screen to flush.
  static std::optional<std::string> TakePendingClipboardWrite();

  Ref<Element> Slot(std::string_view name);
  const std::map<std::string, Ref<Element>, std::less<>>& slots() const {
    return slots_;
  }
  void SetProperty(std::string_view name, std::string_view value);

  virtual std::string GetInterpolatedValue(std::string_view expression) = 0;

  struct BindingLink {
    std::string child_prop;
    ComponentBase* parent;
    std::string parent_prop;
  };

  struct RangeEntry {
    std::string name;
    std::shared_ptr<TypeErasedRange> range;
  };
  std::vector<RangeEntry> range_entries_;

  // Shared with every other instance declaring the same stylesheet text; see
  // GetSharedStyle. Aliasing pointers into one StyleData, so both keep it
  // alive and the index's pointers into the sheet stay valid.
  std::shared_ptr<const css::StyleSheet> stylesheet_;
  std::shared_ptr<const CategorizedRules> categorized_rules_;
  std::vector<std::string> css_strings_;
  std::vector<BindingLink> two_way_bindings_;
  // Per bound property, how the parent renders the value this component last
  // pushed to it, when that differs from what was pushed ("1." pushed into a
  // double renders as "1"). See PropagateBinding().
  std::map<std::string, std::string, std::less<>> binding_echoes_;
  int last_render_terminal_width_ = -1;
  int last_render_terminal_height_ = -1;
  void Render(const xml::Node& node,
              Element* element,
              ComponentBase* source,
              const LocalScope* scope = nullptr);
  /// Restricts which of a node's children a reconcile pass consumes, so that
  /// content projected into a component can be split across several slots by
  /// tag. See `<slot.name select="tag">` in RenderReconcile.
  struct SlotFilter {
    /// Tags claimed by a sibling select-slot; skipped by the default slot.
    const std::vector<std::string>* claimed = nullptr;
    /// When set, only children carrying this tag are consumed.
    const std::string* only = nullptr;
  };
  void RenderReconcile(const xml::Node& node,
                       Element* element,
                       ComponentBase* source,
                       const LocalScope* scope,
                       size_t& child_idx,
                       bool preserve_newlines = false,
                       const SlotFilter* filter = nullptr,
                       const std::string* item_key = nullptr);
  std::string template_;
  std::string xml_string_;
  xml::Nodes xml_nodes_;
  Ref<Element> root_;
  std::map<std::string, Ref<Element>, std::less<>> slots_;
  std::vector<Ref<ComponentBase>> children_;
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
RTXUI_EXPORT int ParseInt(std::string_view str);

template <typename T>
std::string to_string(const T& value) {
  if constexpr (std::is_same_v<T, bool>) {
    return value ? "true" : "false";
  } else if constexpr (std::is_same_v<T, char>) {
    return std::string(1, value);
  }
  if constexpr (std::is_floating_point_v<T>) {
    char buf[64];
    auto [ptr, ec] = std::to_chars(buf, buf + sizeof(buf), value);
    if (ec == std::errc()) {
      return std::string(buf, ptr - buf);
    }
  }
  if constexpr (std::is_convertible_v<T, std::string>) {
    return static_cast<std::string>(value);
  } else if constexpr (requires { std::to_string(value); }) {
    return std::to_string(value);
  } else if constexpr (requires(std::ostream& os, const T& v) { os << v; }) {
    std::stringstream ss;
    ss << value;
    return ss.str();
  } else {
    return "";
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
  } else if constexpr (std::is_floating_point_v<T>) {
    std::string_view s = str;
    while (!s.empty() && std::isspace(static_cast<unsigned char>(s.front()))) {
      s.remove_prefix(1);
    }
    while (!s.empty() && std::isspace(static_cast<unsigned char>(s.back()))) {
      s.remove_suffix(1);
    }
    if (!s.empty()) {
      auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), value);
      if (ec != std::errc()) {
        value = 0.0f;
      }
    } else {
      value = 0.0f;
    }
  } else {
    std::stringstream ss;
    ss << str;
    ss >> value;
  }
}
}  // namespace reflection

#if defined(RTXUI_HAS_REFLECTION)
template <typename T>
consteval size_t get_members_size() {
  return std::meta::nonstatic_data_members_of(
             ^^T, std::meta::access_context::unchecked())
      .size();
}

template <typename T>
consteval auto get_members() {
  constexpr size_t N = get_members_size<T>();
  std::array<std::meta::info, N> arr{};
  auto vec = std::meta::nonstatic_data_members_of(
      ^^T, std::meta::access_context::unchecked());
  for (size_t i = 0; i < N; ++i) {
    arr[i] = vec[i];
  }
  return arr;
}

template <auto Arr, typename T, size_t... Is>
std::string get_field_value_impl(const T& obj,
                                 std::string_view field_name,
                                 std::index_sequence<Is...>) {
  std::string result;
  ((std::meta::identifier_of(Arr[Is]) == field_name
        ? (result = reflection::to_string(obj.[:Arr[Is]:]))
        : std::string{}),
   ...);
  return result;
}

template <typename T>
class ReflectedStructVisitor : public StructVisitor {
  const T& obj_;

 public:
  ReflectedStructVisitor(const T& obj) : obj_(obj) {}

  std::string GetFieldValue(std::string_view field_name) const override {
    constexpr auto members = get_members<T>();
    return get_field_value_impl<members>(
        obj_, field_name, std::make_index_sequence<members.size()>{});
  }
};
#endif

class RTXUI_EXPORT ManualStructVisitor : public StructVisitor {
  std::map<std::string, std::string, std::less<>> fields_;

 public:
  ManualStructVisitor(std::map<std::string, std::string, std::less<>> fields)
      : fields_(std::move(fields)) {}

  std::string GetFieldValue(std::string_view field_name) const override {
    auto it = fields_.find(field_name);
    return (it != fields_.end()) ? it->second : "";
  }
};

template <typename Container>
class TypeErasedRangeImpl : public TypeErasedRange {
  const Container* container_ptr_;
  std::decay_t<Container> snapshot_;
  std::function<std::shared_ptr<StructVisitor>(
      const std::ranges::range_value_t<Container>&)>
      mapper_;

 public:
  TypeErasedRangeImpl(const Container* ptr)
      : container_ptr_(ptr), snapshot_(*ptr) {}

  TypeErasedRangeImpl(const Container* ptr,
                      std::function<std::shared_ptr<StructVisitor>(
                          const std::ranges::range_value_t<Container>&)> mapper)
      : container_ptr_(ptr), snapshot_(*ptr), mapper_(mapper) {}

  size_t Size() const override { return std::ranges::size(*container_ptr_); }

  std::string GetItemString(size_t index) const override {
    auto it = std::ranges::begin(*container_ptr_);
    std::advance(it, index);
    return reflection::to_string(*it);
  }

  std::string_view GetItemStringView(
      size_t index,
      std::string& fallback_storage) const override {
    auto it = std::ranges::begin(*container_ptr_);
    std::advance(it, index);
    using ItemType = std::ranges::range_value_t<Container>;
    if constexpr (std::is_convertible_v<ItemType, std::string_view>) {
      return *it;
    } else {
      fallback_storage = reflection::to_string(*it);
      return fallback_storage;
    }
  }

  std::shared_ptr<StructVisitor> GetItemVisitor(size_t index) const override {
    auto it = std::ranges::begin(*container_ptr_);
    std::advance(it, index);

    if (mapper_) {
      return mapper_(*it);
    }

#if defined(RTXUI_HAS_REFLECTION)
    using ItemType = std::ranges::range_value_t<Container>;
    if constexpr (std::is_class_v<ItemType> &&
                  !std::is_same_v<ItemType, std::string>) {
      return std::make_shared<ReflectedStructVisitor<ItemType>>(*it);
    }
#endif
    return nullptr;
  }

  bool CheckAndUpdate() override {
    if constexpr (requires { *container_ptr_ != snapshot_; }) {
      if (*container_ptr_ != snapshot_) {
        if constexpr (requires {
                        snapshot_.size();
                        snapshot_[0] = (*container_ptr_)[0];
                      }) {
          if (snapshot_.size() != container_ptr_->size()) {
            snapshot_ = *container_ptr_;
          } else {
            for (size_t i = 0; i < container_ptr_->size(); ++i) {
              if (snapshot_[i] != (*container_ptr_)[i]) {
                snapshot_[i] = (*container_ptr_)[i];
              }
            }
          }
        } else {
          snapshot_ = *container_ptr_;
        }
        return true;
      }
    } else {
      if (std::ranges::size(*container_ptr_) != std::ranges::size(snapshot_)) {
        snapshot_ = *container_ptr_;
        return true;
      }
    }
    return false;
  }
};

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

  bool Digest() override {
    StyleResolutionScope scope(this);
    bool changed = false;
    for (auto& entry : entries_) {
      if (entry.check_and_update && entry.check_and_update()) {
        changed = true;
      }
    }
    for (auto& range_entry : range_entries_) {
      if (range_entry.range->CheckAndUpdate()) {
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
    if (expression.find_first_of(" =!<>&|") != std::string_view::npos) {
      ReportDiagnostic("'{" + std::string(expression) + "}' in <" +
                       std::string(Tag()) +
                       ">: templates do not evaluate expressions; bind a "
                       "const method computing the value and use its name");
    } else {
      ReportDiagnostic("'{" + std::string(expression) + "}' in <" +
                       std::string(Tag()) + "> is not a bound name");
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

  // Member Pointer Binding (Variables or Computed Properties)
  template <typename T, typename C, typename... Args>
  void Import(std::string name, T C::* member, Args&&... args) {
    if constexpr (std::is_member_function_pointer_v<T C::*>) {
      if constexpr (requires { (std::declval<const Derived>().*member)(); }) {
        // Const member function -> Computed property
        entries_.push_back(
            {name,
             [this, member]() {
               return reflection::to_string(
                   (static_cast<const Derived*>(this)->*member)());
             },
             nullptr, nullptr});
      } else if constexpr (requires {
                             (std::declval<Derived>().*member)(std::string{});
                           }) {
        Bindings::Import(name, std::function<void(std::string)>(
                                   [this, member](std::string s) {
                                     (static_cast<Derived*>(this)->*member)(s);
                                   }));
      } else {
        // Non-const member function -> Event handler
        Bindings::Import(name, [this, member]() {
          (static_cast<Derived*>(this)->*member)();
        });
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
        Bindings::Import(
            name, std::function<void(std::string)>(std::forward<T>(item)));
      } else if constexpr (std::is_convertible_v<U, ComponentFactory>) {
        Bindings::Import(name,
                         static_cast<ComponentFactory>(std::forward<T>(item)));
      } else {
        Bindings::Import(name, std::function<void()>(std::forward<T>(item)));
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

  template <typename Container>
  void RegisterCollection(std::string name, const Container* ptr) {
    std::string clean_name = name;
    if (clean_name.starts_with("props.")) {
      clean_name = clean_name.substr(6);
    }
    range_entries_.push_back(
        {std::move(clean_name),
         std::make_shared<TypeErasedRangeImpl<Container>>(ptr)});
  }

  template <typename Container>
  void RegisterCollection(
      std::string name,
      const Container* ptr,
      std::function<std::shared_ptr<StructVisitor>(
          const std::ranges::range_value_t<Container>&)> mapper) {
    std::string clean_name = name;
    if (clean_name.starts_with("props.")) {
      clean_name = clean_name.substr(6);
    }
    range_entries_.push_back(
        {std::move(clean_name),
         std::make_shared<TypeErasedRangeImpl<Container>>(ptr, mapper)});
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
      if constexpr (requires(std::stringstream ss) { ss >> *ptr; }) {
        reflection::from_string(val, *ptr);
      }
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

// Bind(x) registers state variables, computed properties, or callbacks.
#define Bind(x, ...) \
  this->Import(#x, &std::decay_t<decltype(*this)>::x, ##__VA_ARGS__)

// BindCollection(name, x) registers a collection with an explicit name.
#define BindCollection(name, ...) this->Import(name, ##__VA_ARGS__)

RTXUI_EXPORT void RegisterGlobalComponent(std::string_view name,
                                          ComponentFactory factory);
RTXUI_EXPORT ComponentFactory GetGlobalComponentFactory(std::string_view name);

/// Reported when a <style> block fails to parse.
struct CssError {
  /// The error message.
  std::string message;

  /// The line where the error occurred, relative to that <style> block's
  /// own text. 0-based.
  int line;

  /// The column where the error occurred. 0-based.
  int column;
};

/// Installs a handler invoked whenever a <style> block fails to parse (e.g.
/// one whose text is reactively interpolated from bound state). The default
/// handler prints a formatted error to stderr, which is fine for a plain
/// terminal session but corrupts a running frame for any app that owns the
/// terminal in raw mode -- apps that render arbitrary/live-edited CSS (e.g.
/// a playground) should install their own handler to surface the error
/// through their own UI instead. Pass nullptr to restore the default.
RTXUI_EXPORT void SetCssErrorHandler(
    std::function<void(const CssError&)> handler);

/// Reported when XML/HTML content generated from live-edited or bound state
/// fails to parse (e.g. the <markdown> component's rendered body plus its
/// wrapped stylesheet).
struct XmlError {
  /// The error message.
  std::string message;

  /// The line where the error occurred, within the generated document.
  /// 0-based.
  int line;

  /// The column where the error occurred. 0-based.
  int column;
};

/// Installs a handler invoked whenever XML/HTML content generated from
/// live-edited or bound state fails to parse (see XmlError). The default
/// handler prints a formatted error to stderr, which corrupts a running
/// frame for any app that owns the terminal in raw mode -- apps that render
/// live-edited content (e.g. a Markdown playground) should install their own
/// handler to surface the error through their own UI instead, the same way
/// SetCssErrorHandler works for <style> blocks. Pass nullptr to restore the
/// default.
RTXUI_EXPORT void SetXmlErrorHandler(
    std::function<void(const XmlError&)> handler);

}  // namespace rtxui

#endif  // RTXUI_COMPONENT_HPP_
