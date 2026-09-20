// Copyright 2024 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include "rtxui/component/default/input/text_input_base.hpp"

#include <algorithm>
#include <cmath>

#include "rtxui/base/string.hpp"
#include "rtxui/component/component_internal.hpp"
#include "rtxui/dom/element.hpp"
#include "rtxui/dom/text_element.hpp"

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

std::string GraphemesToString(const std::vector<Grapheme>& graphemes,
                              size_t start = 0,
                              size_t count = std::string::npos) {
  std::string s;
  size_t end = (count == std::string::npos)
                   ? graphemes.size()
                   : std::min(graphemes.size(), start + count);
  for (size_t i = start; i < end; ++i) {
    s.append(graphemes[i].text);
  }
  return s;
}

int FindWordBoundaryLeft(const std::vector<Grapheme>& graphemes,
                         int start_pos) {
  int pos = start_pos;
  if (pos <= 0) {
    return 0;
  }
  while (pos > 0 &&
         (graphemes[pos - 1].text == " " || graphemes[pos - 1].text == "\t")) {
    pos--;
  }
  while (pos > 0 && graphemes[pos - 1].text != " " &&
         graphemes[pos - 1].text != "\t") {
    pos--;
  }
  return pos;
}

int FindWordBoundaryRight(const std::vector<Grapheme>& graphemes,
                          int start_pos) {
  int pos = start_pos;
  int n = static_cast<int>(graphemes.size());
  if (pos >= n) {
    return n;
  }
  while (pos < n &&
         (graphemes[pos].text == " " || graphemes[pos].text == "\t")) {
    pos++;
  }
  while (pos < n && graphemes[pos].text != " " && graphemes[pos].text != "\t") {
    pos++;
  }
  return pos;
}

void HandleBackspace(std::vector<Grapheme>& graphemes,
                     int& cursor_pos,
                     bool ctrl) {
  if (cursor_pos <= 0) {
    return;
  }
  if (ctrl) {
    int target = FindWordBoundaryLeft(graphemes, cursor_pos);
    graphemes.erase(graphemes.begin() + target, graphemes.begin() + cursor_pos);
    cursor_pos = target;
  } else {
    graphemes.erase(graphemes.begin() + cursor_pos - 1);
    cursor_pos--;
  }
}

void HandleDelete(std::vector<Grapheme>& graphemes,
                  int& cursor_pos,
                  bool ctrl) {
  int n = static_cast<int>(graphemes.size());
  if (cursor_pos >= n) {
    return;
  }
  if (ctrl) {
    int target = FindWordBoundaryRight(graphemes, cursor_pos);
    graphemes.erase(graphemes.begin() + cursor_pos, graphemes.begin() + target);
  } else {
    graphemes.erase(graphemes.begin() + cursor_pos);
  }
}

int GetCursorPositionFromColumn(const std::vector<Grapheme>& graphemes,
                                int target_col) {
  if (target_col < 0) {
    return 0;
  }
  int best_pos = 0;
  int min_dist = std::abs(target_col);
  int current_col = 0;
  for (size_t i = 0; i < graphemes.size(); ++i) {
    current_col += graphemes[i].width;
    int dist = std::abs(current_col - target_col);
    if (dist < min_dist) {
      min_dist = dist;
      best_pos = static_cast<int>(i + 1);
    }
  }
  return best_pos;
}

struct Position2D {
  int line = 0;
  int column = 0;
};

Position2D GetCursor2D(const std::vector<Grapheme>& graphemes, int cursor_pos) {
  Position2D pos;
  int limit = std::min(static_cast<int>(graphemes.size()), cursor_pos);
  for (int i = 0; i < limit; ++i) {
    if (graphemes[i].text == "\n" || graphemes[i].text == "\r\n" ||
        graphemes[i].text == "\r") {
      pos.line++;
      pos.column = 0;
    } else {
      pos.column += graphemes[i].width;
    }
  }
  return pos;
}

int GetCursorPosFrom2D(const std::vector<Grapheme>& graphemes,
                       int target_line,
                       int target_col) {
  int cur_line = 0;
  int cur_col = 0;
  int best_pos = 0;
  int min_dist = -1;
  int n = static_cast<int>(graphemes.size());

  int i = 0;
  while (i < n && cur_line < target_line) {
    if (graphemes[i].text == "\n" || graphemes[i].text == "\r\n" ||
        graphemes[i].text == "\r") {
      cur_line++;
    }
    i++;
  }
  if (cur_line < target_line) {
    return n;
  }

  best_pos = i;
  min_dist = std::abs(target_col);

  while (i < n) {
    if (graphemes[i].text == "\n" || graphemes[i].text == "\r\n" ||
        graphemes[i].text == "\r") {
      break;
    }
    cur_col += graphemes[i].width;
    i++;

    int dist = std::abs(cur_col - target_col);
    if (dist < min_dist) {
      min_dist = dist;
      best_pos = i;
    }
  }
  return best_pos;
}

bool IsSpace(const Grapheme& g) {
  return g.text == " ";
}

// Mirrors the word-wrap algorithm in layout.cpp's LayoutInlineFlow (word
// wrap at the last space, falling back to an emergency character break),
// applied to the whole `value` as one continuous run -- matching how the
// engine actually flows text across the textarea's
// left_unselected/left_selected/cursor_char/right_selected/right_unselected
// spans in a single inline formatting context. Returns the grapheme index
// each visual row starts at (row 0 always starts at index 0), so mouse
// clicks and KeepCursorVisible's scroll-into-view can agree with what's
// actually painted once lines wrap.
//
// This doesn't replicate the engine's per-DOM-node space search (a space
// seen while laying out an earlier span isn't visible when laying out a
// later one), so a wrap point that lands exactly on a word split across two
// of those spans can differ by a few cells from the real layout -- a narrow
}  // namespace

std::vector<int> ComputeRowStarts(const std::vector<Grapheme>& graphemes,
                                  int content_width,
                                  bool overflow_wrap_normal) {
  std::vector<int> row_starts = {0};
  int n = static_cast<int>(graphemes.size());
  if (content_width <= 0) {
    // Not laid out yet: fall back to one row per logical line.
    for (int i = 0; i < n; ++i) {
      if (IsLineBreak(graphemes[i])) {
        row_starts.push_back(i + 1);
      }
    }
    return row_starts;
  }

  int run_start = 0;  // Grapheme index the pending (unbroken) run begins at.
  int col_start = 0;  // Monotonic column value at run_start.
  int cur_col = 0;    // Monotonic column value of the next grapheme.
  int last_space_index = -1;
  int last_space_col = 0;

  auto commit_line = [&](int next_run_start) {
    row_starts.push_back(next_run_start);
    run_start = next_run_start;
    last_space_index = -1;
  };

  for (int i = 0; i < n; ++i) {
    const Grapheme& g = graphemes[i];
    if (IsLineBreak(g)) {
      col_start = cur_col + g.width;
      cur_col = col_start;
      commit_line(i + 1);
      continue;
    }

    if (IsSpace(g)) {
      last_space_index = i;
      last_space_col = cur_col - col_start;
    }

    if (cur_col - col_start + g.width > content_width) {
      if (last_space_index != -1) {
        // cur_col already measures up to the current (overflowing)
        // grapheme; only col_start moves to just past the space, so
        // cur_col - col_start keeps counting the characters typed between
        // the space and here instead of discarding them (mirrors the fix
        // for the same bug in layout.cpp's LayoutInlineFlow).
        col_start += last_space_col + 1;
        commit_line(last_space_index + 1);
        cur_col += g.width;
      } else if (overflow_wrap_normal) {
        cur_col += g.width;  // Unbreakable word: let it overflow.
      } else if (cur_col > col_start) {
        // Emergency break before this grapheme, keeping the row within
        // content_width.
        col_start = cur_col;
        commit_line(i);
        cur_col += g.width;
      } else {
        // A single grapheme wider than content_width: place it anyway so
        // this makes progress.
        cur_col += g.width;
        col_start = cur_col;
        commit_line(i + 1);
      }
    } else {
      cur_col += g.width;
    }
  }
  return row_starts;
}

int RowOfIndex(const std::vector<int>& row_starts, int pos) {
  auto it = std::upper_bound(row_starts.begin(), row_starts.end(), pos);
  return static_cast<int>(std::distance(row_starts.begin(), it)) - 1;
}

int ColOfIndex(const std::vector<Grapheme>& graphemes, int row_start, int pos) {
  int col = 0;
  int n = static_cast<int>(graphemes.size());
  for (int i = row_start; i < pos && i < n; ++i) {
    col += graphemes[i].width;
  }
  return col;
}

// Inverse of RowOfIndex/ColOfIndex: nearest grapheme index within
// `target_row` whose column is closest to `target_col`, matching
// GetCursorPosFrom2D's nearest-distance behavior but scoped to a visual row
// (from `row_starts`) instead of a logical line.
int RowColToIndex(const std::vector<Grapheme>& graphemes,
                  const std::vector<int>& row_starts,
                  int target_row,
                  int target_col) {
  int num_rows = static_cast<int>(row_starts.size());
  target_row = std::clamp(target_row, 0, num_rows - 1);
  int n = static_cast<int>(graphemes.size());
  int row_start = row_starts[target_row];
  int row_end = (target_row + 1 < num_rows) ? row_starts[target_row + 1] : n;

  int i = row_start;
  int cur_col = 0;
  int best_pos = row_start;
  int min_dist = std::abs(target_col);
  while (i < row_end) {
    if (IsLineBreak(graphemes[i])) {
      break;
    }
    cur_col += graphemes[i].width;
    i++;
    int dist = std::abs(cur_col - target_col);
    if (dist < min_dist) {
      min_dist = dist;
      best_pos = i;
    }
  }
  return best_pos;
}

namespace {

int FindLineStart(const std::vector<Grapheme>& graphemes, int start_pos) {
  int pos = start_pos;
  while (pos > 0) {
    if (graphemes[pos - 1].text == "\n" || graphemes[pos - 1].text == "\r\n" ||
        graphemes[pos - 1].text == "\r") {
      break;
    }
    pos--;
  }
  return pos;
}

int FindLineEnd(const std::vector<Grapheme>& graphemes, int start_pos) {
  int pos = start_pos;
  int n = static_cast<int>(graphemes.size());
  while (pos < n) {
    if (graphemes[pos].text == "\n" || graphemes[pos].text == "\r\n" ||
        graphemes[pos].text == "\r") {
      break;
    }
    pos++;
  }
  return pos;
}

std::pair<int, int> GetWordBoundaries(const std::vector<Grapheme>& graphemes,
                                      int click_pos) {
  int n = static_cast<int>(graphemes.size());
  if (n == 0) {
    return {0, 0};
  }
  if (click_pos < 0) {
    click_pos = 0;
  }
  if (click_pos >= n) {
    click_pos = n - 1;
  }

  std::string_view target = graphemes[click_pos].text;

  auto is_word_char = [](std::string_view s) {
    if (s.empty()) {
      return false;
    }
    char c = s[0];
    if (std::isalnum(static_cast<unsigned char>(c)) || c == '_') {
      return true;
    }
    if ((static_cast<unsigned char>(c) & 0x80) != 0) {
      return true;
    }
    return false;
  };

  auto is_space_char = [](std::string_view s) {
    return s == " " || s == "\t" || s == "\n" || s == "\r\n" || s == "\r";
  };

  int start = click_pos;
  int end = click_pos;

  if (is_word_char(target)) {
    while (start > 0 && is_word_char(graphemes[start - 1].text)) {
      start--;
    }
    while (end < n && is_word_char(graphemes[end].text)) {
      end++;
    }
  } else if (is_space_char(target)) {
    while (start > 0 && is_space_char(graphemes[start - 1].text)) {
      start--;
    }
    while (end < n && is_space_char(graphemes[end].text)) {
      end++;
    }
  } else {
    while (start > 0 && !is_word_char(graphemes[start - 1].text) &&
           !is_space_char(graphemes[start - 1].text)) {
      start--;
    }
    while (end < n && !is_word_char(graphemes[end].text) &&
           !is_space_char(graphemes[end].text)) {
      end++;
    }
  }
  return {start, end};
}

std::pair<int, int> GetLineBoundaries(const std::vector<Grapheme>& graphemes,
                                      int click_pos,
                                      bool is_multiline) {
  int n = static_cast<int>(graphemes.size());
  if (n == 0) {
    return {0, 0};
  }
  if (!is_multiline) {
    return {0, n};
  }
  click_pos = std::clamp(click_pos, 0, n);
  int start = FindLineStart(graphemes, click_pos);
  int end = FindLineEnd(graphemes, click_pos);
  return {start, end};
}

std::pair<int, int> GetParagraphBoundaries(
    const std::vector<Grapheme>& graphemes,
    int click_pos,
    bool is_multiline) {
  int n = static_cast<int>(graphemes.size());
  if (n == 0) {
    return {0, 0};
  }
  if (!is_multiline) {
    return {0, n};
  }
  click_pos = std::clamp(click_pos, 0, n);
  int cur_line_start = FindLineStart(graphemes, click_pos);
  int cur_line_end = FindLineEnd(graphemes, click_pos);

  auto is_line_blank = [&](int l_start, int l_end) {
    for (int i = l_start; i < l_end; ++i) {
      if (graphemes[i].text != " " && graphemes[i].text != "\t") {
        return false;
      }
    }
    return true;
  };

  if (is_line_blank(cur_line_start, cur_line_end)) {
    return {cur_line_start, cur_line_end};
  }

  int p_start = cur_line_start;
  while (p_start > 0) {
    int prev_line_start = FindLineStart(graphemes, p_start - 1);
    int prev_line_end = FindLineEnd(graphemes, p_start - 1);
    if (is_line_blank(prev_line_start, prev_line_end)) {
      break;
    }
    p_start = prev_line_start;
  }

  int p_end = cur_line_end;
  while (p_end < n) {
    int next_pos = p_end;
    if (next_pos < n && (graphemes[next_pos].text == "\n" ||
                         graphemes[next_pos].text == "\r\n" ||
                         graphemes[next_pos].text == "\r")) {
      next_pos++;
    }
    if (next_pos >= n) {
      break;
    }
    int next_line_start = next_pos;
    int next_line_end = FindLineEnd(graphemes, next_line_start);
    if (is_line_blank(next_line_start, next_line_end)) {
      break;
    }
    p_end = next_line_end;
  }

  return {p_start, p_end};
}

bool DeleteSelection(std::vector<Grapheme>& graphemes,
                     int& selection_start,
                     int& cursor_pos) {
  if (selection_start != -1 && selection_start != cursor_pos) {
    int start = std::min(selection_start, cursor_pos);
    int end = std::max(selection_start, cursor_pos);
    if (start >= 0 && end <= static_cast<int>(graphemes.size())) {
      graphemes.erase(graphemes.begin() + start, graphemes.begin() + end);
      cursor_pos = start;
      selection_start = -1;
      return true;
    }
  }
  selection_start = -1;
  return false;
}

// Truncates `new_graphemes` (in place) to whatever still fits under
// `maxlength` given `current_count` graphemes already present, mirroring
// how a real browser truncates rather than rejects a paste that would
// overflow a maxlength field. A no-op when maxlength is unlimited (< 0).
void CapInsertionToMaxLength(std::vector<Grapheme>& new_graphemes,
                             int current_count,
                             int maxlength) {
  if (maxlength < 0) {
    return;
  }
  int available = std::max(0, maxlength - current_count);
  if (static_cast<int>(new_graphemes.size()) > available) {
    new_graphemes.resize(available);
  }
}

}  // namespace

namespace {
constexpr size_t kMaxUndoDepth = 200;
}  // namespace

void TextInputBase::BeginEdit(EditKind kind) {
  bool has_selection = (selection_start != -1 && selection_start != cursor_pos);
  bool contiguous = !has_selection && kind != EditKind::Other &&
                    kind == last_edit_kind_ && cursor_pos == last_edit_end_pos_;
  if (!contiguous) {
    undo_stack_.push_back({value, cursor_pos, selection_start});
    if (undo_stack_.size() > kMaxUndoDepth) {
      undo_stack_.erase(undo_stack_.begin());
    }
    redo_stack_.clear();
  }
  last_edit_kind_ = kind;
}

void TextInputBase::KeepCursorVisible(Element* root, bool is_multiline) {
  if (!root) {
    return;
  }

  auto current_graphemes = GetGraphemesList(value);

  // For a wrapped textarea, "line" must mean visual row, not logical line:
  // self's scroll_y (and the mouse-click math in OnEventShared) operate in
  // rendered-row units, which only match logical-line indices when no line
  // wraps. See ComputeRowStarts for why.
  Element* content_el =
      is_multiline ? root->QuerySelector(".content") : nullptr;
  int cursor_line = 0;
  int cursor_col = 0;
  if (is_multiline) {
    int wrap_width = content_el ? content_el->layout_width() : 0;
    bool overflow_wrap_normal =
        content_el && content_el->style.overflow_wrap.value_or(
                          OverflowWrap::Anywhere) == OverflowWrap::Normal;
    auto row_starts =
        ComputeRowStarts(current_graphemes, wrap_width, overflow_wrap_normal);
    cursor_line = RowOfIndex(row_starts, cursor_pos);
    cursor_col =
        ColOfIndex(current_graphemes, row_starts[cursor_line], cursor_pos);
  } else {
    auto pos2d = GetCursor2D(current_graphemes, cursor_pos);
    cursor_col = pos2d.column;
  }

  int cursor_width = (cursor_pos < static_cast<int>(current_graphemes.size()))
                         ? std::max(1, current_graphemes[cursor_pos].width)
                         : 1;

  int border_offset = (root->style.border_style != BorderStyle::None) ? 1 : 0;

  // Horizontal Scroll. For a wrapped textarea, this uses the content box's
  // own width (excluding the line-number gutter, if any) rather than
  // self's, since that's the width ComputeRowStarts wrapped against.
  int visible_width;
  if (is_multiline && content_el) {
    visible_width = content_el->layout_width();
  } else {
    int padding_left = root->style.padding.left;
    int padding_right = root->style.padding.right;
    int border_horiz = border_offset * 2;
    int padding_horiz = padding_left + padding_right;
    visible_width = root->layout_width() - border_horiz - padding_horiz;
  }
  if (visible_width > 0) {
    int curr_scroll_x = root->scroll_x();
    if (cursor_col < curr_scroll_x) {
      root->set_scroll_x(cursor_col);
    } else if (cursor_col + cursor_width > curr_scroll_x + visible_width) {
      root->set_scroll_x(cursor_col + cursor_width - visible_width);
    }
  }

  // Vertical Scroll (for multiline)
  if (is_multiline) {
    int padding_top = root->style.padding.top;
    int padding_bottom = root->style.padding.bottom;
    int border_vert = border_offset * 2;
    int padding_vert = padding_top + padding_bottom;

    int layout_h = root->layout_height();
    int visible_height = layout_h - border_vert - padding_vert;
    if (visible_height > 0) {
      int curr_scroll_y = root->scroll_y();
      if (cursor_line < curr_scroll_y) {
        root->set_scroll_y(cursor_line);
      } else if (cursor_line >= curr_scroll_y + visible_height) {
        root->set_scroll_y(cursor_line - visible_height + 1);
      }
    }
  }
}

int TextInputBase::ClickToCursorPos(Element* root,
                                    bool is_multiline,
                                    int click_x,
                                    int click_y,
                                    const std::vector<Grapheme>& graphemes) {
  // For a wrapped textarea, hit-test against the ".content" box directly:
  // its absolute position already bakes in self's scroll offset, border,
  // padding and (when linenumbers is set) the gutter's width, so no manual
  // offset arithmetic is needed here -- unlike self's own box, which a
  // gutter makes narrower than self's full width.
  Element* content_el =
      is_multiline ? root->QuerySelector(".content") : nullptr;
  if (content_el) {
    int wrap_width = content_el->layout_width();
    int wrap_height = content_el->layout_height();
    int inner_click_x = std::clamp(click_x - content_el->absolute_x(), 0,
                                   std::max(0, wrap_width - 1));
    int inner_click_y = std::clamp(click_y - content_el->absolute_y(), 0,
                                   std::max(0, wrap_height - 1));
    bool overflow_wrap_normal =
        content_el->style.overflow_wrap.value_or(OverflowWrap::Anywhere) ==
        OverflowWrap::Normal;
    auto row_starts =
        ComputeRowStarts(graphemes, wrap_width, overflow_wrap_normal);
    return RowColToIndex(graphemes, row_starts, inner_click_y, inner_click_x);
  }

  int border_offset = (root->style.border_style != BorderStyle::None) ? 1 : 0;
  int padding_left = root->style.padding.left;
  int padding_top = root->style.padding.top;
  int inner_click_x =
      click_x - root->absolute_x() - border_offset - padding_left;
  int inner_click_y =
      click_y - root->absolute_y() - border_offset - padding_top;

  int target_col = inner_click_x + root->scroll_x();
  int target_row = is_multiline ? (inner_click_y + root->scroll_y()) : 0;

  return is_multiline ? GetCursorPosFrom2D(graphemes, target_row, target_col)
                      : GetCursorPositionFromColumn(graphemes, target_col);
}

bool TextInputBase::OnEventShared(ComponentBase* self,
                                  Event event,
                                  bool is_multiline) {
  auto* root = self->Root();
  if (disabled) {
    return false;
  }
  if (event.is<Event::Mouse>()) {
    auto mouse = event.get<Event::Mouse>();
    bool is_captured = (self->GetMouseCapturer() == self);

    if (mouse.button == Event::Mouse::Button::Left) {
      if (mouse.motion == Event::Mouse::Motion::Pressed) {
        if (!root) {
          return false;
        }
        int click_x = mouse.x - 1;
        int click_y = mouse.y - 1;
        int abs_x = root->absolute_x();
        int abs_y = root->absolute_y();
        int layout_w = root->layout_width();
        int layout_h = root->layout_height();

        if (click_x >= abs_x && click_x < abs_x + layout_w &&
            click_y >= abs_y && click_y < abs_y + layout_h) {
          FocusExclusive(root);
          self->CaptureMouse();

          auto graphemes = GetGraphemesList(value);
          int click_pos =
              ClickToCursorPos(root, is_multiline, click_x, click_y, graphemes);

          auto now = std::chrono::steady_clock::now();
          // last_click_time_ defaults to time_point::min() as a "no
          // previous click" sentinel; subtracting from it overflows
          // (signed integer overflow is UB), so check for it explicitly
          // instead of ever computing `now - time_point::min()`.
          bool is_consecutive_click =
              last_click_time_ !=
                  std::chrono::steady_clock::time_point::min() &&
              now - last_click_time_ < std::chrono::milliseconds(500) &&
              (click_pos == last_click_pos_ ||
               (click_x == last_click_x_ && click_y == last_click_y_));

          if (is_consecutive_click) {
            click_count_ = (click_count_ % 4) + 1;
          } else {
            click_count_ = 1;
          }

          if (click_count_ == 1) {
            selection_granularity_ = SelectionGranularity::kCharacter;
            selection_anchor_start_ = click_pos;
            selection_anchor_end_ = click_pos;
            selection_start = click_pos;
            cursor_pos = click_pos;
          } else if (click_count_ == 2) {
            selection_granularity_ = SelectionGranularity::kWord;
            auto [w_start, w_end] = GetWordBoundaries(graphemes, click_pos);
            selection_anchor_start_ = w_start;
            selection_anchor_end_ = w_end;
            selection_start = w_start;
            cursor_pos = w_end;
          } else if (click_count_ == 3) {
            selection_granularity_ = SelectionGranularity::kLine;
            auto [l_start, l_end] =
                GetLineBoundaries(graphemes, click_pos, is_multiline);
            selection_anchor_start_ = l_start;
            selection_anchor_end_ = l_end;
            selection_start = l_start;
            cursor_pos = l_end;
          } else {  // click_count_ == 4
            selection_granularity_ = SelectionGranularity::kParagraph;
            auto [p_start, p_end] =
                GetParagraphBoundaries(graphemes, click_pos, is_multiline);
            selection_anchor_start_ = p_start;
            selection_anchor_end_ = p_end;
            selection_start = p_start;
            cursor_pos = p_end;
          }

          last_click_time_ = now;
          last_click_pos_ = click_pos;
          last_click_x_ = click_x;
          last_click_y_ = click_y;

          auto pos2d = GetCursor2D(graphemes, cursor_pos);
          ideal_column_ = pos2d.column;

          KeepCursorVisible(root, is_multiline);
          return true;
        }
      } else if (mouse.motion == Event::Mouse::Motion::Moved && is_captured) {
        if (!root) {
          return false;
        }
        int click_x = mouse.x - 1;
        int click_y = mouse.y - 1;

        auto graphemes = GetGraphemesList(value);
        int click_pos =
            ClickToCursorPos(root, is_multiline, click_x, click_y, graphemes);

        if (click_pos != last_click_pos_) {
          click_count_ = 0;
        }

        if (selection_granularity_ == SelectionGranularity::kCharacter) {
          cursor_pos = click_pos;
        } else {
          auto get_boundaries = [&](int pos) {
            if (selection_granularity_ == SelectionGranularity::kWord) {
              return GetWordBoundaries(graphemes, pos);
            }
            if (selection_granularity_ == SelectionGranularity::kLine) {
              return GetLineBoundaries(graphemes, pos, is_multiline);
            }
            return GetParagraphBoundaries(graphemes, pos, is_multiline);
          };

          if (click_pos >= selection_anchor_end_) {
            auto [b_start, b_end] = get_boundaries(click_pos);
            selection_start = selection_anchor_start_;
            cursor_pos = b_end;
          } else if (click_pos <= selection_anchor_start_) {
            auto [b_start, b_end] = get_boundaries(click_pos);
            selection_start = selection_anchor_end_;
            cursor_pos = b_start;
          } else {
            selection_start = selection_anchor_start_;
            cursor_pos = selection_anchor_end_;
          }
        }

        auto pos2d = GetCursor2D(graphemes, cursor_pos);
        ideal_column_ = pos2d.column;

        KeepCursorVisible(root, is_multiline);
        return true;
      } else if (mouse.motion == Event::Mouse::Motion::Released &&
                 is_captured) {
        self->ReleaseMouse();
        if (selection_start == cursor_pos) {
          selection_start = -1;
        }
        return true;
      }
    }
    return false;
  }

  if (event.is<Event::Keyboard>()) {
    auto kb = event.get<Event::Keyboard>();
    if (kb.motion == Event::Keyboard::Motion::Pressed ||
        kb.motion == Event::Keyboard::Motion::Repeat) {
      last_click_time_ = std::chrono::steady_clock::time_point::min();
      click_count_ = 0;
      if (!root || !root->focused()) {
        return false;
      }

      auto graphemes = GetGraphemesList(value);
      int n = static_cast<int>(graphemes.size());

      if (event == Event::CtrlA()) {
        selection_start = 0;
        cursor_pos = n;
        auto pos2d = GetCursor2D(graphemes, cursor_pos);
        ideal_column_ = pos2d.column;
        KeepCursorVisible(root, is_multiline);
        return true;
      }

      bool is_redo_chord = kb.codepoint == 'z' && kb.modifier.ctrl &&
                           kb.modifier.shift && !kb.modifier.alt &&
                           !kb.modifier.meta;
      if (event == Event::CtrlZ() || is_redo_chord || event == Event::CtrlY()) {
        if (readonly) {
          return true;
        }
        bool is_redo = is_redo_chord || event == Event::CtrlY();
        auto& from_stack = is_redo ? redo_stack_ : undo_stack_;
        auto& to_stack = is_redo ? undo_stack_ : redo_stack_;
        if (!from_stack.empty()) {
          to_stack.push_back({value, cursor_pos, selection_start});
          HistoryEntry entry = from_stack.back();
          from_stack.pop_back();
          value = entry.value;
          cursor_pos = entry.cursor_pos;
          selection_start = entry.selection_start;
          self->PropagateBinding("value", value);
          last_edit_kind_ = EditKind::Other;
          last_edit_end_pos_ = -1;
          auto new_graphemes = GetGraphemesList(value);
          auto pos2d = GetCursor2D(new_graphemes, cursor_pos);
          ideal_column_ = pos2d.column;
          KeepCursorVisible(root, is_multiline);
        }
        return true;
      }

      if (event == Event::CtrlC() || event == Event::CtrlX()) {
        // With no selection, there's nothing to copy: leave the event
        // unhandled so it falls through to Screen's global Ctrl+C-quits
        // shortcut, rather than silently swallowing it whenever any text
        // input happens to be focused.
        if (selection_start == -1 || selection_start == cursor_pos) {
          return false;
        }
        int sel_min = std::min(selection_start, cursor_pos);
        int sel_max = std::max(selection_start, cursor_pos);
        self->SetClipboard(
            GraphemesToString(graphemes, sel_min, sel_max - sel_min));
        if (event == Event::CtrlX() && !readonly) {
          BeginEdit(EditKind::Other);
          DeleteSelection(graphemes, selection_start, cursor_pos);
          // See the Backspace branch above for why this must come before
          // the `value` reassignment.
          auto pos2d = GetCursor2D(graphemes, cursor_pos);
          ideal_column_ = pos2d.column;
          value = GraphemesToString(graphemes);
          self->PropagateBinding("value", value);
          last_edit_end_pos_ = cursor_pos;
          KeepCursorVisible(root, is_multiline);
        }
        return true;
      }

      if (kb.special == Event::Keyboard::Special::ArrowLeft) {
        if (kb.modifier.shift && selection_start == -1) {
          selection_start = cursor_pos;
        }
        if (!kb.modifier.shift && selection_start != -1 &&
            selection_start != cursor_pos && !kb.modifier.ctrl &&
            !kb.modifier.alt) {
          cursor_pos = std::min(selection_start, cursor_pos);
          selection_start = -1;
        } else {
          if (!kb.modifier.shift) {
            selection_start = -1;
          }
          if (kb.modifier.ctrl || kb.modifier.alt) {
            cursor_pos = FindWordBoundaryLeft(graphemes, cursor_pos);
          } else {
            cursor_pos = std::max(0, cursor_pos - 1);
          }
        }
        auto pos2d = GetCursor2D(graphemes, cursor_pos);
        ideal_column_ = pos2d.column;
        KeepCursorVisible(root, is_multiline);
        return true;
      }
      if (kb.special == Event::Keyboard::Special::ArrowRight) {
        if (kb.modifier.shift && selection_start == -1) {
          selection_start = cursor_pos;
        }
        if (!kb.modifier.shift && selection_start != -1 &&
            selection_start != cursor_pos && !kb.modifier.ctrl &&
            !kb.modifier.alt) {
          cursor_pos = std::max(selection_start, cursor_pos);
          selection_start = -1;
        } else {
          if (!kb.modifier.shift) {
            selection_start = -1;
          }
          if (kb.modifier.ctrl || kb.modifier.alt) {
            cursor_pos = FindWordBoundaryRight(graphemes, cursor_pos);
          } else {
            cursor_pos = std::min(n, cursor_pos + 1);
          }
        }
        auto pos2d = GetCursor2D(graphemes, cursor_pos);
        ideal_column_ = pos2d.column;
        KeepCursorVisible(root, is_multiline);
        return true;
      }
      if (is_multiline && kb.special == Event::Keyboard::Special::ArrowUp) {
        if (kb.modifier.shift && selection_start == -1) {
          selection_start = cursor_pos;
        }
        if (!kb.modifier.shift) {
          selection_start = -1;
        }
        auto pos2d = GetCursor2D(graphemes, cursor_pos);
        cursor_pos =
            GetCursorPosFrom2D(graphemes, pos2d.line - 1, ideal_column_);
        KeepCursorVisible(root, is_multiline);
        return true;
      }
      if (is_multiline && kb.special == Event::Keyboard::Special::ArrowDown) {
        if (kb.modifier.shift && selection_start == -1) {
          selection_start = cursor_pos;
        }
        if (!kb.modifier.shift) {
          selection_start = -1;
        }
        auto pos2d = GetCursor2D(graphemes, cursor_pos);
        cursor_pos =
            GetCursorPosFrom2D(graphemes, pos2d.line + 1, ideal_column_);
        KeepCursorVisible(root, is_multiline);
        return true;
      }
      if (kb.special == Event::Keyboard::Special::Home) {
        if (kb.modifier.shift && selection_start == -1) {
          selection_start = cursor_pos;
        }
        if (!kb.modifier.shift) {
          selection_start = -1;
        }
        if (is_multiline) {
          cursor_pos = FindLineStart(graphemes, cursor_pos);
        } else {
          cursor_pos = 0;
        }
        auto pos2d = GetCursor2D(graphemes, cursor_pos);
        ideal_column_ = pos2d.column;
        KeepCursorVisible(root, is_multiline);
        return true;
      }
      if (kb.special == Event::Keyboard::Special::End) {
        if (kb.modifier.shift && selection_start == -1) {
          selection_start = cursor_pos;
        }
        if (!kb.modifier.shift) {
          selection_start = -1;
        }
        if (is_multiline) {
          cursor_pos = FindLineEnd(graphemes, cursor_pos);
        } else {
          cursor_pos = n;
        }
        auto pos2d = GetCursor2D(graphemes, cursor_pos);
        ideal_column_ = pos2d.column;
        KeepCursorVisible(root, is_multiline);
        return true;
      }
      if (kb.special == Event::Keyboard::Special::Backspace) {
        if (readonly) {
          return true;
        }
        BeginEdit(EditKind::Delete);
        if (DeleteSelection(graphemes, selection_start, cursor_pos)) {
          // Selection deleted
        } else {
          HandleBackspace(graphemes, cursor_pos,
                          kb.modifier.ctrl || kb.modifier.alt);
        }
        // Cursor position must be computed from `graphemes` before `value`
        // is reassigned below: Grapheme::text is a string_view into the
        // string `graphemes` was built from, so reassigning `value` first
        // frees that buffer out from under every grapheme's text (a real
        // heap-use-after-free once `value` is long enough to not fit in
        // std::string's small-string-optimization buffer).
        auto pos2d = GetCursor2D(graphemes, cursor_pos);
        ideal_column_ = pos2d.column;
        value = GraphemesToString(graphemes);
        self->PropagateBinding("value", value);
        last_edit_end_pos_ = cursor_pos;
        KeepCursorVisible(root, is_multiline);
        return true;
      }
      if (kb.special == Event::Keyboard::Special::Delete) {
        if (readonly) {
          return true;
        }
        BeginEdit(EditKind::Delete);
        if (DeleteSelection(graphemes, selection_start, cursor_pos)) {
          // Selection deleted
        } else {
          HandleDelete(graphemes, cursor_pos,
                       kb.modifier.ctrl || kb.modifier.alt);
        }
        // See the Backspace branch above for why this must come before the
        // `value` reassignment.
        auto pos2d = GetCursor2D(graphemes, cursor_pos);
        ideal_column_ = pos2d.column;
        value = GraphemesToString(graphemes);
        self->PropagateBinding("value", value);
        last_edit_end_pos_ = cursor_pos;
        KeepCursorVisible(root, is_multiline);
        return true;
      }
      if (is_multiline && kb.special == Event::Keyboard::Special::Tab) {
        if (readonly) {
          return true;
        }
        BeginEdit(EditKind::Other);
        if (kb.modifier.shift) {
          // Unindent: remove a tab or up to 4 spaces at the start of the line
          int line_start = FindLineStart(graphemes, cursor_pos);
          int to_remove = 0;
          if (line_start < n) {
            if (graphemes[line_start].text == "\t") {
              to_remove = 1;
            } else if (graphemes[line_start].text == " ") {
              to_remove = 1;
              while (to_remove < 4 && line_start + to_remove < n &&
                     graphemes[line_start + to_remove].text == " ") {
                to_remove++;
              }
            }
          }
          if (to_remove > 0) {
            graphemes.erase(graphemes.begin() + line_start,
                            graphemes.begin() + line_start + to_remove);
            cursor_pos = std::max(line_start, cursor_pos - to_remove);
            // See the Backspace branch above for why this must come before
            // the `value` reassignment.
            auto pos2d = GetCursor2D(graphemes, cursor_pos);
            ideal_column_ = pos2d.column;
            value = GraphemesToString(graphemes);
            self->PropagateBinding("value", value);
            KeepCursorVisible(root, is_multiline);
          }
        } else {
          // Indent: insert a tab
          std::string tab = "\t";
          auto new_graphemes = GetGraphemesList(tab);
          CapInsertionToMaxLength(new_graphemes, n, maxlength);
          graphemes.insert(graphemes.begin() + cursor_pos,
                           new_graphemes.begin(), new_graphemes.end());
          cursor_pos += static_cast<int>(new_graphemes.size());
          // See the Backspace branch above for why this must come before
          // the `value` reassignment.
          auto pos2d = GetCursor2D(graphemes, cursor_pos);
          ideal_column_ = pos2d.column;
          value = GraphemesToString(graphemes);
          self->PropagateBinding("value", value);
          KeepCursorVisible(root, is_multiline);
        }
        last_edit_end_pos_ = cursor_pos;
        return true;
      }
      if (is_multiline && kb.special == Event::Keyboard::Special::Return) {
        if (readonly) {
          return true;
        }
        BeginEdit(EditKind::Other);
        DeleteSelection(graphemes, selection_start, cursor_pos);
        n = static_cast<int>(graphemes.size());
        if (cursor_pos < 0) {
          cursor_pos = 0;
        }
        if (cursor_pos > n) {
          cursor_pos = n;
        }

        // Carry over the previous line's leading spaces/tabs so the new line
        // keeps the same indentation.
        int line_start = FindLineStart(graphemes, cursor_pos);
        int indent_end = line_start;
        while (indent_end < cursor_pos &&
               (graphemes[indent_end].text == " " ||
                graphemes[indent_end].text == "\t")) {
          indent_end++;
        }
        std::string character = "\n";
        for (int i = line_start; i < indent_end; ++i) {
          character += graphemes[i].text;
        }
        auto new_graphemes = GetGraphemesList(character);
        CapInsertionToMaxLength(new_graphemes, n, maxlength);

        graphemes.insert(graphemes.begin() + cursor_pos, new_graphemes.begin(),
                         new_graphemes.end());
        cursor_pos += static_cast<int>(new_graphemes.size());
        // See the Backspace branch above for why this must come before the
        // `value` reassignment.
        auto pos2d = GetCursor2D(graphemes, cursor_pos);
        ideal_column_ = pos2d.column;
        value = GraphemesToString(graphemes);
        self->PropagateBinding("value", value);
        last_edit_end_pos_ = cursor_pos;
        KeepCursorVisible(root, is_multiline);
        return true;
      }
      // A literal '\n' codepoint only ever arrives from pasted text (see
      // TerminalInputParser::EmitPastedText): unlike Event::Return() (the
      // "user pressed Enter" key), it must NOT carry over the current
      // line's indentation, since pasted text already has its own. For a
      // single-line field, a pasted '\n' is simply dropped, matching how a
      // real single-line input discards newlines from pasted content.
      bool is_pasted_newline = kb.codepoint == '\n' && is_multiline;
      if (kb.special == Event::Keyboard::Special::None &&
          (kb.codepoint >= 32 || is_pasted_newline) && !kb.modifier.ctrl &&
          !kb.modifier.meta) {
        if (readonly) {
          return true;
        }
        BeginEdit(kb.from_paste ? EditKind::Paste : EditKind::Insert);
        DeleteSelection(graphemes, selection_start, cursor_pos);
        n = static_cast<int>(graphemes.size());
        std::string character = CodePointToString(kb.codepoint);
        auto new_graphemes = GetGraphemesList(character);
        CapInsertionToMaxLength(new_graphemes, n, maxlength);
        if (cursor_pos < 0) {
          cursor_pos = 0;
        }
        if (cursor_pos > n) {
          cursor_pos = n;
        }

        graphemes.insert(graphemes.begin() + cursor_pos, new_graphemes.begin(),
                         new_graphemes.end());
        cursor_pos += static_cast<int>(new_graphemes.size());
        // See the Backspace branch above for why this must come before the
        // `value` reassignment.
        auto pos2d = GetCursor2D(graphemes, cursor_pos);
        ideal_column_ = pos2d.column;
        value = GraphemesToString(graphemes);
        self->PropagateBinding("value", value);
        last_edit_end_pos_ = cursor_pos;
        KeepCursorVisible(root, is_multiline);
        return true;
      }
    }
  }

  return false;
}

bool TextInputBase::DigestShared(ComponentBase* self) {
  auto* root = self->Root();
  if (root) {
    root->set_disabled(disabled);
    root->set_read_only(readonly);
  }
  if (disabled && root && root->focused()) {
    root->set_focused(false);
  }
  bool cur_focused = root ? root->focused() : false;
  is_focused_ = cur_focused;

  placeholder_text = value.empty() ? placeholder : "";

  auto graphemes = GetGraphemesList(value);
  int n = static_cast<int>(graphemes.size());
  if (cursor_pos < 0) {
    cursor_pos = 0;
  }
  if (cursor_pos > n) {
    cursor_pos = n;
  }
  if (selection_start < -1) {
    selection_start = -1;
  }
  if (selection_start > n) {
    selection_start = n;
  }

  int sel_min = cursor_pos;
  int sel_max = cursor_pos;
  if (selection_start != -1 && selection_start != cursor_pos) {
    sel_min = std::min(selection_start, cursor_pos);
    sel_max = std::max(selection_start, cursor_pos);
  }

  auto display_graphemes = graphemes;
  if (type == "password") {
    for (auto& g : display_graphemes) {
      g.text = "*";
      g.width = 1;
    }
  }

  left_unselected = GraphemesToString(display_graphemes, 0, sel_min);
  left_selected =
      GraphemesToString(display_graphemes, sel_min, cursor_pos - sel_min);
  left_text = left_unselected + left_selected;

  bool cursor_is_newline = false;
  if (cursor_pos < n && (graphemes[cursor_pos].text == "\n" ||
                         graphemes[cursor_pos].text == "\r\n" ||
                         graphemes[cursor_pos].text == "\r")) {
    cursor_is_newline = true;
  }

  if (cursor_is_newline) {
    cursor_char = " ";
    int right_sel_start = cursor_pos;
    int right_sel_count = std::max(0, sel_max - right_sel_start);
    right_selected =
        GraphemesToString(display_graphemes, right_sel_start, right_sel_count);
    right_unselected =
        GraphemesToString(display_graphemes, right_sel_start + right_sel_count);
  } else {
    cursor_char =
        (cursor_pos < n)
            ? (type == "password" ? "*"
                                  : std::string(graphemes[cursor_pos].text))
            : " ";
    int right_sel_start = (cursor_pos < n) ? cursor_pos + 1 : n;
    int right_sel_count = std::max(0, sel_max - right_sel_start);
    right_selected =
        GraphemesToString(display_graphemes, right_sel_start, right_sel_count);
    right_unselected =
        GraphemesToString(display_graphemes, right_sel_start + right_sel_count);
  }
  right_text = right_selected + right_unselected;

  bool cursor_is_selected = (cursor_pos >= sel_min && cursor_pos < sel_max);
  if (is_focused_) {
    cursor_class = cursor_is_selected ? "cursor cursor-focused selection"
                                      : "cursor cursor-focused";
  } else {
    cursor_class = cursor_is_selected ? "cursor selection" : "cursor";
  }

  selection_class_left = left_selected.empty() ? "" : "selection";
  selection_class_right = right_selected.empty() ? "" : "selection";

  return false;
}

}  // namespace rtxui
