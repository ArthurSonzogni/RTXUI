// Copyright 2024 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#ifndef ELEMENT_HPP_
#define ELEMENT_HPP_

#include <functional>
#include <string_view>

#include "core/refcounted.hpp"
#include "reactive/reactive.hpp"

namespace rtxui {

class Element : public RefCounted {
 public:
  void Attribute(const std::string_view label, reactive::Reactive value);
  void Event(const std::string_view label);
  void Model(const std::string_view label);
  void Ref(const std::string_view label, reactive::Reactive value);
  void Computed(const std::string_view label,
                std::function<reactive::Reactive(reactive::Reactive&)>);
  void Template(const std::string_view);
  void Style(const std::string_view);

 private:
};

}  // namespace rtxui

#endif  // ELEMENT_HPP_
