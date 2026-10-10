#ifndef RTXUI_LAYOUT_LAYOUT_BOX_HPP
#define RTXUI_LAYOUT_LAYOUT_BOX_HPP

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "rtxui/dom/element.hpp"
#include "rtxui/layout/physical_fragment.hpp"
#include "rtxui/layout/style.hpp"

namespace rtxui {

class LayoutBox;

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
    Table,
    Grid,
  };
  Algorithm algorithm;

  // The style the box is laid out with: its element's computed style, read in
  // place rather than copied -- a box tree is built for every layout, and
  // copying each element's whole style into its box was the largest single
  // cost of building it. The inherited text properties are the exception:
  // they are resolved against the parent's into `text` below, and are read
  // from there, never from here.
  const ComputedStyle& style() const { return *style_; }
  void set_style(const ComputedStyle& style) { style_ = &style; }
  // The style, for the few boxes layout changes: the box takes a copy of its
  // own first, so the element's style is never touched.
  ComputedStyle& mutable_style();

  // The inherited text properties, resolved.
  InheritedTextStyle text;

  Element* dom_node = nullptr;  // Link back to DOM.
  std::vector<std::shared_ptr<LayoutBox>> children;

  // Flags for the algorithm selection.
  bool is_anonymous = false;
  bool is_text = false;
  std::string text_data;

  // True when this box, or anything below it, is absolutely or fixed
  // positioned. Everything in LayoutContext other than `is_measurement` is
  // read only by out-of-flow positioning, so a subtree without any is a pure
  // function of its constraints -- which is what makes `measure_cache` sound.
  bool has_out_of_flow = false;

  // Fragments produced by measurement passes, keyed by the constraints that
  // produced them. Flex and grid measure a child before laying it out for
  // real, and each of those measurements re-measures its own children, so the
  // same (box, constraints) pair is asked for many times per frame -- once
  // per path through the ancestors' passes. The box tree is rebuilt every
  // frame, so the cache's lifetime is the frame, and no cross-frame
  // invalidation is needed.
  //
  // Only measurement passes read or write it: a final pass writes layout
  // results back onto the DOM (ApplyScrollExtent), and those writes have to
  // actually happen.
  struct MeasureCacheEntry {
    LayoutConstraints constraints;
    std::shared_ptr<PhysicalFragment> fragment;
  };
  std::vector<MeasureCacheEntry, LayoutArenaAllocator<MeasureCacheEntry>>
      measure_cache;

  LayoutBox();
  LayoutBox(const LayoutBox&) = delete;
  LayoutBox& operator=(const LayoutBox&) = delete;

  std::string Print(int indent = 0) const;

 private:
  const ComputedStyle* style_;
  std::optional<ComputedStyle> owned_style_;
};

}  // namespace rtxui
#endif  // RTXUI_LAYOUT_LAYOUT_BOX_HPP
