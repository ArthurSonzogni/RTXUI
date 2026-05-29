#include "rtxui/component/component.hpp"

#include <functional>
#include <iomanip>
#include <iostream>
#include <optional>
#include <print>
#include <string>
#include <string_view>
#include <vector>
#include <algorithm>
#include <map>

#include "rtxui/core/string.hpp"
#include "rtxui/dom/element.hpp"
#include "rtxui/dom/slot_element.hpp"
#include "rtxui/dom/text_element.hpp"
#include "rtxui/paint/color.hpp"
#include "rtxui/style/apply_style.hpp"
#include "rtxui/style/style.hpp"
#include "rtxui/xml/xml.hpp"

namespace rtxui {
namespace {

struct ElementState {
  int scroll_x = 0;
  int scroll_y = 0;
  bool focused = false;
};

void CollectElementStates(Element* el, std::vector<int> path, std::map<std::vector<int>, ElementState>& states) {
  if (!el) return;
  if (el->scroll_x() != 0 || el->scroll_y() != 0 || el->focused()) {
    states[path] = {el->scroll_x(), el->scroll_y(), el->focused()};
  }
  for (size_t i = 0; i < el->ChildCount(); ++i) {
    std::vector<int> child_path = path;
    child_path.push_back(static_cast<int>(i));
    CollectElementStates(el->ChildAt(i), child_path, states);
  }
}

void RestoreElementStates(Element* el, std::vector<int> path, const std::map<std::vector<int>, ElementState>& states) {
  if (!el) return;
  auto it = states.find(path);
  if (it != states.end()) {
    el->set_scroll_x(it->second.scroll_x);
    el->set_scroll_y(it->second.scroll_y);
    el->set_focused(it->second.focused);
  }
  for (size_t i = 0; i < el->ChildCount(); ++i) {
    std::vector<int> child_path = path;
    child_path.push_back(static_cast<int>(i));
    RestoreElementStates(el->ChildAt(i), child_path, states);
  }
}

std::string Interpolate(std::string_view text, ComponentBase* source) {
  struct Placeholder {
    size_t open_idx;
    size_t close_idx;
    std::string trimmed_expr;
  };
  std::vector<Placeholder> placeholders;
  std::vector<size_t> stack;

  for (size_t i = 0; i < text.size(); ++i) {
    if (text[i] == '{') {
      stack.push_back(i);
    } else if (text[i] == '}') {
      if (!stack.empty()) {
        size_t open_idx = stack.back();
        stack.pop_back();
        size_t close_idx = i;
        std::string_view expression = text.substr(open_idx + 1, close_idx - open_idx - 1);
        
        // Trim spaces to find a clean identifier
        std::string_view trimmed = expression;
        while (!trimmed.empty() && std::isspace(static_cast<unsigned char>(trimmed.front()))) {
          trimmed.remove_prefix(1);
        }
        while (!trimmed.empty() && std::isspace(static_cast<unsigned char>(trimmed.back()))) {
          trimmed.remove_suffix(1);
        }

        bool is_ident = !trimmed.empty();
        for (char c : trimmed) {
          if (!std::isalnum(static_cast<unsigned char>(c)) && c != '_' && c != '.' && c != '-') {
            is_ident = false;
            break;
          }
        }

        if (is_ident) {
          placeholders.push_back({open_idx, close_idx, std::string(trimmed)});
        }
      }
    }
  }

  // Sort placeholders in descending order of open_idx (right-to-left)
  std::sort(placeholders.begin(), placeholders.end(), [](const Placeholder& a, const Placeholder& b) {
    return a.open_idx > b.open_idx;
  });

  std::string result(text);
  for (const auto& ph : placeholders) {
    std::string val = source->GetInterpolatedValue(ph.trimmed_expr);
    result.replace(ph.open_idx, ph.close_idx - ph.open_idx + 1, val);
  }
  return result;
}

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
  std::map<std::vector<int>, ElementState> saved_states;
  if (root_) {
    CollectElementStates(root_.get(), {}, saved_states);
  }

  children_.clear();
  slots_.clear();

  std::vector<std::string> interpolated_css_strings;

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
        interpolated_css_strings.push_back(Interpolate(node.children[0].text, this));
        const auto& css_str = interpolated_css_strings.back();
        auto maybe_stylesheet = css::Parse(css_str);
        if (maybe_stylesheet) {
          stylesheet = maybe_stylesheet.value();
        } else {
          CssParseError(maybe_stylesheet.error(), css_str);
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

  if (root_) {
    RestoreElementStates(root_.get(), {}, saved_states);
  }
}

void ComponentBase::Render(const xml::Node& node,
                           Element* slot,
                           ComponentBase* import_source) {
  auto Interpolate = [&](std::string_view text) -> std::string {
    return rtxui::Interpolate(text, import_source);
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
            std::string interpolated_value = Interpolate(value);
            child->SetProperty(std::string(key), interpolated_value);
            child->Root()->SetAttribute(std::string(key), interpolated_value);

            if (value.starts_with("{") && value.ends_with("}")) {
              std::string parent_prop = std::string(value.substr(1, value.size() - 2));
              child->two_way_bindings_.push_back({std::string(key), import_source, parent_prop});
            }
          }

          child->Render();

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
        Render(child_node, child_element.get(), import_source);
        break;
      }
    }
  }
}

Ref<Element> ComponentBase::Slot(std::string_view name) {
  auto it = slots_.find(std::string(name));
  return (it != slots_.end()) ? it->second : Ref<Element>();
}

void ComponentBase::SetProperty(std::string_view name, std::string_view value) {
  std::string_view target = name;
  if (target.starts_with("props.")) {
    target = target.substr(6);
  }
  for (auto& entry : entries_) {
    if (entry.name == target && entry.set_value) {
      entry.set_value(value);
      return;
    }
  }
}

void ComponentBase::PropagateBinding(std::string_view child_prop, std::string_view value) {
  for (const auto& binding : two_way_bindings_) {
    if (binding.child_prop == child_prop) {
      binding.parent->SetProperty(binding.parent_prop, value);
    }
  }
}

}  // namespace rtxui
