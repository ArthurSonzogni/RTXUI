// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include "rtxui/component/default/canvas/canvas.hpp"

#include <algorithm>
#include <memory>
#include <string_view>

#include "rtxui/bind.hpp"
#include "rtxui/component.hpp"
#include "rtxui/component/component_internal.hpp"
#include "rtxui/dom/element.hpp"
#include "rtxui/paint/canvas_surface.hpp"

namespace rtxui {

canvas::canvas() : surface_(std::make_unique<CanvasSurface>()) {
  Bind(width);
  Bind(height);
}

canvas::~canvas() {
  if (Element* root = ComponentInternals::Root(*this);
      root && root->canvas_surface() == surface_.get()) {
    root->set_canvas_surface(nullptr);
  }
}

std::string_view canvas::Setup() {
  return R"html(<style>
      self {
        display: inline-block;
        user-select: none;
      }
    </style>)html";
}

bool canvas::Digest() {
  const bool changed = Component<canvas>::Digest();
  Element* root = ComponentInternals::Root(*this);
  if (!root) {
    return changed;
  }
  const int w = std::max(0, width);
  const int h = std::max(0, height);
  if (root->canvas_surface() != surface_.get() ||
      surface_->intrinsic_width != w || surface_->intrinsic_height != h) {
    surface_->intrinsic_width = w;
    surface_->intrinsic_height = h;
    root->set_canvas_surface(surface_.get());
    root->InvalidateLayout();
    return true;
  }
  return changed;
}

}  // namespace rtxui
