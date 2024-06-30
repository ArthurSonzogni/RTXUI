#include "app.hpp"

void App::Register(const std::string_view label,  //
                   std::function<void(Element&)> f) {
  f(elements_[std::string(label)]);
}

void App::Render(const std::string_view label, Texture& texture) {}
