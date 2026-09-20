// Copyright 2024 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#ifndef RTXUI_COMPONENT_DEFAULT_INPUT_TEXT_INPUT_BASE_HPP_
#define RTXUI_COMPONENT_DEFAULT_INPUT_TEXT_INPUT_BASE_HPP_

#include <chrono>
#include <string>
#include <string_view>
#include <vector>

#include "rtxui/base/string.hpp"
#include "rtxui/internal/component.hpp"

namespace rtxui {

std::vector<int> ComputeRowStarts(const std::vector<Grapheme>& graphemes,
                                  int content_width,
                                  bool overflow_wrap_normal = false);
int RowOfIndex(const std::vector<int>& row_starts, int pos);

class TextInputBase {
 public:
  std::string value;
  std::string type = "text";
  int cursor_pos = 0;
  int selection_start = -1;
  bool disabled = false;
  bool readonly = false;
  std::string placeholder;
  int maxlength = -1;  // -1 means unlimited.

  // Render bindings
  std::string left_text;
  std::string cursor_char;
  std::string right_text;
  std::string cursor_class = "cursor";

  std::string left_unselected;
  std::string left_selected;
  std::string right_selected;
  std::string right_unselected;
  std::string selection_class_left;
  std::string selection_class_right;

  // Shows `placeholder` only while `value` is empty, without ever
  // overwriting `placeholder` itself (so it survives type-then-delete).
  std::string placeholder_text;

 protected:
  bool is_focused_ = false;
  int ideal_column_ = 0;

  std::chrono::steady_clock::time_point last_click_time_ =
      std::chrono::steady_clock::time_point::min();
  int last_click_pos_ = -1;
  bool double_clicked_ = false;
  int double_click_anchor_start_ = -1;
  int double_click_anchor_end_ = -1;

  void KeepCursorVisible(Element* root, bool is_multiline);
  bool OnEventShared(ComponentBase* self, Event event, bool is_multiline);
  bool DigestShared(ComponentBase* self);

  // Converts an absolute screen coordinate into a grapheme index into
  // `value`, matching the visual position actually painted (accounting for
  // scroll, the line-number gutter, and word-wrapping). `graphemes` must be
  // `GetGraphemesList(value)`.
  int ClickToCursorPos(Element* root,
                       bool is_multiline,
                       int click_x,
                       int click_y,
                       const std::vector<Grapheme>& graphemes);

  // Undo/redo history. Contiguous edits of the same EditKind at the same
  // cursor position coalesce into a single undo step (so typing "abc"
  // then Ctrl+Z removes all three characters at once); anything else -
  // a cursor move, a selection, a paste next to manual typing, cut,
  // Tab-indent, Enter - starts a new step. See BeginEdit's definition for
  // the exact rule.
  enum class EditKind { None, Insert, Paste, Delete, Other };
  struct HistoryEntry {
    std::string value;
    int cursor_pos = 0;
    int selection_start = -1;
  };
  void BeginEdit(EditKind kind);

  std::vector<HistoryEntry> undo_stack_;
  std::vector<HistoryEntry> redo_stack_;
  EditKind last_edit_kind_ = EditKind::None;
  int last_edit_end_pos_ = -1;
};

}  // namespace rtxui

#endif  // RTXUI_COMPONENT_DEFAULT_INPUT_TEXT_INPUT_BASE_HPP_
