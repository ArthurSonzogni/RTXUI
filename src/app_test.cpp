#include "app.hpp"
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include "paint/texture.hpp"

namespace {

TEST_CASE("App basic", "[app]") {
  App app;
  app.Register("Main", [](Element& element) {
    element.Dom(R"(
      <div>Hello, World!</div>
      <Hello/>
      <World/>
    )");
    element.Style(R"(
      div {
        color: red;
      }
    )");
  });

  app.Register("Hello", [](Element& element) {
    element.Dom(R"(
      <div>Hello</div>
    )");
  });

  app.Register("World", [](Element& element) {
    element.Ref("ref", 42);
    element.Dom(R"(
      <div>World</div>
      {{ref}}
    )");
  });

  Texture output(10, 5);
  app.Render("Main", output);
}

}  // namespace
