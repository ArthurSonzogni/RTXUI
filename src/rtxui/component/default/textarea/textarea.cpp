// Copyright 2024 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include "rtxui/component/default/textarea/textarea.hpp"

#include <algorithm>
#include <cmath>
#include <map>

#include "rtxui/base/string.hpp"
#include "rtxui/dom/element.hpp"

namespace rtxui {

namespace {

std::vector<Grapheme> GetGraphemesList(std::string_view val) {
  std::vector<Grapheme> res;
  for (const auto& g : Graphemes(val)) {
    res.push_back(g);
  }
  return res;
}

bool IsLineBreak(const Grapheme& g) {
  return g.text == "\n" || g.text == "\r\n" || g.text == "\r";
}

std::string PadLeft(std::string s, int width) {
  int len = static_cast<int>(s.size());
  if (len >= width) {
    return s;
  }
  return std::string(width - len, ' ') + s;
}

}  // namespace

void textarea::InitReflection() {
  Bind(value);
  Bind(disabled);
  Bind(readonly);
  Bind(select_on_focus);
  Bind(placeholder);
  Bind(placeholder_text);
  Bind(maxlength);
  Bind(linenumbers);
  Bind(line_start);
  Bind(line_end);
  Bind(line_wrap);
  Bind(show_gutter);
  Bind(gutter_width);
  Bind(highlight_current_line);
  BindCollection("gutter_lines", &gutter_lines, [](const GutterLine& line) {
    return std::make_shared<ManualStructVisitor>(
        std::map<std::string, std::string, std::less<>>{
            {"text", line.text},
            {"css_class", line.css_class},
        });
  });
  BindCollection("content_line_highlights", &content_line_highlights,
                 [](const LineRow& row) {
                   return std::make_shared<ManualStructVisitor>(
                       std::map<std::string, std::string, std::less<>>{
                           {"css_class", row.css_class},
                       });
                 });
  Bind(selection_start);
  Bind(left_text);
  Bind(cursor_char);
  Bind(right_text);
  Bind(cursor_class);
  Bind(left_unselected);
  Bind(left_selected);
  Bind(right_selected);
  Bind(right_unselected);
  Bind(selection_class_left);
  Bind(selection_class_right);
  Component<textarea>::InitReflection();
}

std::string_view textarea::Setup() {
  return R"html(
    <if condition="{show_gutter}">
      <div class="gutter" part="gutter">
        <for each="{gutter_lines}" as="line">
          <div class="{line.css_class}" part="{line.css_class}">{line.text}</div>
        </for>
      </div>
    </if>
    <div class="content-wrapper">
      <div class="line-highlights" if="{highlight_current_line}">
        <for each="{content_line_highlights}" as="row">
          <div class="{row.css_class}" part="{row.css_class}"></div>
        </for>
      </div>
      <div class="content">
        <span>{left_unselected}</span><span class="{selection_class_left}" part="{selection_class_left}">{left_selected}</span><span class="{cursor_class}" part="{cursor_class}">{cursor_char}</span><span class="{selection_class_right}" part="{selection_class_right}">{right_selected}</span><span>{right_unselected}</span><span class="placeholder" part="placeholder">{placeholder_text}</span>
      </div>
    </div>

    <style>
      self {
        display: block;
        width: 40;
        height: 5;
        white-space: pre-wrap;
        padding-left: 1;
        padding-right: 1;
        overflow-y: scroll;
        background-color: rgb(40, 40, 40);
      }
      /* Only matches once the app sets a `linenumbers` attribute, so a
         plain textarea's layout/CSS is completely untouched. */
      self[linenumbers] {
        display: flex;
        flex-direction: row;
        align-items: flex-start;
        padding-left: 0;
      }
      /* Default colors; unlike .selection/.placeholder below, an app has a
         legitimate reason to retheme these to match its own palette --
         every element here also carries a matching part="..." attribute
         (see the template above), so an app can override via e.g.
         `textarea::part(gutter)` / `::part(active)` CSS instead. */
      .gutter {
        display: block;
        flex-shrink: 0;
        text-align: right;
        padding-left: 1;
        padding-right: 1;
        color: rgb(120, 120, 120);
      }
      .line-number {
        display: block;
      }
      .line-number.active {
        color: rgb(230, 230, 230);
      }
      .line-number.wrapped {
        color: rgb(90, 90, 90);
      }
      .content-wrapper {
        display: block;
        position: relative;
        flex-grow: 1;
        width: 100%;
      }
      /* Painted behind .content (negative z-index, below .content's default
         of 0) as a column of full-width, textless row divs; the active one
         gets a background-color, the rest stay transparent. */
      .line-highlights {
        position: absolute;
        top: 0;
        left: 0;
        width: 100%;
        z-index: -1;
      }
      .line {
        display: block;
        height: 1;
        width: 100%;
      }
      /* Fixed absolute color, not lighten(), for the same compounding
         reason as self:hover/self:focus above; overridable via
         `textarea::part(current-line)` CSS, see the .gutter comment above. */
      .current-line {
        background-color: rgb(60, 60, 60);
      }
      .content {
        display: block;
        flex-grow: 1;
        width: 100%;
        white-space: pre-wrap;
      }
      /* Transparent by design, not overridable via ::part(cursor): the
         focused cursor cell is drawn by the terminal's own native cursor
         (a blinking vertical bar), not simulated here, so there's no
         background of ours to show through it either way -- ::part(cursor)
         still lets an app retheme the cursor cell's foreground text
         color, just not this property. */
      .cursor {
        background-color: transparent;
      }
      .cursor-focused {
        background-color: transparent;
      }
      /* An absolute color pair (not derived from self's background) so the
         selection highlight stays readable no matter what color an app
         gives the textarea: self's background-color is a separate element,
         so it can't be referenced here (background-color doesn't inherit,
         and nested-component style resolution finishes before an outer
         app's overrides are known anyway). Overridable via ::part(selection)
         CSS, same reasoning as the gutter colors above. */
      .selection {
        background-color: rgb(38, 79, 120);
        color: white;
      }
      /* Fixed absolute color, not lighten()/dim, for the same reason the
         selection colors are fixed above; overridable via ::part(placeholder)
         CSS. */
      .placeholder {
        color: rgb(150, 150, 150);
      }
      /* Listed after :hover/:focus so it wins the cascade for a
         disabled-and-hovered field (mouse hover isn't gated on
         interactivity, unlike focus, which OnEventShared/DigestShared
         never let a disabled field acquire). */
      self:disabled {
        background-color: rgb(40, 40, 40);
        opacity: 0.4;
      }
    </style>
  )html";
}

bool textarea::OnEvent(Event event) {
  return OnEventShared(this, event, true);
}

bool textarea::Digest() {
  DigestShared(this);
  UpdateGutter();
  bool changed = Component<textarea>::Digest();
  if (changed) {
    KeepCursorVisible(Root(), true);
  }
  return changed;
}

// Recomputes `show_gutter`/`gutter_width`/`gutter_lines` (from
// `linenumbers`, `line_start`, `line_end`, `line_wrap`) and
// `content_line_highlights` (from `highlight_current_line`), matching
// visual rendered rows computed via ComputeRowStarts.
void textarea::UpdateGutter() {
  show_gutter = !linenumbers.empty();
  gutter_lines.clear();
  content_line_highlights.clear();
  if (!show_gutter && !highlight_current_line) {
    return;
  }

  bool relative = (linenumbers == "relative");
  auto graphemes = GetGraphemesList(value);
  int n = static_cast<int>(graphemes.size());

  // Count logical lines and identify which logical line the cursor sits on.
  std::vector<int> logical_line_starts = {0};
  int active_logical_line = 0;
  for (int i = 0; i < n; ++i) {
    if (i == cursor_pos) {
      active_logical_line = static_cast<int>(logical_line_starts.size()) - 1;
    }
    if (IsLineBreak(graphemes[i])) {
      logical_line_starts.push_back(i + 1);
    }
  }
  if (cursor_pos == n) {
    active_logical_line = static_cast<int>(logical_line_starts.size()) - 1;
  }
  int num_logical_lines = static_cast<int>(logical_line_starts.size());

  std::vector<std::string> labels;
  if (show_gutter) {
    labels.resize(num_logical_lines);
    for (int i = 0; i < num_logical_lines; ++i) {
      int absolute_number = line_start + i;
      if (line_end != -1 && absolute_number > line_end) {
        continue;  // Beyond line_end: blank gutter cell.
      }
      int displayed = (relative && i != active_logical_line)
                          ? std::abs(i - active_logical_line)
                          : absolute_number;
      labels[i] = std::to_string(displayed);
    }

    gutter_width = 1;
    for (const auto& label : labels) {
      gutter_width =
          std::max<int>(gutter_width, static_cast<int>(label.size()));
    }
  }

  // Determine content width for visual row wrapping calculation.
  int content_width = 0;
  if (Element* root = Root()) {
    if (Element* content_el = root->QuerySelector(".content")) {
      content_width = content_el->layout_width();
    }
    if (content_width <= 0 && root->layout_width() > 0) {
      int border_offset =
          (root->style.border_style != BorderStyle::None) ? 1 : 0;
      content_width = root->layout_width() - border_offset * 2 -
                      root->style.padding.left - root->style.padding.right -
                      (show_gutter ? (gutter_width + 2) : 0);
    }
  }

  std::vector<int> row_starts;
  if (content_width > 0) {
    row_starts = ComputeRowStarts(graphemes, content_width, false);
  } else {
    row_starts = logical_line_starts;
  }

  int cursor_row = RowOfIndex(row_starts, cursor_pos);

  if (highlight_current_line) {
    content_line_highlights.reserve(row_starts.size());
  }
  if (show_gutter) {
    gutter_lines.reserve(row_starts.size());
  }

  int current_logical_line = 0;
  for (size_t r = 0; r < row_starts.size(); ++r) {
    bool is_new_logical_line =
        (r == 0 ||
         (row_starts[r] > 0 && IsLineBreak(graphemes[row_starts[r] - 1])));
    if (r > 0 && is_new_logical_line) {
      current_logical_line++;
    }

    if (highlight_current_line) {
      bool is_current = (static_cast<int>(r) == cursor_row);
      content_line_highlights.push_back(
          {is_current ? "line current-line" : "line"});
    }

    if (show_gutter) {
      if (line_wrap == "subline") {
        if (is_new_logical_line) {
          std::string css_class = (current_logical_line == active_logical_line)
                                      ? "line-number active"
                                      : "line-number";
          gutter_lines.push_back(
              {PadLeft(labels[current_logical_line], gutter_width), css_class});
        } else {
          gutter_lines.push_back(
              {std::string(gutter_width, ' '), "line-number wrapped"});
        }
      } else {
        // If line_wrap is not "subline", emit one gutter line per logical line.
        if (is_new_logical_line) {
          std::string css_class = (current_logical_line == active_logical_line)
                                      ? "line-number active"
                                      : "line-number";
          gutter_lines.push_back(
              {PadLeft(labels[current_logical_line], gutter_width), css_class});
        }
      }
    }
  }
}

namespace {
int RegisterThis = []() {
  RegisterGlobalComponent("textarea", []() { return Ref<textarea>::New(); });
  return 0;
}();
}  // namespace

}  // namespace rtxui
