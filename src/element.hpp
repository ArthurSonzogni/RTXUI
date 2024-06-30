#ifndef ELEMENT_HPP_
#define ELEMENT_HPP_

#include <functional>
#include <string_view>
#include "reactive/reactive.hpp"

class Element {
 public:
  void Attribute(const std::string_view label, reactive::Reactive value);
  void Event(const std::string_view label);
  void Model(const std::string_view label);
  void Ref(const std::string_view label, reactive::Reactive value);
  void Computed(const std::string_view label,
                std::function<reactive::Reactive(reactive::Reactive&)>);
  void Dom(const std::string_view);
  void Style(const std::string_view);

 private:
};

#endif  // ELEMENT_HPP_
