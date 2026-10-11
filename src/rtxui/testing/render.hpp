// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
//
// Helpers for tests that check what a component paints, cell by cell. To
// check only the text, RenderToString (rtxui/headless.hpp) is simpler.
#ifndef RTXUI_TESTING_RENDER_HPP_
#define RTXUI_TESTING_RENDER_HPP_

#include <string>
#include <string_view>

#include "rtxui/component.hpp"
#include "rtxui/paint/texture.hpp"
#include "rtxui/refcounted.hpp"

namespace rtxui {

// Mounts `component`, lays it out at `width` x `height`, and paints it.
Texture RenderComponent(Ref<ComponentBase> component, int width, int height);

// Lays out and paints a component already mounted, as it is now.
Texture PaintComponent(const ComponentBase& component, int width, int height);

// Row `y` of `texture` as text, without its trailing spaces.
std::string RowText(const Texture& texture, int y);

// The first row of `texture` containing `text`, or -1.
int FindRow(const Texture& texture, std::string_view text);

}  // namespace rtxui

#endif  // RTXUI_TESTING_RENDER_HPP_
