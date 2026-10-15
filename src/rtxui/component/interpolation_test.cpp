// Copyright 2024 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include <catch2/catch_test_macros.hpp>
#include <cmath>
#include <functional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "rtxui/component.hpp"
#include "rtxui/component/component_internal.hpp"
#include "rtxui/dom/element.hpp"
#include "rtxui/dom/text_element.hpp"
#include "rtxui/refcounted.hpp"

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

  InterpolationComponent() {
    RegisterProperties();
    Import("get_greeting", &InterpolationComponent::get_greeting);
    Import("get_next_version", &InterpolationComponent::get_next_version);
  }

  std::string_view view = R"(
    <div id="v{version}" class="{name}">
      <span>{get_greeting}</span>
      <span>Version: {version} (Next: {get_next_version})</span>
    </div>
  )";
};

// A bool reads the way the library itself spells it when it propagates a
// binding ("true"/"false"), and a char is the character, not its code.
class ScalarComponent : public rtxui::Component<ScalarComponent> {
 public:
  bool on = true;
  bool off = false;
  char letter = 'A';
  ScalarComponent() {
    Bind(on);
    Bind(off);
    Bind(letter);
  }
  std::string_view view = "<span>{on} {off} {letter}</span>";
};

TEST_CASE("Interpolating a bool and a char", "[component][interpolation]") {
  auto component = rtxui::Ref<ScalarComponent>::New();
  rtxui::ComponentInternals::Mount(*component);
  CHECK(rtxui::ComponentInternals::Root(*component)
            ->Print()
            .find("true false A") != std::string::npos);
}

TEST_CASE("Interpolation", "[component][interpolation]") {
  auto component = rtxui::Ref<InterpolationComponent>::New();
  rtxui::ComponentInternals::Mount(*component);

  std::string output = rtxui::ComponentInternals::Root(*component)->Print();

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

  NestedInterpolation() { Bind(color); }

  std::string_view view = R"(
    <div style="background-color: {color};">
      Content
    </div>
  )";
};

TEST_CASE("Style Interpolation", "[component][interpolation]") {
  auto component = rtxui::Ref<NestedInterpolation>::New();
  rtxui::ComponentInternals::Mount(*component);

  std::string output = rtxui::ComponentInternals::Root(*component)->Print();
  CHECK(output.find("background-color: red") != std::string::npos);
}

class LoopInterpolation : public rtxui::Component<LoopInterpolation> {
 public:
  std::vector<std::string> items = {"a", "b", "c"};

  LoopInterpolation() { BindCollection("items", &items); }

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
  rtxui::ComponentInternals::Mount(*component);

  std::string output = rtxui::ComponentInternals::Root(*component)->Print();
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

  StructLoopInterpolation() {
    BindCollection(
        "items", &items,
        [](const SubItem& s, rtxui::FieldSink& out) { out("val", s.val); });
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
  rtxui::ComponentInternals::Mount(*component);

  std::string output = rtxui::ComponentInternals::Root(*component)->Print();
  CHECK(output.find("x") != std::string::npos);
  CHECK(output.find("y") != std::string::npos);
}

struct BoundRow {
  std::string name;
  std::string cls;
  bool operator==(const BoundRow& other) const = default;
};

class ReactiveStructCollection
    : public rtxui::Component<ReactiveStructCollection> {
 public:
  std::vector<BoundRow> rows = {{"one", "hot"}, {"two", "cold"}};

  ReactiveStructCollection() {
    BindCollection("rows", &rows,
                   [](const BoundRow& row, rtxui::FieldSink& out) {
                     out("name", row.name);
                     out("cls", row.cls);
                   });
  }

  // A field in text, a field in an attribute, $index alongside them, and a
  // field the mapper does not publish.
  std::string_view view = R"html(
    <div>
      <for each="{rows}" as="r">
        <div class="row {r.cls}">{$index}:{r.name}/{r.missing}|</div>
      </for>
    </div>
  )html";
};

TEST_CASE("A bound struct collection tracks its contents",
          "[component][interpolation]") {
  // The existing struct test checks that the first render shows the values.
  // What was not covered is everything after that: whether a change inside a
  // struct reaches the DOM at all, and whether adding, removing and reordering
  // items land where they should.
  auto component = rtxui::Ref<ReactiveStructCollection>::New();
  rtxui::ComponentInternals::Mount(*component);
  component->Digest();

  auto text = [&]() {
    std::string out;
    rtxui::ComponentInternals::Root(*component)
        ->Visit([&out](rtxui::Element& element) {
          if (element.is_text()) {
            out += static_cast<rtxui::TextElement&>(element).text();
          }
        });
    return out;
  };
  auto classes = [&]() {
    std::string out;
    rtxui::ComponentInternals::Root(*component)
        ->Visit([&out](rtxui::Element& element) {
          for (const auto& name : element.classes()) {
            out += name;
            out += ",";
          }
        });
    return out;
  };

  SECTION("fields render, and an unpublished one is empty rather than fatal") {
    CHECK(text() == "0:one/|1:two/|");
  }

  SECTION("a field reaches an attribute as well as text") {
    CHECK(classes() == "row,hot,row,cold,");
  }

  SECTION("changing a field inside a struct updates the DOM") {
    component->rows[0].name = "ONE";
    component->Digest();
    CHECK(text() == "0:ONE/|1:two/|");

    component->rows[0].cls = "warm";
    component->Digest();
    CHECK(classes() == "row,warm,row,cold,");
  }

  SECTION("adding, removing and reordering items all land correctly") {
    component->rows.push_back({"three", "mild"});
    component->Digest();
    CHECK(text() == "0:one/|1:two/|2:three/|");

    component->rows.erase(component->rows.begin());
    component->Digest();
    CHECK(text() == "0:two/|1:three/|");

    std::swap(component->rows[0], component->rows[1]);
    component->Digest();
    CHECK(text() == "0:three/|1:two/|");

    component->rows.clear();
    component->Digest();
    CHECK(text().empty());
  }
}

struct AutoItem {
  std::string name;
  bool operator==(const AutoItem& other) const = default;
};

class AutoLoopInterpolation : public rtxui::Component<AutoLoopInterpolation> {
 public:
  std::vector<AutoItem> items = {{"Apple"}, {"Banana"}};

  AutoLoopInterpolation() { Bind(items); }

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
  bool is_one() const { return value == 1; }
  bool is_two() const { return value == 2; }

  ConditionalApp() {
    Bind(value);
    Bind(is_one);
    Bind(is_two);
  }

  std::string_view view = R"(
    <div>
      <if condition="{is_one}">
        <span>One</span>
      </if>
      <!-- a comment -->
      <elif condition="{is_two}">
        <span>Two</span>
      </elif>
      
      <else>
        <span>Other</span>
      </else>

      <span if="{is_one}">Always One</span>
    </div>
  )";
};

TEST_CASE("Conditional Rendering", "[component][interpolation]") {
  auto component = rtxui::Ref<ConditionalApp>::New();

  component->value = 1;
  rtxui::ComponentInternals::Mount(*component);
  std::string output = rtxui::ComponentInternals::Root(*component)->Print();
  CHECK(output.find("One") != std::string::npos);
  CHECK(output.find("Always One") != std::string::npos);
  CHECK(output.find("Two") == std::string::npos);
  CHECK(output.find("Other") == std::string::npos);

  component->value = 2;
  rtxui::ComponentInternals::Render(*component);
  output = rtxui::ComponentInternals::Root(*component)->Print();
  CHECK(output.find("One") == std::string::npos);
  CHECK(output.find("Always One") == std::string::npos);
  CHECK(output.find("Two") != std::string::npos);
  CHECK(output.find("Other") == std::string::npos);

  component->value = 3;
  rtxui::ComponentInternals::Render(*component);
  output = rtxui::ComponentInternals::Root(*component)->Print();
  CHECK(output.find("One") == std::string::npos);
  CHECK(output.find("Always One") == std::string::npos);
  CHECK(output.find("Two") == std::string::npos);
  CHECK(output.find("Other") != std::string::npos);
}
}  // namespace

namespace rtxui {
float ApplyEasing(float t, std::string_view timing);

TEST_CASE("Transition Easing Functions", "[dom][easing]") {
  auto IsClose = [](float a, float b) { return std::abs(a - b) < 1e-5f; };

  // Test boundary values
  CHECK(ApplyEasing(0.0f, "linear") == 0.0f);
  CHECK(ApplyEasing(-0.5f, "linear") == 0.0f);
  CHECK(ApplyEasing(1.0f, "linear") == 1.0f);
  CHECK(ApplyEasing(1.5f, "linear") == 1.0f);

  // Test linear
  CHECK(ApplyEasing(0.5f, "linear") == 0.5f);

  // Test standard curves
  CHECK(ApplyEasing(0.5f, "ease") > 0.0f);
  CHECK(ApplyEasing(0.5f, "ease-in") > 0.0f);
  CHECK(ApplyEasing(0.5f, "ease-out") > 0.0f);
  CHECK(ApplyEasing(0.5f, "ease-in-out") > 0.0f);

  // Test new standard easing curves
  // Sine
  CHECK(ApplyEasing(0.0f, "ease-in-sine") == 0.0f);
  CHECK(ApplyEasing(1.0f, "ease-in-sine") == 1.0f);
  CHECK(IsClose(ApplyEasing(0.5f, "ease-in-sine"),
                1.0f - std::cos(0.5f * 3.1415926535f / 2.0f)));
  CHECK(IsClose(ApplyEasing(0.5f, "ease-out-sine"),
                std::sin(0.5f * 3.1415926535f / 2.0f)));
  CHECK(IsClose(ApplyEasing(0.5f, "ease-in-out-sine"), 0.5f));

  // Quad
  CHECK(IsClose(ApplyEasing(0.5f, "ease-in-quad"), 0.25f));
  CHECK(IsClose(ApplyEasing(0.5f, "ease-out-quad"), 0.75f));
  CHECK(IsClose(ApplyEasing(0.25f, "ease-in-out-quad"), 0.125f));
  CHECK(IsClose(ApplyEasing(0.75f, "ease-in-out-quad"), 0.875f));

  // Cubic
  CHECK(IsClose(ApplyEasing(0.5f, "ease-in-cubic"), 0.125f));
  CHECK(IsClose(ApplyEasing(0.5f, "ease-out-cubic"), 0.875f));

  // Quart
  CHECK(IsClose(ApplyEasing(0.5f, "ease-in-quart"), 0.0625f));
  CHECK(IsClose(ApplyEasing(0.5f, "ease-out-quart"), 0.9375f));

  // Quint
  CHECK(IsClose(ApplyEasing(0.5f, "ease-in-quint"), 0.03125f));
  CHECK(IsClose(ApplyEasing(0.5f, "ease-out-quint"), 0.96875f));

  // Expo
  CHECK(IsClose(ApplyEasing(0.5f, "ease-in-expo"), std::pow(2.0f, -5.0f)));
  CHECK(IsClose(ApplyEasing(0.5f, "ease-out-expo"),
                1.0f - std::pow(2.0f, -5.0f)));

  // Circ
  CHECK(IsClose(ApplyEasing(0.5f, "ease-in-circ"), 1.0f - std::sqrt(0.75f)));
  CHECK(IsClose(ApplyEasing(0.5f, "ease-out-circ"), std::sqrt(0.75f)));

  // Back
  CHECK(ApplyEasing(0.5f, "ease-in-back") < 0.5f);
  CHECK(ApplyEasing(0.5f, "ease-out-back") > 0.5f);
}
}  // namespace rtxui
