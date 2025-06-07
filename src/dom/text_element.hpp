#ifndef DOM_TEXT_ELEMENT_HPP_
#define DOM_TEXT_ELEMENT_HPP_

#include "dom/element.hpp"

namespace rtxui {

// A text HTML element
class TextElement : public Element {
 public:
  explicit TextElement(std::string text) : text_(std::move(text)) {}
  std::string Print(int depth) const final;

 private:
  std::string text_;
};

}  // namespace rtxui

#endif  // DOM_TEXT_ELEMENT_HPP_
