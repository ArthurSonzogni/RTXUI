// Copyright 2024 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.

#pragma once

#include <cassert>
#include <utility>

namespace rtxui {

template <typename T>
class Ref;

class RefCounted {
 public:
  RefCounted() = default;
  virtual ~RefCounted() { assert(count_ == 0); }

  void AddRef() { ++count_; }
  void Release() {
    --count_;
    if (count_ == 0) {
      delete this;
    }
  }

 private:
  mutable int count_ = 1;
};

template <typename T>
class Ref {
 public:
  template <typename... Args>
  Ref(Args&&... args) : ptr_(new T(std::forward<Args>(args)...)) {}

  // Allow creating a null Ref.
  Ref(std::nullptr_t) {}

  // Allow implicit conversion from T*.
  Ref(T* ptr) : ptr_(ptr) { ptr_->AddRef(); }

  // Copy and move constructors.
  Ref(const Ref& other) : ptr_(other.ptr_) { ptr_->AddRef(); }
  Ref(Ref&& other) : ptr_(other.ptr_) { other.ptr_ = nullptr; }

  // Copy and move constructor from derived class.
  template <typename U>
  Ref(Ref<U>& other) : ptr_(other.get()) {
    ptr_->AddRef();
  }
  template <typename U>
  Ref(Ref<U>&& other) : ptr_(other.get()) {
    other.get() = nullptr;
  }

  ~Ref() {
    if (ptr_) {
      ptr_->Release();
    }
  }

  Ref& operator=(const Ref& other) {
    if (ptr_) {
      ptr_->Release();
    }
    ptr_ = other.ptr_;
    ptr_->AddRef();
    return *this;
  }

  Ref& operator=(Ref&& other) {
    if (ptr_) {
      ptr_->Release();
    }
    ptr_ = other.ptr_;
    other.ptr_ = nullptr;
    return *this;
  }

  // Allow implicit conversion to T*.
  operator T*() { return ptr_; }

  T* get() { return ptr_; }
  T* operator->() { return ptr_; }
  const T* get() const { return ptr_; }
  const T* operator->() const { return ptr_; }
  T& operator*() { return *ptr_; }
  const T& operator*() const { return *ptr_; }

 private:
  T* ptr_ = nullptr;
};

}  // namespace rtxui
