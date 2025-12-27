#ifndef RTXUI_LAYOUT_LAYOUT_BOX_HPP
#define RTXUI_LAYOUT_LAYOUT_BOX_HPP

#include <memory>
#include <string>
#include <vector>
#include "dom/element.hpp"
#include "layout/style.hpp"

namespace rtxui {

class LayoutBox;

// The "Input Node" for algorithms. Wraps a LayoutBox.
struct LayoutInputNode {
  LayoutBox* box;
};

class LayoutBox {
 public:
  std::string debug_name;
  ComputedStyle style;
  Element* dom_node = nullptr;  // Link back to DOM.
  std::vector<std::shared_ptr<LayoutBox>> children;

  // Flags for the algorithm selection.
  bool is_anonymous = false;
  bool is_text = false;
  std::string text_data;

  LayoutBox(const std::string& name);

  std::string Print(int indent = 0) const;

  // Helper to determine which algorithm to run
  bool IsInlineFormattingContext() const {
    if (children.empty()) {
      return false;
    }
    return children[0]->style.IsInlineLevel();
  }
};

}  // namespace rtxui
#endif  // RTXUI_LAYOUT_LAYOUT_BOX_HPP
