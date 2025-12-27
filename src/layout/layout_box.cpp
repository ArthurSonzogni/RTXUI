#include "layout/layout_box.hpp"

namespace rtxui {

// LayoutBox constructor
LayoutBox::LayoutBox(const std::string& name) : debug_name(name) {}

// Print method implementation
std::string LayoutBox::Print(int indent) const {
  std::string result = "";
  std::string indent_str(indent * 2, ' ');
  result += indent_str + debug_name + "\n";
  for (const auto& child : children) {
    result += child->Print(indent + 1);
  }
  return result;
}

}  // namespace rtxui

