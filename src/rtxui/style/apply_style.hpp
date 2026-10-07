#ifndef RTXUI_STYLE_APPLY_STYLE_HPP
#define RTXUI_STYLE_APPLY_STYLE_HPP

#include <functional>
#include <optional>
#include <string>
#include <string_view>

#include "rtxui/layout/style.hpp"
#include "rtxui/style/style.hpp"

namespace rtxui {
void ApplyStyle(ComputedStyle& style, const css::Declaration& declaration);

/// What a `content` declaration generates: its strings and attr() values,
/// concatenated. `text` is empty for `none` and `normal`, which generate no
/// box. `valid` is false for anything else that is not understood.
struct ContentValue {
  bool valid = false;
  std::optional<std::string> text;
};
ContentValue ParseContent(
    std::string_view value,
    const std::function<const std::string*(std::string_view)>& attribute);
}  // namespace rtxui

#endif
