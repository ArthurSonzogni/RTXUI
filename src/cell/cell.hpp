// Copyright 2024 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#pragma once

#include <algorithm>
#include <functional>
#include <iostream>
#include <utility>
#include <vector>

#include "core/refcounted.hpp"

namespace rtxui {

class Cell : public RefCounted {
 public:
  void InvalidateDependents() {
    std::cerr << __func__ << std::endl;
    for (auto* dependent : dependents_) {
      dependent->Invalidate();
    }
  }
  virtual void Invalidate() {}
  ~Cell() override {
    std::cerr << __func__ << std::endl;
    for (auto* dependent : dependents_) {
      std::erase_if(dependent->dependencies_,
                    [this](const auto& ref) { return ref.get() == this; });
    }
    for (auto& ref : dependencies_) {
      std::erase_if(ref->dependents_, [this](const auto* dependent) {
        return dependent == this;
      });
    }
  }

  void DependsOn(Cell* dependency) {
    std::cerr << __func__ << std::endl;
    dependencies_.push_back(dependency);
    dependency->dependents_.push_back(this);
  }

 protected:
  std::vector<Cell*> dependents_;
  std::vector<Ref<Cell>> dependencies_;
};

std::vector<Cell*>* g_captured_cells = nullptr;

template <typename T>
class TypedCell : public Cell {
 public:
  TypedCell(T value) : previous_value_(value), value_(std::move(value)) {}

  T Value() {
    std::cerr << __func__ << std::endl;
    if (g_captured_cells) {
      g_captured_cells->push_back(this);
    }
    return value_;
  }
  void Value(T value) {
    std::cerr << __func__ << std::endl;
    value_ = std::move(value);
    Invalidate();
  }

  void Invalidate() override {
    std::cerr << __func__ << std::endl;
    if (value_ == previous_value_) {
      return;
    }
    previous_value_ = value_;
    InvalidateDependents();
  }

 protected:
  T previous_value_;
  T value_;
};

template <typename T>
class ComputedTypedCell : public TypedCell<T> {
 public:
  ComputedTypedCell(std::function<T()> callback)
      : TypedCell<T>(callback()), callback_(std::move(callback)) {
    std::cerr << __func__ << std::endl;
    std::vector<Cell*> captured_cells;
    g_captured_cells = &captured_cells;
    callback_();
    for (auto& cell : captured_cells) {
      this->DependsOn(cell);
    }
    g_captured_cells = nullptr;
  }

  void Invalidate() override {
    std::cerr << __func__ << std::endl;
    this->value_ = callback_();
    if (this->value_ == this->previous_value_) {
      return;
    }
    this->previous_value_ = this->value_;
    this->InvalidateDependents();
  }

 private:
  std::function<T()> callback_;
};

class WatcherCell : public Cell {
 public:
  explicit WatcherCell(std::function<void()> callback)
      : callback_(std::move(callback)) {}

  void Invalidate() override { callback_(); }

 private:
  std::function<void()> callback_;
};

class WatcherCaptureCell : public WatcherCell {
 public:
  explicit WatcherCaptureCell(std::function<void()> callback)
      : WatcherCell(callback) {
    CaptureDependencies();
  }

  void Invalidate() override { callback_(); }

 private:
  void CaptureDependencies() {
    std::vector<Cell*> captured_cells_;
    g_captured_cells = &captured_cells_;
    callback_();
    for (auto* cell : captured_cells_) {
      this->DependsOn(cell);
    }
    g_captured_cells = nullptr;
  }

  std::function<void()> callback_;
};

// Operator in between TypedCell, when possible.
// +,-,*,/,%,&,|,^,<<,>>,==,!=,<,>,<=,>=,&&,||,!

#define RTXUI_OPERATOR(op)                                                     \
  template <typename A, typename B>                                            \
  Ref<TypedCell<decltype(std::declval<A>() op std::declval<B>())>>             \
  operator op(Ref<TypedCell<A>>& a, Ref<TypedCell<B>>& b) {                    \
    auto r = Ref<TypedCell<decltype(std::declval<A>() op std::declval<B>())>>( \
        a->Value() op b->Value());                                             \
    r->DependsOn(a.get());                                                     \
    r->DependsOn(b.get());                                                     \
    return r;                                                                  \
  }

RTXUI_OPERATOR(+)
RTXUI_OPERATOR(-)
RTXUI_OPERATOR(*)
RTXUI_OPERATOR(/)
RTXUI_OPERATOR(%)
RTXUI_OPERATOR(&)
RTXUI_OPERATOR(|)
RTXUI_OPERATOR(^)
RTXUI_OPERATOR(<<)
RTXUI_OPERATOR(>>)
RTXUI_OPERATOR(==)
RTXUI_OPERATOR(!=)
RTXUI_OPERATOR(<)
RTXUI_OPERATOR(>)
RTXUI_OPERATOR(<=)
RTXUI_OPERATOR(>=)
RTXUI_OPERATOR(&&)
RTXUI_OPERATOR(||)

}  // namespace rtxui
