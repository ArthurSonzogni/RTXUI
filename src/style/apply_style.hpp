#ifndef RTXUI_STYLE_APPLY_STYLE_HPP
#define RTXUI_STYLE_APPLY_STYLE_HPP

#include "layout/style.hpp"
#include "style/style.hpp"

namespace rtxui {
void ApplyStyle(ComputedStyle& style, const css::Declaration& declaration);
}  // namespace rtxui

#endif
