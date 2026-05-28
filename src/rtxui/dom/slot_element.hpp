#ifndef DOM_TEXT_SLOT_HPP_
#define DOM_TEXT_SLOT_HPP_

#include "rtxui/dom/element.hpp"

namespace rtxui {

// A slot HTML element
class SlotElement : public Element {
 public:
  SlotElement();
  std::string Print(int depth) const final;
};

}  // namespace rtxui

#endif  // DOM_TEXT_SLOT_HPP_
