// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include "rtxui/headless.hpp"

#include <string>
#include <utility>

#include "catch2/catch_test_macros.hpp"
#include "rtxui/component.hpp"
#include "rtxui/component/default_components_internal.hpp"

namespace rtxui {
namespace {

class Greeting : public Component<Greeting> {
 public:
  std::string name = "world";
  void InitReflection() override {
    Bind(name);
    Import<div>();
  }
  std::string_view view = R"html(
    <div>Hello {name}</div>
  )html";
};

TEST_CASE("RenderToString returns the screen as text", "[headless]") {
  // One line per row, trailing spaces trimmed, so a fixed-size frame compares
  // as a plain string.
  CHECK(RenderToString(Ref<Greeting>::New(), 20, 3) == "Hello world\n\n\n");
}

class RightAligned : public Component<RightAligned> {
 public:
  void InitReflection() override { Import<div>(); }
  std::string_view view = R"html(
    <div style="text-align: right">end</div>
  )html";
};

TEST_CASE("A terminal wider than 255 columns is drawn in full", "[headless]") {
  // The screen buffer once stored its size in a byte: 300 columns became 44,
  // and everything past them was never drawn.
  HeadlessScreen screen(Ref<RightAligned>::New(), 300, 2);
  CHECK(screen.Text() == std::string(297, ' ') + "end\n\n");
}

class Form : public Component<Form> {
 public:
  std::string text;
  int clicks = 0;
  void Add() { clicks++; }
  void InitReflection() override {
    Bind(text);
    Bind(clicks);
    Bind(Add);
    Import<div>();
    Import<input>();
    Import<button>();
  }
  std::string_view view = R"html(
    <div>
      <div onclick="Add">clicks: {clicks}</div>
      <input value="{text}"/>
      <div>typed: {text}</div>
    </div>
  )html";
};

TEST_CASE("HeadlessScreen takes input and shows its effect", "[headless]") {
  auto app = Ref<Form>::New();
  HeadlessScreen screen(app, 30, 6);
  CHECK(screen.Text().starts_with("clicks: 0\n"));

  SECTION("a click runs the handler") {
    screen.Click(0, 0);
    CHECK(app->clicks == 1);
    CHECK(screen.Text().starts_with("clicks: 1\n"));
  }

  SECTION("typed text reaches the focused input") {
    screen.Input("\t");  // Focus the input.
    screen.Input("abc");
    CHECK(app->text == "abc");
    CHECK(screen.Text().find("typed: abc") != std::string::npos);
  }

  SECTION("a resize changes the frame") {
    screen.Resize(12, 2);
    CHECK(screen.Text().starts_with("clicks: 0\n"));
    CHECK(screen.Text().size() <= 2 * 13);  // Two rows of at most 12 + '\n'.
  }
}

}  // namespace
}  // namespace rtxui

namespace rtxui {
namespace {

class ConfirmDialog : public Component<ConfirmDialog> {
 public:
  bool open = true;
  void InitReflection() override {
    Bind(open);
    Import<div>();
    Import<span>();
    Import<button>();
    Import<dialog>();
  }
  std::string_view view = R"html(
    <div>
      <span>Status</span>
      <button>Delete</button>
      <dialog open="{open}" title="Confirm">
        <p>Are you sure?</p>
        <div><button>Yes</button><button>No</button></div>
      </dialog>
    </div>
  )html";
};

TEST_CASE("A dialog lays out the same at every screen size", "[headless]") {
  // Found by the agent evaluation: at exactly 80x24 the dialog's frame
  // collapsed to four rows and its buttons spilled out below it.
  for (auto [width, height] : {std::pair{80, 24}, std::pair{81, 24}}) {
    CAPTURE(width, height);
    std::string text = RenderToString(Ref<ConfirmDialog>::New(), width, height);
    auto bottom = text.find("╚");
    REQUIRE(bottom != std::string::npos);
    CHECK(text.find("Yes") < bottom);
    CHECK(text.find("Are you sure?") < bottom);
  }
}

}  // namespace
}  // namespace rtxui
