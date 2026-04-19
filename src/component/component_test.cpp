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

class Hello : public rtxui::Component<Hello> {
 public:
  std::string_view Setup() override {
    return R"(
      Hello
    )";
  }
};

class World : public rtxui::Component<World> {
 public:
  std::string_view Setup() override {
    return R"(
      World
    )";
  }
};

class HelloWorld : public rtxui::Component<HelloWorld> {
 public:
  std::string_view Setup() override {
    Import<Hello>();
    Import<World>();
    return R"(
      Hello, World!
      <Hello/>
      <World/>

      <style>
        self {
          display: block;
          background-color: red;
          color: white;
        }
      </style>
    )";
  }
};

TEST_CASE("Tag", "[component]") {
  REQUIRE(HelloWorld::StaticTag() == "HelloWorld");
  REQUIRE(Hello::StaticTag() == "Hello");
  REQUIRE(World::StaticTag() == "World");
}

TEST_CASE("Mount", "[component]") {
  auto component = rtxui::Ref<HelloWorld>::New();
  component->Mount();

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

  REQUIRE(component->Root()->Print() == StripIndent(expected));
}

class Page : public rtxui::Component<Page> {
 public:
  std::string_view Setup() override {
   return R"(
    <slot.header/>
    <slot/>
    <slot.footer/>
  )";
 }
};

class MyPage : public rtxui::Component<MyPage> {
 public:
  std::string_view Setup() override {
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
};

TEST_CASE("Component with named slots", "[component]") {
  auto mypage = rtxui::Ref<MyPage>::New();
  mypage->Mount();

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

  REQUIRE(mypage->Root()->Print() == StripIndent(expected));
}

class Inverted : public rtxui::Component<Inverted> {
 public:
  std::string_view Setup() override {
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
};

TEST_CASE("Import alias", "[component]") {
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
  
  auto inverted = rtxui::Ref<Inverted>::New();
  inverted->Mount();
  REQUIRE(inverted->Root()->Print() == StripIndent(expected));
}

class Counter : public rtxui::Component<Counter> {
 public:
  int count = 0;
  int double_count() const { return count * 2; }

  void increment() { count++; }
  void reset() { count = 0; }

  std::string_view Setup() override {
    Import<rtxui::div>();
    Import<rtxui::ul>();
    Import<rtxui::li>();
    Import<rtxui::p>();
    Import<rtxui::button>();

    return R"(
      <ul>
        <li>Count: {count}</li>
        <li>Double: {double_count}</li>
      </ul>

      <button onclick="increment">Increment</button>
      <button onclick="reset">Reset</button>
    )";
  }
};

TEST_CASE("Transparent Reactivity", "[component]") {
  auto counter = rtxui::Ref<Counter>::New();
  counter->Mount();
  
  REQUIRE(counter->count == 0);
  
  // Simulate an action
  counter->increment();
  // The increment() method is an Action, so it should have auto-digested.
  // But since we are calling it from C++, we need to manually Digest() 
  // unless we use the internal binding logic.
  counter->Digest();
  
  REQUIRE(counter->count == 1);
}

}  // namespace
