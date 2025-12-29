#include "layout/layout_box.hpp"

namespace rtxui {

// LayoutBox constructor
LayoutBox::LayoutBox() = default;

std::string AlgorithmToString(LayoutBox::Algorithm algo) {
  switch (algo) {
    case LayoutBox::Algorithm::InlineFlow:
      return "InlineFlow";
    case LayoutBox::Algorithm::BlockFlow:
      return "BlockFlow";
    case LayoutBox::Algorithm::Flex:
      return "Flex";
    case LayoutBox::Algorithm::Text:
      return "Text";
    default:
      return "Unknown";
  }
}

// Print method implementation
std::string LayoutBox::Print(int indent) const {
  std::string result = "";
  std::string indent_str(indent * 2, ' ');
  result += indent_str + " - " + AlgorithmToString(algorithm) + "\n";
  for (const auto& child : children) {
    result += child->Print(indent + 1);
  }
  return result;
}

}  // namespace rtxui

