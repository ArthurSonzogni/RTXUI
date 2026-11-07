// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#ifndef RTXUI_COMPONENT_DEFAULT_CANVAS_HPP_
#define RTXUI_COMPONENT_DEFAULT_CANVAS_HPP_

#include <memory>
#include <string_view>

#include "rtxui/component.hpp"

namespace rtxui {

struct CanvasSurface;

// <canvas ondraw="Draw" onpointer="OnPointer">: the application draws its
// cells (rtxui::Canvas). The screen calls `ondraw` once laid out, when the
// size or the owner's state changed, and routes the mouse to `onpointer`.
class canvas : public Component<canvas> {
 public:
  // Its size when CSS sets none, in cells.
  int width = 30;
  int height = 8;

  canvas();
  ~canvas() override;
  std::string_view Setup();
  bool Digest() override;

 private:
  std::unique_ptr<CanvasSurface> surface_;
};

}  // namespace rtxui

#endif  // RTXUI_COMPONENT_DEFAULT_CANVAS_HPP_
