#include "dom/slot_element.hpp"

namespace rtxui {

std::string SlotElement::Print(int depth) const {
  std::string out;
  for (const auto& child : children_) {
    out += child->Print(depth);
  }
  return out;
}

}  // namespace rtxui
