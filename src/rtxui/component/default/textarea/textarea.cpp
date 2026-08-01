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
  BindCollection("gutter_lines", &gutter_lines,
                 [](const GutterLine& line) {
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
      <div class="gutter">
        <for each="{gutter_lines}" as="line">
          <div class="{line.css_class}">{line.text}</div>
        </for>
      </div>
      <if condition="{highlight_current_line}">
        <div class="content-wrapper">
          <div class="line-highlights">
            <for each="{content_line_highlights}" as="row">
              <div class="{row.css_class}"></div>
            </for>
          </div>
          <div class="content">
            <span>{left_unselected}</span><span class="{selection_class_left}">{left_selected}</span><span class="{cursor_class}">{cursor_char}</span><span class="{selection_class_right}">{right_selected}</span><span>{right_unselected}</span><span class="placeholder">{placeholder_text}</span>
          </div>
        </div>
      </if>
      <else>
        <div class="content">
          <span>{left_unselected}</span><span class="{selection_class_left}">{left_selected}</span><span class="{cursor_class}">{cursor_char}</span><span class="{selection_class_right}">{right_selected}</span><span>{right_unselected}</span><span class="placeholder">{placeholder_text}</span>
        </div>
      </else>
    </if>
    <else>
      <if condition="{highlight_current_line}">
        <div class="content-wrapper">
          <div class="line-highlights">
            <for each="{content_line_highlights}" as="row">
              <div class="{row.css_class}"></div>
            </for>
          </div>
          <div class="content">
            <span>{left_unselected}</span><span class="{selection_class_left}">{left_selected}</span><span class="{cursor_class}">{cursor_char}</span><span class="{selection_class_right}">{right_selected}</span><span>{right_unselected}</span><span class="placeholder">{placeholder_text}</span>
          </div>
        </div>
      </if>
      <else>
        <span>{left_unselected}</span><span class="{selection_class_left}">{left_selected}</span><span class="{cursor_class}">{cursor_char}</span><span class="{selection_class_right}">{right_selected}</span><span>{right_unselected}</span><span class="placeholder">{placeholder_text}</span>
      </else>
    </else>
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
        opacity: 0.8;
        transition: background-color 0.1s linear, opacity 0.1s linear, color 0.1s linear;
      }
      /* Only matches once the app sets a `linenumbers` attribute, so a
         plain textarea's layout/CSS is completely untouched. */
      self[linenumbers] {
        display: flex;
        flex-direction: row;
        align-items: flex-start;
        padding-left: 0;
      }
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
         reason as self:hover/self:focus above. */
      .current-line {
        background-color: rgb(60, 60, 60);
      }
      .content {
        display: block;
        flex-grow: 1;
        width: 100%;
        white-space: pre-wrap;
      }
      /* Fixed absolute colors, not lighten(): self:hover and self:focus can
         both match at once (e.g. clicking focuses the textarea while the
         mouse is still over it, so it's also hovered), and since both
         rules apply to the same style in cascade order, lighten() would
         compound -- self:focus lightening the already-lightened
         self:hover result -- producing a much brighter, washed-out color
         instead of a stable focus look. */
      self:hover {
        background-color: rgb(75, 75, 75);
        opacity: 0.9;
      }
      self:focus {
        background-color: rgb(101, 101, 101);
        opacity: 1.0;
      }
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
         app's overrides are known anyway). */
      .selection {
        background-color: rgb(38, 79, 120);
        color: white;
      }
      /* Fixed absolute color, not lighten()/dim, for the same reason the
         selection colors are fixed above. */
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
// `content_line_highlights` (from `highlight_current_line`), all derived
// from `value` and `cursor_pos`. One entry per logical line (not per
// rendered row: the whole value is always rendered, there's no
// viewport-based windowing), so both collections stay in sync with the
// content even while scrolled.
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

  // Split into logical lines, tracking each line's display width (for the
  // "subline" wrap estimate below) and which line the cursor sits on.
  std::vector<int> line_widths;
  int active_line = 0;
  int current_width = 0;
  int line = 0;
  for (int i = 0; i < n; ++i) {
    if (i == cursor_pos) {
      active_line = line;
    }
    if (IsLineBreak(graphemes[i])) {
      line_widths.push_back(current_width);
      current_width = 0;
      line++;
    } else {
      current_width += graphemes[i].width;
    }
  }
  if (cursor_pos == n) {
    active_line = line;
  }
  line_widths.push_back(current_width);
  int num_lines = static_cast<int>(line_widths.size());

  if (highlight_current_line) {
    content_line_highlights.reserve(num_lines);
    for (int i = 0; i < num_lines; ++i) {
      content_line_highlights.push_back(
          {(i == active_line) ? "line current-line" : "line"});
    }
  }

  if (!show_gutter) {
    return;
  }

  // First pass: compute every line's label so the gutter can be sized to
  // the widest one before padding them all to that width.
  std::vector<std::string> labels(num_lines);
  for (int i = 0; i < num_lines; ++i) {
    int absolute_number = line_start + i;
    if (line_end != -1 && absolute_number > line_end) {
      continue;  // Beyond line_end: blank gutter cell.
    }
    int displayed = (relative && i != active_line)
                        ? std::abs(i - active_line)
                        : absolute_number;
    labels[i] = std::to_string(displayed);
  }

  gutter_width = 1;
  for (const auto& label : labels) {
    gutter_width = std::max<int>(gutter_width, static_cast<int>(label.size()));
  }

  // Wrapped-row estimate for line_wrap="subline": uses the content box's
  // width from the *previous* frame's layout (same one-frame lag as
  // KeepCursorVisible above), and sums grapheme widths rather than
  // replicating the layout engine's actual word-break logic, so this is an
  // approximation, not a guarantee of exact row alignment.
  int content_width = 0;
  if (line_wrap == "subline") {
    if (Element* root = Root()) {
      int border_offset =
          (root->style.border_style != BorderStyle::None) ? 1 : 0;
      content_width = root->layout_width() - border_offset * 2 -
                      root->style.padding.left - root->style.padding.right -
                      gutter_width - 2;
    }
  }

  gutter_lines.reserve(num_lines);
  for (int i = 0; i < num_lines; ++i) {
    std::string css_class = (i == active_line) ? "line-number active"
                                                : "line-number";
    gutter_lines.push_back({PadLeft(labels[i], gutter_width), css_class});

    if (content_width > 0 && line_widths[i] > content_width) {
      int wrapped_rows =
          (line_widths[i] + content_width - 1) / content_width - 1;
      for (int w = 0; w < wrapped_rows; ++w) {
        gutter_lines.push_back(
            {std::string(gutter_width, ' '), "line-number wrapped"});
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
