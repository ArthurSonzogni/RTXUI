#include "component.hpp"

#include <functional>
#include <iomanip>
#include <iostream>
#include <optional>
#include <print>
#include <string>
#include <string_view>
#include <vector>

#include "core/string.hpp"
#include "dom/element.hpp"
#include "dom/slot_element.hpp"
#include "dom/text_element.hpp"
#include "paint/color.hpp"
#include "style/apply_style.hpp"
#include "style/style.hpp"
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

void CssParseError(const css::Error& error, std::string_view css_string) {
  std::cerr << "======== Error parsing CSS ========" << std::endl;
  int error_line = error.line;
  int error_column = error.column;

  std::vector<std::string_view> css_lines = Split(css_string, '\n');
  std::cerr << "    ┌" << Repeat("─", 76) << std::endl;
  for (int line = 0; line < css_lines.size(); line++) {
    std::cerr << std::setw(4) << line << "│ " << css_lines[line] << std::endl;
    if (line != error_line) {
      continue;
    }
    std::string arrow_line = Repeat("-", std::max(76, error_column));
    if (error_column < arrow_line.length()) {
      arrow_line[error_column + 1] = '^';
    }
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
    template_ = StripIndent(Setup());
  }
  return template_;
}

void ComponentBase::Mount() {
  InitReflection();
  template_ = Template();
  xml_string_ = StripIndent(template_);

  Expected<xml::Nodes, xml::Error> nodes = xml::Parse(xml_string_);
  if (!nodes) {
    XmlParseError(nodes.error(), xml_string_);
  }
  xml_nodes_ = std::move(nodes.value());
  Render();
}

void ComponentBase::Render() {
  // Create the root element, if it doesn't exist.
  if (!root_) {
    root_ = Ref<Element>::New(this);
    root_->id = id_;
    root_->classes = classes_;
  }

  // Create a fake "<template>" xml element that contains every children of the
  // component.
  xml::Node template_node;
  template_node.type = xml::Node::Type::kElement;
  template_node.tag = "template";
  template_node.children.reserve(xml_nodes_.size());

  std::optional<css::StyleSheet> stylesheet;

  for (const auto& node : xml_nodes_) {
    if (node.type == xml::Node::Type::kElement) {
      if (node.tag == "style") {
        if (node.children.empty() ||
            node.children[0].type != xml::Node::Type::kText) {
          continue;
        }
        auto maybe_stylesheet = css::Parse(node.children[0].text);
        if (maybe_stylesheet) {
          stylesheet = maybe_stylesheet.value();
        } else {
          CssParseError(maybe_stylesheet.error(), node.children[0].text);
        }
        continue;
      }
      template_node.children.push_back(node);
    } else if (node.type == xml::Node::Type::kText) {
      template_node.children.push_back(node);
    } else if (node.type == xml::Node::Type::kComment) {
      // Ignore comments.
    } else {
      std::cerr << "Unknown XML node type: " << static_cast<int>(node.type)
                << std::endl;
      std::exit(1);
    }
  }

  Render(template_node, root_.get(), this);

  if (stylesheet) {
    auto ApplyRuleset = [](Element* element, const css::Ruleset& ruleset) {
      std::string_view selector = ruleset.selector;
      bool match = false;
      if (selector.starts_with("#")) {
        if (!element->id.empty() && element->id == selector.substr(1)) {
          match = true;
        }
      } else if (selector.starts_with(".")) {
        std::string_view class_name = selector.substr(1);
        for (const auto& cls : element->classes) {
          if (cls == class_name) {
            match = true;
            break;
          }
        }
      } else {
        if (element->tag() == selector) {
          match = true;
        }
      }

      if (match) {
        for (const auto& declaration : ruleset.declarations) {
          ApplyStyle(element->style, declaration);
        }
      }
    };

    for (const auto& ruleset : *stylesheet) {
      // Handle "self" selector.
      if (ruleset.selector == "self") {
        for (const auto& declaration : ruleset.declarations) {
          ApplyStyle(root_->style, declaration);
        }
        continue;
      }

      std::function<void(Element*, const css::Ruleset&)> StyleDescendants =
          [&](Element* element, const css::Ruleset& ruleset) {
            ApplyRuleset(element, ruleset);
            for (int i = 0; i < element->ChildCount(); ++i) {
              StyleDescendants(element->ChildAt(i), ruleset);
            }
          };

      StyleDescendants(root_.get(), ruleset);
    }
  }
}

// Render the `<template>` node inside the `element`.
void ComponentBase::Render(const xml::Node& node,
                           Element* slot,
                           ComponentBase* import_source) {
  int element_index = 0;
  for (const auto& child_node : node.children) {
    if (element_index < slot->ChildCount()) {
      continue;
    }

    switch (child_node.type) {
      case xml::Node::Type::kComment: {
        break;
      }

      case xml::Node::Type::kText: {
        std::string text = std::string(child_node.text);
        // Replace all '\n' with ' '.
        for (char& c : text) {
          if (c == '\n' || c == '\r') {
            c = ' ';
          }
        }
        slot->AddChild(Ref<TextElement>::New(text));
        break;
      }

      case xml::Node::Type::kElement: {
        if (child_node.tag == "style") {
          break;
        }

        if (child_node.tag == "slot" || child_node.tag.starts_with("slot.")) {
          std::string slot_name = child_node.tag == "slot"
                                      ? ""
                                      : std::string(child_node.tag.substr(5));
          auto slot_element = Ref<SlotElement>::New();

          if (child_node.attributes.contains("id")) {
            slot_element->id = child_node.attributes.at("id");
          }
          if (child_node.attributes.contains("class")) {
            auto class_views = Split(child_node.attributes.at("class"), ' ');
            slot_element->classes.assign(class_views.begin(), class_views.end());
          }

          slots_[slot_name] = slot_element;
          slot->AddChild(slot_element);
          break;
        }

        if (child_node.tag.starts_with("template.")) {
          std::string template_name = std::string(child_node.tag.substr(9));
          Ref<Element> target_slot = Slot(template_name);
          if (!target_slot) {
            break;
          }
          Render(child_node, target_slot.get(), import_source);
          break;
        }

        // Find the ComponentFactory from the `imports_` map.
        auto it = import_source->imports_.find(std::string(child_node.tag));
        if (it == import_source->imports_.end()) {
          std::print(stderr,
                     "\n"
                     "Error:\n"
                     "  The component <{}> is using the component <{}>, but it "
                     "has not "
                     "been imported.\n"
                     "\n"
                     "  Known components are:\n",
                     import_source->Tag(), child_node.tag);

          for (const auto& [key, _] : import_source->imports_) {
            std::print(stderr, "    - {}\n", key);
          }
          std::exit(1);
          return;
        }

        auto child = it->second();
        children_.insert(child);

        if (child_node.attributes.contains("id")) {
          child->id_ = child_node.attributes.at("id");
        }
        if (child_node.attributes.contains("class")) {
          auto class_views = Split(child_node.attributes.at("class"), ' ');
          child->classes_.assign(class_views.begin(), class_views.end());
        }

        child->Mount();
        slot->AddChild(child->Root());

        // At this point, `child` has been mounted and rendered. Its default
        // slot is now available and empty. We can inject our own content.
        Ref<Element> default_slot = child->Slot("");
        if (default_slot) {
          child->Render(child_node, default_slot.get(), this);
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
