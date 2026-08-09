// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#ifndef RTXUI_LAYOUT_STICKY_HPP
#define RTXUI_LAYOUT_STICKY_HPP

#include <algorithm>

#include "rtxui/layout/style.hpp"

namespace rtxui {

/// Everything `position: sticky` needs to know about the box it sticks inside.
///
/// The offset has to be recomputed wherever a fragment tree is walked with
/// scrolling applied -- painting, hit testing, scroll-into-view -- and it used
/// to be copy-pasted at five such places. Any change (including finishing
/// `bottom`/`right`) therefore had to be made five times, which is why it never
/// was. One definition, five callers.
struct StickyContext {
  /// True when the containing fragment is itself the scroll container. The
  /// sticky child then pins inside that fragment's own content box; otherwise
  /// the viewport comes from an ancestor and the child must additionally stay
  /// within the containing fragment's box.
  bool parent_clips = false;

  /// Content-box origin of the scrolling viewport the child pins against.
  int viewport_x = 0;
  int viewport_y = 0;

  /// The containing fragment's absolute origin and size.
  int parent_x = 0;
  int parent_y = 0;
  int parent_width = 0;
  int parent_height = 0;

  /// The containing fragment's border and padding, used to keep the child
  /// inside its box. Only the trailing edges are read today; the leading ones
  /// are what `bottom`/`right` support will need, and carrying them keeps the
  /// context complete at every call site rather than in the helper's caller.
  int parent_border_left = 0;
  int parent_border_top = 0;
  int parent_border_right = 0;
  int parent_border_bottom = 0;
  int parent_padding_left = 0;
  int parent_padding_top = 0;
  int parent_padding_right = 0;
  int parent_padding_bottom = 0;
};

/// Adjusts a sticky child's absolute position in place.
///
/// `style` is the child's computed style, `child_width`/`child_height` its
/// fragment size. Only `top` and `left` are honoured; `bottom` and `right` are
/// accepted by the parser but have no effect yet (see the Positioning guide).
inline void ApplyStickyOffset(const ComputedStyle& style,
                              int child_width,
                              int child_height,
                              const StickyContext& context,
                              int& child_x,
                              int& child_y) {
  if (style.top.unit != Unit::Auto) {
    const int min_y = context.viewport_y + style.top.Resolve(0);
    child_y = std::max(child_y, min_y);

    if (!context.parent_clips) {
      const int parent_content_bottom =
          context.parent_y + context.parent_height - context.parent_border_bottom -
          context.parent_padding_bottom;
      child_y = std::min(child_y, parent_content_bottom - child_height);
    }
  }

  if (style.left.unit != Unit::Auto) {
    const int min_x = context.viewport_x + style.left.Resolve(0);
    child_x = std::max(child_x, min_x);

    if (!context.parent_clips) {
      const int parent_content_right =
          context.parent_x + context.parent_width - context.parent_border_right -
          context.parent_padding_right;
      child_x = std::min(child_x, parent_content_right - child_width);
    }
  }
}

}  // namespace rtxui

#endif  // RTXUI_LAYOUT_STICKY_HPP
