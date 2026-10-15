// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#ifndef RTXUI_INTERNAL_BINDING_HPP_
#define RTXUI_INTERNAL_BINDING_HPP_

// Not part of the API: how Bind() erases the type of what it binds. In a
// public header only because Bind() instantiates it with the application's own
// types; the library reads the result through the abstract classes below.

#include <concepts>
#include <cstddef>
#include <functional>
#include <iterator>
#include <memory>
#include <ranges>
#include <rtxui/rtxui_export.hpp>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>
// C++26 static reflection, when the code including this header is compiled
// with it (e.g. GCC 16 with -std=c++26 -freflection). Each application decides
// for itself: the library's own code never needs it, so a library built as
// C++23 serves applications compiled either way.
#if defined(__cpp_impl_reflection) && __has_include(<meta>)
#define RTXUI_HAS_REFLECTION 1
#include <array>
#include <meta>
#endif

#include "rtxui/field_sink.hpp"
#include "rtxui/internal/convert.hpp"

namespace rtxui::internal {

/// The fields of one item of a bound collection, by name.
class RTXUI_EXPORT StructVisitor {
 public:
  virtual ~StructVisitor() = default;
  virtual std::string GetFieldValue(std::string_view field_name) const = 0;
};

/// A bound collection, whatever its type.
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

/// A bound name: how to read its value and, for state, how to tell that it
/// changed since the last look and how to set it from a string.
struct ValueBinding {
  std::string name;
  std::function<std::string()> get_value;
  std::function<bool()> check_and_update;
  std::function<void(std::string_view)> set_value;
};

/// A bound collection, for `<for>`.
struct RangeBinding {
  std::string name;
  std::shared_ptr<TypeErasedRange> range;
};

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
        ? (result = to_string(obj.[:Arr[Is]:]))
        : std::string{}),
   ...);
  return result;
}

template <typename T>
class ReflectedStructVisitor : public StructVisitor {
  const T& obj_;

 public:
  explicit ReflectedStructVisitor(const T& obj) : obj_(obj) {}

  std::string GetFieldValue(std::string_view field_name) const override {
    constexpr auto members = get_members<T>();
    return get_field_value_impl<members>(
        obj_, field_name, std::make_index_sequence<members.size()>{});
  }
};
#endif

class FieldSinkVisitor : public StructVisitor {
  FieldSink fields_;

 public:
  explicit FieldSinkVisitor(FieldSink fields) : fields_(std::move(fields)) {}

  std::string GetFieldValue(std::string_view field_name) const override {
    return std::string(fields_.Get(field_name));
  }
};

// Whether `T` describes its fields with a Fields(const T&, FieldSink&)
// function, found by argument-dependent lookup.
template <typename T>
concept HasFields =
    requires(const T& item, FieldSink& out) { Fields(item, out); };

template <typename Container>
class TypeErasedRangeImpl : public TypeErasedRange {
  using Item = std::ranges::range_value_t<Container>;
  using FieldsMapper = std::function<void(const Item&, FieldSink&)>;

  const Container* container_ptr_;
  std::decay_t<Container> snapshot_;
  // The binding's own fields, which take precedence over the type's.
  FieldsMapper fields_mapper_;
  // For items without operator==: their fields, to find what changed.
  std::vector<FieldSink> fields_snapshot_;

 public:
  explicit TypeErasedRangeImpl(const Container* ptr)
      : container_ptr_(ptr), snapshot_(*ptr) {
    fields_snapshot_ = CurrentFields();
  }

  TypeErasedRangeImpl(const Container* ptr, FieldsMapper fields_mapper)
      : container_ptr_(ptr),
        snapshot_(*ptr),
        fields_mapper_(std::move(fields_mapper)) {
    fields_snapshot_ = CurrentFields();
  }

  size_t Size() const override { return std::ranges::size(*container_ptr_); }

  std::string GetItemString(size_t index) const override {
    auto it = std::ranges::begin(*container_ptr_);
    std::advance(it, index);
    return to_string(*it);
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
      fallback_storage = to_string(*it);
      return fallback_storage;
    }
  }

  std::shared_ptr<StructVisitor> GetItemVisitor(size_t index) const override {
    auto it = std::ranges::begin(*container_ptr_);
    std::advance(it, index);

    if (HasItemFields()) {
      FieldSink fields;
      ItemFields(*it, fields);
      return std::make_shared<FieldSinkVisitor>(std::move(fields));
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
    if constexpr (!std::equality_comparable<Item>) {
      // Without operator==, compare what the template reads of each item.
      if (HasItemFields()) {
        std::vector<FieldSink> fields = CurrentFields();
        if (fields != fields_snapshot_) {
          fields_snapshot_ = std::move(fields);
          return true;
        }
        return false;
      }
    }
    if constexpr (std::equality_comparable<Item>) {
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

 private:
  bool HasItemFields() const { return fields_mapper_ || HasFields<Item>; }

  void ItemFields(const Item& item, FieldSink& out) const {
    if (fields_mapper_) {
      fields_mapper_(item, out);
    } else if constexpr (HasFields<Item>) {
      Fields(item, out);
    }
  }

  std::vector<FieldSink> CurrentFields() const {
    std::vector<FieldSink> fields;
    if constexpr (!std::equality_comparable<Item>) {
      if (HasItemFields()) {
        for (const auto& item : *container_ptr_) {
          ItemFields(item, fields.emplace_back());
        }
      }
    }
    return fields;
  }
};

}  // namespace rtxui::internal

#endif  // RTXUI_INTERNAL_BINDING_HPP_
