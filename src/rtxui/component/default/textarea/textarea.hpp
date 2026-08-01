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
struct GutterLine {
  std::string text;
  std::string css_class;

  bool operator==(const GutterLine&) const = default;
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

  // Render bindings
  bool show_gutter = false;
  int gutter_width = 3;
  std::vector<GutterLine> gutter_lines;

  void InitReflection() override;
  std::string_view Setup() override;
  bool OnEvent(Event event) override;
  bool Digest() override;

 private:
  void UpdateGutter();
};

}  // namespace rtxui

#endif  // RTXUI_COMPONENT_DEFAULT_TEXTAREA_TEXTAREA_HPP_
