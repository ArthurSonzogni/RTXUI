#ifndef DOM_TEXT_SLOT_HPP_
#define DOM_TEXT_SLOT_HPP_

#include "dom/element.hpp"

namespace rtxui {

// A slot HTML element
class SlotElement : public Element {
 public:
  explicit SlotElement() = default;
  std::string Print(int depth) const final;
};

}  // namespace rtxui

#endif  // DOM_TEXT_SLOT_HPP_
