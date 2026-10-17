// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include <catch2/catch_test_macros.hpp>
#include <string>
#include <string_view>

#include "rtxui/component.hpp"
#include "rtxui/headless.hpp"
#include "rtxui/refcounted.hpp"

using namespace rtxui;

namespace {

class FieldsetTestComponent : public rtxui::Component<FieldsetTestComponent> {
 public:
  std::string_view view = R"(
    <fieldset>
      <legend>Title</legend>
      body
    </fieldset>
  )";
};

}  // namespace

TEST_CASE("The legend sits in a top border that runs on past it",
          "[component][fieldset][regression]") {
  // The legend's row only fit its text while flex containers shrank to
  // their content; filling the line, its background hid the whole border.
  std::string text = RenderToString(Ref<FieldsetTestComponent>::New(), 20, 6);
  // The first line is the fieldset's margin.
  CHECK(text.starts_with("\n┌─ Title ──────────┐\n"));
}
