// Copyright 2024 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include "app.hpp"

void App::Register(const std::string_view label,  //
                   std::function<void(Element&)> f) {
  f(elements_[std::string(label)]);
}

void App::Render(const std::string_view label, Texture& texture) {}
