#include "rtxui/internal/component.hpp"

#include <algorithm>
#include <array>
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
#include "rtxui/component/default_components_internal.hpp"

#include "rtxui/component/component_internal.hpp"

namespace rtxui {

ComponentBase* GetOwningComponent(Element* element) {
  while (element) {
    if (element->component()) {
      return const_cast<ComponentBase*>(element->component());
    }
    element = element->Parent();
  }
  return nullptr;
}

ComponentBase* GetAttributeOwnerComponent(Element* element) {
  if (!element) {
    return nullptr;
  }
  if (element->component()) {
    return GetOwningComponent(element->Parent());
  }
  return GetOwningComponent(element);
}

ComponentBase* GetParentComponent(ComponentBase* comp) {
  if (!comp || !comp->Root()) {
    return nullptr;
  }
  return GetOwningComponent(comp->Root()->Parent());
}

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


std::string_view ComponentBase::Setup() {
  return "";
}

void ComponentBase::InitReflection() {}

namespace {
ComponentBase* g_mouse_capturer = nullptr;
}  // namespace

void ComponentBase::CaptureMouse() {
  g_mouse_capturer = this;
}

void ComponentBase::ReleaseMouse() {
  if (g_mouse_capturer == this) {
    g_mouse_capturer = nullptr;
  }
}

ComponentBase* ComponentBase::GetMouseCapturer() {
  return g_mouse_capturer;
}

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
  static auto* registry = []() {
    auto* reg = new std::unordered_map<std::string, ComponentFactory>();
    (*reg)["b"] = []() { return Ref<b>::New(); };
    (*reg)["button"] = []() { return Ref<button>::New(); };
    (*reg)["checkbox"] = []() { return Ref<checkbox>::New(); };
    (*reg)["div"] = []() { return Ref<div>::New(); };
    (*reg)["h1"] = []() { return Ref<h1>::New(); };
    (*reg)["h2"] = []() { return Ref<h2>::New(); };
    (*reg)["h3"] = []() { return Ref<h3>::New(); };
    (*reg)["h4"] = []() { return Ref<h4>::New(); };
    (*reg)["h5"] = []() { return Ref<h5>::New(); };
    (*reg)["h6"] = []() { return Ref<h6>::New(); };
    (*reg)["hr"] = []() { return Ref<hr>::New(); };
    (*reg)["input"] = []() { return Ref<input>::New(); };
    (*reg)["li"] = []() { return Ref<li>::New(); };
    (*reg)["markdown"] = []() { return Ref<markdown>::New(); };
    (*reg)["ol"] = []() { return Ref<ol>::New(); };
    (*reg)["option"] = []() { return Ref<option>::New(); };
    (*reg)["p"] = []() { return Ref<p>::New(); };
    (*reg)["progress"] = []() { return Ref<progress>::New(); };
    (*reg)["select"] = []() { return Ref<select>::New(); };
    (*reg)["slider"] = []() { return Ref<slider>::New(); };
    (*reg)["span"] = []() { return Ref<span>::New(); };
    (*reg)["strong"] = []() { return Ref<strong>::New(); };
    (*reg)["textarea"] = []() { return Ref<textarea>::New(); };
    (*reg)["ul"] = []() { return Ref<ul>::New(); };
    return reg;
  }();
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

struct CategorizedRules {
  std::vector<const css::Ruleset*> universal;
  std::unordered_map<std::string_view, std::vector<const css::Ruleset*>> by_id;
  std::unordered_map<std::string_view, std::vector<const css::Ruleset*>> by_class;
  std::unordered_map<std::string_view, std::vector<const css::Ruleset*>> by_tag;
  bool has_pseudo_classes = false;
};

ComponentBase::ComponentBase() = default;
ComponentBase::~ComponentBase() {
  ReleaseMouse();
}


namespace {

struct ElementPath {
  std::array<int, 32> indices{};
  size_t depth = 0;

  void push_back(int index) noexcept {
    if (depth < indices.size()) {
      indices[depth++] = index;
    }
  }

  void pop_back() noexcept {
    if (depth > 0) {
      depth--;
    }
  }

  bool operator<(const ElementPath& other) const noexcept {
    if (depth != other.depth) {
      return depth < other.depth;
    }
    for (size_t i = 0; i < depth; ++i) {
      if (indices[i] != other.indices[i]) {
        return indices[i] < other.indices[i];
      }
    }
    return false;
  }

  bool operator==(const ElementPath& other) const noexcept {
    if (depth != other.depth) {
      return false;
    }
    for (size_t i = 0; i < depth; ++i) {
      if (indices[i] != other.indices[i]) {
        return false;
      }
    }
    return true;
  }
};

struct ElementState {
  int scroll_x = 0;
  int scroll_y = 0;
  bool focused = false;
  bool hovered = false;
  bool active = false;
  bool scrollbar_hovered = false;
  bool scrollbar_active = false;
  bool scrollbar_thumb_hovered = false;
  bool scrollbar_thumb_active = false;
  ComputedStyle style;
  ActiveTransitionsMap active_transitions;
};

void CollectElementStates(Element* el,
                          ElementPath& path,
                          std::vector<std::pair<ElementPath, ElementState>>& states) {
  if (!el) {
    return;
  }
  bool has_state = el->scroll_x() != 0 ||
                   el->scroll_y() != 0 ||
                   el->focused() ||
                   el->hovered() ||
                   el->active() ||
                   el->scrollbar_hovered() ||
                   el->scrollbar_active() ||
                   el->scrollbar_thumb_hovered() ||
                   el->scrollbar_thumb_active() ||
                   !el->active_transitions.empty() ||
                   (el->target_style.transitions && !el->target_style.transitions->empty());
  if (has_state) {
    states.push_back({path, {el->scroll_x(), el->scroll_y(), el->focused(), el->hovered(), el->active(),
                             el->scrollbar_hovered(), el->scrollbar_active(), el->scrollbar_thumb_hovered(), el->scrollbar_thumb_active(),
                             el->style, el->active_transitions}});
  }
  for (size_t i = 0; i < el->ChildCount(); ++i) {
    path.push_back(static_cast<int>(i));
    CollectElementStates(el->ChildAt(i), path, states);
    path.pop_back();
  }
}

void RestoreElementStates(
    Element* el,
    ElementPath& path,
    const std::vector<std::pair<ElementPath, ElementState>>& states) {
  if (!el) {
    return;
  }
  for (const auto& pair : states) {
    if (pair.first == path) {
      el->set_scroll_x(pair.second.scroll_x);
      el->set_scroll_y(pair.second.scroll_y);
      el->set_focused(pair.second.focused);
      el->set_hovered(pair.second.hovered);
      el->set_active(pair.second.active);
      el->set_scrollbar_hovered(pair.second.scrollbar_hovered);
      el->set_scrollbar_active(pair.second.scrollbar_active);
      el->set_scrollbar_thumb_hovered(pair.second.scrollbar_thumb_hovered);
      el->set_scrollbar_thumb_active(pair.second.scrollbar_thumb_active);
      el->style = pair.second.style;
      el->active_transitions = pair.second.active_transitions;
      break;
    }
  }
  for (size_t i = 0; i < el->ChildCount(); ++i) {
    path.push_back(static_cast<int>(i));
    RestoreElementStates(el->ChildAt(i), path, states);
    path.pop_back();
  }
}

void RestoreElementFocusHoverActive(
    Element* el,
    ElementPath& path,
    const std::vector<std::pair<ElementPath, ElementState>>& states) {
  if (!el) {
    return;
  }
  for (const auto& pair : states) {
    if (pair.first == path) {
      el->set_focused(pair.second.focused);
      el->set_hovered(pair.second.hovered);
      el->set_active(pair.second.active);
      el->set_scrollbar_hovered(pair.second.scrollbar_hovered);
      el->set_scrollbar_active(pair.second.scrollbar_active);
      el->set_scrollbar_thumb_hovered(pair.second.scrollbar_thumb_hovered);
      el->set_scrollbar_thumb_active(pair.second.scrollbar_thumb_active);
      break;
    }
  }
  for (size_t i = 0; i < el->ChildCount(); ++i) {
    path.push_back(static_cast<int>(i));
    RestoreElementFocusHoverActive(el->ChildAt(i), path, states);
    path.pop_back();
  }
}


std::string Interpolate(std::string_view text,
                        ComponentBase* source,
                        std::shared_ptr<LocalScope> scope) {
  if (text.find('{') == std::string_view::npos) {
    return std::string(text);
  }
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

  // Sort placeholders in ascending order of open_idx (left-to-right)
  std::sort(placeholders.begin(), placeholders.end(),
            [](const Placeholder& a, const Placeholder& b) {
              return a.open_idx < b.open_idx;
            });

  std::string result;
  result.reserve(text.size() + placeholders.size() * 16);
  size_t last_pos = 0;
  for (const auto& ph : placeholders) {
    if (ph.open_idx > last_pos) {
      result.append(text.substr(last_pos, ph.open_idx - last_pos));
    }
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
    result.append(val);
    last_pos = ph.close_idx + 1;
  }
  if (last_pos < text.size()) {
    result.append(text.substr(last_pos));
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
      if (pseudo == "scrollbar-hover" && !element->scrollbar_hovered()) {
        return false;
      }
      if (pseudo == "scrollbar-active" && !element->scrollbar_active()) {
        return false;
      }
      if (pseudo == "scrollbar-thumb-hover" && !element->scrollbar_thumb_hovered()) {
        return false;
      }
      if (pseudo == "scrollbar-thumb-active" && !element->scrollbar_thumb_active()) {
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

bool MatchPseudos(const Element* element, const std::vector<std::string>& pseudo_classes) {
  for (const auto& pseudo : pseudo_classes) {
    if (pseudo == "hover" && !element->hovered()) {
      return false;
    }
    if (pseudo == "focus" && !element->focused()) {
      return false;
    }
    if (pseudo == "active" && !element->active()) {
      return false;
    }
    if (pseudo == "scrollbar-hover" && !element->scrollbar_hovered()) {
      return false;
    }
    if (pseudo == "scrollbar-active" && !element->scrollbar_active()) {
      return false;
    }
    if (pseudo == "scrollbar-thumb-hover" && !element->scrollbar_thumb_hovered()) {
      return false;
    }
    if (pseudo == "scrollbar-thumb-active" && !element->scrollbar_thumb_active()) {
      return false;
    }
  }
  return true;
}

void ResolveStylesRecursive(Element* element,
                             const ComponentBase* component,
                             bool check_pseudos) {
  if (!element || !component || !component->categorized_rules()) {
    return;
  }

  if (IsStyledByComponent(element, component)) {
    if (!check_pseudos) {
      if (element->IsStyleResolvedFor(component)) {
        goto recurse;
      }
      if (!element->styled_by_1 && !element->styled_by_2) {
        element->base_style = ComputedStyle();
      }
    }

    const auto* categorized = component->categorized_rules();

    auto match_and_apply = [&](const std::vector<const css::Ruleset*>& rulesets, bool is_universal) {
      for (const auto* ruleset : rulesets) {
        if (!css::EvaluateMediaQuery(ruleset->media_query)) {
          continue;
        }
        const auto& parsed = ruleset->parsed_selector;
        if (is_universal && parsed.base == "self" && element != component->Root()) {
          continue;
        }

        bool classes_match = true;
        for (const auto& required_class : parsed.classes) {
          bool found = false;
          for (const auto& el_class : element->classes) {
            if (el_class == required_class) {
              found = true;
              break;
            }
          }
          if (!found) {
            classes_match = false;
            break;
          }
        }
        if (!classes_match) continue;

        bool attributes_match = true;
        for (const auto& attr : parsed.attributes) {
          const std::string* el_attr = element->GetAttribute(attr.name);
          if (!el_attr) {
            attributes_match = false;
            break;
          }
          if (attr.has_value) {
            if (*el_attr != attr.value) {
              attributes_match = false;
              break;
            }
          }
        }
        if (!attributes_match) continue;

        if (check_pseudos) {
          if (!parsed.pseudo_classes.empty() && MatchPseudos(element, parsed.pseudo_classes)) {
            for (const auto& declaration : ruleset->declarations) {
              ApplyStyle(element->target_style, declaration);
            }
          }
        } else {
          if (parsed.pseudo_classes.empty()) {
            for (const auto& declaration : ruleset->declarations) {
              ApplyStyle(element->base_style, declaration);
            }
          }
        }
      }
    };

    match_and_apply(categorized->universal, true);

    auto it_tag = categorized->by_tag.find(element->tag());
    if (it_tag != categorized->by_tag.end()) {
      match_and_apply(it_tag->second, false);
    }

    if (!element->id.empty()) {
      auto it_id = categorized->by_id.find(element->id);
      if (it_id != categorized->by_id.end()) {
        match_and_apply(it_id->second, false);
      }
    }

    for (const auto& cls : element->classes) {
      auto it_class = categorized->by_class.find(cls);
      if (it_class != categorized->by_class.end()) {
        match_and_apply(it_class->second, false);
      }
    }

    if (!check_pseudos) {
      element->MarkStyleResolvedFor(component);
    }
  }

recurse:
  if (element->component() && element->component() != component) {
    bool has_slot_children = false;
    for (const auto& [name, slot_el] : element->component()->slots()) {
      if (slot_el && slot_el->ChildCount() > 0) {
        has_slot_children = true;
        break;
      }
    }
    if (!has_slot_children) {
      return;
    }
  }

  for (size_t i = 0; i < element->ChildCount(); ++i) {
    ResolveStylesRecursive(element->ChildAt(i), component,
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
  last_render_terminal_width_ = css::g_terminal_width;
  last_render_terminal_height_ = css::g_terminal_height;
  // Optimization: Use a flat vector of pairs instead of std::map<ElementPath, ElementState>.
  // Since very few elements actually hold state (scroll, focus, transitions), this avoids
  // the dynamic allocation and key-comparison overhead of a red-black tree map.
  std::vector<std::pair<ElementPath, ElementState>> saved_states;
  Element* saved_parent = nullptr;
  if (root_) {
    saved_parent = root_->Parent();
    root_->set_parent(nullptr);
    ElementPath path;
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

  if (!root_) {
    root_ = Ref<Element>::New(this);
  }
  root_->style = ComputedStyle();
  root_->base_style = ComputedStyle();
  root_->target_style = ComputedStyle();
  root_->ClearResolvedStyles();
  root_->id = id_;
  root_->classes = classes_;

  xml::Node template_node;
  template_node.type = xml::Node::Type::kElement;
  template_node.tag = "template";
  template_node.children.reserve(xml_nodes_.size());

  std::vector<std::string> new_css_strings;
  for (const auto& node : xml_nodes_) {
    if (node.type == xml::Node::Type::kElement && node.tag == "style") {
      if (!node.children.empty() &&
          node.children[0].type == xml::Node::Type::kText) {
        new_css_strings.push_back(
            Interpolate(node.children[0].text, this, nullptr));
      }
    }
  }

  bool css_changed = (new_css_strings != css_strings_);
  if (css_changed) {
    if (root_) {
      root_->Visit([](Element& el) {
        el.ClearResolvedStyles();
      });
    }
    css_strings_ = std::move(new_css_strings);
    stylesheet_ = nullptr;
    categorized_rules_ = nullptr;
    for (const auto& css_str : css_strings_) {
      auto maybe_stylesheet = css::Parse(css_str);
      if (maybe_stylesheet) {
        stylesheet_ = std::make_unique<css::StyleSheet>(
            std::move(maybe_stylesheet.value()));
        categorized_rules_ = std::make_unique<CategorizedRules>();
        for (const auto& ruleset : *stylesheet_) {
          if (!ruleset.parsed_selector.pseudo_classes.empty()) {
            categorized_rules_->has_pseudo_classes = true;
          }
          std::string_view selector = ruleset.parsed_selector.base;
          if (selector == "self") {
            categorized_rules_->universal.push_back(&ruleset);
          } else if (selector.starts_with("#")) {
            categorized_rules_->by_id[selector.substr(1)].push_back(&ruleset);
          } else if (selector.starts_with(".")) {
            categorized_rules_->by_class[selector.substr(1)].push_back(&ruleset);
          } else if (selector.empty()) {
            categorized_rules_->universal.push_back(&ruleset);
          } else {
            categorized_rules_->by_tag[selector].push_back(&ruleset);
          }
        }
      } else {
        CssParseError(maybe_stylesheet.error(), css_str);
      }
    }
  }

  for (const auto& node : xml_nodes_) {
    if (node.type == xml::Node::Type::kElement) {
      if (node.tag == "style") {
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

  if (root_) {
    ElementPath path;
    RestoreElementFocusHoverActive(root_.get(), path, saved_states);
  }

  ResolveStylesRecursive(root_.get(), this, false);
  if (root_ && root_->owner_component() && root_->owner_component() != this) {
    ResolveStylesRecursive(root_.get(), root_->owner_component(), false);
  }

  auto CopyBaseStyles = [&](auto& self, Element* element) -> void {
    if (element) {
      if (IsStyledByComponent(element, this)) {
        element->target_style = element->base_style;
        element->style = element->base_style;
      }
      for (size_t i = 0; i < element->ChildCount(); ++i) {
        self(self, element->ChildAt(i));
      }
    }
  };
  CopyBaseStyles(CopyBaseStyles, root_.get());

  if (root_) {
    ElementPath path;
    RestoreElementStates(root_.get(), path, saved_states);
    if (saved_parent) {
      root_->set_parent(saved_parent);
    }
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

  if (!HasAnyPseudoClasses()) {
    auto HasActive = [](auto& self, Element* element) -> bool {
      if (!element) return false;
      if (!element->active_transitions.empty() || element->IsAnimatingScroll()) return true;
      for (size_t i = 0; i < element->ChildCount(); ++i) {
        if (self(self, element->ChildAt(i))) return true;
      }
      return false;
    };
    if (!HasActive(HasActive, root_.get())) {
      return;
    }
  }

  auto ResetTarget = [](auto& self, Element* element) -> void {
    if (element) {
      element->target_style = element->base_style;
      for (size_t i = 0; i < element->ChildCount(); ++i) {
        self(self, element->ChildAt(i));
      }
    }
  };
  ResetTarget(ResetTarget, root_.get());

  auto ResolveAll = [](auto& self, ComponentBase* comp, ComponentBase* root_comp) -> void {
    if (!comp || !comp->Root()) {
      return;
    }
    // Optimization: Skip resolving target styles if the component stylesheet has no
    // pseudo-classes (hover, active, focus). Yields ~18% speedup in DOM Digest.
    if (comp->categorized_rules() && comp->categorized_rules()->has_pseudo_classes) {
      ResolveStylesRecursive(comp->Root(), comp, true);
    }
    if (comp == root_comp && comp->Root()->owner_component() && comp->Root()->owner_component() != comp) {
      const ComponentBase* owner = comp->Root()->owner_component();
      if (owner->categorized_rules() && owner->categorized_rules()->has_pseudo_classes) {
        ResolveStylesRecursive(comp->Root(), owner, true);
      }
    }
    for (auto& child : comp->children_) {
      self(self, child.get(), root_comp);
    }
  };
  ResolveAll(ResolveAll, this, this);

  auto TriggerAll = [](auto& self, Element* element, double current_time_ms) -> void {
    if (element) {
      element->TriggerTransitions(current_time_ms);
      for (size_t i = 0; i < element->ChildCount(); ++i) {
        self(self, element->ChildAt(i), current_time_ms);
      }
    }
  };
  TriggerAll(TriggerAll, root_.get(), current_time_ms);
}

void ComponentBase::Render(const xml::Node& node,
                           Element* slot,
                           ComponentBase* import_source,
                           std::shared_ptr<LocalScope> scope) {
  size_t child_idx = 0;
  bool preserve = false;
  for (Element* curr = slot; curr; curr = curr->Parent()) {
    if (curr->tag() == "textarea" || curr->tag() == "pre" || curr->tag() == "slider") {
      preserve = true;
      break;
    }
  }
  RenderReconcile(node, slot, import_source, scope, child_idx, preserve);
  slot->TruncateChildren(child_idx);
}

void ComponentBase::RenderReconcile(const xml::Node& node,
                                    Element* slot,
                                    ComponentBase* import_source,
                                    std::shared_ptr<LocalScope> scope,
                                    size_t& child_idx,
                                    bool preserve_newlines) {
  Ref<Element> slot_keep_alive(slot);
  auto Interpolate = [&](std::string_view text) -> std::string {
    return rtxui::Interpolate(text, import_source, scope);
  };

  bool last_condition_chain_met = false;

  for (const auto& child_node : node.children) {
    switch (child_node.type) {
      case xml::Node::Type::kComment:
        break;

      case xml::Node::Type::kText: {
        bool has_placeholder = (child_node.text.find('{') != std::string_view::npos);
        bool has_newline = !preserve_newlines && (child_node.text.find('\n') != std::string_view::npos || child_node.text.find('\r') != std::string_view::npos);

        if (!has_placeholder && !has_newline) {
          bool is_whitespace = true;
          for (char c : child_node.text) {
            if (!std::isspace(static_cast<unsigned char>(c))) {
              is_whitespace = false;
              break;
            }
          }
          if (!is_whitespace) {
            last_condition_chain_met = true;
          }

          if (child_idx < slot->ChildCount() && slot->ChildAt(child_idx)->is_text()) {
            auto* text_el = static_cast<TextElement*>(slot->ChildAt(child_idx));
            if (text_el->text() != child_node.text) {
              text_el->set_text(std::string(child_node.text));
            }
          } else {
            // Look ahead for a text element
            size_t match_idx = -1;
            for (size_t i = child_idx + 1; i < slot->ChildCount(); ++i) {
              if (slot->ChildAt(i)->is_text()) {
                match_idx = i;
                break;
              }
            }
            Ref<Element> text_el;
            if (match_idx != -1) {
              slot->MoveChild(match_idx, child_idx);
              text_el = slot->children()[child_idx];
              auto* t_el = static_cast<TextElement*>(text_el.get());
              if (t_el->text() != child_node.text) {
                t_el->set_text(std::string(child_node.text));
              }
            } else {
              text_el = Ref<TextElement>::New(std::string(child_node.text));
              text_el->set_owner_component(import_source);
              if (child_idx < slot->ChildCount()) {
                slot->ReplaceChild(child_idx, text_el);
              } else {
                slot->AddChild(text_el);
              }
            }
          }
          child_idx++;
          break;
        }

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

        if (!preserve_newlines) {
          for (char& c : text) {
            if (c == '\n' || c == '\r') {
              c = ' ';
            }
          }
        }

        // Reconcile/reuse or replace TextElement
        if (child_idx < slot->ChildCount() && slot->ChildAt(child_idx)->is_text()) {
          auto* text_el = static_cast<TextElement*>(slot->ChildAt(child_idx));
          if (text_el->text() != text) {
            text_el->set_text(text);
          }
        } else {
          // Look ahead for a text element
          size_t match_idx = -1;
          for (size_t i = child_idx + 1; i < slot->ChildCount(); ++i) {
            if (slot->ChildAt(i)->is_text()) {
              match_idx = i;
              break;
            }
          }
          Ref<Element> text_el;
          if (match_idx != -1) {
            slot->MoveChild(match_idx, child_idx);
            text_el = slot->children()[child_idx];
            auto* t_el = static_cast<TextElement*>(text_el.get());
            if (t_el->text() != text) {
              t_el->set_text(text);
            }
          } else {
            text_el = Ref<TextElement>::New(text);
            text_el->set_owner_component(import_source);
            if (child_idx < slot->ChildCount()) {
              slot->ReplaceChild(child_idx, text_el);
            } else {
              slot->AddChild(text_el);
            }
          }
        }
        child_idx++;
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
            RenderReconcile(child_node, slot, import_source, scope, child_idx, preserve_newlines);
          }
          break;
        }

        if (child_node.tag == "elif") {
          if (!last_condition_chain_met) {
            std::string cond =
                Interpolate(child_node.attributes.at("condition"));
            if (cond == "true" || cond == "1") {
              last_condition_chain_met = true;
              RenderReconcile(child_node, slot, import_source, scope, child_idx, preserve_newlines);
            }
          }
          break;
        }

        if (child_node.tag == "else") {
          if (!last_condition_chain_met) {
            RenderReconcile(child_node, slot, import_source, scope, child_idx, preserve_newlines);
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

              RenderReconcile(child_node, slot, import_source, new_scope, child_idx, preserve_newlines);
            }
          }
          break;
        }

        if (child_node.tag == "slot" || child_node.tag.starts_with("slot.")) {
          std::string slot_name = child_node.tag == "slot"
                                      ? ""
                                      : std::string(child_node.tag.substr(5));

          Ref<Element> slot_element;
          if (child_idx < slot->ChildCount() && slot->ChildAt(child_idx)->is_slot()) {
            slot_element = slot->children()[child_idx];
          } else {
            // Look ahead for a slot element
            size_t match_idx = -1;
            for (size_t i = child_idx + 1; i < slot->ChildCount(); ++i) {
              if (slot->ChildAt(i)->is_slot()) {
                match_idx = i;
                break;
              }
            }
            if (match_idx != -1) {
              slot->MoveChild(match_idx, child_idx);
              slot_element = slot->children()[child_idx];
            } else {
              slot_element = Ref<SlotElement>::New();
              slot_element->set_owner_component(import_source);
              if (child_idx < slot->ChildCount()) {
                slot->ReplaceChild(child_idx, slot_element);
              } else {
                slot->AddChild(slot_element);
              }
            }
          }
          import_source->slots_[slot_name] = slot_element;
          child_idx++;
          break;
        }

        if (child_node.tag.starts_with("template.")) {
          std::string template_name = std::string(child_node.tag.substr(9));
          Ref<Element> target_slot = Slot(template_name);
          if (target_slot) {
            size_t sub_child_idx = 0;
            RenderReconcile(child_node, target_slot.get(), import_source, scope, sub_child_idx, preserve_newlines);
            target_slot->TruncateChildren(sub_child_idx);
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

          bool id_changed = false;
          if (child_node.attributes.contains("id")) {
            std::string new_id = Interpolate(child_node.attributes.at("id"));
            if (new_id != child->id_) {
              id_changed = true;
              child->id_ = std::move(new_id);
            }
          } else if (!child->id_.empty()) {
            id_changed = true;
            child->id_.clear();
          }

          bool classes_changed = false;
          if (child_node.attributes.contains("class")) {
            std::string interpolated_class =
                Interpolate(child_node.attributes.at("class"));
            auto class_views = Split(interpolated_class, ' ');
            std::vector<std::string> new_classes(class_views.begin(), class_views.end());
            if (new_classes != child->classes_) {
              classes_changed = true;
              child->classes_ = std::move(new_classes);
            }
          } else if (!child->classes_.empty()) {
            classes_changed = true;
            child->classes_.clear();
          }

          if (is_new) {
            child->Mount();
          } else {
            child->two_way_bindings_.clear();
          }

          bool attribute_changed = false;
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

            const std::string* current_val = child->Root() ? child->Root()->GetAttribute(key) : nullptr;
            if (!current_val || *current_val != interpolated_value) {
              attribute_changed = true;
            }

            child->SetProperty(key, interpolated_value);
            if (child->Root()) {
              child->Root()->SetAttribute(key, interpolated_value);
            }

            if (actual_value.starts_with("{") && actual_value.ends_with("}")) {
              std::string parent_prop =
                  std::string(actual_value.substr(1, actual_value.size() - 2));
              child->two_way_bindings_.push_back(
                  {key, import_source, parent_prop});
            }
          }

          bool terminal_size_changed = (child->last_render_terminal_width_ != css::g_terminal_width ||
                                        child->last_render_terminal_height_ != css::g_terminal_height);

          bool needs_render = is_new || id_changed || classes_changed || attribute_changed || terminal_size_changed;

          if (needs_render) {
            child->Render();
          } else {
            child->Digest();
          }
          child->Root()->set_owner_component(import_source);

          // Now reconcile slot with child->Root()
          if (child_idx < slot->ChildCount() && slot->ChildAt(child_idx) == child->Root()) {
            child->Root()->set_parent(slot);
          } else {
            size_t match_idx = -1;
            for (size_t i = child_idx + 1; i < slot->ChildCount(); ++i) {
              if (slot->ChildAt(i) == child->Root()) {
                match_idx = i;
                break;
              }
            }
            if (match_idx != -1) {
              slot->MoveChild(match_idx, child_idx);
            } else {
              if (child_idx < slot->ChildCount()) {
                slot->ReplaceChild(child_idx, child->Root());
              } else {
                slot->AddChild(child->Root());
              }
            }
          }

          Ref<Element> default_slot = child->Slot("");
          if (default_slot) {
            if (!needs_render) {
              std::vector<Ref<ComponentBase>> slot_components;
              for (auto& [name, slot_el] : child->slots()) {
                if (slot_el) {
                  for (auto& child_el : slot_el->children()) {
                    child_el->Visit([&](Element& el) {
                      if (el.component()) {
                        for (auto& comp : child->children_) {
                          if (comp.get() == el.component()) {
                            if (std::find(slot_components.begin(),
                                          slot_components.end(),
                                          comp) == slot_components.end()) {
                              slot_components.push_back(comp);
                            }
                          }
                        }
                      }
                    });
                  }
                }
              }
              child->old_children_ = slot_components;
              for (auto& comp : slot_components) {
                child->children_.erase(comp);
              }
            }

            size_t sub_child_idx = 0;
            child->RenderReconcile(child_node, default_slot.get(), import_source, scope, sub_child_idx, preserve_newlines);
            default_slot->TruncateChildren(sub_child_idx);

            if (!needs_render) {
              child->old_children_.clear();
            }
          }
          child_idx++;
          break;
        }

        // Reconcile standard element
        Ref<Element> child_element;
        bool is_reused = false;

        if (child_idx < slot->ChildCount() &&
            !slot->ChildAt(child_idx)->is_text() &&
            !slot->ChildAt(child_idx)->is_slot() &&
            slot->ChildAt(child_idx)->tag() == child_node.tag) {
          child_element = slot->children()[child_idx];
          is_reused = true;
        } else {
          // Look ahead for an element with the same tag
          size_t match_idx = -1;
          for (size_t i = child_idx + 1; i < slot->ChildCount(); ++i) {
            if (!slot->ChildAt(i)->is_text() &&
                !slot->ChildAt(i)->is_slot() &&
                !slot->ChildAt(i)->component() &&
                slot->ChildAt(i)->tag() == child_node.tag) {
              match_idx = i;
              break;
            }
          }
          if (match_idx != -1) {
            slot->MoveChild(match_idx, child_idx);
            child_element = slot->children()[child_idx];
            is_reused = true;
          } else {
            child_element = Ref<Element>::New();
            child_element->set_owner_component(import_source);
            child_element->SetTag(std::string(child_node.tag));
          }
        }

        std::vector<std::string> updated_keys;
        for (auto& [key_view, value] : child_node.attributes) {
          std::string key(key_view);
          bool dynamic = false;
          if (key.starts_with(':')) {
            key = key.substr(1);
            dynamic = true;
          }

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
              key = "on" + key.substr(1);
            }
          }

          if (dynamic) {
            std::string actual_value(value);
            if (!actual_value.starts_with('{')) {
              actual_value = "{" + actual_value + "}";
            }
            std::string new_val = Interpolate(actual_value);
            const std::string* current_val = child_element->GetAttribute(key);
            if (!current_val || *current_val != new_val) {
              child_element->SetAttribute(key, std::move(new_val));
            }
          } else {
            if (value.find('{') == std::string_view::npos) {
              const std::string* current_val = child_element->GetAttribute(key);
              if (!current_val || *current_val != value) {
                child_element->SetAttribute(key, std::string(value));
              }
            } else {
              std::string new_val = Interpolate(value);
              const std::string* current_val = child_element->GetAttribute(key);
              if (!current_val || *current_val != new_val) {
                child_element->SetAttribute(key, std::move(new_val));
              }
            }
          }
          updated_keys.push_back(std::move(key));
        }

        if (is_reused) {
          // Remove obsolete attributes
          std::vector<std::string> keys_to_remove;
          for (const auto& [key, val] : child_element->Attributes()) {
            if (std::find(updated_keys.begin(), updated_keys.end(), key) == updated_keys.end()) {
              keys_to_remove.push_back(key);
            }
          }
          for (const auto& key : keys_to_remove) {
            child_element->RemoveAttribute(key);
          }
        }

        if (!is_reused) {
          if (child_idx < slot->ChildCount()) {
            slot->ReplaceChild(child_idx, child_element);
          } else {
            slot->AddChild(child_element);
          }
        }

        size_t sub_child_idx = 0;
        RenderReconcile(child_node, child_element.get(), import_source, scope, sub_child_idx,
                        preserve_newlines || child_element->tag() == "textarea" ||
                            child_element->tag() == "pre" || child_element->tag() == "slider");
        child_element->TruncateChildren(sub_child_idx);

        child_idx++;
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

bool ComponentBase::HasAnyPseudoClasses() const {
  if (categorized_rules_ && categorized_rules_->has_pseudo_classes) {
    return true;
  }
  for (const auto& child : children_) {
    if (child && child->HasAnyPseudoClasses()) {
      return true;
    }
  }
  return false;
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
