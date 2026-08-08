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

#include "rtxui/component/component_internal.hpp"
#include "rtxui/component/default_components_internal.hpp"
#include "rtxui/base/string.hpp"
#include "rtxui/dom/element.hpp"
#include "rtxui/dom/slot_element.hpp"
#include "rtxui/dom/text_element.hpp"
#include "rtxui/paint/color.hpp"
#include "rtxui/style/apply_style.hpp"
#include "rtxui/style/style.hpp"
#include "rtxui/xml/xml.hpp"

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

void FocusExclusive(Element* element) {
  if (!element) {
    return;
  }
  if (element->Parent()) {
    Element* document_root = element;
    while (document_root->Parent()) {
      document_root = document_root->Parent();
    }
    document_root->Visit([](Element& el) { el.set_focused(false); });
  }
  element->set_focused(true);
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
std::optional<std::string> g_pending_clipboard_write;
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

void ComponentBase::SetClipboard(std::string_view text) {
  g_pending_clipboard_write = std::string(text);
}

std::optional<std::string> ComponentBase::TakePendingClipboardWrite() {
  std::optional<std::string> result = std::move(g_pending_clipboard_write);
  g_pending_clipboard_write.reset();
  return result;
}

bool ComponentBase::OnEvent(Event event) {
  return false;
}

Element* ComponentBase::Root() const {
  return root_.get();
}

bool Bindings::RunCallback(std::string_view name, std::string_view arg) {
  if (arg.empty()) {
    auto it = callbacks_.find(name);
    if (it != callbacks_.end()) {
      it->second();
      return true;
    }
  }

  auto it_param = parameterized_callbacks_.find(name);
  if (it_param != parameterized_callbacks_.end()) {
    it_param->second(std::string(arg));
    return true;
  }

  return false;
}

void Bindings::Import(std::string_view name, std::function<void()> callback) {
  if (callbacks_.count(name) ||
      parameterized_callbacks_.count(name)) {
    std::println("Error: Callback '{}' is already imported.", name);
    std::exit(1);
  }

  callbacks_[std::string(name)] = std::move(callback);
}

void Bindings::Import(std::string_view name,
                      std::function<void(std::string)> callback) {
  if (callbacks_.count(name) ||
      parameterized_callbacks_.count(name)) {
    std::println("Error: Callback '{}' is already imported.", name);
    std::exit(1);
  }

  parameterized_callbacks_[std::string(name)] = std::move(callback);
}

void Bindings::Import(std::string_view name, ComponentFactory factory) {
  if (imports_.count(name)) {
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
    (*reg)["i"] = []() { return Ref<i>::New(); };
    (*reg)["em"] = []() { return Ref<em>::New(); };
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
    (*reg)["u"] = []() { return Ref<u>::New(); };
    (*reg)["s"] = []() { return Ref<s>::New(); };
    (*reg)["strike"] = []() { return Ref<strike>::New(); };
    (*reg)["del"] = []() { return Ref<del>::New(); };
    (*reg)["code"] = []() { return Ref<code>::New(); };
    (*reg)["tooltip"] = []() { return Ref<tooltip>::New(); };
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
  std::unordered_map<std::string_view, std::vector<const css::Ruleset*>>
      by_class;
  std::unordered_map<std::string_view, std::vector<const css::Ruleset*>> by_tag;
  bool has_pseudo_classes = false;
  // Rulesets with a trailing `::part(name)`, e.g. `textarea::part(gutter)`.
  // Matched separately (see MatchPartSelector): unlike every other bucket
  // above, these can apply to elements this component doesn't itself own.
  std::vector<const css::Ruleset*> part_rules;
};

ComponentBase::ComponentBase() = default;
ComponentBase::~ComponentBase() {
  ReleaseMouse();
  HotReloadManager::Unregister(this);
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

void CollectElementStates(
    Element* el,
    ElementPath& path,
    std::vector<std::pair<ElementPath, ElementState>>& states) {
  if (!el) {
    return;
  }
  bool has_state =
      el->scroll_x() != 0 || el->scroll_y() != 0 || el->focused() ||
      el->hovered() || el->active() || el->scrollbar_hovered() ||
      el->scrollbar_active() || el->scrollbar_thumb_hovered() ||
      el->scrollbar_thumb_active() || !el->active_transitions.empty() ||
      (el->target_style.transitions && !el->target_style.transitions->empty());
  if (has_state) {
    states.push_back(
        {path,
         {el->scroll_x(), el->scroll_y(), el->focused(), el->hovered(),
          el->active(), el->scrollbar_hovered(), el->scrollbar_active(),
          el->scrollbar_thumb_hovered(), el->scrollbar_thumb_active(),
          el->style, el->active_transitions}});
  }
  for (size_t i = 0; i < el->ChildCount(); ++i) {
    path.push_back(static_cast<int>(i));
    CollectElementStates(el->ChildAt(i), path, states);
    path.pop_back();
  }
}

Element* FindElementByPath(Element* root, const ElementPath& path) {
  Element* el = root;
  for (size_t i = 0; i < path.depth; ++i) {
    int child_idx = path.indices[i];
    if (!el || child_idx < 0 ||
        static_cast<size_t>(child_idx) >= el->ChildCount()) {
      return nullptr;
    }
    el = el->ChildAt(child_idx);
  }
  return el;
}

void RestoreElementStates(
    Element* root,
    const std::vector<std::pair<ElementPath, ElementState>>& states) {
  for (const auto& pair : states) {
    Element* el = FindElementByPath(root, pair.first);
    if (el) {
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
    }
  }
}

void RestoreElementFocusHoverActive(
    Element* root,
    const std::vector<std::pair<ElementPath, ElementState>>& states) {
  for (const auto& pair : states) {
    Element* el = FindElementByPath(root, pair.first);
    if (el) {
      el->set_focused(pair.second.focused);
      el->set_hovered(pair.second.hovered);
      el->set_active(pair.second.active);
      el->set_scrollbar_hovered(pair.second.scrollbar_hovered);
      el->set_scrollbar_active(pair.second.scrollbar_active);
      el->set_scrollbar_thumb_hovered(pair.second.scrollbar_thumb_hovered);
      el->set_scrollbar_thumb_active(pair.second.scrollbar_thumb_active);
    }
  }
}

std::string Interpolate(std::string_view text,
                        ComponentBase* source,
                        const LocalScope* scope) {
  if (text.find('{') == std::string_view::npos) {
    return std::string(text);
  }

  std::string result;
  result.reserve(text.size() + 32);
  size_t last_pos = 0;

  while (true) {
    size_t open_idx = text.find('{', last_pos);
    if (open_idx == std::string_view::npos) {
      break;
    }
    size_t close_idx = text.find('}', open_idx);
    if (close_idx == std::string_view::npos) {
      break;
    }

    std::string_view expression =
        text.substr(open_idx + 1, close_idx - open_idx - 1);

    std::string_view trimmed = expression;
    if (!trimmed.empty() &&
        (std::isspace(static_cast<unsigned char>(trimmed.front())) ||
         std::isspace(static_cast<unsigned char>(trimmed.back())))) {
      while (!trimmed.empty() &&
             std::isspace(static_cast<unsigned char>(trimmed.front()))) {
        trimmed.remove_prefix(1);
      }
      while (!trimmed.empty() &&
             std::isspace(static_cast<unsigned char>(trimmed.back()))) {
        trimmed.remove_suffix(1);
      }
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

    if (!is_ident) {
      result.append(text.substr(last_pos, open_idx - last_pos + 1));
      last_pos = open_idx + 1;
      continue;
    }

    if (open_idx > last_pos) {
      result.append(text.substr(last_pos, open_idx - last_pos));
    }

    bool resolved = false;
    if (scope) {
      auto dot_pos = trimmed.find('.');
      std::string_view var_name = (dot_pos == std::string_view::npos)
                                      ? trimmed
                                      : trimmed.substr(0, dot_pos);
      auto val_opt = scope->Get(var_name);
      if (val_opt) {
        resolved = true;
        if (dot_pos == std::string_view::npos) {
          if (std::holds_alternative<std::string_view>(*val_opt)) {
            result.append(std::get<std::string_view>(*val_opt));
          }
        } else {
          std::string_view field_name = trimmed.substr(dot_pos + 1);
          if (std::holds_alternative<std::shared_ptr<StructVisitor>>(
                  *val_opt)) {
            auto visitor = std::get<std::shared_ptr<StructVisitor>>(*val_opt);
            if (visitor) {
              result.append(visitor->GetFieldValue(field_name));
            }
          }
        }
      }
    }

    if (!resolved) {
      result.append(source->GetInterpolatedValue(trimmed));
    }
    last_pos = close_idx + 1;
  }

  if (last_pos < text.size()) {
    result.append(text.substr(last_pos));
  }
  return result;
}

} // namespace

void PrintCompilerStyleError(std::string_view source_string,
                             int error_line,
                             int error_column,
                             std::string_view message,
                             std::string_view label) {
  std::vector<std::string_view> lines = Split(source_string, '\n');
  std::cerr << "======== " << label << " Error ========" << std::endl;
  std::cerr << "Error: " << message << std::endl;
  std::cerr << "Line " << (error_line + 1) << ", Column " << (error_column + 1) << ":" << std::endl;
  std::cerr << "      ┌" << Repeat("─", 76) << std::endl;

  int start_line = std::max(0, error_line - 2);
  int end_line = std::min(static_cast<int>(lines.size()) - 1, error_line + 2);

  for (int line = start_line; line <= end_line; ++line) {
    if (line == error_line) {
      std::cerr << " > " << std::setw(4) << (line + 1) << " │ " << lines[line] << std::endl;
      std::cerr << "   " << "     │ " << Repeat(" ", error_column) << "^" << std::endl;
    } else {
      std::cerr << "   " << std::setw(4) << (line + 1) << " │ " << lines[line] << std::endl;
    }
  }
  std::cerr << "      └" << Repeat("─", 76) << std::endl;
  std::cerr << std::flush;
}

namespace {

std::function<void(const CssError&)>& GetCssErrorHandler() {
  static std::function<void(const CssError&)> handler;
  return handler;
}

}  // namespace

void SetCssErrorHandler(std::function<void(const CssError&)> handler) {
  GetCssErrorHandler() = std::move(handler);
}

namespace {

std::function<void(const XmlError&)>& GetXmlErrorHandler() {
  static std::function<void(const XmlError&)> handler;
  return handler;
}

}  // namespace

void SetXmlErrorHandler(std::function<void(const XmlError&)> handler) {
  GetXmlErrorHandler() = std::move(handler);
}

void ReportXmlError(const XmlError& error, std::string_view xml_string) {
  if (const auto& handler = GetXmlErrorHandler()) {
    handler(error);
    return;
  }
  PrintCompilerStyleError(xml_string, error.line, error.column, error.message,
                           "XML");
}

namespace {

void XmlParseError(const xml::Error& error, std::string_view xml_string) {
  PrintCompilerStyleError(xml_string, error.line, error.column, error.message, "DOM");
  std::exit(1);
}

// Unlike XmlParseError, this is reachable every time a <style> block's text
// is re-interpolated with changed bound state (i.e. on every Digest()), so it
// must not crash the running process on a malformed value. Defaults to
// printing to stderr (see PrintCompilerStyleError), but that corrupts a
// running frame for any app that owns the terminal in raw mode -- see
// SetCssErrorHandler.
void CssParseError(const css::Error& error, std::string_view css_string) {
  if (const auto& handler = GetCssErrorHandler()) {
    handler(CssError{error.message, error.line, error.column});
    return;
  }
  PrintCompilerStyleError(css_string, error.line, error.column, error.message, "CSS");
}

bool MatchSelector(const Element* element,
                   const Element* root,
                   const css::ParsedSelector& selector,
                   bool check_pseudos) {
  if (!selector.base.empty()) {
    if (selector.base == "self") {
      if (element != root) return false;
    } else {
      if (element->tag() != selector.base) return false;
    }
  }
  if (!selector.id.empty()) {
    if (element->id != selector.id) return false;
  }
  for (const auto& cls : selector.classes) {
    bool found = false;
    for (const auto& el_cls : element->classes) {
      if (el_cls == cls) {
        found = true;
        break;
      }
    }
    if (!found) return false;
  }
  for (const auto& attr : selector.attributes) {
    const std::string* el_attr = element->GetAttribute(attr.name);
    if (!el_attr) return false;
    if (attr.has_value && *el_attr != attr.value) return false;
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
      if (pseudo == "scrollbar-thumb-hover" &&
          !element->scrollbar_thumb_hovered()) {
        return false;
      }
      if (pseudo == "scrollbar-thumb-active" &&
          !element->scrollbar_thumb_active()) {
        return false;
      }
      if (pseudo == "disabled" && !element->disabled()) {
        return false;
      }
      if (pseudo == "read-only" && !element->read_only()) {
        return false;
      }
    }
  }

  if (!selector.parents.empty()) {
    // Forward declaration check
    bool MatchSelectorParents(const Element* element, const Element* root, const std::vector<css::SelectorPart>& parents);
    if (!MatchSelectorParents(element, root, selector.parents)) {
      return false;
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

// Matches `component`'s `component_selector::part(name)` rules against
// `element`: an element whose own `part` attribute lists `name` as one of
// its (space-separated, like classes) tokens, that lives somewhere inside
// the subtree of an ancestor element `component` directly instantiated
// (that ancestor's owner_component() == component), and whose ancestor
// matches the selector's base/id/classes/attributes (everything before
// ::part()). This forwards through any number of intermediate component
// boundaries automatically -- e.g. if <Foo>'s own template instantiates
// <Bar part="baz">, an app that only ever writes <Foo class="thing"> can
// still reach it with `.thing::part(baz)`, with no exportparts-style
// ceremony needed from Foo.

bool MatchPartSelector(const Element* element,
                       const ComponentBase* component,
                       const css::ParsedSelector& selector) {
  const std::string* part_attr = element->GetAttribute("part");
  if (!part_attr) {
    return false;
  }
  bool has_part = false;
  for (auto token : Split(*part_attr, ' ')) {
    if (token == selector.part) {
      has_part = true;
      break;
    }
  }
  if (!has_part) {
    return false;
  }

  // Find the ancestor element `component` directly instantiated (its
  // owner_component() == component) -- that's the "host" the selector's
  // base/id/classes/attributes match against, e.g. in
  // `textarea::part(gutter)`, `textarea` describes the <textarea> usage
  // site, not `element` itself. This is NOT simply
  // `element->component()->Root()`: every XML tag is itself a component
  // (see class_name.hpp's per-tag Component<T> subclasses), so a `part`
  // element several tags deep inside another component's own template
  // (e.g. a <div part="..."> written inside <textarea>'s Setup()) has its
  // own immediate per-tag component wrapper in between -- walking up the
  // DOM parent chain skips over all of those uniformly, however many
  // there are, to find the actual outer instantiation boundary.
  const Element* host = element->Parent();
  while (host && host->owner_component() != component) {
    host = host->Parent();
  }
  if (!host) {
    return false;
  }

  if (!selector.base.empty() && host->tag() != selector.base) {
    return false;
  }
  if (!selector.id.empty() && host->id != selector.id) {
    return false;
  }
  for (const auto& required_class : selector.classes) {
    bool found = false;
    for (const auto& host_class : host->classes) {
      if (host_class == required_class) {
        found = true;
        break;
      }
    }
    if (!found) {
      return false;
    }
  }
  for (const auto& attr : selector.attributes) {
    const std::string* host_attr = host->GetAttribute(attr.name);
    if (!host_attr) {
      return false;
    }
    if (attr.has_value && *host_attr != attr.value) {
      return false;
    }
  }
  return true;
}

bool MatchPseudos(const Element* element,
                  const std::vector<std::string>& pseudo_classes) {
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
    if (pseudo == "scrollbar-thumb-hover" &&
        !element->scrollbar_thumb_hovered()) {
      return false;
    }
    if (pseudo == "scrollbar-thumb-active" &&
        !element->scrollbar_thumb_active()) {
      return false;
    }
    if (pseudo == "disabled" && !element->disabled()) {
      return false;
    }
    if (pseudo == "read-only" && !element->read_only()) {
      return false;
    }
    if (pseudo == "first-child") {
      const Element* parent = element->Parent();
      if (!parent) return false;
      std::vector<const Element*> siblings;
      for (const auto& child : parent->children()) {
        if (child->tag() != "style" && !child->is_text()) {
          siblings.push_back(child.get());
        }
      }
      if (siblings.empty() || siblings[0] != element) {
        return false;
      }
    }
    if (pseudo == "last-child") {
      const Element* parent = element->Parent();
      if (!parent) return false;
      std::vector<const Element*> siblings;
      for (const auto& child : parent->children()) {
        if (child->tag() != "style" && !child->is_text()) {
          siblings.push_back(child.get());
        }
      }
      if (siblings.empty() || siblings.back() != element) {
        return false;
      }
    }
    if (pseudo.starts_with("nth-child(")) {
      if (!pseudo.ends_with(")")) return false;
      std::string_view arg = std::string_view(pseudo).substr(10, pseudo.size() - 11);
      const Element* parent = element->Parent();
      if (!parent) return false;
      std::vector<const Element*> siblings;
      for (const auto& child : parent->children()) {
        if (child->tag() != "style" && !child->is_text()) {
          siblings.push_back(child.get());
        }
      }
      int index = -1;
      for (size_t i = 0; i < siblings.size(); ++i) {
        if (siblings[i] == element) {
          index = static_cast<int>(i) + 1;
          break;
        }
      }
      if (index == -1) return false;

      if (arg == "even") {
        if (index % 2 != 0) return false;
      } else if (arg == "odd") {
        if (index % 2 == 0) return false;
      } else {
        int target = 0;
        auto [ptr, ec] = std::from_chars(arg.data(), arg.data() + arg.size(), target);
        if (ec != std::errc{} || target != index) {
          return false;
        }
      }
    }
  }
  return true;
}

bool MatchSelectorPart(const Element* element, const Element* root, const css::SelectorPart& part) {
  if (!part.base.empty()) {
    if (part.base == "self") {
      if (element != root) return false;
    } else {
      if (element->tag() != part.base) return false;
    }
  }
  if (!part.id.empty()) {
    if (element->id != part.id) return false;
  }
  for (const auto& cls : part.classes) {
    bool found = false;
    for (const auto& el_cls : element->classes) {
      if (el_cls == cls) {
        found = true;
        break;
      }
    }
    if (!found) return false;
  }
  for (const auto& attr : part.attributes) {
    const std::string* el_attr = element->GetAttribute(attr.name);
    if (!el_attr) return false;
    if (attr.has_value && *el_attr != attr.value) return false;
  }
  return true;
}

namespace {
const Element* GetRealParent(const Element* element) {
  if (!element) return nullptr;
  const Element* parent = element->Parent();
  while (parent && parent->is_slot()) {
    parent = parent->Parent();
  }
  return parent;
}

const Element* GetPrecedingSibling(const Element* element) {
  if (!element) return nullptr;
  const Element* parent = element->Parent();
  if (!parent) return nullptr;
  const auto& children = parent->children();

  size_t idx = -1;
  for (size_t i = 0; i < children.size(); ++i) {
    if (children[i].get() == element) {
      idx = i;
      break;
    }
  }
  if (idx == -1 || idx == 0) return nullptr;

  for (int i = static_cast<int>(idx) - 1; i >= 0; --i) {
    const Element* sibling = children[i].get();
    if (sibling->tag() != "style") {
      if (sibling->is_text()) continue;
      return sibling;
    }
  }
  return nullptr;
}
} // namespace

bool MatchSelectorParents(const Element* element, const Element* root, const std::vector<css::SelectorPart>& parents) {
  const Element* curr = element;
  for (const auto& parent_part : parents) {
    if (parent_part.combinator == '>') {
      curr = GetRealParent(curr);
      if (!curr) return false;
      if (!MatchSelectorPart(curr, root, parent_part)) return false;
    } else if (parent_part.combinator == '+') {
      curr = GetPrecedingSibling(curr);
      if (!curr) return false;
      if (!MatchSelectorPart(curr, root, parent_part)) return false;
    } else if (parent_part.combinator == '~') {
      bool found = false;
      while (true) {
        curr = GetPrecedingSibling(curr);
        if (!curr) break;
        if (MatchSelectorPart(curr, root, parent_part)) {
          found = true;
          break;
        }
      }
      if (!found) return false;
    } else {
      bool found = false;
      while (true) {
        curr = GetRealParent(curr);
        if (!curr) break;
        if (MatchSelectorPart(curr, root, parent_part)) {
          found = true;
          break;
        }
      }
      if (!found) return false;
    }
  }
  return true;
}

void ResolveStylesRecursive(Element* element,
                            const ComponentBase* component,
                            bool check_pseudos) {
  if (!element || !component) {
    return;
  }

  // Custom properties inherit through every element in the tree — including
  // slot and other unstyled elements — so rebuild the resolved map from the
  // DOM parent up front (parents are visited before children). The element's
  // own --* declarations are overlaid again after collection below.
  if (!check_pseudos) {
    const Element* parent = element->Parent();
    if (parent) {
      element->custom_properties = parent->custom_properties;
    } else {
      element->custom_properties.clear();
    }
    for (const auto& [name, val] : element->own_custom_properties) {
      element->custom_properties[name] = val;
    }
  }
  // Resolve nested component internal styles first, so that parent styles (template/classes)
  // take precedence and override the child's internal styles.
  if (element->component() && element->component() != component) {
    ResolveStylesRecursive(element, element->component(), check_pseudos);
  }

  bool is_styled = IsStyledByComponent(element, component);
  const auto* categorized_for_parts = component->categorized_rules();
  bool has_part_rules =
      categorized_for_parts && !categorized_for_parts->part_rules.empty();
  if (is_styled || has_part_rules) {
    if (!check_pseudos) {
      if (element->IsStyleResolvedFor(component)) {
        goto recurse;
      }
      if (!element->styled_by_1 && !element->styled_by_2) {
        element->base_style = ComputedStyle();
        element->own_custom_properties.clear();
      }
    }

    {
    // Matching rulesets are collected first (in bucket order), so that all
    // --* declarations are known before var() substitution happens.
    std::vector<const css::Ruleset*> matched;
    const auto* categorized = component->categorized_rules();
    if (categorized && is_styled) {
      auto match_and_apply = [&](const std::vector<const css::Ruleset*>& rulesets,
                                 bool is_universal) {
        for (const auto* ruleset : rulesets) {
          if (!css::EvaluateMediaQuery(ruleset->media_query)) {
            continue;
          }
          const auto& parsed = ruleset->parsed_selector;
          if (is_universal && parsed.base == "self" &&
              element != component->Root()) {
            continue;
          }

          if (!parsed.base.empty() && parsed.base != "self" &&
              element->tag() != parsed.base) {
            continue;
          }

          if (!parsed.id.empty() && element->id != parsed.id) {
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


          if (!classes_match) {
            continue;
          }

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
          if (!attributes_match) {
            continue;
          }

          if (!parsed.parents.empty()) {
            if (!MatchSelectorParents(element, component->Root(), parsed.parents)) {
              continue;
            }
          }

          if (check_pseudos) {
            if (!parsed.pseudo_classes.empty() &&
                MatchPseudos(element, parsed.pseudo_classes)) {
              matched.push_back(ruleset);
            }
          } else {
            if (parsed.pseudo_classes.empty()) {
              matched.push_back(ruleset);
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
    }

    // ::part() rules: unlike every bucket above, these can match `element`
    // even when `is_styled` is false (see MatchPartSelector) -- checked
    // regardless of `categorized && is_styled` above.
    if (categorized) {
      for (const auto* ruleset : categorized->part_rules) {
        if (!css::EvaluateMediaQuery(ruleset->media_query)) {
          continue;
        }
        const auto& parsed = ruleset->parsed_selector;
        if (!MatchPartSelector(element, component, parsed)) {
          continue;
        }
        if (check_pseudos) {
          if (!parsed.pseudo_classes.empty() &&
              MatchPseudos(element, parsed.pseudo_classes)) {
            matched.push_back(ruleset);
          }
        } else {
          if (parsed.pseudo_classes.empty()) {
            matched.push_back(ruleset);
          }
        }
      }
    }

    // Inline style attribute parsing. The declarations are string_views into
    // css_rule, which must stay alive until they are applied below. Skipped
    // when this pass was entered solely for ::part() matching (is_styled
    // false): an element's own inline style is applied once, by the
    // component that actually owns/rendered it, not by every outer
    // component whose ::part() rules happen to reach in.
    std::string css_rule;
    std::vector<css::Declaration> inline_declarations;
    const std::string* inline_style =
        is_styled ? element->GetAttribute("style") : nullptr;
    if (inline_style && !inline_style->empty()) {
      css_rule = "dummy { " + *inline_style + " }";
      auto maybe_stylesheet = css::Parse(css_rule);
      if (maybe_stylesheet && !maybe_stylesheet.value().empty()) {
        inline_declarations = std::move(maybe_stylesheet.value()[0].declarations);
      }
    }

    // Phase 1 (base pass only): accumulate this element's own --*
    // declarations, then rebuild the resolved map as the DOM parent's
    // resolved properties overlaid with the own ones. Rebuilding on every
    // pass matters: an element hosting a nested component is styled by the
    // nested component first, before the outer component has applied the
    // parent's --* declarations.
    if (!check_pseudos) {
      auto collect_custom_properties = [&](const css::Declaration& declaration) {
        if (declaration.property.starts_with("--")) {
          element->own_custom_properties[std::string(declaration.property)] =
              std::string(declaration.value);
        }
      };
      for (const auto* ruleset : matched) {
        for (const auto& declaration : ruleset->declarations) {
          collect_custom_properties(declaration);
        }
      }
      for (const auto& declaration : inline_declarations) {
        collect_custom_properties(declaration);
      }

      const Element* parent = element->Parent();
      if (parent) {
        element->custom_properties = parent->custom_properties;
      } else {
        element->custom_properties.clear();
      }
      for (const auto& [name, val] : element->own_custom_properties) {
        element->custom_properties[name] = val;
      }
    }

    // In the pseudo pass, --* declarations from matching pseudo-class rules
    // (e.g. ":hover { --glow: ...; }") overlay the element's resolved map
    // for this element's own substitutions. The overlay is intentionally not
    // stored: it must not leak into descendants or later frames.
    css::CustomProperties pseudo_props;
    bool has_pseudo_props = false;
    if (check_pseudos) {
      for (const auto* ruleset : matched) {
        for (const auto& declaration : ruleset->declarations) {
          if (declaration.property.starts_with("--")) {
            if (!has_pseudo_props) {
              pseudo_props = element->custom_properties;
              has_pseudo_props = true;
            }
            pseudo_props[std::string(declaration.property)] =
                std::string(declaration.value);
          }
        }
      }
    }
    const css::CustomProperties& active_props =
        has_pseudo_props ? pseudo_props : element->custom_properties;

    // Phase 2: apply regular declarations, expanding var() references.
    ComputedStyle& style_out =
        check_pseudos ? element->target_style : element->base_style;
    auto apply_with_vars = [&](const css::Declaration& declaration) {
      if (declaration.property.starts_with("--")) {
        return;
      }
      if (declaration.value.find("var(") != std::string_view::npos) {
        auto expanded =
            css::SubstituteVars(declaration.value, active_props);
        if (!expanded) {
          return;  // Undefined variable without fallback: ignore.
        }
        ApplyStyle(style_out, {declaration.property, *expanded});
        return;
      }
      ApplyStyle(style_out, declaration);
    };
    // Normal declarations first, then !important ones, so important wins
    // regardless of rule order (rules before inline within each round).
    auto apply_round = [&](bool important) {
      for (const auto* ruleset : matched) {
        for (const auto& declaration : ruleset->declarations) {
          if (declaration.important == important) {
            apply_with_vars(declaration);
          }
        }
      }
      for (const auto& declaration : inline_declarations) {
        if (declaration.important == important) {
          apply_with_vars(declaration);
        }
      }
    };
    apply_round(false);
    apply_round(true);
    }  // matched/inline declarations scope (bypassed by the goto above).

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
    // Without slot content, `component`'s own selectors could never reach
    // anything inside this nested component's subtree -- UNLESS it has
    // ::part() rules, which are specifically designed to reach in there.
    if (!has_slot_children && !has_part_rules) {
      return;
    }
  }

  for (size_t i = 0; i < element->ChildCount(); ++i) {
    ResolveStylesRecursive(element->ChildAt(i), component, check_pseudos);
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
  // Optimization: Use a flat vector of pairs instead of std::map<ElementPath,
  // ElementState>. Since very few elements actually hold state (scroll, focus,
  // transitions), this avoids the dynamic allocation and key-comparison
  // overhead of a red-black tree map.
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

  old_children_ = std::move(children_);
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
      root_->Visit([](Element& el) { el.ClearResolvedStyles(); });
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
          if (!ruleset.parsed_selector.part.empty()) {
            categorized_rules_->part_rules.push_back(&ruleset);
            continue;
          }
          std::string_view selector_base = ruleset.parsed_selector.base;
          std::string_view selector_id = ruleset.parsed_selector.id;

          if (!selector_id.empty()) {
            categorized_rules_->by_id[selector_id].push_back(&ruleset);
          } else if (!ruleset.parsed_selector.classes.empty()) {
            categorized_rules_->by_class[ruleset.parsed_selector.classes[0]].push_back(&ruleset);
          } else if (selector_base == "self") {
            categorized_rules_->universal.push_back(&ruleset);
          } else if (selector_base.empty()) {
            categorized_rules_->universal.push_back(&ruleset);
          } else {
            categorized_rules_->by_tag[selector_base].push_back(&ruleset);
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
    if (std::find(children_.begin(), children_.end(), comp) ==
        children_.end()) {
      children_.push_back(comp);
    }
  }

  if (root_ && !saved_states.empty()) {
    RestoreElementFocusHoverActive(root_.get(), saved_states);
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
    if (!saved_states.empty()) {
      RestoreElementStates(root_.get(), saved_states);
    }
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
      if (!element) {
        return false;
      }
      if (!element->active_transitions.empty() ||
          element->IsAnimatingScroll()) {
        return true;
      }
      for (size_t i = 0; i < element->ChildCount(); ++i) {
        if (self(self, element->ChildAt(i))) {
          return true;
        }
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

  auto ResolveAll = [](auto& self, ComponentBase* comp,
                       ComponentBase* root_comp) -> void {
    if (!comp || !comp->Root()) {
      return;
    }
    // Optimization: Skip resolving target styles if the component stylesheet
    // has no pseudo-classes (hover, active, focus). Yields ~18% speedup in DOM
    // Digest.
    if (comp->categorized_rules() &&
        comp->categorized_rules()->has_pseudo_classes) {
      ResolveStylesRecursive(comp->Root(), comp, true);
    }
    if (comp == root_comp && comp->Root()->owner_component() &&
        comp->Root()->owner_component() != comp) {
      const ComponentBase* owner = comp->Root()->owner_component();
      if (owner->categorized_rules() &&
          owner->categorized_rules()->has_pseudo_classes) {
        ResolveStylesRecursive(comp->Root(), owner, true);
      }
    }
    for (auto& child : comp->children_) {
      self(self, child.get(), root_comp);
    }
  };
  ResolveAll(ResolveAll, this, this);

  auto TriggerAll = [](auto& self, Element* element,
                       double current_time_ms) -> void {
    if (element) {
      element->TriggerTransitions(current_time_ms);
      for (size_t i = 0; i < element->ChildCount(); ++i) {
        self(self, element->ChildAt(i), current_time_ms);
      }
    }
  };
  TriggerAll(TriggerAll, root_.get(), current_time_ms);
}

namespace {
const std::string& GetIndexString(size_t index) {
  static const size_t kMaxCached = 1000;
  static const auto& cache = *[]() {
    auto* v = new std::vector<std::string>();
    v->reserve(kMaxCached);
    for (size_t i = 0; i < kMaxCached; ++i) {
      v->push_back(std::to_string(i));
    }
    return v;
  }();
  if (index < kMaxCached) {
    return cache[index];
  }
  static thread_local std::string fallback;
  fallback = std::to_string(index);
  return fallback;
}
}  // namespace

void ComponentBase::Render(const xml::Node& node,
                           Element* slot,
                           ComponentBase* import_source,
                           const LocalScope* scope) {
  size_t child_idx = 0;
  bool preserve = false;
  for (Element* curr = slot; curr; curr = curr->Parent()) {
    if (curr->tag() == "textarea" || curr->tag() == "pre" ||
        curr->tag() == "slider") {
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
                                    const LocalScope* scope,
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
        bool has_placeholder =
            (child_node.text.find('{') != std::string_view::npos);
        bool has_newline =
            !preserve_newlines &&
            (child_node.text.find('\n') != std::string_view::npos ||
             child_node.text.find('\r') != std::string_view::npos);

        if (!preserve_newlines) {
          bool is_whitespace = true;
          for (char c : child_node.text) {
            if (!std::isspace(static_cast<unsigned char>(c))) {
              is_whitespace = false;
              break;
            }
          }
          if (is_whitespace && has_newline) {
            break;  // Skip formatting whitespace entirely!
          }
        }

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

          if (child_idx < slot->ChildCount() &&
              slot->ChildAt(child_idx)->is_text()) {
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

        std::string text_storage;
        std::string_view text;
        if (has_placeholder) {
          text_storage = Interpolate(child_node.text);
          text = text_storage;
        } else {
          text = child_node.text;
        }

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

        // Newlines in content text are kept in the DOM; the layout tree
        // builder converts or preserves them based on the element's resolved
        // white-space style.

        // Reconcile/reuse or replace TextElement
        if (child_idx < slot->ChildCount() &&
            slot->ChildAt(child_idx)->is_text()) {
          auto* text_el = static_cast<TextElement*>(slot->ChildAt(child_idx));
          if (text_el->text() != text) {
            text_el->set_text(std::string(text));
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
              t_el->set_text(std::string(text));
            }
          } else {
            text_el = Ref<TextElement>::New(std::string(text));
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
          const auto& cond_attr = child_node.attributes.at("condition");
          std::string cond;
          if (cond_attr.find('{') == std::string::npos) {
            cond = cond_attr;
          } else {
            cond = Interpolate(cond_attr);
          }
          last_condition_chain_met = (cond == "true" || cond == "1");
          if (last_condition_chain_met) {
            RenderReconcile(child_node, slot, import_source, scope, child_idx,
                            preserve_newlines);
          }
          break;
        }

        if (child_node.tag == "elif") {
          if (!last_condition_chain_met) {
            const auto& cond_attr = child_node.attributes.at("condition");
            std::string cond;
            if (cond_attr.find('{') == std::string::npos) {
              cond = cond_attr;
            } else {
              cond = Interpolate(cond_attr);
            }
            if (cond == "true" || cond == "1") {
              last_condition_chain_met = true;
              RenderReconcile(child_node, slot, import_source, scope, child_idx,
                              preserve_newlines);
            }
          }
          break;
        }

        if (child_node.tag == "else") {
          if (!last_condition_chain_met) {
            RenderReconcile(child_node, slot, import_source, scope, child_idx,
                            preserve_newlines);
          }
          last_condition_chain_met = true;
          break;
        }

        last_condition_chain_met = true;

        if (child_node.attributes.contains("if")) {
          const auto& cond_attr = child_node.attributes.at("if");
          std::string cond;
          if (cond_attr.find('{') == std::string::npos) {
            cond = cond_attr;
          } else {
            cond = Interpolate(cond_attr);
          }
          if (!(cond == "true" || cond == "1")) {
            break;
          }
        }

        if (child_node.tag == "for") {
          const auto& each_attr = child_node.attributes.at("each");
          // Strip {} if present
          std::string_view range_name = each_attr;
          if (range_name.starts_with("{") && range_name.ends_with("}")) {
            range_name = range_name.substr(1, range_name.size() - 2);
          }

          std::string_view as_attr = "item";
          if (child_node.attributes.contains("as")) {
            as_attr = child_node.attributes.at("as");
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
              LocalScope index_scope;
              index_scope.parent = scope;
              index_scope.name = "$index";
              index_scope.value = std::string_view(GetIndexString(i));

              LocalScope item_scope;
              item_scope.parent = &index_scope;
              item_scope.name = as_attr;

              auto visitor = range->GetItemVisitor(i);
              if (visitor) {
                item_scope.value = visitor;
                RenderReconcile(child_node, slot, import_source, &item_scope,
                                child_idx, preserve_newlines);
              } else {
                std::string fallback_storage;
                item_scope.value =
                    range->GetItemStringView(i, fallback_storage);
                RenderReconcile(child_node, slot, import_source, &item_scope,
                                child_idx, preserve_newlines);
              }
            }
          }
          break;
        }

        if (child_node.tag == "slot" || child_node.tag.starts_with("slot.")) {
          std::string slot_name = child_node.tag == "slot"
                                      ? ""
                                      : std::string(child_node.tag.substr(5));

          Ref<Element> slot_element;
          if (child_idx < slot->ChildCount() &&
              slot->ChildAt(child_idx)->is_slot()) {
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
            RenderReconcile(child_node, target_slot.get(), import_source, scope,
                            sub_child_idx, preserve_newlines);
            target_slot->TruncateChildren(sub_child_idx);
          }
          break;
        }

        ComponentFactory factory = nullptr;
        auto it = import_source->imports_.find(child_node.tag);
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
          if (std::find(children_.begin(), children_.end(), child) ==
              children_.end()) {
            children_.push_back(child);
          }

          bool id_changed = false;
          if (child_node.attributes.contains("id")) {
            const auto& id_attr = child_node.attributes.at("id");
            if (id_attr.find('{') == std::string::npos) {
              if (id_attr != child->id_) {
                id_changed = true;
                child->id_ = id_attr;
              }
            } else {
              std::string new_id = Interpolate(id_attr);
              if (new_id != child->id_) {
                id_changed = true;
                child->id_ = std::move(new_id);
              }
            }
          } else if (!child->id_.empty()) {
            id_changed = true;
            child->id_.clear();
          }

          bool classes_changed = false;
          if (child_node.attributes.contains("class")) {
            const auto& class_attr = child_node.attributes.at("class");
            if (class_attr.find('{') == std::string::npos) {
              auto class_views = Split(class_attr, ' ');
              bool matches = (class_views.size() == child->classes_.size());
              if (matches) {
                for (size_t i = 0; i < class_views.size(); ++i) {
                  if (class_views[i] != child->classes_[i]) {
                    matches = false;
                    break;
                  }
                }
              }
              if (!matches) {
                classes_changed = true;
                child->classes_.assign(class_views.begin(), class_views.end());
              }
            } else {
              std::string interpolated_class = Interpolate(class_attr);
              auto class_views = Split(interpolated_class, ' ');
              bool matches = (class_views.size() == child->classes_.size());
              if (matches) {
                for (size_t i = 0; i < class_views.size(); ++i) {
                  if (class_views[i] != child->classes_[i]) {
                    matches = false;
                    break;
                  }
                }
              }
              if (!matches) {
                classes_changed = true;
                child->classes_.assign(class_views.begin(), class_views.end());
              }
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
          std::vector<std::string> child_custom_keys;
          for (auto& [key_view, value] : child_node.attributes) {
            std::string_view key = key_view;
            bool dynamic = false;
            if (key.starts_with(':')) {
              key.remove_prefix(1);
              dynamic = true;
            }

            if (key.starts_with('@')) {
              if (key == "@click" || key == "@click.left") {
                key = "onclick";
              } else if (key == "@click.right") {
                key = "oncontextmenu";
              } else if (key == "@change") {
                key = "onchange";
              } else {
                child_custom_keys.push_back("on" + std::string(key.substr(1)));
                key = child_custom_keys.back();
              }
            }

            if (key == "id" || key == "class") {
              continue;
            }

            std::string interpolated_storage;
            std::string_view interpolated_value;
            if (dynamic) {
              std::string actual_value;
              if (!value.starts_with('{')) {
                actual_value = "{" + value + "}";
              } else {
                actual_value = value;
              }
              interpolated_storage = Interpolate(actual_value);
              interpolated_value = interpolated_storage;
            } else if (value.find('{') != std::string::npos) {
              interpolated_storage = Interpolate(value);
              interpolated_value = interpolated_storage;
            } else {
              interpolated_value = value;
            }

            const std::string* current_val =
                child->Root() ? child->Root()->GetAttribute(std::string(key))
                              : nullptr;
            if (!current_val || *current_val != interpolated_value) {
              attribute_changed = true;
            }

            child->SetProperty(key, interpolated_value);
            if (child->Root()) {
              child->Root()->SetAttribute(std::string(key),
                                          std::string(interpolated_value));
            }

            if (dynamic || (value.starts_with("{") && value.ends_with("}"))) {
              std::string_view expr = value;
              if (expr.starts_with("{") && expr.ends_with("}")) {
                expr = expr.substr(1, expr.size() - 2);
              }
              child->two_way_bindings_.push_back(
                  {std::string(key), import_source, std::string(expr)});
            }
          }

          bool terminal_size_changed =
              (child->last_render_terminal_width_ != css::g_terminal_width ||
               child->last_render_terminal_height_ != css::g_terminal_height);

          bool needs_render = is_new || id_changed || classes_changed ||
                              attribute_changed || terminal_size_changed;

          if (needs_render) {
            child->Render();
          } else {
            child->Digest();
          }
          child->Root()->set_owner_component(import_source);

          // Now reconcile slot with child->Root()
          if (child_idx < slot->ChildCount() &&
              slot->ChildAt(child_idx) == child->Root()) {
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
                auto it = std::find(child->children_.begin(),
                                    child->children_.end(), comp);
                if (it != child->children_.end()) {
                  child->children_.erase(it);
                }
              }
            }

            size_t sub_child_idx = 0;
            child->RenderReconcile(child_node, default_slot.get(),
                                   import_source, scope, sub_child_idx,
                                   preserve_newlines);
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
            if (!slot->ChildAt(i)->is_text() && !slot->ChildAt(i)->is_slot() &&
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

        std::vector<std::string_view> updated_keys;
        std::vector<std::string> custom_event_keys;
        for (auto& [key_view, value] : child_node.attributes) {
          std::string_view key = key_view;
          bool dynamic = false;
          if (key.starts_with(':')) {
            key.remove_prefix(1);
            dynamic = true;
          }

          if (key.starts_with('@')) {
            if (key == "@click" || key == "@click.left") {
              key = "onclick";
            } else if (key == "@click.right") {
              key = "oncontextmenu";
            } else if (key == "@change") {
              key = "onchange";
            } else {
              custom_event_keys.push_back("on" + std::string(key.substr(1)));
              key = custom_event_keys.back();
            }
          }

          if (dynamic) {
            std::string actual_value;
            if (!value.starts_with('{')) {
              actual_value = "{" + value + "}";
            } else {
              actual_value = value;
            }
            std::string new_val = Interpolate(actual_value);
            const std::string* current_val =
                child_element->GetAttribute(std::string(key));
            if (!current_val || *current_val != new_val) {
              child_element->SetAttribute(std::string(key), std::move(new_val));
            }
          } else {
            if (value.find('{') == std::string::npos) {
              const std::string* current_val =
                  child_element->GetAttribute(std::string(key));
              if (!current_val || *current_val != value) {
                child_element->SetAttribute(std::string(key),
                                            std::string(value));
              }
            } else {
              std::string new_val = Interpolate(value);
              const std::string* current_val =
                  child_element->GetAttribute(std::string(key));
              if (!current_val || *current_val != new_val) {
                child_element->SetAttribute(std::string(key),
                                            std::move(new_val));
              }
            }
          }
          updated_keys.push_back(key);
        }

        if (is_reused) {
          // Remove obsolete attributes
          std::vector<std::string> keys_to_remove;
          for (const auto& [key, val] : child_element->Attributes()) {
            if (std::find(updated_keys.begin(), updated_keys.end(), key) ==
                updated_keys.end()) {
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
        RenderReconcile(child_node, child_element.get(), import_source, scope,
                        sub_child_idx,
                        preserve_newlines ||
                            child_element->tag() == "textarea" ||
                            child_element->tag() == "pre" ||
                            child_element->tag() == "slider");
        child_element->TruncateChildren(sub_child_idx);

        child_idx++;
        break;
      }
    }
  }
}

Ref<Element> ComponentBase::Slot(std::string_view name) {
  auto it = slots_.find(name);
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

void ComponentBase::EnableHotReload(std::string_view view_var_name, std::source_location location) {
  HotReloadManager::Register(this, view_var_name, location.file_name());
}

void ComponentBase::EnableHotReload(std::string_view view_var_name, std::string_view filepath) {
  HotReloadManager::Register(this, view_var_name, filepath);
}

void ComponentBase::HotReload(std::string_view new_template) {
  template_ = StripIndent(std::string(new_template));
  xml_string_ = StripIndent(template_);

  Expected<xml::Nodes, xml::Error> nodes = xml::Parse(xml_string_);
  if (!nodes) {
    std::cerr << "XML parse error during hot reload: " << nodes.error().message << std::endl;
    return;
  }
  xml_nodes_ = std::move(nodes.value());
  Render();
}

namespace {
std::vector<HotReloadInfo>& GetRegisteredHotReloads() {
  static std::vector<HotReloadInfo> registered;
  return registered;
}

std::string ExtractHotReloadTemplate(const std::string& filepath, const std::string& var_name) {
  std::ifstream file(filepath);
  if (!file.is_open()) return "";
  
  std::string content((std::istreambuf_iterator<char>(file)),
                       std::istreambuf_iterator<char>());

  size_t search_start = 0;
  if (!var_name.empty()) {
    size_t var_pos = content.find(var_name);
    if (var_pos != std::string::npos) {
      search_start = var_pos;
    }
  }

  // Look for R"html(
  size_t start_pos = content.find("R\"html(", search_start);
  std::string end_delim = ")html\"";
  size_t content_start = 0;
  
  if (start_pos != std::string::npos) {
    content_start = start_pos + 7;
  } else {
    // Try R"(
    start_pos = content.find("R\"(", search_start);
    if (start_pos != std::string::npos) {
      content_start = start_pos + 3;
      end_delim = ")\"";
    } else {
      return "";
    }
  }

  size_t end_pos = content.find(end_delim, content_start);
  if (end_pos == std::string::npos) return "";

  return content.substr(content_start, end_pos - content_start);
}
} // namespace

void HotReloadManager::Register(ComponentBase* component, std::string_view view_var_name, std::string_view filepath) {
  std::string path_str(filepath);
  if (!std::filesystem::exists(path_str)) {
    return;
  }
  
  auto& list = GetRegisteredHotReloads();
  list.erase(std::remove_if(list.begin(), list.end(), 
    [component](const HotReloadInfo& info) { return info.component == component; }), 
    list.end());
    
  std::error_code ec;
  auto mod_time = std::filesystem::last_write_time(path_str, ec);
  if (!ec) {
    list.push_back({component, std::string(view_var_name), path_str, mod_time});
  }
}

void HotReloadManager::Unregister(ComponentBase* component) {
  auto& list = GetRegisteredHotReloads();
  list.erase(std::remove_if(list.begin(), list.end(), 
    [component](const HotReloadInfo& info) { return info.component == component; }), 
    list.end());
}

bool HotReloadManager::PollChanges() {
  bool reloaded = false;
  auto& list = GetRegisteredHotReloads();
  for (auto& info : list) {
    std::error_code ec;
    if (!std::filesystem::exists(info.filepath, ec)) {
      continue;
    }
    auto current_time = std::filesystem::last_write_time(info.filepath, ec);
    if (ec) {
      continue;
    }
    if (current_time != info.last_modified) {
      info.last_modified = current_time;
      
      std::string new_template = ExtractHotReloadTemplate(info.filepath, info.view_var_name);
      if (!new_template.empty()) {
        info.component->HotReload(new_template);
        reloaded = true;
      }
    }
  }
  return reloaded;
}

}  // namespace rtxui
