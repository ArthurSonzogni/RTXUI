#ifndef RTXUI_LAYOUT_LAYOUT_BOX_HPP
#define RTXUI_LAYOUT_LAYOUT_BOX_HPP

#include <memory>
#include <string>
#include <vector>

#include "rtxui/dom/element.hpp"
#include "rtxui/layout/physical_fragment.hpp"
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
    Table,
    Grid,
  };
  Algorithm algorithm;

  ComputedStyle style;
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

  std::string Print(int indent = 0) const;
};

}  // namespace rtxui
#endif  // RTXUI_LAYOUT_LAYOUT_BOX_HPP
