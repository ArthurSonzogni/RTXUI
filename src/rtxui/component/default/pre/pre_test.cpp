// Copyright 2024 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.

#include <catch2/catch_test_macros.hpp>
#include <string>
#include <string_view>

#include "rtxui/component.hpp"
#include "rtxui/paint/texture.hpp"
#include "rtxui/refcounted.hpp"
#include "rtxui/testing/render.hpp"

using namespace rtxui;

namespace {

struct PreTestComponent : public rtxui::Component<PreTestComponent> {
  std::string_view view = R"(
    <div>
      <pre>Line 1
  Line 2 with spaces
Line 3</pre>
    </div>
  )";
};

}  // namespace

TEST_CASE("Pre component whitespace preservation", "[component][pre]") {
  auto container = rtxui::Ref<PreTestComponent>::New();
  auto texture = RenderComponent(container, 80, 24);

  std::string rendered = texture.Render();
  CHECK(rendered.find("Line 1") != std::string::npos);
  CHECK(rendered.find("  Line 2 with spaces") != std::string::npos);
  CHECK(rendered.find("Line 3") != std::string::npos);
}

namespace {

struct PreCodeTestComponent : public rtxui::Component<PreCodeTestComponent> {
  std::string_view view = R"(
    <div>
      <pre><code>int main() {
  return 0;
}</code></pre>
    </div>
  )";
};

}  // namespace

TEST_CASE("Pre code newlines preserved", "[component][pre]") {
  auto container = rtxui::Ref<PreCodeTestComponent>::New();
  auto texture = RenderComponent(container, 80, 24);

  // Each source line must land on its own row.
  std::string rendered = texture.Render();
  CHECK(rendered.find("int main() {") != std::string::npos);
  CHECK(rendered.find("  return 0;") != std::string::npos);
  CHECK(rendered.find("int main() {   return 0; }") == std::string::npos);
  auto pos_main = rendered.find("int main()");
  auto pos_return = rendered.find("return 0;");
  REQUIRE(pos_main != std::string::npos);
  REQUIRE(pos_return != std::string::npos);
  CHECK(rendered.find('\n', pos_main) < pos_return);
}

namespace {

struct PreCodeBindingTestComponent
    : public rtxui::Component<PreCodeBindingTestComponent> {
  std::string source = "int main() {\n  return 0;\n}";
  PreCodeBindingTestComponent() { Bind(source); }
  std::string_view view = R"(
    <div>
      <pre><code>{source}</code></pre>
    </div>
  )";
};

}  // namespace

TEST_CASE("Pre code newlines preserved through interpolation",
          "[component][pre]") {
  auto container = rtxui::Ref<PreCodeBindingTestComponent>::New();
  container->Mount();

  SECTION("initial value") {}
  SECTION("value updated after mount") {
    container->source = "// updated\nint main() {\n  return 0;\n}";
    container->Digest();
  }

  auto texture = PaintComponent(*container, 80, 24);

  std::string rendered = texture.Render();
  auto pos_main = rendered.find("int main()");
  auto pos_return = rendered.find("return 0;");
  REQUIRE(pos_main != std::string::npos);
  REQUIRE(pos_return != std::string::npos);
  CHECK(rendered.find('\n', pos_main) < pos_return);
}
