// Copyright 2024 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#ifndef RTXUI_COMPONENT_DEFAULT_TEXTAREA_TEXTAREA_HPP_
#define RTXUI_COMPONENT_DEFAULT_TEXTAREA_TEXTAREA_HPP_

#include <string>
#include <string_view>
#include <vector>

#include "rtxui/component/default/input/text_input_base.hpp"
#include "rtxui/internal/component.hpp"

namespace rtxui {

// One rendered row of the line-number gutter (see textarea::UpdateGutter).
// `color` is one of textarea::gutter_color/gutter_active_color/
// gutter_wrapped_color, picked per row and applied via inline style (not a
// CSS class) so an app can override it just by setting the corresponding
// attribute on <textarea> -- default components don't use CSS custom
// properties, and outer stylesheets can't reach elements generated inside
// another component's own template anyway.
struct GutterLine {
  std::string text;
  std::string css_class;
  std::string color;

  bool operator==(const GutterLine&) const = default;
};

// One rendered row of the current-line background highlight (see
// textarea::UpdateGutter); a full-width, textless block so it paints as a
// background band behind that row's actual text. `color` is
// textarea::current_line_color on the active row, "transparent" elsewhere
// (see GutterLine::color for why this is inline style, not a CSS class).
struct LineRow {
  std::string css_class;
  std::string color;

  bool operator==(const LineRow&) const = default;
};

class textarea : public Component<textarea>, public TextInputBase {
 public:
  // "" (off), "true"/"absolute" (1, 2, 3, ...), or "relative" (vim-style:
  // distance from the active line, which shows its absolute number).
  std::string linenumbers;
  // The displayed number of the first logical line (an offset, e.g. to
  // show a textarea as an excerpt starting at line 42 of a larger file).
  int line_start = 1;
  // The last displayed number to show a gutter entry for; -1 (default)
  // means unbounded. Lines beyond it still render, just with a blank
  // gutter cell.
  int line_end = -1;
  // "" (default) or "ignore": one gutter entry per logical line; if a
  // line word-wraps, the gutter numbers stay logical-line-accurate but
  // may drift out of row alignment with the wrapped continuation text.
  // "subline": estimates wrapped row counts (see UpdateGutter) so
  // continuation rows get their own (blank, `.wrapped`-classed) gutter
  // entry instead.
  std::string line_wrap;
  // Background band behind the logical line the cursor is on. Independent
  // of `linenumbers`: works with or without a gutter.
  bool highlight_current_line = false;
  // Gutter text color for a normal, non-active, non-wrapped-continuation
  // line.
  std::string gutter_color = "rgb(120, 120, 120)";
  // Gutter text color for the active line's number.
  std::string gutter_active_color = "rgb(230, 230, 230)";
  // Gutter text color for a wrapped-continuation entry (line_wrap="subline").
  std::string gutter_wrapped_color = "rgb(90, 90, 90)";
  // Background color of the highlight_current_line band.
  std::string current_line_color = "rgb(60, 60, 60)";

  // Render bindings
  bool show_gutter = false;
  int gutter_width = 3;
  std::vector<GutterLine> gutter_lines;
  std::vector<LineRow> content_line_highlights;

  void InitReflection() override;
  std::string_view Setup() override;
  bool OnEvent(Event event) override;
  bool Digest() override;

 private:
  void UpdateGutter();
};

}  // namespace rtxui

#endif  // RTXUI_COMPONENT_DEFAULT_TEXTAREA_TEXTAREA_HPP_
