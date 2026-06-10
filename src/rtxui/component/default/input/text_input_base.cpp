// Copyright 2024 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include "rtxui/component/default/input/text_input_base.hpp"

#include <algorithm>
#include <cmath>

#include "rtxui/core/string.hpp"
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

}  // namespace

void TextInputBase::KeepCursorVisible(Element* root, bool is_multiline) {
  if (!root) {
    return;
  }

  auto current_graphemes = GetGraphemesList(value);
  auto pos2d = GetCursor2D(current_graphemes, cursor_pos);
  int cursor_line = pos2d.line;
  int cursor_col = pos2d.column;

  int border_offset = (root->style.border_style != BorderStyle::None) ? 1 : 0;

  // Horizontal Scroll
  int padding_left = root->style.padding.left;
  int padding_right = root->style.padding.right;
  int border_horiz = border_offset * 2;
  int padding_horiz = padding_left + padding_right;

  int layout_w = root->layout_width();
  int visible_width = layout_w - border_horiz - padding_horiz;
  if (visible_width > 0) {
    int curr_scroll_x = root->scroll_x();
    if (cursor_col < curr_scroll_x) {
      root->set_scroll_x(cursor_col);
    } else if (cursor_col >= curr_scroll_x + visible_width) {
      root->set_scroll_x(cursor_col - visible_width + 1);
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

bool TextInputBase::OnEventShared(ComponentBase* self,
                                  Event event,
                                  bool is_multiline) {
  auto* root = self->Root();
  if (event.is<Event::Mouse>()) {
    auto mouse = event.get<Event::Mouse>();
    if (mouse.button == Event::Mouse::Button::Left &&
        mouse.motion == Event::Mouse::Motion::Pressed) {
      if (!root) {
        return false;
      }
      int click_x = mouse.x - 1;
      int click_y = mouse.y - 1;
      int abs_x = root->absolute_x();
      int abs_y = root->absolute_y();
      int layout_w = root->layout_width();
      int layout_h = root->layout_height();

      if (click_x >= abs_x && click_x < abs_x + layout_w && click_y >= abs_y &&
          click_y < abs_y + layout_h) {
        // Unfocus all other elements
        if (root->Parent()) {
          Element* root_el = root;
          while (root_el->Parent()) {
            root_el = root_el->Parent();
          }
          root_el->Visit([](Element& el) { el.set_focused(false); });
        }
        root->set_focused(true);

        int border_offset =
            (root->style.border_style != BorderStyle::None) ? 1 : 0;
        int padding_left = root->style.padding.left;
        int padding_top = root->style.padding.top;
        int inner_click_x = click_x - abs_x - border_offset - padding_left;
        int inner_click_y = click_y - abs_y - border_offset - padding_top;

        int target_col = inner_click_x + root->scroll_x();
        int target_row = is_multiline ? (inner_click_y + root->scroll_y()) : 0;

        auto graphemes = GetGraphemesList(value);
        cursor_pos = is_multiline
                         ? GetCursorPosFrom2D(graphemes, target_row, target_col)
                         : GetCursorPositionFromColumn(graphemes, target_col);

        auto pos2d = GetCursor2D(graphemes, cursor_pos);
        ideal_column_ = pos2d.column;

        KeepCursorVisible(root, is_multiline);
        return true;
      }
    }
    return false;
  }

  if (event.is<Event::Keyboard>()) {
    auto kb = event.get<Event::Keyboard>();
    if (kb.motion == Event::Keyboard::Motion::Pressed ||
        kb.motion == Event::Keyboard::Motion::Repeat) {
      if (!root || !root->focused()) {
        return false;
      }

      auto graphemes = GetGraphemesList(value);
      int n = static_cast<int>(graphemes.size());

      if (kb.special == Event::Keyboard::Special::ArrowLeft) {
        if (kb.modifier.ctrl || kb.modifier.alt) {
          cursor_pos = FindWordBoundaryLeft(graphemes, cursor_pos);
        } else {
          cursor_pos = std::max(0, cursor_pos - 1);
        }
        auto pos2d = GetCursor2D(graphemes, cursor_pos);
        ideal_column_ = pos2d.column;
        KeepCursorVisible(root, is_multiline);
        return true;
      }
      if (kb.special == Event::Keyboard::Special::ArrowRight) {
        if (kb.modifier.ctrl || kb.modifier.alt) {
          cursor_pos = FindWordBoundaryRight(graphemes, cursor_pos);
        } else {
          cursor_pos = std::min(n, cursor_pos + 1);
        }
        auto pos2d = GetCursor2D(graphemes, cursor_pos);
        ideal_column_ = pos2d.column;
        KeepCursorVisible(root, is_multiline);
        return true;
      }
      if (is_multiline && kb.special == Event::Keyboard::Special::ArrowUp) {
        auto pos2d = GetCursor2D(graphemes, cursor_pos);
        cursor_pos =
            GetCursorPosFrom2D(graphemes, pos2d.line - 1, ideal_column_);
        KeepCursorVisible(root, is_multiline);
        return true;
      }
      if (is_multiline && kb.special == Event::Keyboard::Special::ArrowDown) {
        auto pos2d = GetCursor2D(graphemes, cursor_pos);
        cursor_pos =
            GetCursorPosFrom2D(graphemes, pos2d.line + 1, ideal_column_);
        KeepCursorVisible(root, is_multiline);
        return true;
      }
      if (kb.special == Event::Keyboard::Special::Home) {
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
        HandleBackspace(graphemes, cursor_pos,
                        kb.modifier.ctrl || kb.modifier.alt);
        value = GraphemesToString(graphemes);
        self->PropagateBinding("value", value);
        auto pos2d = GetCursor2D(graphemes, cursor_pos);
        ideal_column_ = pos2d.column;
        KeepCursorVisible(root, is_multiline);
        return true;
      }
      if (kb.special == Event::Keyboard::Special::Delete) {
        HandleDelete(graphemes, cursor_pos,
                     kb.modifier.ctrl || kb.modifier.alt);
        value = GraphemesToString(graphemes);
        self->PropagateBinding("value", value);
        auto pos2d = GetCursor2D(graphemes, cursor_pos);
        ideal_column_ = pos2d.column;
        KeepCursorVisible(root, is_multiline);
        return true;
      }
      if (is_multiline && kb.special == Event::Keyboard::Special::Tab) {
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
            value = GraphemesToString(graphemes);
            self->PropagateBinding("value", value);
            auto pos2d = GetCursor2D(graphemes, cursor_pos);
            ideal_column_ = pos2d.column;
            KeepCursorVisible(root, is_multiline);
          }
        } else {
          // Indent: insert a tab
          std::string tab = "\t";
          auto new_graphemes = GetGraphemesList(tab);
          graphemes.insert(graphemes.begin() + cursor_pos,
                           new_graphemes.begin(), new_graphemes.end());
          cursor_pos += static_cast<int>(new_graphemes.size());
          value = GraphemesToString(graphemes);
          self->PropagateBinding("value", value);
          auto pos2d = GetCursor2D(graphemes, cursor_pos);
          ideal_column_ = pos2d.column;
          KeepCursorVisible(root, is_multiline);
        }
        return true;
      }
      if (is_multiline && kb.special == Event::Keyboard::Special::Return) {
        std::string character = "\n";
        auto new_graphemes = GetGraphemesList(character);
        if (cursor_pos < 0) {
          cursor_pos = 0;
        }
        if (cursor_pos > n) {
          cursor_pos = n;
        }

        graphemes.insert(graphemes.begin() + cursor_pos, new_graphemes.begin(),
                         new_graphemes.end());
        cursor_pos += static_cast<int>(new_graphemes.size());
        value = GraphemesToString(graphemes);
        self->PropagateBinding("value", value);

        auto pos2d = GetCursor2D(graphemes, cursor_pos);
        ideal_column_ = pos2d.column;
        KeepCursorVisible(root, is_multiline);
        return true;
      }
      if (kb.special == Event::Keyboard::Special::None && kb.codepoint >= 32 &&
          !kb.modifier.ctrl && !kb.modifier.meta) {
        std::string character = CodePointToString(kb.codepoint);
        auto new_graphemes = GetGraphemesList(character);
        if (cursor_pos < 0) {
          cursor_pos = 0;
        }
        if (cursor_pos > n) {
          cursor_pos = n;
        }

        graphemes.insert(graphemes.begin() + cursor_pos, new_graphemes.begin(),
                         new_graphemes.end());
        cursor_pos += static_cast<int>(new_graphemes.size());
        value = GraphemesToString(graphemes);
        self->PropagateBinding("value", value);

        auto pos2d = GetCursor2D(graphemes, cursor_pos);
        ideal_column_ = pos2d.column;
        KeepCursorVisible(root, is_multiline);
        return true;
      }
    }
  }

  return false;
}

bool TextInputBase::DigestShared(ComponentBase* self) {
  auto* root = self->Root();
  bool cur_focused = root ? root->focused() : false;
  is_focused_ = cur_focused;

  auto graphemes = GetGraphemesList(value);
  int n = static_cast<int>(graphemes.size());
  if (cursor_pos < 0) {
    cursor_pos = 0;
  }
  if (cursor_pos > n) {
    cursor_pos = n;
  }

  left_text = GraphemesToString(graphemes, 0, cursor_pos);
  if (cursor_pos < n && (graphemes[cursor_pos].text == "\n" ||
                         graphemes[cursor_pos].text == "\r\n" ||
                         graphemes[cursor_pos].text == "\r")) {
    cursor_char = " ";
    right_text = GraphemesToString(graphemes, cursor_pos);
  } else {
    cursor_char =
        (cursor_pos < n) ? std::string(graphemes[cursor_pos].text) : " ";
    right_text = (cursor_pos < n) ? GraphemesToString(graphemes, cursor_pos + 1) : "";
  }
  if (is_focused_) {
    cursor_class = "cursor cursor-focused";
  } else {
    cursor_class = "cursor";
  }

  return false;
}

}  // namespace rtxui
