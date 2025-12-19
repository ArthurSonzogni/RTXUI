#include "component.hpp"

#include <iomanip>
#include <iostream>
#include <string>
#include <print>
#include <string_view>
#include <vector>

#include "core/string.hpp"
#include "dom/slot_element.hpp"
#include "dom/text_element.hpp"
#include "xml/xml.hpp"

namespace rtxui {
namespace {

int indent = 0;
std::string Indent() {
  return std::string(indent * 2, ' ');
}

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

}  // namespace

std::string_view ComponentBase::Template() {
  if (template_.empty()) {
    template_ = StripIndent(RunSetup());
  }
  return template_;
}

void ComponentBase::Mount() {
  indent++;
  template_ = Template();
  xml_string_ = StripIndent(template_);

  Expected<xml::Nodes, xml::Error> nodes = xml::Parse(xml_string_);
  if (!nodes) {
    XmlParseError(nodes.error(), xml_string_);
  }
  xml_nodes_ = std::move(nodes.value());
  Render();
  indent--;
}

void ComponentBase::Render() {
  // Create a fake "<template>" xml element that contains every children of the
  // component.
  xml::Node template_node;
  template_node.type = xml::Node::Type::kElement;
  template_node.tag = "template";
  template_node.children.reserve(xml_nodes_.size());
  for (const auto& node : xml_nodes_) {
    if (node.type == xml::Node::Type::kElement ||
        node.type == xml::Node::Type::kText) {
      template_node.children.push_back(node);
    } else if (node.type == xml::Node::Type::kComment) {
      // Ignore comments.
    } else {
      std::cerr << "Unknown XML node type: " << static_cast<int>(node.type)
                << std::endl;
      std::exit(1);
    }
  }

  // Create the root element, if it doesn't exist.
  if (!root_) {
    root_ = Ref<Element>::New(this);
  }

  Render(template_node, root_.get(), this);
}

// Render the `<template>` node inside the `element`.
void ComponentBase::Render(const xml::Node& node,
                           Element* slot,
                           ComponentBase* import_source) {
  int element_index = 0;
  for (const auto& node : node.children) {
    if (element_index < slot->ChildCount()) {
      continue;
    }

    switch (node.type) {
      case xml::Node::Type::kComment: {
        break;
      }

      case xml::Node::Type::kText: {
        slot->AddChild(Ref<TextElement>::New(std::string(node.text)));
        break;
      }

      case xml::Node::Type::kElement: {
        if (node.tag == "style") {
          break;
        }

        if (node.tag == "slot" || node.tag.starts_with("slot.")) {
          std::string slot_name =
              node.tag == "slot" ? "" : std::string(node.tag.substr(5));
          auto slot_element = Ref<SlotElement>::New();
          slots_[slot_name] = slot_element;
          slot->AddChild(slot_element.get());
          break;
        }

        if (node.tag.starts_with("template.")) {
          std::string template_name = std::string(node.tag.substr(9));
          Ref<Element> target_slot = Slot(template_name);
          if (!target_slot) {
            break;
          }
          Render(node, target_slot.get(), import_source);
          break;
        }

        // Find the ComponentFactory from the `imports_` map.
        auto it = import_source->imports_.find(std::string(node.tag));
        if (it == import_source->imports_.end()) {
          std::print(
              stderr,
              "\n"
              "Error:\n"
              "  The component <{}> is using the component <{}>, but it has not "
              "been imported.\n"
              "\n"
              "  Known components are:\n",
              import_source->Tag(), node.tag);

          for (const auto& [key, _] : import_source->imports_) {
            std::print(stderr, "    - {}\n", key);
          }
          std::exit(1);
          return;
        }

        auto child = it->second();
        children_.insert(child);
        child->Mount();
        slot->AddChild(child->Root());

        // At this point, `child` has been mounted and rendered. Its default
        // slot is now available and empty. We can inject our own content.
        Ref<Element> default_slot = child->Slot("");
        if (default_slot) {
          child->Render(node, default_slot.get(), this);
        }
      }
    }

    element_index++;
  }
}

Ref<Element> ComponentBase::Slot(std::string_view name) {
  auto it = slots_.find(std::string(name));
  if (it != slots_.end()) {
    return it->second;
  } else {
    return {};
  }
}

}  // namespace rtxui
