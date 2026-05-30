// Copyright 2024 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.

#pragma once

#include <concepts>
#include <cstdint>
#include <utility>

namespace rtxui {

template <typename T>
class Ref;

class RefCounted {
 public:
  RefCounted() = default;
  virtual ~RefCounted();

  void AddRef() const;
  void Release() const;

 private:
  mutable uint16_t count_ = 0;
};

template <typename T>
class Ref {
 public:
  Ref() = default;

  template <typename... Args>
  static Ref<T> New(Args&&... args) {
    return new T(std::forward<Args>(args)...);
  }

  Ref(T* ptr) : ptr_(ptr) {
    if (ptr_) {
      ptr_->AddRef();
    }
  }
  ~Ref() {
    if (ptr_) {
      ptr_->Release();
      ptr_ = nullptr;
    }
  }

  Ref(std::nullptr_t) {}

  // Comparisons.
  auto operator<=>(const Ref& other) const { return ptr_ <=> other.ptr_; }

  // Access to the underlying pointer. -----------------------------------------
  operator T*() const { return ptr_; }
  T* get() const { return ptr_; }
  T* operator->() const { return ptr_; }
  T& operator*() const { return *ptr_; }

  // Downcast from derived class. ----------------------------------------------
  template <typename U>
    requires std::derived_from<U, T>
  Ref(const Ref<U>& other) : ptr_(other.ptr_) {
    if (ptr_) {
      ptr_->AddRef();
    }
  }
  Ref(const Ref& other) : ptr_(other.ptr_) {
    if (ptr_) {
      ptr_->AddRef();
    }
  }
  template <typename U>
    requires std::derived_from<U, T>
  Ref(Ref<U>&& other) {
    ptr_ = other.ptr_;
    other.ptr_ = nullptr;
  }
  Ref(Ref&& other) { std::swap(ptr_, other.ptr_); }

  Ref& operator=(std::nullptr_t) {
    if (ptr_) {
      ptr_->Release();
      ptr_ = nullptr;
    }
    return *this;
  }

  template <typename U>
    requires std::derived_from<U, T>
  Ref& operator=(Ref<U>& other) {
    if (other.ptr_) {
      other.ptr_->AddRef();
    }
    if (ptr_) {
      ptr_->Release();
    }
    ptr_ = other.ptr_;
    return *this;
  }
  Ref& operator=(const Ref& other) {
    if (other.ptr_) {
      other.ptr_->AddRef();
    }
    if (ptr_) {
      ptr_->Release();
    }
    ptr_ = other.ptr_;
    return *this;
  }
  template <typename U>
    requires std::derived_from<U, T>
  Ref& operator=(Ref<U>&& other) {
    if (ptr_) {
      ptr_->Release();
    }
    ptr_ = other.ptr_;
    other.ptr_ = nullptr;
    return *this;
  }
  Ref& operator=(Ref&& other) {
    std::swap(ptr_, other.ptr_);
    return *this;
  }

 private:
  template <typename U>
  friend class Ref;

  T* ptr_ = nullptr;
};

}  // namespace rtxui
