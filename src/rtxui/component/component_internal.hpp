// Copyright 2024 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#ifndef RTXUI_COMPONENT_INTERNAL_HPP_
#define RTXUI_COMPONENT_INTERNAL_HPP_

#include "rtxui/dom/element.hpp"
#include "rtxui/internal/component.hpp"

namespace rtxui {

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

}  // namespace rtxui

#endif  // RTXUI_COMPONENT_INTERNAL_HPP_
