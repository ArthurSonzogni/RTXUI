#ifndef RTXUI_LAYOUT_LAYOUT_TREE_BUILDER_HPP
#define RTXUI_LAYOUT_LAYOUT_TREE_BUILDER_HPP

#include <memory>
#include <vector>

#include "rtxui/dom/element.hpp"
#include "rtxui/dom/text_element.hpp"
#include "rtxui/layout/layout_box.hpp"

namespace rtxui {

// Text-related style properties that inherit down the DOM tree. Build()
// resolves each dom_node's own (possibly unset) values against this,
// producing the InheritedTextStyle passed to its children.
struct InheritedTextStyle {
  TextAlign align = TextAlign::Left;
  WhiteSpace white_space = WhiteSpace::Normal;
  TextTransform text_transform = TextTransform::None;
  std::optional<Color> fg;
  std::optional<bool> bold;
  std::optional<bool> dim;
  std::optional<bool> italic;
  std::optional<bool> underlined;
  std::optional<bool> underlined_double;
  std::optional<bool> strikethrough;
  std::optional<bool> overlined;
  std::optional<bool> blink;
  int letter_spacing = 0;
  // 8 is the tab stop every terminal and CSS itself default to.
  int tab_size = 8;
  int line_height = 1;
  OverflowWrap overflow_wrap = OverflowWrap::Anywhere;
  WordBreak word_break = WordBreak::Normal;
};

class LayoutTreeBuilder {
 public:
  static std::shared_ptr<LayoutBox> Build(
      Element* dom_node,
      InheritedTextStyle parent = {});
};
}  // namespace rtxui
#endif  // RTXUI_LAYOUT_LAYOUT_TREE_BUILDER_HPP
