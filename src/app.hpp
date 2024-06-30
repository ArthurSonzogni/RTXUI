#ifndef APP_HPP_
#define APP_HPP_

#include <functional>
#include <map>
#include <string_view>
#include "element.hpp"
#include "paint/texture.hpp"

// An App is a collection of components.
class App {
 public:
  App() = default;

  // Registers a component with the given label.
  void Register(const std::string_view label, std::function<void(Element&)> f);

  void Render(const std::string_view label, Texture& texture);

 private:
  std::map<std::string, Element> elements_;
};

#endif  // APP_HPP_
