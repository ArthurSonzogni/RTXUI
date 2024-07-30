#include "component.hpp"

#include <iomanip>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

#include "core/string.hpp"
#include "dom/slot_element.hpp"
#include "dom/text_element.hpp"
#include "xml/xml.hpp"

namespace rtxui {
namespace {

void XmlParseError(const xml::Error& error, std::string_view xml_string) {
  std::cerr << "======== Error parsing DOM ========" << std::endl;
  int error_line = error.line;
  int error_column = error.column;

  std::vector<std::string_view> dom_lines = Split(xml_string, '\n');
  std::cerr << "    ┌" << Repeat("─", 76) << std::endl;
  for (int line = 0; line < dom_lines.size(); line++) {
    // std::cerr << line << "│ " << dom_lines[line] << std::endl;
    std::cerr << std::setw(4) << line << "│ " << dom_lines[line] << std::endl;
    if (line != error_line) {
      continue;
    }
    std::string arrow_line = Repeat("-", std::max(76, error_column));
    arrow_line[error_column + 1] = '^';
    std::cerr << "    └" << arrow_line << std::endl;
    std::cerr << "      " << Repeat(" ", error_column) << "|" << std::endl;
    std::cerr << error_line << ":" << error_column << ": " << error.message
              << std::endl;
    std::cerr << std::endl;
    std::cerr << "    ┌" << Repeat("─", 76) << std::endl;
  }
  std::cerr << "    └" << Repeat("─", 76) << std::endl;
  std::cerr << std::flush;
  std::exit(1);
}

void UnknownComponentError(std::string_view tag) {
  std::cerr << "Unknown component: " << tag << std::endl;
  std::cerr << "" << std::endl;
  std::cerr << "Did you defined the component and correctly linked it?"
            << std::endl;
  std::cerr << "Known components: " << std::endl;
  std::cerr << Register::Print() << std::endl;
  std::exit(1);
}

}  // namespace

void Component::Bind(std::string_view name, Ref<Cell> value) {
  bindings_.insert({std::string(name), std::move(value)});
}

void Component::Mount() {
  xml_string_ = StripIndent(Setup());
  Expected<xml::Nodes, xml::Error> nodes = xml::Parse(xml_string_);
  if (!nodes) {
    XmlParseError(nodes.error(), xml_string_);
  }
  xml_nodes_ = std::move(nodes.value());
  Render();
}

void Component::Render() {
  // Find the <template> tag.
  xml::Node template_node;
  for (const auto& node : xml_nodes_) {
    if (node.type == xml::Node::Type::kElement && node.tag == "template") {
      template_node = node;
      break;
    }
  }

  if (template_node.tag.empty()) {
    std::cerr << "No <template> tag found in the " << Tag() << " component."
              << std::endl;
    std::exit(1);
  }

  // Create the root element, if it doesn't exist.
  if (!root_) {
    root_ = Ref<Element>::New(this);
  }

  Render(template_node, root_.get());
}

// Render the `<template>` node inside the `element`.
void Component::Render(const xml::Node& node, Element* slot) {
  int element_index = 0;
  for (const auto& node : node.children) {
    if (element_index < slot->ChildCount()) {
      continue;
    }

    switch (node.type) {
      case xml::Node::Type::kText: {
        slot->AddChild(Ref<TextElement>::New(std::string(node.text)));
        break;
      }

      case xml::Node::Type::kElement: {
        if (node.tag == "slot") {
          assert(default_slot_ == nullptr);
          default_slot_ = Ref<SlotElement>::New();
          slot->AddChild(default_slot_);
          break;
        }

        if (node.tag.starts_with("slot.")) {
          std::string slot_name = std::string(node.tag.substr(5));
          slots_[slot_name] = Ref<SlotElement>::New();
          slot->AddChild(slots_[slot_name]);
          break;
        }

        if (node.tag.starts_with("template.")) {
          std::string template_name = std::string(node.tag.substr(9));
          std::cerr << "Rendering template " << template_name << std::endl;
          Element* slot = Slot(template_name);
          if (!slot) {
            std::cerr << "Slot not found: " << template_name << std::endl;
            std::exit(1);
          }
          Render(node, slot);
          break;
        }

        std::cerr << Join(Namespaces(), ".") << "." << Tag() << " -> "
                  << node.tag << std::endl;

        auto child_template = Register::Get(node.tag, Namespaces());
        if (!child_template) {
          UnknownComponentError(node.tag);
        }

        auto child = child_template->New();
        children_.insert(child);
        child->Mount();
        slot->AddChild(child->Root());

        Element* default_slot = child->DefaultSlot();
        if (default_slot) {
          child->Render(node, default_slot);
        }
      }

      case xml::Node::Type::kComment: {
        break;
      }
    }

    element_index++;
  }
}

Element* Component::DefaultSlot() {
  return default_slot_.get();
}

Element* Component::Slot(std::string_view name) {
  if (slots_.count(std::string(name))) {
    return slots_[std::string(name)].get();
  }
  return nullptr;
}

}  // namespace rtxui
