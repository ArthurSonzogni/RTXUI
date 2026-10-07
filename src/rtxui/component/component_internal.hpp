// Copyright 2024 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#ifndef RTXUI_COMPONENT_INTERNAL_HPP_
#define RTXUI_COMPONENT_INTERNAL_HPP_

#include <filesystem>
#include <string>
#include <string_view>

#include "rtxui/component.hpp"
#include "rtxui/dom/element.hpp"

namespace rtxui {

// The parts of ComponentBase that the rest of the library needs but that are
// not part of the public API.
struct ComponentInternals {
  static const CategorizedRules* categorized_rules(const ComponentBase& c) {
    return c.categorized_rules();
  }
  // The same, sharing ownership of the stylesheet the rules point into.
  static const std::shared_ptr<const CategorizedRules>& shared_rules(
      const ComponentBase& c) {
    return c.categorized_rules_;
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

// Syncs a component's bound `disabled` state onto its root Element, for
// :disabled CSS matching and Screen's tab-navigation/click gating (both
// keyed off Element::disabled()) - and drops focus if the element is
// currently focused while disabled, mirroring TextInputBase's behavior.
void SyncDisabled(Element* root, bool disabled);

// Syncs a component's bound `checked` state onto its root Element for
// :checked CSS matching.
void SyncChecked(Element* root, bool checked);

/// Routes an XML/HTML parse error to the handler installed via
/// SetXmlErrorHandler, or prints it to stderr if none was installed.
RTXUI_EXPORT void ReportXmlError(const XmlError& error,
                                 std::string_view xml_string);

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
