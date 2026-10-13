#ifndef DOM_TEXT_ELEMENT_HPP_
#define DOM_TEXT_ELEMENT_HPP_

#include "rtxui/dom/element.hpp"

namespace rtxui {

// A text HTML element
class TextElement : public Element {
 public:
  explicit TextElement(std::string text);
  std::string Print(int depth) const final;
  std::string_view tag() const final { return "#text"; }
  const std::string& text() const { return text_; }
  void set_text(std::string text) {
    if (text != text_) {
      text_ = std::move(text);
      InvalidateLayout();
      // Whether the parent is :empty.
      if (Parent()) {
        Parent()->MarkPseudoInputsChanged();
      }
    }
  }

 private:
  std::string text_;
};

}  // namespace rtxui

#endif  // DOM_TEXT_ELEMENT_HPP_
