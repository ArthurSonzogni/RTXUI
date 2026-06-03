#include "rtxui/internal/component.hpp"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <charconv>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <map>
#include <optional>
#include <print>
#include <string>
#include <string_view>
#include <vector>

#include "rtxui/core/string.hpp"
#include "rtxui/dom/element.hpp"
#include "rtxui/dom/slot_element.hpp"
#include "rtxui/dom/text_element.hpp"
#include "rtxui/paint/color.hpp"
#include "rtxui/style/apply_style.hpp"
#include "rtxui/style/style.hpp"
#include "rtxui/xml/xml.hpp"

namespace rtxui {

RefCounted::~RefCounted() {
  assert(count_ == 0);
}

void RefCounted::AddRef() const {
  ++count_;
}

void RefCounted::Release() const {
  --count_;
  if (count_ == 0) {
    delete this;
  }
}

ComponentBase::ComponentBase() = default;
ComponentBase::~ComponentBase() = default;

std::string_view ComponentBase::Setup() {
  return "";
}

void ComponentBase::InitReflection() {}

bool ComponentBase::OnEvent(Event event) {
  return false;
}

Element* ComponentBase::Root() const {
  return root_.get();
}

bool Bindings::RunCallback(std::string_view name, std::string_view arg) {
  if (arg.empty()) {
    auto it = callbacks_.find(std::string(name));
    if (it != callbacks_.end()) {
      it->second();
      return true;
    }
  }

  auto it_param = parameterized_callbacks_.find(std::string(name));
  if (it_param != parameterized_callbacks_.end()) {
    it_param->second(std::string(arg));
    return true;
  }

  return false;
}

void Bindings::Import(std::string_view name, std::function<void()> callback) {
  if (callbacks_.count(std::string(name)) ||
      parameterized_callbacks_.count(std::string(name))) {
    std::println("Error: Callback '{}' is already imported.", name);
    std::exit(1);
  }

  callbacks_[std::string(name)] = std::move(callback);
}

void Bindings::Import(std::string_view name,
                      std::function<void(std::string)> callback) {
  if (callbacks_.count(std::string(name)) ||
      parameterized_callbacks_.count(std::string(name))) {
    std::println("Error: Callback '{}' is already imported.", name);
    std::exit(1);
  }

  parameterized_callbacks_[std::string(name)] = std::move(callback);
}

void Bindings::Import(std::string_view name, ComponentFactory factory) {
  if (imports_.count(std::string(name))) {
    std::println("Error: Component '{}' is already imported.", name);
    std::exit(1);
  }

  imports_[std::string(name)] = std::move(factory);
}

namespace {
std::unordered_map<std::string, ComponentFactory>& GetGlobalRegistry() {
  static auto* registry =
      new std::unordered_map<std::string, ComponentFactory>();
  return *registry;
}
}  // namespace

void RegisterGlobalComponent(std::string_view name, ComponentFactory factory) {
  GetGlobalRegistry()[std::string(name)] = std::move(factory);
}

ComponentFactory GetGlobalComponentFactory(std::string_view name) {
  auto& reg = GetGlobalRegistry();
  auto it = reg.find(std::string(name));
  return (it != reg.end()) ? it->second : nullptr;
}

namespace {

struct ElementState {
  int scroll_x = 0;
  int scroll_y = 0;
  bool focused = false;
  ComputedStyle style;
  std::map<std::string, ActiveTransition> active_transitions;
};

void CollectElementStates(Element* el,
                          std::vector<int>& path,
                          std::map<std::vector<int>, ElementState>& states) {
  if (!el) {
    return;
  }
  bool has_state = el->scroll_x() != 0 ||
                   el->scroll_y() != 0 ||
                   el->focused() ||
                   !el->active_transitions.empty() ||
                   !el->target_style.transitions.empty();
  if (has_state) {
    states[path] = {el->scroll_x(), el->scroll_y(), el->focused(), el->style, el->active_transitions};
  }
  for (size_t i = 0; i < el->ChildCount(); ++i) {
    path.push_back(static_cast<int>(i));
    CollectElementStates(el->ChildAt(i), path, states);
    path.pop_back();
  }
}

void RestoreElementStates(
    Element* el,
    std::vector<int>& path,
    const std::map<std::vector<int>, ElementState>& states) {
  if (!el) {
    return;
  }
  auto it = states.find(path);
  if (it != states.end()) {
    el->set_scroll_x(it->second.scroll_x);
    el->set_scroll_y(it->second.scroll_y);
    el->set_focused(it->second.focused);
    el->style = it->second.style;
    el->active_transitions = it->second.active_transitions;
  }
  for (size_t i = 0; i < el->ChildCount(); ++i) {
    path.push_back(static_cast<int>(i));
    RestoreElementStates(el->ChildAt(i), path, states);
    path.pop_back();
  }
}


std::string Interpolate(std::string_view text,
                        ComponentBase* source,
                        std::shared_ptr<LocalScope> scope) {
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
        std::string_view expression =
            text.substr(open_idx + 1, close_idx - open_idx - 1);

        // Trim spaces to find a clean identifier
        std::string_view trimmed = expression;
        while (!trimmed.empty() &&
               std::isspace(static_cast<unsigned char>(trimmed.front()))) {
          trimmed.remove_prefix(1);
        }
        while (!trimmed.empty() &&
               std::isspace(static_cast<unsigned char>(trimmed.back()))) {
          trimmed.remove_suffix(1);
        }

        bool is_ident = !trimmed.empty();
        for (char c : trimmed) {
          if (!std::isalnum(static_cast<unsigned char>(c)) && c != '_' &&
              c != '.' && c != '-' && c != '$' && c != ' ' && c != '=' &&
              c != '!' && c != '>' && c != '<' && c != '&' && c != '|') {
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
  std::sort(placeholders.begin(), placeholders.end(),
            [](const Placeholder& a, const Placeholder& b) {
              return a.open_idx > b.open_idx;
            });

  std::string result(text);
  for (const auto& ph : placeholders) {
    std::string val;
    bool resolved = false;
    if (scope) {
      auto dot_pos = ph.trimmed_expr.find('.');
      std::string var_name =
          (dot_pos == std::string_view::npos)
              ? ph.trimmed_expr
              : std::string(ph.trimmed_expr.substr(0, dot_pos));
      auto val_opt = scope->Get(var_name);
      if (val_opt) {
        resolved = true;
        if (dot_pos == std::string_view::npos) {
          if (std::holds_alternative<std::string>(*val_opt)) {
            val = std::get<std::string>(*val_opt);
          }
        } else {
          std::string_view expr_view = ph.trimmed_expr;
          std::string_view field_name = expr_view.substr(dot_pos + 1);
          if (std::holds_alternative<std::shared_ptr<StructVisitor>>(
                  *val_opt)) {
            auto visitor = std::get<std::shared_ptr<StructVisitor>>(*val_opt);
            if (visitor) {
              val = visitor->GetFieldValue(field_name);
            }
          }
        }
      }
    }
    if (!resolved) {
      val = source->GetInterpolatedValue(ph.trimmed_expr);
    }
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

bool MatchSelector(const Element* element,
                   const Element* root,
                   const css::ParsedSelector& selector,
                   bool check_pseudos) {
  bool base_match = false;
  if (selector.base == "self") {
    base_match = (element == root);
  } else if (selector.base.starts_with("#")) {
    base_match =
        (!element->id.empty() && element->id == selector.base.substr(1));
  } else if (selector.base.starts_with(".")) {
    std::string_view class_name = selector.base.substr(1);
    for (const auto& cls : element->classes) {
      if (cls == class_name) {
        base_match = true;
        break;
      }
    }
  } else {
    base_match = (element->tag() == selector.base);
  }

  if (!base_match) {
    return false;
  }

  if (check_pseudos) {
    for (const auto& pseudo : selector.pseudo_classes) {
      if (pseudo == "hover" && !element->hovered()) {
        return false;
      }
      if (pseudo == "focus" && !element->focused()) {
        return false;
      }
      if (pseudo == "active" && !element->active()) {
        return false;
      }
    }
  }

  return true;
}

bool IsStyledByComponent(const Element* element,
                         const ComponentBase* component) {
  if (!element || !component) {
    return false;
  }
  if (element->component() == component) {
    return true;
  }
  if (element->owner_component() == component) {
    return true;
  }
  return false;
}

void ResolveStylesRecursive(Element* element,
                            const ComponentBase* component,
                            const std::unique_ptr<css::StyleSheet>& stylesheet,
                            bool check_pseudos) {
  if (!element) {
    return;
  }

  if (IsStyledByComponent(element, component)) {
    if (check_pseudos) {
      if (stylesheet) {
        for (const auto& ruleset : *stylesheet) {
          if (!css::EvaluateMediaQuery(ruleset.media_query)) {
            continue;
          }
          const auto& parsed = ruleset.parsed_selector;
          if (!parsed.pseudo_classes.empty() &&
              MatchSelector(element, component->Root(), parsed, true)) {
            for (const auto& declaration : ruleset.declarations) {
              ApplyStyle(element->target_style, declaration);
            }
          }
        }
      }
    } else {
      if (stylesheet) {
        for (const auto& ruleset : *stylesheet) {
          if (!css::EvaluateMediaQuery(ruleset.media_query)) {
            continue;
          }
          const auto& parsed = ruleset.parsed_selector;
          if (parsed.pseudo_classes.empty() &&
              MatchSelector(element, component->Root(), parsed, false)) {
            for (const auto& declaration : ruleset.declarations) {
              ApplyStyle(element->base_style, declaration);
            }
          }
        }
      }
    }
  }

  for (size_t i = 0; i < element->ChildCount(); ++i) {
    ResolveStylesRecursive(element->ChildAt(i), component, stylesheet,
                           check_pseudos);
  }
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
    std::vector<int> path;
    CollectElementStates(root_.get(), path, saved_states);
  }

  // Save projected slot children and clear parent pointers
  std::map<std::string, std::vector<Ref<Element>>> saved_slot_children;
  std::vector<Ref<ComponentBase>> saved_slot_components;
  for (auto& [name, slot_el] : slots_) {
    if (slot_el) {
      saved_slot_children[name] = slot_el->children();
      for (auto& child_el : slot_el->children()) {
        child_el->Visit([&](Element& el) {
          if (el.component()) {
            for (auto& comp : children_) {
              if (comp.get() == el.component()) {
                if (std::find(saved_slot_components.begin(),
                              saved_slot_components.end(),
                              comp) == saved_slot_components.end()) {
                  saved_slot_components.push_back(comp);
                }
              }
            }
          }
        });
      }
      slot_el->RemoveChildren();
    }
  }

  old_children_ =
      std::vector<Ref<ComponentBase>>(children_.begin(), children_.end());
  children_.clear();
  slots_.clear();

  css_strings_.clear();

  if (!root_) {
    root_ = Ref<Element>::New(this);
  }
  root_->style = ComputedStyle();
  root_->base_style = ComputedStyle();
  root_->target_style = ComputedStyle();
  root_->id = id_;
  root_->classes = classes_;
  root_->RemoveChildren();

  xml::Node template_node;
  template_node.type = xml::Node::Type::kElement;
  template_node.tag = "template";
  template_node.children.reserve(xml_nodes_.size());

  stylesheet_ = nullptr;

  for (const auto& node : xml_nodes_) {
    if (node.type == xml::Node::Type::kElement) {
      if (node.tag == "style") {
        if (node.children.empty() ||
            node.children[0].type != xml::Node::Type::kText) {
          continue;
        }
        css_strings_.push_back(
            Interpolate(node.children[0].text, this, nullptr));
        const auto& css_str = css_strings_.back();
        auto maybe_stylesheet = css::Parse(css_str);
        if (maybe_stylesheet) {
          stylesheet_ = std::make_unique<css::StyleSheet>(
              std::move(maybe_stylesheet.value()));
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

  // Restore projected slot children
  for (auto& [name, children] : saved_slot_children) {
    auto it = slots_.find(name);
    if (it != slots_.end() && it->second) {
      for (auto& child_el : children) {
        it->second->AddChild(child_el);
      }
    }
  }

  for (auto& comp : saved_slot_components) {
    children_.insert(comp);
  }

  ResolveStylesRecursive(root_.get(), this, stylesheet_, false);

  std::function<void(Element*)> CopyBaseStyles = [&](Element* element) {
    if (IsStyledByComponent(element, this)) {
      element->target_style = element->base_style;
      element->style = element->base_style;
    }
    for (size_t i = 0; i < element->ChildCount(); ++i) {
      CopyBaseStyles(element->ChildAt(i));
    }
  };
  CopyBaseStyles(root_.get());

  if (root_) {
    std::vector<int> path;
    RestoreElementStates(root_.get(), path, saved_states);
  }

  ResolveTargetStyles();
}

void ComponentBase::ResolveTargetStyles() {
  ResolveTargetStyles(time::GetTimeMs());
}

void ComponentBase::ResolveTargetStyles(double current_time_ms) {
  if (!root_) {
    return;
  }

  std::function<void(Element*)> ResetTarget = [&](Element* element) {
    if (element) {
      element->target_style = element->base_style;
      for (size_t i = 0; i < element->ChildCount(); ++i) {
        ResetTarget(element->ChildAt(i));
      }
    }
  };
  ResetTarget(root_.get());

  std::function<void(ComponentBase*)> ResolveAll = [&](ComponentBase* comp) {
    if (!comp || !comp->Root()) {
      return;
    }
    ResolveStylesRecursive(comp->Root(), comp, comp->stylesheet_, true);
    for (auto& child : comp->children_) {
      ResolveAll(child.get());
    }
  };
  ResolveAll(this);

  std::function<void(Element*)> TriggerAll = [&](Element* element) {
    if (element) {
      element->TriggerTransitions(current_time_ms);
      for (size_t i = 0; i < element->ChildCount(); ++i) {
        TriggerAll(element->ChildAt(i));
      }
    }
  };
  TriggerAll(root_.get());
}

void ComponentBase::Render(const xml::Node& node,
                           Element* slot,
                           ComponentBase* import_source,
                           std::shared_ptr<LocalScope> scope) {
  auto Interpolate = [&](std::string_view text) -> std::string {
    return rtxui::Interpolate(text, import_source, scope);
  };

  bool last_condition_chain_met = false;

  for (const auto& child_node : node.children) {
    switch (child_node.type) {
      case xml::Node::Type::kComment:
        break;

      case xml::Node::Type::kText: {
        std::string text = Interpolate(child_node.text);

        bool is_whitespace = true;
        for (char c : text) {
          if (!std::isspace(static_cast<unsigned char>(c))) {
            is_whitespace = false;
            break;
          }
        }
        if (!is_whitespace) {
          last_condition_chain_met = true;
        }

        bool preserve_newlines = false;
        for (Element* curr = slot; curr; curr = curr->Parent()) {
          if (curr->tag() == "textarea" || curr->tag() == "pre") {
            preserve_newlines = true;
            break;
          }
        }
        if (!preserve_newlines) {
          for (char& c : text) {
            if (c == '\n' || c == '\r') {
              c = ' ';
            }
          }
        }
        auto text_el = Ref<TextElement>::New(text);
        text_el->set_owner_component(import_source);
        slot->AddChild(text_el);
        break;
      }

      case xml::Node::Type::kElement: {
        if (child_node.tag == "style") {
          break;
        }

        if (child_node.tag == "if") {
          std::string cond = Interpolate(child_node.attributes.at("condition"));
          last_condition_chain_met = (cond == "true" || cond == "1");
          if (last_condition_chain_met) {
            Render(child_node, slot, import_source, scope);
          }
          break;
        }

        if (child_node.tag == "elif") {
          if (!last_condition_chain_met) {
            std::string cond =
                Interpolate(child_node.attributes.at("condition"));
            if (cond == "true" || cond == "1") {
              last_condition_chain_met = true;
              Render(child_node, slot, import_source, scope);
            }
          }
          break;
        }

        if (child_node.tag == "else") {
          if (!last_condition_chain_met) {
            Render(child_node, slot, import_source, scope);
          }
          last_condition_chain_met = true;
          break;
        }

        last_condition_chain_met = true;

        if (child_node.attributes.contains("if")) {
          std::string cond = Interpolate(child_node.attributes.at("if"));
          if (!(cond == "true" || cond == "1")) {
            break;
          }
        }

        if (child_node.tag == "for") {
          std::string each_attr = std::string(child_node.attributes.at("each"));
          // Strip {} if present
          std::string range_name = each_attr;
          if (range_name.starts_with("{") && range_name.ends_with("}")) {
            range_name = range_name.substr(1, range_name.size() - 2);
          }

          std::string as_attr = "item";
          if (child_node.attributes.contains("as")) {
            as_attr = std::string(child_node.attributes.at("as"));
          }

          std::shared_ptr<TypeErasedRange> range;
          for (const auto& entry : import_source->range_entries_) {
            if (entry.name == range_name) {
              range = entry.range;
              break;
            }
          }

          if (range) {
            for (size_t i = 0; i < range->Size(); ++i) {
              auto new_scope = std::make_shared<LocalScope>();
              new_scope->parent = scope;

              auto visitor = range->GetItemVisitor(i);
              if (visitor) {
                new_scope->variables[as_attr] = visitor;
              } else {
                new_scope->variables[as_attr] = range->GetItemString(i);
              }
              new_scope->variables["$index"] = std::to_string(i);

              Render(child_node, slot, import_source, new_scope);
            }
          }
          break;
        }

        if (child_node.tag == "slot" || child_node.tag.starts_with("slot.")) {
          std::string slot_name = child_node.tag == "slot"
                                      ? ""
                                      : std::string(child_node.tag.substr(5));
          auto slot_element = Ref<SlotElement>::New();
          slot_element->set_owner_component(import_source);
          import_source->slots_[slot_name] = slot_element;
          slot->AddChild(slot_element);
          break;
        }

        if (child_node.tag.starts_with("template.")) {
          std::string template_name = std::string(child_node.tag.substr(9));
          Ref<Element> target_slot = Slot(template_name);
          if (target_slot) {
            target_slot->RemoveChildren();
            Render(child_node, target_slot.get(), import_source, scope);
          }
          break;
        }

        ComponentFactory factory = nullptr;
        auto it = import_source->imports_.find(std::string(child_node.tag));
        if (it != import_source->imports_.end()) {
          factory = it->second;
        } else {
          factory = GetGlobalComponentFactory(child_node.tag);
        }

        if (factory) {
          Ref<ComponentBase> child;
          for (auto it_old = old_children_.begin();
               it_old != old_children_.end(); ++it_old) {
            if ((*it_old)->Tag() == child_node.tag) {
              child = *it_old;
              old_children_.erase(it_old);
              break;
            }
          }

          bool is_new = false;
          if (!child) {
            child = factory();
            is_new = true;
          }
          children_.insert(child);

          if (child_node.attributes.contains("id")) {
            child->id_ = Interpolate(child_node.attributes.at("id"));
          }
          if (child_node.attributes.contains("class")) {
            std::string interpolated_class =
                Interpolate(child_node.attributes.at("class"));
            auto class_views = Split(interpolated_class, ' ');
            child->classes_.assign(class_views.begin(), class_views.end());
          }

          if (is_new) {
            child->Mount();
          } else {
            child->two_way_bindings_.clear();
          }

          for (auto& [key_view, value] : child_node.attributes) {
            std::string key(key_view);
            std::string actual_value(value);

            // Handle Vue-style dynamic attribute prefix ':'
            if (key.starts_with(':')) {
              key = key.substr(1);
              if (!actual_value.starts_with('{')) {
                actual_value = "{" + actual_value + "}";
              }
            }

            // Handle Vue-style event prefix '@'
            if (key.starts_with('@')) {
              if (key == "@click") {
                key = "onclick";
              } else if (key == "@click.left") {
                key = "onclick";
              } else if (key == "@click.right") {
                key = "oncontextmenu";
              } else if (key == "@change") {
                key = "onchange";
              } else {
                // Generic mapping: @event -> onevent
                key = "on" + key.substr(1);
              }
            }

            if (key == "id" || key == "class") {
              continue;
            }
            std::string interpolated_value = Interpolate(actual_value);
            child->SetProperty(key, interpolated_value);
            child->Root()->SetAttribute(key, interpolated_value);

            if (actual_value.starts_with("{") && actual_value.ends_with("}")) {
              std::string parent_prop =
                  std::string(actual_value.substr(1, actual_value.size() - 2));
              child->two_way_bindings_.push_back(
                  {key, import_source, parent_prop});
            }
          }

          child->Render();
          child->Root()->set_owner_component(import_source);

          slot->AddChild(child->Root());

          Ref<Element> default_slot = child->Slot("");
          if (default_slot) {
            default_slot->RemoveChildren();
            child->Render(child_node, default_slot.get(), import_source, scope);
          }
          break;
        }

        auto child_element = Ref<Element>::New();
        child_element->set_owner_component(import_source);
        child_element->SetTag(std::string(child_node.tag));
        for (auto& [key_view, value] : child_node.attributes) {
          std::string key(key_view);
          std::string actual_value(value);

          // Handle Vue-style dynamic attribute prefix ':'
          if (key.starts_with(':')) {
            key = key.substr(1);
            if (!actual_value.starts_with('{')) {
              actual_value = "{" + actual_value + "}";
            }
          }

          // Handle Vue-style event prefix '@'
          if (key.starts_with('@')) {
            if (key == "@click") {
              key = "onclick";
            } else if (key == "@click.left") {
              key = "onclick";
            } else if (key == "@click.right") {
              key = "oncontextmenu";
            } else if (key == "@change") {
              key = "onchange";
            } else {
              // Generic mapping: @event -> onevent
              key = "on" + key.substr(1);
            }
          }

          child_element->SetAttribute(key, Interpolate(actual_value));
        }
        slot->AddChild(child_element);
        Render(child_node, child_element.get(), import_source, scope);
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
      PropagateBinding(target, value);
      return;
    }
  }
}

void ComponentBase::PropagateBinding(std::string_view child_prop,
                                     std::string_view value) {
  std::string_view clean_child_prop = child_prop;
  if (clean_child_prop.starts_with("props.")) {
    clean_child_prop = clean_child_prop.substr(6);
  }
  for (const auto& binding : two_way_bindings_) {
    std::string_view clean_binding_prop = binding.child_prop;
    if (clean_binding_prop.starts_with("props.")) {
      clean_binding_prop = clean_binding_prop.substr(6);
    }
    if (clean_binding_prop == clean_child_prop) {
      binding.parent->SetProperty(binding.parent_prop, value);
    }
  }
}

const css::StyleSheet* ComponentBase::stylesheet() const {
  return stylesheet_.get();
}

namespace reflection {
int ParseInt(std::string_view str) {
  while (!str.empty() &&
         std::isspace(static_cast<unsigned char>(str.front()))) {
    str.remove_prefix(1);
  }
  while (!str.empty() && std::isspace(static_cast<unsigned char>(str.back()))) {
    str.remove_suffix(1);
  }
  int val = 0;
  auto [ptr, ec] = std::from_chars(str.data(), str.data() + str.size(), val);
  if (ec == std::errc()) {
    return val;
  }
  return 0;
}
}  // namespace reflection

}  // namespace rtxui
