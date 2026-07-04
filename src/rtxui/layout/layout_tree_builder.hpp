#ifndef RTXUI_LAYOUT_LAYOUT_TREE_BUILDER_HPP
#define RTXUI_LAYOUT_LAYOUT_TREE_BUILDER_HPP

#include <memory>
#include <vector>

#include "rtxui/dom/element.hpp"
#include "rtxui/dom/text_element.hpp"
#include "rtxui/layout/layout_box.hpp"

namespace rtxui {
class LayoutTreeBuilder {
 public:
  static std::shared_ptr<LayoutBox> Build(
      Element* dom_node,
      TextAlign parent_align = TextAlign::Left,
      WhiteSpace parent_ws = WhiteSpace::Normal,
      TextTransform parent_text_transform = TextTransform::None,
      std::optional<Color> parent_fg = std::nullopt,
      std::optional<bool> parent_bold = std::nullopt,
      std::optional<bool> parent_italic = std::nullopt,
      std::optional<bool> parent_underlined = std::nullopt,
      std::optional<bool> parent_underlined_double = std::nullopt,
      std::optional<bool> parent_strikethrough = std::nullopt,
      std::optional<bool> parent_blink = std::nullopt);
};
}  // namespace rtxui
#endif  // RTXUI_LAYOUT_LAYOUT_TREE_BUILDER_HPP
