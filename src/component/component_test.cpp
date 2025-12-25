// Copyright 2024 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include "component/component.hpp"
#include "component/default_components.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <iostream>

#include "core/string.hpp"

namespace {

RTXUI_COMPONENT(Hello) {
  return R"(
    Hello
  )";
}

RTXUI_COMPONENT(World) {
  return R"(
    World
  )";
}

RTXUI_COMPONENT(HelloWorld) {
  // Import components
  Import<Hello>();
  Import<World>();

  //// Define internal reactive state.
  //auto count = State(0);

  //// Define computed state.
  //auto computed = Computed([count]() {
    //return "Count is: " + std::to_string(count->Value());
  //});

  //// Define callbacks.
  //auto increment = [count] {
    //count->SetValue(count->Value() + 1);
  //}

  //auto reset = [count] {
    //count->SetValue(0);
  //}

  //// Bind the state and computed values to the template.
  //Import("count", count);
  //Import("computed", computed);
  //Import("increment", increment);

  return R"(
    Hello, World!
    <Hello/>
    <World/>

    <style>
      self {
        display-inside: flow;
        display-outside: block;
        background-color: red;
        foreground-color: white;
        decoration: bold;
      }
    </style>
  )";
}

TEST_CASE("Tag", "[component]") {
  REQUIRE(HelloWorld().Tag() == "HelloWorld");
  REQUIRE(Hello().Tag() == "Hello");
  REQUIRE(World().Tag() == "World");
}

TEST_CASE("Setup", "[component]") {
  HelloWorld component;
  REQUIRE(component.Template() == StripIndent(R"(
    Hello, World!
    <Hello/>
    <World/>

    <style>
      self {
        display-inside: flow;
        display-outside: block;
        background-color: red;
        foreground-color: white;
        decoration: bold;
      }
    </style>
  )"));
}

TEST_CASE("Mount", "[component]") {
  auto component = HelloWorld();
  component.Mount();

  const std::string expected = R"(
    <HelloWorld>
      Hello, World!
      <Hello>
        Hello
      </Hello>
      <World>
        World
      </World>
    </HelloWorld>
  )";

  REQUIRE(component.Root()->Print() == StripIndent(expected));
}

 RTXUI_COMPONENT(Page) {
   return R"(
    <slot.header/>
    <slot/>
    <slot.footer/>
  )";
}

RTXUI_COMPONENT(MyPage) {
  Import<rtxui::p>();
  Import<Page>();
  return R"(
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
  )";
}

TEST_CASE("Component with named slots", "[component]") {
  MyPage mypage;
  mypage.Mount();

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

  REQUIRE(mypage.Root()->Print() == StripIndent(expected));
}

RTXUI_COMPONENT(Inverted) {
  // Test the Import alias feature, by inverting the semantics of `p` and `div`.
  Import<rtxui::p>("div");
  Import<rtxui::div>("p");

  return R"(
    <div>
      <p>
        This is a div inside a p.
      </p>
      <div>
        This is a p inside a p.
      </div>
    </div>

    <p>
      <div>
        This is a p inside a div.
      </div>
      <p>
        This is a div inside a div.
      </p>
    </p>
  )";
}

TEST_CASE("Import alias", "[component]") {
  // Here we try to test the Import alias feature by importing `p` as `div` and
  // `div` as `p`. This should invert the semantics of the two elements in the
  // template.
  const std::string expected = R"(
    <Inverted>
      <p>
        <div>
          This is a div inside a p.
        </div>
        <p>
          This is a p inside a p.
        </p>
      </p>
      <div>
        <p>
          This is a p inside a div.
        </p>
        <div>
          This is a div inside a div.
        </div>
      </div>
    </Inverted>
  )";
  
  Inverted inverted;
  inverted.Mount();
  REQUIRE(inverted.Root()->Print() == StripIndent(expected));
}

RTXUI_COMPONENT(Counter) {
  // Define internal reactive state.
  auto count = State(0);

  // Define computed state.
  auto double_count = Computed<int>([=] { return count->Value() * 2; });

  // Define callbacks that will be called when the user interacts with the
  // component.
  auto increment = [=] { count->Value(count->Value() + 1); };
  auto reset = [=] { count->Value(0); };

  // Import the components used in the template.
  //
  // Note that there are no "special" components in rtxui. All components are
  // regular components that can be imported and used in the template.
  //
  // RTXUI provides a set of default components that can be imported, such as
  // `p`, `span`, and `button`. These components are defined in the
  // `default_components.hpp` header file, which is included in the
  // `components` directory.
  //
  // Import are scoped to the component we are defining. This means that every
  // component can import the same component without conflicts.
  Import<rtxui::div>();
  Import<rtxui::ul>();
  Import<rtxui::li>();

  // Bind the state and callbacks to make them available to the template.
  Import("count", count);
  Import("double_count", double_count);
  Import("increment", increment);
  Import("reset", reset);

  // Return the template of the component.
  return R"(
    <ul>
      <li>Count: {count}<p>
      <li>Count: {count}</p>
    </ul>

    <button onclick="{increment}">Increment</button>
    <button onclick="{reset}">Reset</button>

    <style>
      p {
        background-color:red;
        color: white;
        decoration: bold;
      }

      button {
        background-color: blue;
        color: white;
        padding: 5px;
        border-radius: 3px;
      }
    </style>
  )";
}

}  // namespace
