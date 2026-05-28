#ifndef RTXUI_STYLE_APPLY_STYLE_HPP
#define RTXUI_STYLE_APPLY_STYLE_HPP

#include "rtxui/layout/style.hpp"
#include "rtxui/style/style.hpp"

namespace rtxui {
void ApplyStyle(ComputedStyle& style, const css::Declaration& declaration);
}  // namespace rtxui

#endif
