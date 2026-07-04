
#include "rtxui/layout/layout_tree_builder.hpp"
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
}  // namespace

// Static Build method implementation
std::shared_ptr<LayoutBox> LayoutTreeBuilder::Build(Element* dom_node,
                                                    TextAlign parent_align,
                                                    WhiteSpace parent_ws,
                                                    TextTransform parent_text_transform,
                                                    std::optional<Color> parent_fg,
                                                    std::optional<bool> parent_bold,
                                                    std::optional<bool> parent_dim,
                                                    std::optional<bool> parent_italic,
                                                    std::optional<bool> parent_underlined,
                                                    std::optional<bool> parent_underlined_double,
                                                    std::optional<bool> parent_strikethrough,
                                                    std::optional<bool> parent_blink) {
  if (!dom_node || dom_node->style.display_none) {
    return nullptr;
  }
  bool is_text = dom_node->is_text();

  auto box = std::allocate_shared<LayoutBox, LayoutArenaAllocator<LayoutBox>>(
      LayoutArenaAllocator<LayoutBox>());
  box->style = dom_node->style;
  box->dom_node = dom_node;

  TextAlign resolved_align = dom_node->style.text_align.value_or(parent_align);
  box->style.text_align = resolved_align;

  WhiteSpace resolved_ws = dom_node->style.white_space.value_or(parent_ws);
  box->style.white_space = resolved_ws;

  TextTransform resolved_text_transform =
      dom_node->style.text_transform.value_or(parent_text_transform);
  box->style.text_transform = resolved_text_transform;

  std::optional<Color> resolved_fg = dom_node->style.foreground_color.has_value() ? dom_node->style.foreground_color : parent_fg;
  box->style.foreground_color = resolved_fg;

  std::optional<bool> resolved_bold = dom_node->style.bold.has_value() ? dom_node->style.bold : parent_bold;
  box->style.bold = resolved_bold;

  std::optional<bool> resolved_dim = dom_node->style.dim.has_value() ? dom_node->style.dim : parent_dim;
  box->style.dim = resolved_dim;

  std::optional<bool> resolved_italic = dom_node->style.italic.has_value() ? dom_node->style.italic : parent_italic;
  box->style.italic = resolved_italic;

  std::optional<bool> resolved_underlined = dom_node->style.underlined.has_value() ? dom_node->style.underlined : parent_underlined;
  box->style.underlined = resolved_underlined;

  std::optional<bool> resolved_underlined_double = dom_node->style.underlined_double.has_value() ? dom_node->style.underlined_double : parent_underlined_double;
  box->style.underlined_double = resolved_underlined_double;

  std::optional<bool> resolved_strikethrough = dom_node->style.strikethrough.has_value() ? dom_node->style.strikethrough : parent_strikethrough;
  box->style.strikethrough = resolved_strikethrough;

  std::optional<bool> resolved_blink = dom_node->style.blink.has_value() ? dom_node->style.blink : parent_blink;
  box->style.blink = resolved_blink;

  // Text nodes don't usually run an algorithm themselves;
  // they are consumed by the parent's InlineFlow.
  if (is_text) {
    auto text_node = static_cast<TextElement*>(dom_node);
    box->is_text = true;
    box->text_data = text_node->text();
    ApplyTextTransform(box->text_data, resolved_text_transform);
    box->algorithm = LayoutBox::Algorithm::Text;

    return box;
  }

  std::vector<std::shared_ptr<LayoutBox>> raw_children;
  for (auto& child_dom : dom_node->children()) {
    if (child_dom.get()->is_slot()) {
      // Skip elements with no tag (e.g., SlotElement)
      auto slot_style = child_dom.get()->style;
      TextAlign slot_align = slot_style.text_align.value_or(resolved_align);
      WhiteSpace slot_ws = slot_style.white_space.value_or(resolved_ws);
      TextTransform slot_text_transform =
          slot_style.text_transform.value_or(resolved_text_transform);
      std::optional<Color> slot_fg = slot_style.foreground_color.has_value() ? slot_style.foreground_color : resolved_fg;
      std::optional<bool> slot_bold = slot_style.bold.has_value() ? slot_style.bold : resolved_bold;
      std::optional<bool> slot_dim = slot_style.dim.has_value() ? slot_style.dim : resolved_dim;
      std::optional<bool> slot_italic = slot_style.italic.has_value() ? slot_style.italic : resolved_italic;
      std::optional<bool> slot_underlined = slot_style.underlined.has_value() ? slot_style.underlined : resolved_underlined;
      std::optional<bool> slot_underlined_double = slot_style.underlined_double.has_value() ? slot_style.underlined_double : resolved_underlined_double;
      std::optional<bool> slot_strikethrough = slot_style.strikethrough.has_value() ? slot_style.strikethrough : resolved_strikethrough;
      std::optional<bool> slot_blink = slot_style.blink.has_value() ? slot_style.blink : resolved_blink;

      for (auto& grandchild_dom : child_dom.get()->children()) {
        auto grandchild_box =
            Build(grandchild_dom.get(), slot_align, slot_ws, slot_text_transform, slot_fg, slot_bold, slot_dim, slot_italic, slot_underlined, slot_underlined_double, slot_strikethrough, slot_blink);
        if (grandchild_box) {
          raw_children.push_back(grandchild_box);
        }
      }
      continue;
    }

    auto child_box = Build(child_dom.get(), resolved_align, resolved_ws, resolved_text_transform, resolved_fg, resolved_bold, resolved_dim, resolved_italic, resolved_underlined, resolved_underlined_double, resolved_strikethrough, resolved_blink);
    if (child_box) {
      raw_children.push_back(child_box);
    }
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
          anonymous_box->style.text_align = resolved_align;
          anonymous_box->style.white_space = resolved_ws;
          refined_children.push_back(anonymous_box);
        }
        anonymous_box->children.push_back(child_box);
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
