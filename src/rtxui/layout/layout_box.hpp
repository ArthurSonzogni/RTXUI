#ifndef RTXUI_LAYOUT_LAYOUT_BOX_HPP
#define RTXUI_LAYOUT_LAYOUT_BOX_HPP

#include <memory>
#include <string>
#include <vector>

#include "rtxui/dom/element.hpp"
#include "rtxui/layout/style.hpp"

namespace rtxui {

class LayoutBox;

// The "Input Node" for algorithms. Wraps a LayoutBox.
struct LayoutInputNode {
  LayoutBox* box;
};

class LayoutBox {
 public:
  enum Algorithm {
    InlineFlow,
    BlockFlow,
    Flex,
    Text,
  };
  Algorithm algorithm;

  ComputedStyle style;
  Element* dom_node = nullptr;  // Link back to DOM.
  std::vector<std::shared_ptr<LayoutBox>> children;

  // Flags for the algorithm selection.
  bool is_anonymous = false;
  bool is_text = false;
  bool is_anonymous_ = false;
  std::string text_data;

  LayoutBox();

  std::string Print(int indent = 0) const;
};

}  // namespace rtxui
#endif  // RTXUI_LAYOUT_LAYOUT_BOX_HPP
