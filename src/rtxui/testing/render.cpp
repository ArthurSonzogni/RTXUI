// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include "rtxui/testing/render.hpp"

#include <string>
#include <string_view>

#include "rtxui/component.hpp"
#include "rtxui/layout/layout.hpp"
#include "rtxui/layout/layout_tree_builder.hpp"
#include "rtxui/layout/style.hpp"
#include "rtxui/paint/paint.hpp"
#include "rtxui/paint/texture.hpp"
#include "rtxui/refcounted.hpp"

namespace rtxui {

Texture RenderComponent(Ref<ComponentBase> component, int width, int height) {
  component->Mount();
  return PaintComponent(*component, width, height);
}

Texture PaintComponent(const ComponentBase& component, int width, int height) {
  auto layout_box = LayoutTreeBuilder::Build(component.Root());
  Texture texture(width, height);
  if (layout_box) {
    LayoutConstraints constraints;
    constraints.width = {width, MeasureMode::Exactly};
    constraints.height = {height, MeasureMode::Exactly};
    auto fragment = RunLayout({layout_box.get()}, constraints);
    Paint(fragment, texture);
  }
  return texture;
}

std::string RowText(const Texture& texture, int y) {
  std::string row;
  for (int x = 0; x < texture.width(); ++x) {
    row += texture[x, y].character;
  }
  while (!row.empty() && row.back() == ' ') {
    row.pop_back();
  }
  return row;
}

int FindRow(const Texture& texture, std::string_view text) {
  for (int y = 0; y < texture.height(); ++y) {
    if (RowText(texture, y).find(text) != std::string::npos) {
      return y;
    }
  }
  return -1;
}

}  // namespace rtxui
