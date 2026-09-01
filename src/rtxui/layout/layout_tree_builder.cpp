
#include "rtxui/layout/layout_tree_builder.hpp"

#include "rtxui/base/string.hpp"
#include "rtxui/layout/layout_arena.hpp"

namespace rtxui {

namespace {
// Applies CSS text-transform to text in place. Only ASCII letters are
// transformed; multi-byte UTF-8 sequences (bytes >= 0x80) pass through
// untouched, so the string stays valid UTF-8 and its length never changes.
void ApplyTextTransform(std::string& text, TextTransform transform) {
  switch (transform) {
    case TextTransform::None:
      return;
    case TextTransform::Uppercase:
      for (char& c : text) {
        if (c >= 'a' && c <= 'z') {
          c -= 'a' - 'A';
        }
      }
      return;
    case TextTransform::Lowercase:
      for (char& c : text) {
        if (c >= 'A' && c <= 'Z') {
          c += 'a' - 'A';
        }
      }
      return;
    case TextTransform::Capitalize: {
      bool at_word_start = true;
      for (char& c : text) {
        bool is_alpha = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
        if (at_word_start && c >= 'a' && c <= 'z') {
          c -= 'a' - 'A';
        }
        at_word_start = !is_alpha && !(c >= '0' && c <= '9') &&
                        static_cast<unsigned char>(c) < 0x80;
      }
      return;
    }
  }
}

// Folds CR and CRLF into LF, and replaces every other control character with
// U+FFFD.
//
// Text reaching layout comes from the application, and an application displays
// text it did not write: a filename, a log line, something off the network. A
// raw ESC in there is not a character, it is a command -- it repaints, moves
// the cursor, changes colours the engine believes it owns, and on some
// terminals sets the window title. Everything after it on that line is then
// somewhere other than where the layout thinks, and the cell diff goes on
// describing a screen that never existed. BEL rings, BS and DEL walk the
// cursor backwards, and NUL/VT/FF are equally unwelcome.
//
// Only LF and TAB come out the other side, because layout gives those a
// meaning of its own -- hard breaks and tab stops. Everything else becomes one
// visible replacement character, which keeps the byte from reaching the
// terminal while still showing that something was there, and costs exactly the
// one cell the layout counted.
void SanitizeControlCharacters(std::string& text) {
  bool needs_work = false;
  for (const unsigned char c : text) {
    if ((c < 0x20 && c != '\n' && c != '\t') || c == 0x7f) {
      needs_work = true;
      break;
    }
  }
  if (!needs_work) {
    return;
  }
  static constexpr std::string_view kReplacement = "\xef\xbf\xbd";  // U+FFFD
  std::string result;
  result.reserve(text.size());
  for (size_t i = 0; i < text.size(); ++i) {
    const unsigned char c = text[i];
    if (c == '\r') {
      // A segment break either way, as CSS has it; CRLF is one of them.
      if (i + 1 < text.size() && text[i + 1] == '\n') {
        ++i;
      }
      result += '\n';
      continue;
    }
    // DEL sits just above the control block rather than inside it.
    if (c < 0x20 || c == 0x7f) {
      result += kReplacement;
      continue;
    }
    // Bytes at or above 0x80 are part of a UTF-8 sequence and pass through
    // untouched along with the rest of it.
    result += static_cast<char>(c);
  }
  text = std::move(result);
}

// white-space: normal | nowrap render newlines as plain spaces. (Template
// text keeps its newlines in the DOM; the conversion is style-driven here.)
// Tabs go the same way: CSS treats them as collapsible whitespace in these
// modes, and a literal one must not reach the terminal in any mode -- see
// ExpandTabs for why.
void ReplaceSegmentBreaksAndTabsWithSpaces(std::string& text) {
  if (text.find('\n') == std::string::npos &&
      text.find('\r') == std::string::npos &&
      text.find('\t') == std::string::npos) {
    return;
  }
  std::string result;
  result.reserve(text.size());
  for (size_t i = 0; i < text.size(); ++i) {
    char c = text[i];
    if (c == '\r' && i + 1 < text.size() && text[i + 1] == '\n') {
      continue;  // CRLF collapses to a single space via the '\n'.
    }
    // A tab is collapsible whitespace in these modes, exactly like a segment
    // break, and must not survive as a literal either.
    result += (c == '\n' || c == '\r' || c == '\t') ? ' ' : c;
  }
  text = std::move(result);
}

// Replaces each tab with spaces up to the next multiple of `tab_size`,
// counting cells from the start of the line.
//
// No tab may reach the terminal. The engine measures one as a single cell,
// while a terminal advances the cursor to its own next tab stop, so a lone tab
// inside a <pre> shifts everything after it on that line and leaves the cell
// diff describing a screen that does not exist. Expanding here is what the
// terminal would have done anyway, except that now the engine's own accounting
// is the one that decides -- which is the only way the diff can stay correct.
void ExpandTabs(std::string& text, int tab_size) {
  if (text.find('\t') == std::string::npos) {
    return;
  }
  std::string result;
  result.reserve(text.size());
  int column = 0;
  for (const Grapheme& grapheme : Graphemes(text)) {
    if (grapheme.text == "\t") {
      // A tab still has to occupy at least nothing rather than going through
      // untouched, so tab-size: 0 simply deletes it.
      const int next_stop =
          tab_size > 0 ? ((column / tab_size) + 1) * tab_size : column;
      result.append(static_cast<size_t>(next_stop - column), ' ');
      column = next_stop;
      continue;
    }
    if (grapheme.text == "\n" || grapheme.text == "\r") {
      result += grapheme.text;
      column = 0;
      continue;
    }
    result += grapheme.text;
    // Counted in cells, not bytes or codepoints, so a wide glyph moves the
    // next tab stop by two.
    column += grapheme.width;
  }
  text = std::move(result);
}

// white-space: pre-line collapses runs of spaces/tabs to a single space and
// drops spaces adjacent to newlines, while newlines themselves are preserved
// (the inline flow honors them as hard breaks).
void CollapseWhitespacePreLine(std::string& text) {
  std::string result;
  result.reserve(text.size());
  bool pending_space = false;
  for (size_t i = 0; i < text.size(); ++i) {
    char c = text[i];
    if (c == ' ' || c == '\t') {
      pending_space = !result.empty() && result.back() != '\n';
      continue;
    }
    if (c == '\r' || c == '\n') {
      if (c == '\r' && i + 1 < text.size() && text[i + 1] == '\n') {
        ++i;  // CRLF is one newline.
      }
      pending_space = false;
      result += '\n';
      continue;
    }
    if (pending_space) {
      result += ' ';
      pending_space = false;
    }
    result += c;
  }
  text = std::move(result);
}
// letter-spacing inserts non-breaking spaces (U+00A0) between grapheme
// clusters, so the extra cells take part in measurement and painting while
// the line breaker (which only breaks at ASCII spaces) keeps spaced words
// intact. Newlines keep their hard-break role: no spacing is inserted next
// to them.
void ApplyLetterSpacing(std::string& text, int spacing) {
  if (spacing <= 0 || text.empty()) {
    return;
  }
  static constexpr std::string_view kNbsp = "\xc2\xa0";
  std::string result;
  result.reserve(text.size() * (1 + static_cast<size_t>(spacing)));
  std::string_view previous;
  for (const Grapheme& g : Graphemes(text)) {
    if (!previous.empty() && previous != "\n" && g.text != "\n") {
      for (int i = 0; i < spacing; ++i) {
        result += kNbsp;
      }
    }
    result += g.text;
    previous = g.text;
  }
  text = std::move(result);
}
}  // namespace

// Static Build method implementation
std::shared_ptr<LayoutBox> LayoutTreeBuilder::Build(Element* dom_node,
                                                    InheritedTextStyle parent) {
  if (!dom_node || dom_node->style.display_none) {
    return nullptr;
  }
  bool is_text = dom_node->is_text();

  auto box = std::allocate_shared<LayoutBox, LayoutArenaAllocator<LayoutBox>>(
      LayoutArenaAllocator<LayoutBox>());
  box->style = dom_node->style;
  box->dom_node = dom_node;

  InheritedTextStyle resolved;
  resolved.align = dom_node->style.text_align.value_or(parent.align);
  box->style.text_align = resolved.align;

  resolved.white_space = dom_node->style.white_space.value_or(parent.white_space);
  box->style.white_space = resolved.white_space;

  resolved.text_transform =
      dom_node->style.text_transform.value_or(parent.text_transform);
  box->style.text_transform = resolved.text_transform;

  resolved.letter_spacing =
      dom_node->style.letter_spacing.value_or(parent.letter_spacing);
  box->style.letter_spacing = resolved.letter_spacing;
  resolved.tab_size = dom_node->style.tab_size.value_or(parent.tab_size);
  box->style.tab_size = resolved.tab_size;

  resolved.line_height =
      dom_node->style.line_height.value_or(parent.line_height);
  box->style.line_height = resolved.line_height;

  resolved.overflow_wrap =
      dom_node->style.overflow_wrap.value_or(parent.overflow_wrap);
  box->style.overflow_wrap = resolved.overflow_wrap;

  resolved.word_break = dom_node->style.word_break.value_or(parent.word_break);
  box->style.word_break = resolved.word_break;

  resolved.fg = dom_node->style.foreground_color.has_value() ? dom_node->style.foreground_color : parent.fg;
  box->style.foreground_color = resolved.fg;

  resolved.bold = dom_node->style.bold.has_value() ? dom_node->style.bold : parent.bold;
  box->style.bold = resolved.bold;

  resolved.dim = dom_node->style.dim.has_value() ? dom_node->style.dim : parent.dim;
  box->style.dim = resolved.dim;

  resolved.italic = dom_node->style.italic.has_value() ? dom_node->style.italic : parent.italic;
  box->style.italic = resolved.italic;

  resolved.underlined = dom_node->style.underlined.has_value() ? dom_node->style.underlined : parent.underlined;
  box->style.underlined = resolved.underlined;

  resolved.underlined_double = dom_node->style.underlined_double.has_value() ? dom_node->style.underlined_double : parent.underlined_double;
  box->style.underlined_double = resolved.underlined_double;

  resolved.strikethrough = dom_node->style.strikethrough.has_value() ? dom_node->style.strikethrough : parent.strikethrough;
  box->style.strikethrough = resolved.strikethrough;
  resolved.overlined = dom_node->style.overlined.has_value() ? dom_node->style.overlined : parent.overlined;
  box->style.overlined = resolved.overlined;

  resolved.blink = dom_node->style.blink.has_value() ? dom_node->style.blink : parent.blink;
  box->style.blink = resolved.blink;

  // Text nodes don't usually run an algorithm themselves;
  // they are consumed by the parent's InlineFlow.
  if (is_text) {
    auto text_node = static_cast<TextElement*>(dom_node);
    box->is_text = true;
    box->text_data = text_node->text();
    // Before anything measures or transforms it: what follows counts cells,
    // and a control character is not one.
    SanitizeControlCharacters(box->text_data);
    ApplyTextTransform(box->text_data, resolved.text_transform);
    switch (resolved.white_space) {
      case WhiteSpace::Normal:
      case WhiteSpace::Nowrap:
        ReplaceSegmentBreaksAndTabsWithSpaces(box->text_data);
        break;
      case WhiteSpace::PreLine:
        CollapseWhitespacePreLine(box->text_data);
        break;
      case WhiteSpace::Pre:
      case WhiteSpace::PreWrap:
        // Newlines are preserved and honored as hard breaks; tabs are the one
        // thing that cannot be left as written.
        ExpandTabs(box->text_data, resolved.tab_size);
        break;
    }
    ApplyLetterSpacing(box->text_data, resolved.letter_spacing);
    box->algorithm = LayoutBox::Algorithm::Text;

    return box;
  }

  std::vector<std::shared_ptr<LayoutBox>> raw_children;
  for (auto& child_dom : dom_node->children()) {
    if (child_dom.get()->is_slot()) {
      // Skip elements with no tag (e.g., SlotElement)
      auto slot_style = child_dom.get()->style;
      InheritedTextStyle slot = resolved;
      slot.align = slot_style.text_align.value_or(resolved.align);
      slot.white_space = slot_style.white_space.value_or(resolved.white_space);
      slot.text_transform =
          slot_style.text_transform.value_or(resolved.text_transform);
      slot.fg = slot_style.foreground_color.has_value() ? slot_style.foreground_color : resolved.fg;
      slot.bold = slot_style.bold.has_value() ? slot_style.bold : resolved.bold;
      slot.dim = slot_style.dim.has_value() ? slot_style.dim : resolved.dim;
      slot.italic = slot_style.italic.has_value() ? slot_style.italic : resolved.italic;
      slot.underlined = slot_style.underlined.has_value() ? slot_style.underlined : resolved.underlined;
      slot.underlined_double = slot_style.underlined_double.has_value() ? slot_style.underlined_double : resolved.underlined_double;
      slot.strikethrough = slot_style.strikethrough.has_value() ? slot_style.strikethrough : resolved.strikethrough;
      slot.overlined = slot_style.overlined.has_value() ? slot_style.overlined : resolved.overlined;
      slot.blink = slot_style.blink.has_value() ? slot_style.blink : resolved.blink;
      slot.letter_spacing =
          slot_style.letter_spacing.value_or(resolved.letter_spacing);
      slot.tab_size = slot_style.tab_size.value_or(resolved.tab_size);
      slot.line_height =
          slot_style.line_height.value_or(resolved.line_height);
      slot.overflow_wrap =
          slot_style.overflow_wrap.value_or(resolved.overflow_wrap);
      slot.word_break = slot_style.word_break.value_or(resolved.word_break);

      for (auto& grandchild_dom : child_dom.get()->children()) {
        auto grandchild_box = Build(grandchild_dom.get(), slot);
        if (grandchild_box) {
          raw_children.push_back(grandchild_box);
        }
      }
      continue;
    }

    auto child_box = Build(child_dom.get(), resolved);
    if (child_box) {
      raw_children.push_back(child_box);
    }
  }

  // Whether anything in this subtree is positioned out of flow, which is what
  // decides in layout.cpp whether the subtree's fragments may be cached
  // across measurement passes.
  box->has_out_of_flow =
      box->style.position == PositionType::Absolute ||
      box->style.position == PositionType::Fixed;
  for (const auto& child_box : raw_children) {
    box->has_out_of_flow |= child_box->has_out_of_flow;
  }

  // --- Algorithm Selection & Tree Refinement ---
  if (dom_node->tag() == "table") {
    box->children = raw_children;
    box->algorithm = LayoutBox::Algorithm::Table;
    box->style.display_outside = DisplayOutside::Block;

    return box;
  }

  if (box->style.display_inside == DisplayInside::Flex) {
    box->children = raw_children;
    box->algorithm = LayoutBox::Algorithm::Flex;

    return box;
  }

  if (box->style.display_inside == DisplayInside::Grid) {
    box->children = raw_children;
    box->algorithm = LayoutBox::Algorithm::Grid;

    return box;
  }

  if (box->style.display_outside == DisplayOutside::Block || dom_node->is_slot()) {
    box->algorithm = LayoutBox::Algorithm::BlockFlow;
    std::vector<std::shared_ptr<LayoutBox>> refined_children;
    std::shared_ptr<LayoutBox> anonymous_box = nullptr;
    for (const auto& child_box : raw_children) {
      // Out-of-flow elements (fixed/absolute) must remain direct children
      // of their containing block so LayoutOutOfFlowChildren can find them.
      // They should not be wrapped in anonymous inline boxes.
      if (child_box->style.position == PositionType::Absolute ||
          child_box->style.position == PositionType::Fixed) {
        refined_children.push_back(child_box);
        continue;
      }
      if (child_box->style.display_outside == DisplayOutside::Inline) {
        if (!anonymous_box) {
          anonymous_box = std::allocate_shared<LayoutBox, LayoutArenaAllocator<LayoutBox>>(
              LayoutArenaAllocator<LayoutBox>());
          anonymous_box->is_anonymous = true;
          anonymous_box->algorithm = LayoutBox::Algorithm::InlineFlow;
          anonymous_box->style.text_align = resolved.align;
          anonymous_box->style.white_space = resolved.white_space;
          anonymous_box->style.line_height = resolved.line_height;
          anonymous_box->style.overflow_wrap = resolved.overflow_wrap;
          anonymous_box->style.word_break = resolved.word_break;
          refined_children.push_back(anonymous_box);
        }
        anonymous_box->children.push_back(child_box);
        anonymous_box->has_out_of_flow |= child_box->has_out_of_flow;
      } else {
        anonymous_box = nullptr;
        refined_children.push_back(child_box);
      }
    }
    box->children = refined_children;
  } else {  // Inline
    box->algorithm = LayoutBox::Algorithm::InlineFlow;
    box->children = raw_children;
  }


  return box;
}

}  // namespace rtxui
