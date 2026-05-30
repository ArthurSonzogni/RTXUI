#include "rtxui/internal/default_components.hpp"

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>
#include "rtxui/core/string.hpp"
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

struct Position2D {
  int line = 0;
  int column = 0;
};

Position2D GetCursor2D(const std::vector<Grapheme>& graphemes, int cursor_pos) {
  Position2D pos;
  int limit = std::min(static_cast<int>(graphemes.size()), cursor_pos);
  for (int i = 0; i < limit; ++i) {
    if (graphemes[i].text == "\n" || graphemes[i].text == "\r\n" || graphemes[i].text == "\r") {
      pos.line++;
      pos.column = 0;
    } else {
      pos.column += graphemes[i].width;
    }
  }
  return pos;
}

int GetCursorPosFrom2D(const std::vector<Grapheme>& graphemes, int target_line, int target_col) {
  int cur_line = 0;
  int cur_col = 0;
  int best_pos = 0;
  int min_dist = -1;
  int n = static_cast<int>(graphemes.size());

  int i = 0;
  while (i < n && cur_line < target_line) {
    if (graphemes[i].text == "\n" || graphemes[i].text == "\r\n" || graphemes[i].text == "\r") {
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
    if (graphemes[i].text == "\n" || graphemes[i].text == "\r\n" || graphemes[i].text == "\r") {
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
    if (graphemes[pos - 1].text == "\n" || graphemes[pos - 1].text == "\r\n" || graphemes[pos - 1].text == "\r") {
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
    if (graphemes[pos].text == "\n" || graphemes[pos].text == "\r\n" || graphemes[pos].text == "\r") {
      break;
    }
    pos++;
  }
  return pos;
}

void TextInputBase::KeepCursorVisible(Element* root, bool is_multiline) {
  if (!root) return;

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

bool TextInputBase::OnEventShared(ComponentBase* self, Event event, bool is_multiline) {
  auto* root = self->Root();
  if (event.is<Event::Mouse>()) {
    auto mouse = event.get<Event::Mouse>();
    if (mouse.button == Event::Mouse::Button::Left && mouse.motion == Event::Mouse::Motion::Pressed) {
      if (!root) return false;
      int click_x = mouse.x - 1;
      int click_y = mouse.y - 1;
      int abs_x = root->absolute_x();
      int abs_y = root->absolute_y();
      int layout_w = root->layout_width();
      int layout_h = root->layout_height();

      if (click_x >= abs_x && click_x < abs_x + layout_w &&
          click_y >= abs_y && click_y < abs_y + layout_h) {
        
        // Unfocus all other elements
        if (root->Parent()) {
          Element* root_el = root;
          while (root_el->Parent()) {
            root_el = root_el->Parent();
          }
          root_el->Visit([](Element& el) {
            el.set_focused(false);
          });
        }
        root->set_focused(true);

        int border_offset = (root->style.border_style != BorderStyle::None) ? 1 : 0;
        int padding_left = root->style.padding.left;
        int padding_top = root->style.padding.top;
        int inner_click_x = click_x - abs_x - border_offset - padding_left;
        int inner_click_y = click_y - abs_y - border_offset - padding_top;

        int target_col = inner_click_x + root->scroll_x();
        int target_row = is_multiline ? (inner_click_y + root->scroll_y()) : 0;

        auto graphemes = GetGraphemesList(value);
        cursor_pos = is_multiline ? GetCursorPosFrom2D(graphemes, target_row, target_col)
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
    if (kb.motion == Event::Keyboard::Motion::Pressed || kb.motion == Event::Keyboard::Motion::Repeat) {
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
        cursor_pos = GetCursorPosFrom2D(graphemes, pos2d.line - 1, ideal_column_);
        KeepCursorVisible(root, is_multiline);
        return true;
      }
      if (is_multiline && kb.special == Event::Keyboard::Special::ArrowDown) {
        auto pos2d = GetCursor2D(graphemes, cursor_pos);
        cursor_pos = GetCursorPosFrom2D(graphemes, pos2d.line + 1, ideal_column_);
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
        HandleBackspace(graphemes, cursor_pos, kb.modifier.ctrl || kb.modifier.alt);
        value = GraphemesToString(graphemes);
        self->PropagateBinding("value", value);
        auto pos2d = GetCursor2D(graphemes, cursor_pos);
        ideal_column_ = pos2d.column;
        KeepCursorVisible(root, is_multiline);
        return true;
      }
      if (kb.special == Event::Keyboard::Special::Delete) {
        HandleDelete(graphemes, cursor_pos, kb.modifier.ctrl || kb.modifier.alt);
        value = GraphemesToString(graphemes);
        self->PropagateBinding("value", value);
        auto pos2d = GetCursor2D(graphemes, cursor_pos);
        ideal_column_ = pos2d.column;
        KeepCursorVisible(root, is_multiline);
        return true;
      }
      if (is_multiline && kb.special == Event::Keyboard::Special::Return) {
        std::string character = "\n";
        auto new_graphemes = GetGraphemesList(character);
        if (cursor_pos < 0) cursor_pos = 0;
        if (cursor_pos > n) cursor_pos = n;

        graphemes.insert(graphemes.begin() + cursor_pos, new_graphemes.begin(), new_graphemes.end());
        cursor_pos += static_cast<int>(new_graphemes.size());
        value = GraphemesToString(graphemes);
        self->PropagateBinding("value", value);

        auto pos2d = GetCursor2D(graphemes, cursor_pos);
        ideal_column_ = pos2d.column;
        KeepCursorVisible(root, is_multiline);
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
  if (cursor_pos < n && (graphemes[cursor_pos].text == "\n" || graphemes[cursor_pos].text == "\r\n" || graphemes[cursor_pos].text == "\r")) {
    cursor_char = " ";
    right_text = GraphemesToString(graphemes, cursor_pos);
  } else {
    cursor_char = (cursor_pos < n) ? std::string(graphemes[cursor_pos].text) : " ";
    right_text = GraphemesToString(graphemes, cursor_pos + 1);
  }
  cursor_class = is_focused_ ? "cursor cursor-focused" : "cursor";

  return false;
}

void input::InitReflection() {
  Bind(value);
  Bind(left_text);
  Bind(cursor_char);
  Bind(right_text);
  Bind(cursor_class);
  Component<input>::InitReflection();
}

std::string_view input::Setup() {
  return R"html(<span>{left_text}</span><span class="{cursor_class}">{cursor_char}</span><span>{right_text}</span><style>
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
    </style>)html";
}

bool input::OnEvent(Event event) {
  return OnEventShared(this, event, false);
}

bool input::Digest() {
  DigestShared(this);
  bool changed = Component<input>::Digest();
  if (changed) {
    KeepCursorVisible(Root(), false);
  }
  return changed;
}

void textarea::InitReflection() {
  Bind(value);
  Bind(left_text);
  Bind(cursor_char);
  Bind(right_text);
  Bind(cursor_class);
  Component<textarea>::InitReflection();
}

std::string_view textarea::Setup() {
  return R"html(<span>{left_text}</span><span class="{cursor_class}">{cursor_char}</span><span>{right_text}</span><style>
      self {
        display: block;
        border: solid;
        border-color: #555;
        padding-left: 1;
        padding-right: 1;
        overflow-y: scroll;
      }
      .cursor {
        background-color: transparent;
      }
      .cursor-focused {
        background-color: white;
        color: black;
      }
    </style>)html";
}

bool textarea::OnEvent(Event event) {
  return OnEventShared(this, event, true);
}

bool textarea::Digest() {
  DigestShared(this);
  bool changed = Component<textarea>::Digest();
  if (changed) {
    KeepCursorVisible(Root(), true);
  }
  return changed;
}

void checkbox::InitReflection() {
  Bind(checked);
  Bind(checked_char);
  Bind(focus_class);
  Component<checkbox>::InitReflection();
}

std::string_view checkbox::Setup() {
  return R"html(<span class="{focus_class}">[<span class="checkmark">{checked_char}</span>] <slot></slot></span><style>
      self {
        display: inline-block;
        cursor: pointer;
      }
      .focused {
        background-color: #333;
        color: #fff;
      }
      .checkmark {
        font-weight: bold;
        color: #38bdf8;
      }
    </style>)html";
}

bool checkbox::OnEvent(Event event) {
  auto* root = Root();
  if (event.is<Event::Mouse>()) {
    auto mouse = event.get<Event::Mouse>();
    if (mouse.button == Event::Mouse::Button::Left && mouse.motion == Event::Mouse::Motion::Pressed) {
      if (!root) return false;
      int click_x = mouse.x - 1;
      int click_y = mouse.y - 1;
      int abs_x = root->absolute_x();
      int abs_y = root->absolute_y();
      int layout_w = root->layout_width();
      int layout_h = root->layout_height();

      if (click_x >= abs_x && click_x < abs_x + layout_w &&
          click_y >= abs_y && click_y < abs_y + layout_h) {
        
        // Focus this element
        if (root->Parent()) {
          Element* root_el = root;
          while (root_el->Parent()) {
            root_el = root_el->Parent();
          }
          root_el->Visit([](Element& el) {
            el.set_focused(false);
          });
        }
        root->set_focused(true);

        checked = !checked;
        PropagateBinding("checked", checked ? "true" : "false");

        // Run onchange callback if present
        if (root->Attributes().count("onchange")) {
          std::string onchange_cb = root->Attributes().at("onchange");
          Element* parent_el = root->Parent();
          ComponentBase* parent_comp = nullptr;
          while (parent_el) {
            if (parent_el->component()) {
              parent_comp = const_cast<ComponentBase*>(parent_el->component());
              break;
            }
            parent_el = parent_el->Parent();
          }
          while (parent_comp) {
            if (parent_comp->RunCallback(onchange_cb)) {
              break;
            }
            if (parent_comp->Root()) {
              parent_el = parent_comp->Root()->Parent();
              parent_comp = nullptr;
              while (parent_el) {
                if (parent_el->component()) {
                  parent_comp = const_cast<ComponentBase*>(parent_el->component());
                  break;
                }
                parent_el = parent_el->Parent();
              }
            } else {
              break;
            }
          }
        }

        return true;
      }
    }
  }

  if (event.is<Event::Keyboard>()) {
    auto kb = event.get<Event::Keyboard>();
    if (kb.motion == Event::Keyboard::Motion::Pressed || kb.motion == Event::Keyboard::Motion::Repeat) {
      if (!root || !root->focused()) {
        return false;
      }

      if (kb.special == Event::Keyboard::Special::None && kb.codepoint == 32) { // Space
        checked = !checked;
        PropagateBinding("checked", checked ? "true" : "false");

        // Run onchange callback if present
        if (root->Attributes().count("onchange")) {
          std::string onchange_cb = root->Attributes().at("onchange");
          Element* parent_el = root->Parent();
          ComponentBase* parent_comp = nullptr;
          while (parent_el) {
            if (parent_el->component()) {
              parent_comp = const_cast<ComponentBase*>(parent_el->component());
              break;
            }
            parent_el = parent_el->Parent();
          }
          while (parent_comp) {
            if (parent_comp->RunCallback(onchange_cb)) {
              break;
            }
            if (parent_comp->Root()) {
              parent_el = parent_comp->Root()->Parent();
              parent_comp = nullptr;
              while (parent_el) {
                if (parent_el->component()) {
                  parent_comp = const_cast<ComponentBase*>(parent_el->component());
                  break;
                }
                parent_el = parent_el->Parent();
              }
            } else {
              break;
            }
          }
        }

        return true;
      }
    }
  }

  return false;
}

bool checkbox::Digest() {
  auto* root = Root();
  bool is_focused = root ? root->focused() : false;
  focus_class = is_focused ? "focused" : "";
  checked_char = checked ? "x" : " ";
  return Component<checkbox>::Digest();
}

void slider::InitReflection() {
  Bind(value);
  Bind(min);
  Bind(max);
  Bind(step);
  Bind(width);
  Bind(track_left);
  Bind(thumb_char);
  Bind(track_right);
  Bind(focus_class);
  Component<slider>::InitReflection();
}

std::string_view slider::Setup() {
  return R"html(<span class="{focus_class}"><span class="track-left">{track_left}</span><span class="thumb">{thumb_char}</span><span class="track-right">{track_right}</span></span><style>
      self {
        display: inline-block;
        cursor: pointer;
      }
      .focused {
        background-color: #333;
        color: #fff;
      }
      .track-left {
        color: #38bdf8;
      }
      .track-right {
        color: #555;
      }
      .thumb {
        font-weight: bold;
        color: #38bdf8;
      }
    </style>)html";
}

bool slider::OnEvent(Event event) {
  auto* root = Root();
  if (!root) return false;

  bool value_changed = false;

  if (event.is<Event::Mouse>()) {
    auto mouse = event.get<Event::Mouse>();
    if (mouse.button == Event::Mouse::Button::Left && mouse.motion == Event::Mouse::Motion::Pressed) {
      int click_x = mouse.x - 1;
      int click_y = mouse.y - 1;
      int abs_x = root->absolute_x();
      int abs_y = root->absolute_y();
      int layout_w = root->layout_width();
      int layout_h = root->layout_height();

      if (click_x >= abs_x && click_x < abs_x + layout_w &&
          click_y >= abs_y && click_y < abs_y + layout_h) {
        
        // Focus this element
        if (root->Parent()) {
          Element* root_el = root;
          while (root_el->Parent()) {
            root_el = root_el->Parent();
          }
          root_el->Visit([](Element& el) {
            el.set_focused(false);
          });
        }
        root->set_focused(true);

        // Click positioning logic: width of slider track
        int inner_x = click_x - abs_x;
        int track_w = std::max(2, width);
        int pos = std::clamp(inner_x, 0, track_w - 1);
        
        // Map pos to [min, max]
        double pct = static_cast<double>(pos) / (track_w - 1);
        int raw_val = min + static_cast<int>(std::round(pct * (max - min)));
        
        // Snap to nearest step
        int remainder = (raw_val - min) % step;
        int new_val = raw_val;
        if (remainder < step / 2.0) {
          new_val = raw_val - remainder;
        } else {
          new_val = raw_val + (step - remainder);
        }
        new_val = std::clamp(new_val, min, max);

        if (new_val != value) {
          value = new_val;
          value_changed = true;
        }
      }
    }
  }

  if (event.is<Event::Keyboard>()) {
    auto kb = event.get<Event::Keyboard>();
    if (kb.motion == Event::Keyboard::Motion::Pressed || kb.motion == Event::Keyboard::Motion::Repeat) {
      if (root->focused()) {
        int delta = 0;
        if (kb.special == Event::Keyboard::Special::ArrowLeft || kb.special == Event::Keyboard::Special::ArrowDown) {
          delta = -step;
        } else if (kb.special == Event::Keyboard::Special::ArrowRight || kb.special == Event::Keyboard::Special::ArrowUp) {
          delta = step;
        }

        if (delta != 0) {
          int new_val = std::clamp(value + delta, min, max);
          if (new_val != value) {
            value = new_val;
            value_changed = true;
          }
        }
      }
    }
  }

  if (value_changed) {
    PropagateBinding("value", std::to_string(value));

    // Run onchange callback if present
    if (root->Attributes().count("onchange")) {
      std::string onchange_cb = root->Attributes().at("onchange");
      Element* parent_el = root->Parent();
      ComponentBase* parent_comp = nullptr;
      while (parent_el) {
        if (parent_el->component()) {
          parent_comp = const_cast<ComponentBase*>(parent_el->component());
          break;
        }
        parent_el = parent_el->Parent();
      }
      while (parent_comp) {
        if (parent_comp->RunCallback(onchange_cb)) {
          break;
        }
        if (parent_comp->Root()) {
          parent_el = parent_comp->Root()->Parent();
          parent_comp = nullptr;
          while (parent_el) {
            if (parent_el->component()) {
              parent_comp = const_cast<ComponentBase*>(parent_el->component());
              break;
            }
            parent_el = parent_el->Parent();
          }
        } else {
          break;
        }
      }
    }
    return true;
  }

  return false;
}

bool slider::Digest() {
  auto* root = Root();
  bool is_focused = root ? root->focused() : false;
  focus_class = is_focused ? "focused" : "";

  int track_w = std::max(2, width);
  int range = max - min;
  int pos = 0;
  if (range > 0) {
    pos = static_cast<int>(std::round(static_cast<double>(value - min) / range * (track_w - 1)));
  }
  pos = std::clamp(pos, 0, track_w - 1);

  track_left = "";
  for (int i = 0; i < pos; ++i) {
    track_left += "─";
  }
  thumb_char = "●";
  track_right = "";
  for (int i = pos + 1; i < track_w; ++i) {
    track_right += "─";
  }

  return Component<slider>::Digest();
}

void progress::InitReflection() {
  Bind(value);
  Bind(max);
  Bind(width);
  Bind(filled_track);
  Bind(empty_track);
  Component<progress>::InitReflection();
}

std::string_view progress::Setup() {
  return R"html(<span class="filled">{filled_track}</span><span class="empty">{empty_track}</span><style>
      self {
        display: inline-block;
      }
      .filled {
        color: #38bdf8;
      }
      .empty {
        color: #444;
      }
    </style>)html";
}

bool progress::Digest() {
  int track_w = std::max(1, width);
  double range = max;
  int pos = 0;
  if (range > 0) {
    pos = static_cast<int>(std::round(std::clamp(value / range, 0.0, 1.0) * track_w));
  }
  pos = std::clamp(pos, 0, track_w);

  filled_track = "";
  for (int i = 0; i < pos; ++i) {
    filled_track += "█";
  }
  empty_track = "";
  for (int i = pos; i < track_w; ++i) {
    empty_track += " ";
  }

  return Component<progress>::Digest();
}

void select::InitReflection() {
  Bind(value);
  Bind(selected_label);
  Bind(arrow_char);
  Bind(dropdown_class);
  Bind(focus_class);
  Component<select>::InitReflection();
}

std::string_view select::Setup() {
  return R"html(<div class="select-btn {focus_class}">
      <span class="select-label">{selected_label}</span>
      <span class="select-arrow">{arrow_char}</span>
    </div>
    <div class="dropdown-list {dropdown_class}">
      <slot></slot>
    </div>
  <style>
    self {
      display: inline flex;
      flex-direction: column;
    }
    .select-btn {
      display: flex;
      flex-direction: row;
      justify-content: space-between;
      border: solid;
      border-color: #555;
      background-color: #1e293b;
      color: white;
      padding-left: 1;
      padding-right: 1;
      cursor: pointer;
    }
    .focused {
      border-color: #38bdf8;
    }
    .dropdown-list {
      display: flex;
      flex-direction: column;
      border: solid;
      border-top: none;
      border-color: #555;
      background-color: #0f172a;
    }
    .closed {
      display: none;
    }
  </style>)html";
}

std::vector<OptionInfo> select::GetOptions() {
  std::vector<OptionInfo> opts;
  auto* root = Root();
  if (!root) return opts;

  root->Visit([&](Element& el) {
    if (el.tag() == "option") {
      std::string val;
      if (el.Attributes().count("value")) {
        val = el.Attributes().at("value");
      }
      std::string label;
      el.Visit([&](Element& child) {
        if (child.is_text()) {
          label += static_cast<TextElement&>(child).text();
        }
      });
      opts.push_back({val, label, &el});
    }
  });
  std::cout << "GET OPTIONS SIZE: " << opts.size() << " (value=" << value << ")" << std::endl;
  for (auto& opt : opts) {
    std::cout << "  OPT value=" << opt.value << " label=" << opt.label << std::endl;
  }
  return opts;
}

bool select::OnEvent(Event event) {
  auto* root = Root();
  if (!root) return false;

  bool changed = false;

  if (event.is<Event::Mouse>()) {
    auto mouse = event.get<Event::Mouse>();
    if (mouse.button == Event::Mouse::Button::Left && mouse.motion == Event::Mouse::Motion::Pressed) {
      int click_x = mouse.x - 1;
      int click_y = mouse.y - 1;
      int abs_x = root->absolute_x();
      int abs_y = root->absolute_y();
      int layout_w = root->layout_width();
      int layout_h = root->layout_height();

      if (click_x >= abs_x && click_x < abs_x + layout_w &&
          click_y >= abs_y && click_y < abs_y + layout_h) {
        
        // Focus this element
        if (root->Parent()) {
          Element* root_el = root;
          while (root_el->Parent()) {
            root_el = root_el->Parent();
          }
          root_el->Visit([](Element& el) {
            el.set_focused(false);
          });
        }
        root->set_focused(true);

        is_open = !is_open;
        if (is_open) {
          auto options = GetOptions();
          hovered_index = 0;
          for (int i = 0; i < static_cast<int>(options.size()); ++i) {
            if (options[i].value == value) {
              hovered_index = i;
              break;
            }
          }
        }
        changed = true;
      } else {
        if (is_open) {
          is_open = false;
          changed = true;
        }
      }
    }
  }

  if (event.is<Event::Keyboard>()) {
    auto kb = event.get<Event::Keyboard>();
    if (kb.motion == Event::Keyboard::Motion::Pressed || kb.motion == Event::Keyboard::Motion::Repeat) {
      if (root->focused()) {
        auto options = GetOptions();
        if (is_open) {
          if (kb.special == Event::Keyboard::Special::ArrowDown) {
            if (!options.empty()) {
              hovered_index = (hovered_index + 1) % options.size();
              changed = true;
            }
            return true;
          }
          if (kb.special == Event::Keyboard::Special::ArrowUp) {
            if (!options.empty()) {
              hovered_index = (hovered_index - 1 + options.size()) % options.size();
              changed = true;
            }
            return true;
          }
          if (kb.special == Event::Keyboard::Special::Return || (kb.special == Event::Keyboard::Special::None && kb.codepoint == 32)) {
            if (hovered_index >= 0 && hovered_index < static_cast<int>(options.size())) {
              SelectOption(options[hovered_index].value);
            } else {
              is_open = false;
            }
            changed = true;
            return true;
          }
          if (kb.special == Event::Keyboard::Special::Escape) {
            is_open = false;
            changed = true;
            return true;
          }
        } else {
          if (kb.special == Event::Keyboard::Special::ArrowDown || kb.special == Event::Keyboard::Special::ArrowUp) {
            if (!options.empty()) {
              int curr_idx = -1;
              for (int i = 0; i < static_cast<int>(options.size()); ++i) {
                if (options[i].value == value) {
                  curr_idx = i;
                  break;
                }
              }
              int new_idx = curr_idx;
              if (kb.special == Event::Keyboard::Special::ArrowDown) {
                new_idx = (curr_idx == -1) ? 0 : std::min(static_cast<int>(options.size() - 1), curr_idx + 1);
              } else {
                new_idx = (curr_idx == -1) ? 0 : std::max(0, curr_idx - 1);
              }
              if (new_idx != curr_idx) {
                SelectOption(options[new_idx].value);
                changed = true;
              }
            }
            return true;
          }
          if (kb.special == Event::Keyboard::Special::Return || (kb.special == Event::Keyboard::Special::None && kb.codepoint == 32)) {
            is_open = true;
            if (!options.empty()) {
              hovered_index = 0;
              for (int i = 0; i < static_cast<int>(options.size()); ++i) {
                if (options[i].value == value) {
                  hovered_index = i;
                  break;
                }
              }
            }
            changed = true;
            return true;
          }
        }
      }
    }
  }

  return changed;
}

bool select::Digest() {
  std::cout << "select::Digest this=" << this << " value=" << value << " is_open=" << is_open << std::endl;
  auto* root = Root();
  if (root) {
    bool is_focused = root->focused();
    focus_class = is_focused ? "focused" : "";
  }
  dropdown_class = is_open ? "open" : "closed";
  arrow_char = is_open ? "▴" : "▾";

  if (root) {
    auto options = GetOptions();
    selected_label = "Select...";
    for (size_t i = 0; i < options.size(); ++i) {
      auto& opt = options[i];
      opt.element->classes.clear();
      if (opt.value == value) {
        selected_label = opt.label;
        opt.element->classes.push_back("selected");
      }
      if (is_open && static_cast<int>(i) == hovered_index) {
        opt.element->classes.push_back("hovered");
      }
    }
  }

  return Component<select>::Digest();
}

void select::SelectOption(std::string_view opt_val) {
  value = std::string(opt_val);
  PropagateBinding("value", value);
  is_open = false;

  auto* root = Root();
  if (root && root->Attributes().count("onchange")) {
    std::string onchange_cb = root->Attributes().at("onchange");
    Element* parent_el = root->Parent();
    ComponentBase* parent_comp = nullptr;
    while (parent_el) {
      if (parent_el->component()) {
        parent_comp = const_cast<ComponentBase*>(parent_el->component());
        break;
      }
      parent_el = parent_el->Parent();
    }
    while (parent_comp) {
      if (parent_comp->RunCallback(onchange_cb)) {
        break;
      }
      if (parent_comp->Root()) {
        parent_el = parent_comp->Root()->Parent();
        parent_comp = nullptr;
        while (parent_el) {
          if (parent_el->component()) {
            parent_comp = const_cast<ComponentBase*>(parent_el->component());
            break;
          }
          parent_el = parent_el->Parent();
        }
      } else {
        break;
      }
    }
  }
}

void option::InitReflection() {
  Bind(value);
  Component<option>::InitReflection();
}

std::string_view option::Setup() {
  return R"html(<span class="option-container"><slot></slot></span><style>
    self {
      display: block;
      cursor: pointer;
    }
    .selected {
      color: #38bdf8;
      font-weight: bold;
    }
    .hovered {
      background-color: #333;
      color: #fff;
    }
  </style>)html";
}

bool option::OnEvent(Event event) {
  auto* root = Root();
  if (!root) return false;

  if (event.is<Event::Mouse>()) {
    auto mouse = event.get<Event::Mouse>();
    if (mouse.button == Event::Mouse::Button::Left && mouse.motion == Event::Mouse::Motion::Pressed) {
      int click_x = mouse.x - 1;
      int click_y = mouse.y - 1;
      int abs_x = root->absolute_x();
      int abs_y = root->absolute_y();
      int layout_w = root->layout_width();
      int layout_h = root->layout_height();

      if (click_x >= abs_x && click_x < abs_x + layout_w &&
          click_y >= abs_y && click_y < abs_y + layout_h) {
        
        Element* parent_el = root->Parent();
        while (parent_el) {
          if (parent_el->tag() == "select" && parent_el->component()) {
            auto* select_comp = const_cast<ComponentBase*>(parent_el->component());
            auto* sel = static_cast<rtxui::select*>(select_comp);
            sel->SelectOption(value);
            return true;
          }
          parent_el = parent_el->Parent();
        }
      }
    }
  }
  return false;
}

void hr::InitReflection() {
  Bind(line_chars);
  Component<hr>::InitReflection();
}

std::string_view hr::Setup() {
  return R"html(<span class="hr-span">{line_chars}</span><style>
    self {
      display: block;
      margin-top: 1;
      margin-bottom: 1;
      overflow: hidden;
      white-space: nowrap;
    }
    .hr-span {
      color: #555;
    }
  </style>)html";
}

bool hr::Digest() {
  auto* root = Root();
  if (root) {
    int layout_w = root->layout_width();
    if (layout_w <= 0) {
      layout_w = 80;
    }
    std::string new_line;
    for (int i = 0; i < layout_w; ++i) {
      new_line += "─";
    }
    if (line_chars != new_line) {
      line_chars = new_line;
      return true;
    }
  }
  return Component<hr>::Digest();
}

}  // namespace rtxui
