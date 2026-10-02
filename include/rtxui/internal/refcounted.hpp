// Copyright 2024 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.

#pragma once

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <rtxui/rtxui_export.hpp>
#include <utility>

namespace rtxui {

template <typename T>
class Ref;

/// Base class of objects owned through Ref<T>. The count is not atomic: a
/// RefCounted object, and every Ref to it, belongs to one thread. To hand work
/// back to the UI thread, capture plain values and use PostTask().
class RTXUI_EXPORT RefCounted {
 public:
  RefCounted() = default;
  virtual ~RefCounted();

  void AddRef() const;
  void Release() const;

 private:
  mutable uint32_t count_ = 0;
};

/// An owning pointer to a RefCounted object. Create one with Ref<T>::New().
template <typename T>
class Ref {
 public:
  Ref() = default;
  Ref(std::nullptr_t) {}

  template <typename... Args>
  static Ref<T> New(Args&&... args) {
    return Ref<T>(new T(std::forward<Args>(args)...));
  }

  /// Takes a reference to `ptr`. Explicit, so that adopting a raw pointer is
  /// visible at the call site.
  explicit Ref(T* ptr) : ptr_(ptr) {
    if (ptr_) {
      ptr_->AddRef();
    }
  }
  ~Ref() {
    if (ptr_) {
      ptr_->Release();
    }
  }

  Ref(const Ref& other) : Ref(other.ptr_) {}
  Ref(Ref&& other) noexcept : ptr_(std::exchange(other.ptr_, nullptr)) {}

  // Upcast from a derived class.
  template <typename U>
    requires std::derived_from<U, T>
  Ref(const Ref<U>& other) : Ref(static_cast<T*>(other.ptr_)) {}
  template <typename U>
    requires std::derived_from<U, T>
  Ref(Ref<U>&& other) noexcept : ptr_(std::exchange(other.ptr_, nullptr)) {}

  // Assigns by copy-and-swap, which also covers the upcasts above and keeps
  // self-assignment safe.
  Ref& operator=(Ref other) noexcept {
    std::swap(ptr_, other.ptr_);
    return *this;
  }

  // Comparisons.
  auto operator<=>(const Ref& other) const { return ptr_ <=> other.ptr_; }

  // Access to the underlying pointer. -----------------------------------------
  operator T*() const { return ptr_; }
  T* get() const { return ptr_; }
  T* operator->() const { return ptr_; }
  T& operator*() const { return *ptr_; }

 private:
  template <typename U>
  friend class Ref;

  T* ptr_ = nullptr;
};

}  // namespace rtxui
