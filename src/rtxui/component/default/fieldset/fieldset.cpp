// Copyright 2024 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include "rtxui/component/default/fieldset/fieldset.hpp"

#include "rtxui/component/component_internal.hpp"
#include "rtxui/dom/element.hpp"

namespace rtxui {

void fieldset::InitReflection() {
  Bind(legend_class);
  Component<fieldset>::InitReflection();
}

std::string_view fieldset::Setup() {
  return R"html(
    <div class="fieldset-wrapper">
      <div class="legend-line {legend_class}">
        <slot.legend></slot.legend>
      </div>
      <div class="fieldset-body">
        <slot></slot>
      </div>
    </div>
    <style>
      self {
        display: block;
        margin-top: 1;
        margin-bottom: 1;
      }
      .legend-line {
        display: flex;
        flex-direction: row;
        font-weight: bold;
        color: rgb(59, 130, 246);
        margin-bottom: -1;
        margin-left: 2;
        padding-left: 1;
        padding-right: 1;
        z-index: 1;
        background-color: rgb(18, 18, 18);
      }
      .fieldset-body {
        display: block;
        border: solid;
        border-color: rgb(74, 85, 104);
        padding: 1;
      }
      .no-legend {
        display: none;
      }
    </style>
  )html";
}

bool fieldset::Digest() {
  auto default_slot = Slot("");
  auto legend_slot = Slot("legend");
  if (default_slot && legend_slot) {
    auto& default_children = const_cast<std::vector<Ref<Element>>&>(default_slot->children());
    for (auto it = default_children.begin(); it != default_children.end(); ) {
      if ((*it)->tag() == "legend") {
        auto legend_el = *it;
        it = default_children.erase(it);
        // Manually detached from the default slot above; clear the stale
        // parent before re-attaching (AddChild requires an orphan).
        legend_el->set_parent(nullptr);

        legend_slot->RemoveChildren();
        legend_slot->AddChild(legend_el);
      } else {
        ++it;
      }
    }
  }

  legend_class = (legend_slot && legend_slot->ChildCount() > 0) ? "has-legend" : "no-legend";

  return Component<fieldset>::Digest();
}

namespace {
int RegisterThis = []() {
  RegisterGlobalComponent("fieldset", []() { return Ref<fieldset>::New(); });
  return 0;
}();
}  // namespace
}  // namespace rtxui
