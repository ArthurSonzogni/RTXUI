// Copyright 2024 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#ifndef RTXUI_COMPONENT_HPP_
#define RTXUI_COMPONENT_HPP_

#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <ranges>
#include <set>
#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <variant>
#include <vector>
#if defined(RTXUI_HAS_REFLECTION)
#include <meta>
#endif

#include "rtxui/internal/class_name.hpp"
#include "rtxui/internal/event.hpp"
#include "rtxui/internal/import.hpp"
#include "rtxui/internal/refcounted.hpp"

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

class StructVisitor {
 public:
  virtual ~StructVisitor() = default;
  virtual std::string GetFieldValue(std::string_view field_name) const = 0;
};

class TypeErasedRange {
 public:
  virtual ~TypeErasedRange() = default;
  virtual size_t Size() const = 0;
  virtual std::string GetItemString(size_t index) const = 0;
  virtual std::shared_ptr<StructVisitor> GetItemVisitor(size_t index) const = 0;
  virtual bool CheckAndUpdate() = 0;
};

struct LocalScope {
  std::shared_ptr<LocalScope> parent;
  std::unordered_map<std::string,
                     std::variant<std::string, std::shared_ptr<StructVisitor>>>
      variables;

  std::optional<std::variant<std::string, std::shared_ptr<StructVisitor>>> Get(
      std::string_view name) const {
    auto it = variables.find(std::string(name));
    if (it != variables.end()) {
      return it->second;
    }
    if (parent) {
      return parent->Get(name);
    }
    return std::nullopt;
  }
};

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
  void ResolveTargetStyles();
  void ResolveTargetStyles(double current_time_ms);
  const css::StyleSheet* stylesheet() const;
  virtual bool Digest() = 0;
  virtual void InitReflection();
  virtual bool OnEvent(Event event);

  Element* Root() const;
  Ref<Element> Slot(std::string_view name);
  const std::map<std::string, Ref<Element>>& slots() const { return slots_; }
  void SetProperty(std::string_view name, std::string_view value);
  void PropagateBinding(std::string_view child_prop, std::string_view value);

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

 protected:
  std::unique_ptr<css::StyleSheet> stylesheet_;
  std::vector<std::string> css_strings_;
  std::vector<BindingLink> two_way_bindings_;
  void Render(const xml::Node& node,
              Element* element,
              ComponentBase* source,
              std::shared_ptr<LocalScope> scope = nullptr);
  void RenderReconcile(const xml::Node& node,
                       Element* element,
                       ComponentBase* source,
                       std::shared_ptr<LocalScope> scope,
                       size_t& child_idx);
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

class ManualStructVisitor : public StructVisitor {
  std::unordered_map<std::string, std::string> fields_;

 public:
  ManualStructVisitor(std::unordered_map<std::string, std::string> fields)
      : fields_(std::move(fields)) {}

  std::string GetFieldValue(std::string_view field_name) const override {
    auto it = fields_.find(std::string(field_name));
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

  std::shared_ptr<StructVisitor> GetItemVisitor(size_t index) const override {
    using ItemType = std::ranges::range_value_t<Container>;
    auto it = std::ranges::begin(*container_ptr_);
    std::advance(it, index);

    if (mapper_) {
      return mapper_(*it);
    }

#if defined(RTXUI_HAS_REFLECTION)
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
        snapshot_ = *container_ptr_;
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

  // Generic Binding (Event Handlers, Collections, or State References)
  template <typename T, typename... Args>
    requires(!std::is_member_pointer_v<std::decay_t<T>>)
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

// BindComputed(x) and BindCallback(x) register member functions.
#define BindComputed(x) this->Import(#x, &std::decay_t<decltype(*this)>::x)
#define BindCallback(x) this->Import(#x, &std::decay_t<decltype(*this)>::x)

// BindCollection(name, x) registers a collection with an explicit name.
#define BindCollection(name, ...) this->Import(name, ##__VA_ARGS__)

void RegisterGlobalComponent(std::string_view name, ComponentFactory factory);
ComponentFactory GetGlobalComponentFactory(std::string_view name);

}  // namespace rtxui

#endif  // RTXUI_COMPONENT_HPP_
