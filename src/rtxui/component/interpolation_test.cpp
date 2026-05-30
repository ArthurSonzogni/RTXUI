// Copyright 2024 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include <catch2/catch_test_macros.hpp>

#include "rtxui/component/default_components_internal.hpp"
#include "rtxui/core/string.hpp"
#include "rtxui/dom/element.hpp"
#include "rtxui/internal/component.hpp"

namespace {

class InterpolationComponent : public rtxui::Component<InterpolationComponent> {
 public:
  std::string name = "RTXUI";
  int version = 5;

  void RegisterProperties() {
    Import("name", name);
    Import("version", version);
  }

  std::string get_greeting() const { return "Hello " + name; }
  int get_next_version() const { return version + 1; }

  void InitReflection() override {
    RegisterProperties();
    Import("get_greeting", &InterpolationComponent::get_greeting);
    Import("get_next_version", &InterpolationComponent::get_next_version);

    Import<rtxui::div>();
    Import<rtxui::span>();
    rtxui::Component<InterpolationComponent>::InitReflection();
  }

  std::string_view view = R"(
    <div id="v{version}" class="{name}">
      <span>{get_greeting}</span>
      <span>Version: {version} (Next: {get_next_version})</span>
    </div>
  )";
};

TEST_CASE("Interpolation", "[component][interpolation]") {
  auto component = rtxui::Ref<InterpolationComponent>::New();
  component->Mount();

  std::string output = component->Root()->Print();

  // Verify text interpolation
  CHECK(output.find("Hello RTXUI") != std::string::npos);
  CHECK(output.find("Version: 5") != std::string::npos);
  CHECK(output.find("(Next: 6)") != std::string::npos);

  // Verify attribute interpolation
  CHECK(output.find("id=\"v5\"") != std::string::npos);
  CHECK(output.find("class=\"RTXUI\"") != std::string::npos);
}

class NestedInterpolation : public rtxui::Component<NestedInterpolation> {
 public:
  std::string color = "red";

  void InitReflection() override {
    RegisterState("color", &color);
    Import<rtxui::div>();
    rtxui::Component<NestedInterpolation>::InitReflection();
  }

  std::string_view view = R"(
    <div style="background-color: {color};">
      Content
    </div>
  )";
};

TEST_CASE("Style Interpolation", "[component][interpolation]") {
  auto component = rtxui::Ref<NestedInterpolation>::New();
  component->Mount();

  std::string output = component->Root()->Print();
  CHECK(output.find("background-color: red") != std::string::npos);
}

}  // namespace
