// Copyright 2024 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#pragma once

#include <map>
#include <string>
#include <string_view>

#include "cell/cell.hpp"

namespace rtxui {

class Component {
 public:
  virtual ~Component() = default;
  virtual std::string Setup() = 0;

  // Bind a name in the template to a cell.
  void Bind(std::string_view name, Ref<Cell> value);

  template <typename T>
  auto State(T&& value) {
    return Ref<TypedCell<T>>(std::forward<T>(value));
  }

  template <typename T>
  auto Computed(std::function<T()> f) {
    return Ref<ComputedTypedCell<T>>(std::move(f));
  }

  auto Watch(std::function<void()> f) {
    return Ref<WatcherCaptureCell>(std::move(f));
  }

  auto Watch(Ref<Cell> cell, std::function<void()> f) {
    auto out = Ref<WatcherCell>(std::move(f));
    out->DependsOn(cell.get());
    return out;
  }

 private:
  std::map<std::string, Ref<Cell>> bindings_;
};

}  // namespace rtxui

#define RTXUI(name)                               \
  struct RTXUI_##name : public rtxui::Component { \
    std::string Setup() final;                    \
  };                                              \
  namespace {                                     \
  rtxui::Register<RTXUI_##name> reg(#name);       \
  }                                               \
  std::string RTXUI_##name::Setup()

#define BIND(x) Bind(#x, x)
