// Copyright 2024 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include "component.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <iostream>

#include "core/string.hpp"
#include "register.hpp"

namespace rtxui::test {
namespace {

RTXUI_COMPONENT(Component) {
  return R"(
    <template>
      Hello, World!
      <Hello/>
      <World/>
    </template>

    <style>
      this {
        display-inside: flow;
        display-outside: block;
        background-color: red;
        foreground-color: white;
        decoration: bold
      }
    </style>
  )";
}

RTXUI_COMPONENT(Hello) {
  return R"(
    <template>
      <div>
        Hello
      </div>
    </template>
  )";
}

RTXUI_COMPONENT(World) {
  return R"(
    <template>
      <div>
        World
      </div>
    </template>
  )";
}

TEST_CASE("RegisterAll", "[component]") {
  std::string expected = R"(
    div
    p
    rtxui {
      test {
        Component
        Hello
        World
      }
    }
  )";
  std::cerr << "```" << std::endl;
  std::cerr << Register::Print() << std::endl;
  std::cerr << "```" << std::endl;
}

TEST_CASE("Registered", "[component]") {
  REQUIRE(Register::Get("Component") != nullptr);
  REQUIRE(Register::Get("Hello") != nullptr);
  REQUIRE(Register::Get("World") != nullptr);
  REQUIRE(Register::Get("Unknown") == nullptr);
}

TEST_CASE("Setup", "[component]") {
  REQUIRE(Register::Get("Component")->New()->Setup() == R"(
    <template>
      Hello, World!
      <Hello/>
      <World/>
    </template>

    <style>
      this {
        display-inside: flow;
        display-outside: block;
        background-color: red;
        foreground-color: white;
        decoration: bold
      }
    </style>
  )");

  REQUIRE(Register::Get("Hello")->New()->Setup() == R"(
    <template>
      <div>
        Hello
      </div>
    </template>
  )");

  REQUIRE(Register::Get("World")->New()->Setup() == R"(
    <template>
      <div>
        World
      </div>
    </template>
  )");
}

TEST_CASE("Mount", "[component]") {
  auto component = Register::Get("Component")->New();
  component->Mount();

  const std::string expected = R"(
    <Component>
      Hello, World!
      <Hello>
        <div>
          Hello
        </div>
      </Hello>
      <World>
        <div>
          World
        </div>
      </World>
    </Component>
  )";

  REQUIRE(component->Root()->Print() == StripIndent(expected));
}

RTXUI_COMPONENT(Page) {
  return R"(
    <template>
      <slot.header/>
      <slot/>
      <slot.footer/>
    </template>
  )";
}

RTXUI_COMPONENT(MyPage) {
  return R"(
    <template>
      <p> This is a page: </p>
      <Page>
        <template.header>
          <p>
            A website by Arthur Sonzogni
          </p>
        </template.header>

        <template.footer>
          <p>
            © 2024 Arthur Sonzogni
          </p>
        </template.footer>

        <p>
          Hello, World!
        </p>
      </Page>
    </template>
  )";
}

TEST_CASE("Component with named slots", "[component]") {
  auto component = Register::Get("MyPage")->New();
  component->Mount();

  const std::string expected = R"(
    <MyPage>
      <p>
        This is a page:
      </p>
      <Page>
        <p>
          A website by Arthur Sonzogni
        </p>
        <p>
          Hello, World!
        </p>
        <p>
          © 2024 Arthur Sonzogni
        </p>
      </Page>
    </MyPage>
  )";

  REQUIRE(component->Root()->Print() == StripIndent(expected));
}

namespace a {
RTXUI_COMPONENT(div) {
  return R"(
      <template>
        <p>
          This is a "ftxui.test.a.div", hiding the builtin "div".
        </p>

        <slot/>
      </template>
    )";
}

RTXUI_COMPONENT(div_user) {
  return R"(
      <template>
        <div>
          <p>What div is this?</p>
        </div>
      </template>
    )";
}
}  // namespace a

RTXUI_COMPONENT(TwoDiv) {
  return R"(
    <template>

      <div>
        First div.
      </div>

      <a.div>
        Second div.
      </a.div>

      <a.div_user/>

    </template>
  )";
}

TEST_CASE("Component namespace", "[component]") {
  auto component = Register::Get("TwoDiv")->New();
  component->Mount();

  const std::string expected = R"(
    <TwoDiv>
      <div>
        First div.
      </div>
      <div>
        <p>
          This is a "ftxui.test.a.div", hiding the builtin "div".
        </p>
        Second div.
      </div>
      <div_user>
        <div>
          <p>
            This is a "ftxui.test.a.div", hiding the builtin "div".
          </p>
          <p>
            What div is this?
          </p>
        </div>
      </div_user>
    </TwoDiv>
  )";

  REQUIRE(component->Root()->Print() == StripIndent(expected));
}

}  // namespace
}  // namespace rtxui::test
