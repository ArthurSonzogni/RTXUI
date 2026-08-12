#ifndef RTXUI_LAYOUT_LAYOUT_HPP
#define RTXUI_LAYOUT_LAYOUT_HPP

#include <memory>

#include "rtxui/layout/layout_box.hpp"
#include "rtxui/layout/physical_fragment.hpp"
#include "rtxui/layout/style.hpp"

namespace rtxui {
struct LayoutContext {
  // Nearest positioned ancestor dimensions:
  int npa_w = 80;
  int npa_h = 24;
  // Accumulated offsets from the nearest positioned ancestor to the current
  // container's content box origin:
  int npa_offset_x = 0;
  int npa_offset_y = 0;
  int viewport_offset_x = 0;
  int viewport_offset_y = 0;

  // Viewport/Screen dimensions (for fixed positioning):
  int viewport_w = 80;
  int viewport_h = 24;

  bool is_measurement = false;
};

std::shared_ptr<PhysicalFragment> RunLayout(LayoutInputNode node,
                                            LayoutConstraints constraints,
                                            LayoutContext context = {});

void ResetLayoutArena();

// Number of layout algorithm executions since ResetLayoutRunCount(); calls
// served from the measurement cache are not counted. Unlike wall-clock time
// this is deterministic, which is what makes it worth asserting on: it pins
// how layout cost scales with the shape of the tree.
int LayoutRunCount();
void ResetLayoutRunCount();
}  // namespace rtxui

#endif  // RTXUI_LAYOUT_LAYOUT_HPP
