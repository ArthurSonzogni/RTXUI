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

class LoopInterpolation : public rtxui::Component<LoopInterpolation> {
 public:
  std::vector<std::string> items = {"a", "b", "c"};

  void InitReflection() override {
    BindCollection("items", &items);
    Import<rtxui::div>();
    Import<rtxui::span>();
    rtxui::Component<LoopInterpolation>::InitReflection();
  }

  std::string_view view = R"(
    <div>
      <for each="{items}" as="it">
        <span>{it}</span>
      </for>
    </div>
  )";
};

TEST_CASE("Loop Interpolation", "[component][interpolation]") {
  auto component = rtxui::Ref<LoopInterpolation>::New();
  component->Mount();

  std::string output = component->Root()->Print();
  // UNCOMMENT to debug: std::cout << output << std::endl;
  CHECK(output.find("a") != std::string::npos);
  CHECK(output.find("b") != std::string::npos);
  CHECK(output.find("c") != std::string::npos);
}

struct SubItem {
  std::string val;
  bool operator==(const SubItem& other) const = default;
};

class StructLoopInterpolation
    : public rtxui::Component<StructLoopInterpolation> {
 public:
  std::vector<SubItem> items = {{"x"}, {"y"}};

  void InitReflection() override {
    BindCollection("items", &items, [](const SubItem& s) {
      return std::make_shared<rtxui::ManualStructVisitor>(
          std::unordered_map<std::string, std::string>{{"val", s.val}});
    });
    Import<rtxui::div>();
    Import<rtxui::span>();
    rtxui::Component<StructLoopInterpolation>::InitReflection();
  }

  std::string_view view = R"(
    <div>
      <for each="{items}" as="it">
        <span>{it.val}</span>
      </for>
    </div>
  )";
};

TEST_CASE("Struct Loop Interpolation", "[component][interpolation]") {
  auto component = rtxui::Ref<StructLoopInterpolation>::New();
  component->Mount();

  std::string output = component->Root()->Print();
  CHECK(output.find("x") != std::string::npos);
  CHECK(output.find("y") != std::string::npos);
}

struct AutoItem {
  std::string name;
  bool operator==(const AutoItem& other) const = default;
};

class AutoLoopInterpolation : public rtxui::Component<AutoLoopInterpolation> {
 public:
  std::vector<AutoItem> items = {{"Apple"}, {"Banana"}};

  void InitReflection() override {
    Bind(items);
    Import<rtxui::div>();
    Import<rtxui::span>();
    rtxui::Component<AutoLoopInterpolation>::InitReflection();
  }

  std::string_view view = R"(
    <div>
      <for each="{items}" as="it">
        <span>{it.name}</span>
      </for>
    </div>
  )";
};

TEST_CASE("Automatic Reflection Loop Interpolation",
          "[component][interpolation]") {
#if defined(RTXUI_HAS_REFLECTION)
  auto component = rtxui::Ref<AutoLoopInterpolation>::New();
  component->Mount();

  std::string output = component->Root()->Print();
  CHECK(output.find("Apple") != std::string::npos);
  CHECK(output.find("Banana") != std::string::npos);
#else
  SKIP("Reflection not supported by compiler");
#endif
}

class ConditionalApp : public rtxui::Component<ConditionalApp> {
 public:
  int value = 1;

  void InitReflection() override {
    Bind(value);
    Import<rtxui::div>();
    Import<rtxui::span>();
    rtxui::Component<ConditionalApp>::InitReflection();
  }

  std::string_view view = R"(
    <div>
      <if condition="{value == 1}">
        <span>One</span>
      </if>
      <!-- a comment -->
      <elif condition="{value == 2}">
        <span>Two</span>
      </elif>
      
      <else>
        <span>Other</span>
      </else>

      <span if="{value == 1}">Always One</span>
    </div>
  )";

  std::string GetInterpolatedValue(std::string_view expr) override {
    if (expr == "value == 1") {
      return value == 1 ? "true" : "false";
    }
    if (expr == "value == 2") {
      return value == 2 ? "true" : "false";
    }
    return rtxui::Component<ConditionalApp>::GetInterpolatedValue(expr);
  }
};

TEST_CASE("Conditional Rendering", "[component][interpolation]") {
  auto component = rtxui::Ref<ConditionalApp>::New();

  component->value = 1;
  component->Mount();
  std::string output = component->Root()->Print();
  CHECK(output.find("One") != std::string::npos);
  CHECK(output.find("Always One") != std::string::npos);
  CHECK(output.find("Two") == std::string::npos);
  CHECK(output.find("Other") == std::string::npos);

  component->value = 2;
  component->Render();
  output = component->Root()->Print();
  CHECK(output.find("One") == std::string::npos);
  CHECK(output.find("Always One") == std::string::npos);
  CHECK(output.find("Two") != std::string::npos);
  CHECK(output.find("Other") == std::string::npos);

  component->value = 3;
  component->Render();
  output = component->Root()->Print();
  CHECK(output.find("One") == std::string::npos);
  CHECK(output.find("Always One") == std::string::npos);
  CHECK(output.find("Two") == std::string::npos);
  CHECK(output.find("Other") != std::string::npos);
}

}  // namespace
