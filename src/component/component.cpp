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

void XmlParseError(const xml::Error& error, std::string_view xml_string) {
  std::cerr << "======== Error parsing DOM ========" << std::endl;
  int error_line = error.line;
  int error_column = error.column;

  std::vector<std::string_view> dom_lines = Split(xml_string, '\n');
  std::cerr << "    ┌" << Repeat("─", 76) << std::endl;
  for (int line = 0; line < dom_lines.size(); line++) {
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
    template_ = StripIndent(std::string(GetView()));
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
  children_.clear();
  slots_.clear();

  if (!root_) {
    root_ = Ref<Element>::New(this);
    root_->id = id_;
    root_->classes = classes_;
  }
  root_->RemoveChildren();

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
      // Ignore
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

void ComponentBase::Render(const xml::Node& node,
                           Element* slot,
                           ComponentBase* import_source) {
  auto Interpolate = [&](std::string_view text) -> std::string {
    std::string result;
    size_t last_pos = 0;
    while (true) {
      size_t open_brace = text.find('{', last_pos);
      if (open_brace == std::string_view::npos) {
        result += text.substr(last_pos);
        break;
      }
      result += text.substr(last_pos, open_brace - last_pos);
      size_t close_brace = text.find('}', open_brace);
      if (close_brace == std::string_view::npos) {
        result += text.substr(open_brace);
        break;
      }
      std::string_view expression =
          text.substr(open_brace + 1, close_brace - open_brace - 1);
      result += import_source->GetInterpolatedValue(expression);
      last_pos = close_brace + 1;
    }
    return result;
  };

  for (const auto& child_node : node.children) {
    switch (child_node.type) {
      case xml::Node::Type::kComment:
        break;

      case xml::Node::Type::kText: {
        std::string text = Interpolate(child_node.text);
        for (char& c : text) {
          if (c == '\n' || c == '\r') c = ' ';
        }
        slot->AddChild(Ref<TextElement>::New(text));
        break;
      }

      case xml::Node::Type::kElement: {
        if (child_node.tag == "style") break;

        if (child_node.tag == "slot" || child_node.tag.starts_with("slot.")) {
          std::string slot_name = child_node.tag == "slot"
                                      ? ""
                                      : std::string(child_node.tag.substr(5));
          auto slot_element = Ref<SlotElement>::New();
          slots_[slot_name] = slot_element;
          slot->AddChild(slot_element);
          break;
        }

        if (child_node.tag.starts_with("template.")) {
          std::string template_name = std::string(child_node.tag.substr(9));
          Ref<Element> target_slot = Slot(template_name);
          if (target_slot) {
            Render(child_node, target_slot.get(), import_source);
          }
          break;
        }

        auto it = import_source->imports_.find(std::string(child_node.tag));
        if (it != import_source->imports_.end()) {
          auto child = it->second();
          children_.insert(child);

          if (child_node.attributes.contains("id")) {
            child->id_ = Interpolate(child_node.attributes.at("id"));
          }
          if (child_node.attributes.contains("class")) {
            std::string interpolated_class = Interpolate(child_node.attributes.at("class"));
            auto class_views = Split(interpolated_class, ' ');
            child->classes_.assign(class_views.begin(), class_views.end());
          }

          child->Mount();

          for (auto& [key, value] : child_node.attributes) {
            if (key == "id" || key == "class") continue;
            child->Root()->SetAttribute(std::string(key), Interpolate(value));
          }

          slot->AddChild(child->Root());

          Ref<Element> default_slot = child->Slot("");
          if (default_slot) {
            child->Render(child_node, default_slot.get(), import_source);
          }
          break;
        }

        auto child_element = Ref<Element>::New();
        child_element->SetTag(std::string(child_node.tag));
        for (auto& [key, value] : child_node.attributes) {
          child_element->SetAttribute(std::string(key), Interpolate(value));
        }
        slot->AddChild(child_element);
        for (auto& grandchild : child_node.children) {
          Render(grandchild, child_element.get(), import_source);
        }
        break;
      }
    }
  }
}

Ref<Element> ComponentBase::Slot(std::string_view name) {
  auto it = slots_.find(std::string(name));
  return (it != slots_.end()) ? it->second : Ref<Element>();
}

}  // namespace rtxui
