#ifndef DOM_TEXT_SLOT_HPP_
#define DOM_TEXT_SLOT_HPP_

#include "rtxui/dom/element.hpp"
#include "rtxui/xml/xml.hpp"

namespace rtxui {

// A slot HTML element
class SlotElement : public Element {
 public:
  SlotElement();
  std::string Print(int depth) const final;

  /// The content written between the slot's own tags, shown when the consumer
  /// projects nothing into it.
  ///
  /// Held as an owned copy rather than a pointer into the declaring
  /// component's parsed template: projection happens in a later pass than the
  /// one that creates the slot, and that template can be replaced in between
  /// -- <markdown> rebuilds its own on every keystroke. xml::Node owns its
  /// strings, so a copy is self-contained. It is small: a slot's fallback is a
  /// few nodes at most.
  const xml::Node& fallback() const { return fallback_; }
  void set_fallback(const xml::Node& node) { fallback_ = node; }

 private:
  xml::Node fallback_;
};

}  // namespace rtxui

#endif  // DOM_TEXT_SLOT_HPP_
