// Copyright 2024 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#ifndef RTXUI_COMPONENT_INTERNAL_HPP_
#define RTXUI_COMPONENT_INTERNAL_HPP_

#include <filesystem>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "rtxui/component.hpp"
#include "rtxui/dom/element.hpp"
#include "rtxui/style/style.hpp"
#include "rtxui/xml/xml.hpp"

namespace rtxui {

/// What a component holds. It lives here rather than in the public header so
/// that a component subclass cannot reach it, and so that changing it does not
/// change the layout of every component.
struct ComponentBase::Data {
  std::vector<Entry> entries_;
  std::vector<RangeEntry> range_entries_;

  // Shared with every other instance declaring the same stylesheet text; see
  // GetSharedStyle. Aliasing pointers into one StyleData, so both keep it
  // alive and the index's pointers into the sheet stay valid.
  std::shared_ptr<const css::StyleSheet> stylesheet_;
  std::shared_ptr<const CategorizedRules> categorized_rules_;
  std::vector<std::string> css_strings_;

  struct BindingLink {
    std::string child_prop;
    ComponentBase* parent;
    std::string parent_prop;
  };
  std::vector<BindingLink> two_way_bindings_;
  // Per bound property, how the parent renders the value this component last
  // pushed to it, when that differs from what was pushed ("1." pushed into a
  // double renders as "1"). See PropagateBinding().
  std::map<std::string, std::string, std::less<>> binding_echoes_;
  int last_render_terminal_width_ = -1;
  int last_render_terminal_height_ = -1;

  std::string template_;
  std::string xml_string_;
  xml::Nodes xml_nodes_;
  Ref<Element> root_;
  std::map<std::string, Ref<Element>, std::less<>> slots_;
  // Set when rendering created a slot element afresh, cleared once the
  // consumer has projected its content into the slots. Still set after a
  // Digest() means this component re-rendered on its own -- an <if> around a
  // <slot> turned true, say -- and left a slot that only the consumer can
  // fill, so the consumer has to render again.
  bool slots_recreated_ = false;

  // What each `<for virtual="">` rendered last, to tell when scrolling has
  // moved past it. Keyed by the loop's node in the template.
  struct VirtualWindow {
    const void* node = nullptr;
    // The element the loop's items are rendered into.
    Ref<Element> container;
    size_t first = 0;
    size_t last = 0;
    size_t count = 0;
    int item_height = 1;
  };
  std::vector<VirtualWindow> virtual_windows_;
  // The scroll offset and height of each loop's scroll container, as last
  // seen once laid out. Render() detaches the component while it runs, which
  // hides a scroll container outside it; this is what it falls back to.
  std::map<const void*, VirtualListViewport> virtual_viewports_;

  // The tag a template wrote to create this component, which reconciliation
  // matches on to reuse it. Not Tag(): that is the class name, which an alias
  // or a hyphenated tag (`tab-pane` is `tab_pane`) does not match.
  std::string created_tag_;
  std::vector<Ref<ComponentBase>> children_;
  std::vector<Ref<ComponentBase>> old_children_;
  std::string id_;
  std::vector<std::string> classes_;
};

// The parts of ComponentBase that the rest of the library needs but that are
// not part of the public API.
struct ComponentInternals {
  static const CategorizedRules* categorized_rules(const ComponentBase& c) {
    return c.categorized_rules();
  }
  // The same, sharing ownership of the stylesheet the rules point into.
  static const std::shared_ptr<const CategorizedRules>& shared_rules(
      const ComponentBase& c) {
    return c.data_->categorized_rules_;
  }
  static const std::map<std::string, Ref<Element>, std::less<>>& slots(
      const ComponentBase& c) {
    return c.slots();
  }
  static Ref<Element> Slot(ComponentBase& c, std::string_view name) {
    return c.Slot(name);
  }
  static std::string GetInterpolatedValue(ComponentBase& c,
                                          std::string_view expression) {
    return c.GetInterpolatedValue(expression);
  }
  static void SetProperty(ComponentBase& c,
                          std::string_view name,
                          std::string_view value) {
    c.SetProperty(name, value);
  }
  static void ResolveStyles(ComponentBase& c) { c.ResolveStyles(); }
  static ComponentBase* GetMouseCapturer() {
    return ComponentBase::GetMouseCapturer();
  }
  // Whether `c` or a component inside it has a :has() selector.
  static bool UsesRelationalSelectors(const ComponentBase& c);
  // Whether `c` or a component inside it rendered a `<for virtual="">`.
  static bool HasVirtualLists(const ComponentBase& c);
};

// Number of elements the style walk has visited since ResetStyleVisitCount(),
// both passes together. Deterministic, unlike wall-clock time, so a benchmark
// can flag any growth in how much work style resolution does.
int StyleVisitCount();
void ResetStyleVisitCount();

// Whether a base style pass has resolved any element since the last call,
// which then starts over. A :has() rule can depend on any of them, wherever
// it sits in the tree.
bool TakeBaseStylesResolved();

// The element that last appeared carrying `autofocus` since the last call,
// which then starts over, or null.
Ref<Element> TakePendingAutofocus();

ComponentBase* GetOwningComponent(Element* element);
ComponentBase* GetAttributeOwnerComponent(Element* element);
ComponentBase* GetParentComponent(ComponentBase* comp);

// Focuses `element` and clears focus from every other element in the
// document, so only one element is ever focused at a time.
void FocusExclusive(Element* element);

// Records how the user last interacted: with the mouse (true) or the
// keyboard (false). `:focus-visible` hides the focus of an element focused by
// a click, as browsers do, except on text fields.
void SetPointerInteraction(bool pointer);
bool PointerInteraction();

// Syncs a component's bound `disabled` state onto its root Element, for
// :disabled CSS matching and Screen's tab-navigation/click gating (both
// keyed off Element::disabled()) - and drops focus if the element is
// currently focused while disabled, mirroring TextInputBase's behavior.
void SyncDisabled(Element* root, bool disabled);

// Syncs a component's bound `checked` state onto its root Element for
// :checked CSS matching.
void SyncChecked(Element* root, bool checked);

/// Reports a syntax error in `source`: to the diagnostic handler when one is
/// installed, otherwise printed to stderr with the lines around it (and an
/// abort under RTXUI_STRICT=1).
RTXUI_EXPORT void ReportSyntaxError(const Diagnostic& diagnostic,
                                    std::string_view source);

struct HotReloadInfo {
  ComponentBase* component;
  std::string view_var_name;
  std::string filepath;
  std::filesystem::file_time_type last_modified;
};

class HotReloadManager {
 public:
  static void Register(ComponentBase* component,
                       std::string_view view_var_name,
                       std::string_view filepath);
  static void Unregister(ComponentBase* component);
  static bool PollChanges();
};

}  // namespace rtxui

#endif  // RTXUI_COMPONENT_INTERNAL_HPP_
