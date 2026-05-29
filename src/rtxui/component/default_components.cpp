#include "rtxui/component/default_components.hpp"

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>
#include "rtxui/core/string.hpp"

namespace rtxui {

namespace {

std::vector<Grapheme> GetGraphemesList(std::string_view val) {
  std::vector<Grapheme> res;
  for (const auto& g : Graphemes(val)) {
    res.push_back(g);
  }
  return res;
}

std::string GraphemesToString(const std::vector<Grapheme>& graphemes, size_t start = 0, size_t count = std::string::npos) {
  std::string s;
  size_t end = (count == std::string::npos) ? graphemes.size() : std::min(graphemes.size(), start + count);
  for (size_t i = start; i < end; ++i) {
    s.append(graphemes[i].text);
  }
  return s;
}

int FindWordBoundaryLeft(const std::vector<Grapheme>& graphemes, int start_pos) {
  int pos = start_pos;
  if (pos <= 0) return 0;
  while (pos > 0 && (graphemes[pos - 1].text == " " || graphemes[pos - 1].text == "\t")) {
    pos--;
  }
  while (pos > 0 && graphemes[pos - 1].text != " " && graphemes[pos - 1].text != "\t") {
    pos--;
  }
  return pos;
}

int FindWordBoundaryRight(const std::vector<Grapheme>& graphemes, int start_pos) {
  int pos = start_pos;
  int n = static_cast<int>(graphemes.size());
  if (pos >= n) return n;
  while (pos < n && (graphemes[pos].text == " " || graphemes[pos].text == "\t")) {
    pos++;
  }
  while (pos < n && graphemes[pos].text != " " && graphemes[pos].text != "\t") {
    pos++;
  }
  return pos;
}

void HandleBackspace(std::vector<Grapheme>& graphemes, int& cursor_pos, bool ctrl) {
  if (cursor_pos <= 0) return;
  if (ctrl) {
    int target = FindWordBoundaryLeft(graphemes, cursor_pos);
    graphemes.erase(graphemes.begin() + target, graphemes.begin() + cursor_pos);
    cursor_pos = target;
  } else {
    graphemes.erase(graphemes.begin() + cursor_pos - 1);
    cursor_pos--;
  }
}

void HandleDelete(std::vector<Grapheme>& graphemes, int& cursor_pos, bool ctrl) {
  int n = static_cast<int>(graphemes.size());
  if (cursor_pos >= n) return;
  if (ctrl) {
    int target = FindWordBoundaryRight(graphemes, cursor_pos);
    graphemes.erase(graphemes.begin() + cursor_pos, graphemes.begin() + target);
  } else {
    graphemes.erase(graphemes.begin() + cursor_pos);
  }
}

int GetCursorPositionFromColumn(const std::vector<Grapheme>& graphemes, int target_col) {
  if (target_col < 0) return 0;
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

} // namespace

void input::InitReflection() {
  Bind(value);
  Bind(left_text);
  Bind(cursor_char);
  Bind(right_text);
  Bind(cursor_class);
  Component<input>::InitReflection();
}

std::string_view input::Setup() {
  return R"html(
    <span>{left_text}</span><span class="{cursor_class}">{cursor_char}</span><span>{right_text}</span>
    <style>
      self {
        display: inline flex;
        flex-direction: row;
        border: solid;
        border-color: #555;
        padding-left: 1;
        padding-right: 1;
        overflow-x: scroll;
        scrollbar-width: none;
        white-space: nowrap;
      }
      .cursor {
        background-color: transparent;
      }
      .cursor-focused {
        background-color: white;
        color: black;
      }
    </style>
  )html";
}

bool input::OnEvent(Event event) {
  if (event.is<Event::Mouse>()) {
    auto mouse = event.get<Event::Mouse>();
    if (mouse.button == Event::Mouse::Button::Left && mouse.motion == Event::Mouse::Motion::Pressed) {
      if (!Root()) return false;
      int click_x = mouse.x - 1;
      int click_y = mouse.y - 1;
      int abs_x = Root()->absolute_x();
      int abs_y = Root()->absolute_y();
      int layout_w = Root()->layout_width();
      int layout_h = Root()->layout_height();

      if (click_x >= abs_x && click_x < abs_x + layout_w &&
          click_y >= abs_y && click_y < abs_y + layout_h) {
        
        // Unfocus all other elements
        if (Root()->Parent()) {
          Element* root_el = Root();
          while (root_el->Parent()) {
            root_el = root_el->Parent();
          }
          root_el->Visit([](Element& el) {
            el.set_focused(false);
          });
        }
        Root()->set_focused(true);

        int border_offset = (Root()->style.border_style != BorderStyle::None) ? 1 : 0;
        int padding_left = Root()->style.padding.left;
        int inner_click_x = click_x - abs_x - border_offset - padding_left;
        int target_col = inner_click_x + Root()->scroll_x();

        auto graphemes = GetGraphemesList(value);
        cursor_pos = GetCursorPositionFromColumn(graphemes, target_col);

        // Keep cursor visible after move
        KeepCursorVisible();
        return true;
      }
    }
    return false;
  }

  if (event.is<Event::Keyboard>()) {
    auto kb = event.get<Event::Keyboard>();
    if (kb.motion == Event::Keyboard::Motion::Pressed || kb.motion == Event::Keyboard::Motion::Repeat) {
      if (!Root() || !Root()->focused()) {
        return false;
      }

      auto graphemes = GetGraphemesList(value);
      int n = static_cast<int>(graphemes.size());

      if (kb.special == Event::Keyboard::Special::ArrowLeft) {
        if (kb.modifier.ctrl) {
          cursor_pos = FindWordBoundaryLeft(graphemes, cursor_pos);
        } else {
          cursor_pos = std::max(0, cursor_pos - 1);
        }
        KeepCursorVisible();
        return true;
      }
      if (kb.special == Event::Keyboard::Special::ArrowRight) {
        if (kb.modifier.ctrl) {
          cursor_pos = FindWordBoundaryRight(graphemes, cursor_pos);
        } else {
          cursor_pos = std::min(n, cursor_pos + 1);
        }
        KeepCursorVisible();
        return true;
      }
      if (kb.special == Event::Keyboard::Special::Backspace) {
        HandleBackspace(graphemes, cursor_pos, kb.modifier.ctrl);
        value = GraphemesToString(graphemes);
        PropagateBinding("value", value);
        KeepCursorVisible();
        return true;
      }
      if (kb.special == Event::Keyboard::Special::Delete) {
        HandleDelete(graphemes, cursor_pos, kb.modifier.ctrl);
        value = GraphemesToString(graphemes);
        PropagateBinding("value", value);
        KeepCursorVisible();
        return true;
      }
      if (kb.special == Event::Keyboard::Special::None && kb.codepoint >= 32 && !kb.modifier.ctrl && !kb.modifier.meta) {
        std::string character = CodePointToString(kb.codepoint);
        auto new_graphemes = GetGraphemesList(character);
        if (cursor_pos < 0) cursor_pos = 0;
        if (cursor_pos > n) cursor_pos = n;

        graphemes.insert(graphemes.begin() + cursor_pos, new_graphemes.begin(), new_graphemes.end());
        cursor_pos += static_cast<int>(new_graphemes.size());
        value = GraphemesToString(graphemes);
        PropagateBinding("value", value);
        KeepCursorVisible();
        return true;
      }
    }
  }

  return false;
}

bool input::Digest() {
  bool changed = Component<input>::Digest();

  bool cur_focused = Root() ? Root()->focused() : false;
  if (cur_focused != is_focused_) {
    is_focused_ = cur_focused;
    changed = true;
  }

  auto graphemes = GetGraphemesList(value);
  int n = static_cast<int>(graphemes.size());
  if (cursor_pos < 0) {
    cursor_pos = 0;
    changed = true;
  }
  if (cursor_pos > n) {
    cursor_pos = n;
    changed = true;
  }

  std::string new_left = GraphemesToString(graphemes, 0, cursor_pos);
  std::string new_cursor = (cursor_pos < n) ? std::string(graphemes[cursor_pos].text) : " ";
  std::string new_right = GraphemesToString(graphemes, cursor_pos + 1);
  std::string new_class = is_focused_ ? "cursor cursor-focused" : "cursor";

  if (new_left != left_text || new_cursor != cursor_char || new_right != right_text || new_class != cursor_class) {
    left_text = std::move(new_left);
    cursor_char = std::move(new_cursor);
    right_text = std::move(new_right);
    cursor_class = std::move(new_class);
    changed = true;
  }

  if (changed) {
    KeepCursorVisible();
  }

  return changed;
}

void input::KeepCursorVisible() {
  if (!Root()) return;

  auto current_graphemes = GetGraphemesList(value);
  int cursor_col = 0;
  int limit = std::min(static_cast<int>(current_graphemes.size()), cursor_pos);
  for (int i = 0; i < limit; ++i) {
    cursor_col += current_graphemes[i].width;
  }

  int border_offset = (Root()->style.border_style != BorderStyle::None) ? 1 : 0;
  int padding_left = Root()->style.padding.left;
  int padding_right = Root()->style.padding.right;
  int border_horiz = border_offset * 2;
  int padding_horiz = padding_left + padding_right;

  int layout_w = Root()->layout_width();
  int visible_width = layout_w - border_horiz - padding_horiz;
  if (visible_width <= 0) {
    return;
  }

  int curr_scroll_x = Root()->scroll_x();
  if (cursor_col < curr_scroll_x) {
    Root()->set_scroll_x(cursor_col);
  } else if (cursor_col >= curr_scroll_x + visible_width) {
    Root()->set_scroll_x(cursor_col - visible_width + 1);
  }
}

}  // namespace rtxui
