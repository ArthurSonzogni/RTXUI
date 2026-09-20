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

  /// Content-box origin and size of the scrolling viewport the child pins
  /// against. The size is what `bottom`/`right` measure back from.
  int viewport_x = 0;
  int viewport_y = 0;
  int viewport_width = 0;
  int viewport_height = 0;

  /// The containing fragment's absolute origin and size.
  int parent_x = 0;
  int parent_y = 0;
  int parent_width = 0;
  int parent_height = 0;

  /// The containing fragment's border and padding, used to keep the child
  /// inside its box: the trailing edges bound a `top`/`left` pin, the leading
  /// ones bound a `bottom`/`right` pin.
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
/// fragment size. All four offsets are honoured. When both edges of an axis
/// are set the leading one is applied last and therefore wins, matching how a
/// browser resolves an over-constrained sticky box.
inline void ApplyStickyOffset(const ComputedStyle& style,
                              int child_width,
                              int child_height,
                              const StickyContext& context,
                              int& child_x,
                              int& child_y) {
  if (style.bottom.unit != Unit::Auto) {
    // Pin when the box would otherwise sit below the viewport's bottom edge.
    const int viewport_bottom = context.viewport_y + context.viewport_height;
    const int max_y = viewport_bottom - style.bottom.Resolve(0) - child_height;
    child_y = std::min(child_y, max_y);

    if (!context.parent_clips) {
      const int parent_content_top = context.parent_y +
                                     context.parent_border_top +
                                     context.parent_padding_top;
      child_y = std::max(child_y, parent_content_top);
    }
  }

  if (style.top.unit != Unit::Auto) {
    const int min_y = context.viewport_y + style.top.Resolve(0);
    child_y = std::max(child_y, min_y);

    if (!context.parent_clips) {
      const int parent_content_bottom =
          context.parent_y + context.parent_height -
          context.parent_border_bottom - context.parent_padding_bottom;
      child_y = std::min(child_y, parent_content_bottom - child_height);
    }
  }

  if (style.right.unit != Unit::Auto) {
    const int viewport_right = context.viewport_x + context.viewport_width;
    const int max_x = viewport_right - style.right.Resolve(0) - child_width;
    child_x = std::min(child_x, max_x);

    if (!context.parent_clips) {
      const int parent_content_left = context.parent_x +
                                      context.parent_border_left +
                                      context.parent_padding_left;
      child_x = std::max(child_x, parent_content_left);
    }
  }

  if (style.left.unit != Unit::Auto) {
    const int min_x = context.viewport_x + style.left.Resolve(0);
    child_x = std::max(child_x, min_x);

    if (!context.parent_clips) {
      const int parent_content_right = context.parent_x + context.parent_width -
                                       context.parent_border_right -
                                       context.parent_padding_right;
      child_x = std::min(child_x, parent_content_right - child_width);
    }
  }
}

}  // namespace rtxui

#endif  // RTXUI_LAYOUT_STICKY_HPP
