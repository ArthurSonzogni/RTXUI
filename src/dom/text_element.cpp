#include "dom/text_element.hpp"

namespace rtxui {

std::string TextElement::Print(int depth) const {
  return std::string(depth, ' ') + text_ + "\n";
}

}  // namespace rtxui
