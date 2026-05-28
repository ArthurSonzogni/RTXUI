#include "rtxui/dom/text_element.hpp"

namespace rtxui {

TextElement::TextElement(std::string text) {
  text_ = std::move(text);
  is_text_ = true;
}

std::string TextElement::Print(int depth) const {
  return std::string(depth, ' ') + text_ + "\n";
}

}  // namespace rtxui
