// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include <catch2/catch_test_macros.hpp>
#include <string>
#include <string_view>
#include <vector>

#include "rtxui/color.hpp"
#include "rtxui/component.hpp"
#include "rtxui/diagnostic.hpp"
#include "rtxui/dom/element.hpp"
#include "rtxui/headless.hpp"
#include "rtxui/style/style.hpp"

namespace rtxui {
namespace {

class DiagnosticRecorder {
 public:
  DiagnosticRecorder() {
    SetDiagnosticHandler(
        [this](const Diagnostic& d) { messages.push_back(d.message); });
  }
  ~DiagnosticRecorder() { SetDiagnosticHandler(nullptr); }
  DiagnosticRecorder(const DiagnosticRecorder&) = delete;
  DiagnosticRecorder& operator=(const DiagnosticRecorder&) = delete;

  bool Has(std::string_view fragment) const {
    for (const auto& message : messages) {
      if (message.find(fragment) != std::string::npos) {
        return true;
      }
    }
    return false;
  }

  std::vector<std::string> messages;
};

class Decorated : public Component<Decorated> {
 public:
  std::string_view view = R"html(
    <div>
      <div class="item">first</div>
      <div class="item done">second</div>
      <div><span class="link" href="docs">see</span></div>
      <div class="arrow">go</div>
      <div class="banner">body</div>
      <div class="hidden">plain</div>
    </div>
    <style>
      .item::before { content: "- "; }
      .done::before { content: "x "; color: rgb(0, 200, 0); }
      .link::after { content: " (" attr(href) ")"; }
      .arrow::before { content: "\25B6  "; }
      .banner::before { content: "== title =="; display: block; }
      .hidden::before { content: "never "; }
      .hidden::before { content: none; }
    </style>
  )html";
};

TEST_CASE("::before and ::after generate content", "[component][generated]") {
  DiagnosticRecorder diagnostics;
  CHECK(RenderToString(Ref<Decorated>::New(), 20, 7) ==
        "- first\n"
        "x second\n"
        "see (docs)\n"
        "▶ go\n"
        "== title ==\n"
        "body\n"
        "plain\n");
  CHECK(diagnostics.messages.empty());
}

TEST_CASE("A ::before box takes its own style, and inherits the rest",
          "[component][generated]") {
  auto app = Ref<Decorated>::New();
  app->Mount();
  Element* done = app->Root()->QuerySelector(".done");
  REQUIRE(done != nullptr);
  Element* before = done->GeneratedBox(/*after=*/false);
  REQUIRE(before != nullptr);
  CHECK(before->style.foreground_color == Color::RGB(0, 200, 0));
  // Events on it reach the element it decorates.
  CHECK(before->Parent() == done);
  // It is not part of the DOM: selectors and walks never meet it.
  CHECK(done->GeneratedBox(/*after=*/true) == nullptr);
  CHECK(app->Root()->QuerySelector(".hidden")->GeneratedBox(false) == nullptr);
}

class Stateful : public Component<Stateful> {
 public:
  bool open = false;
  std::string state() const { return open ? "open" : "closed"; }
  Stateful() {
    Bind(open);
    Bind(state);
  }
  std::string_view view = R"html(
    <div>
      <div class="{state}">menu</div>
      <div class="row">a</div>
      <div class="row">b</div>
    </div>
    <style>
      .closed::before { content: "+ "; }
      .open::before { content: "- "; }
      .row:first-child::before { content: "first "; }
      .row:last-child::after { content: " last"; }
    </style>
  )html";
};

TEST_CASE("Generated content follows classes and pseudo-classes",
          "[component][generated]") {
  auto app = Ref<Stateful>::New();
  HeadlessScreen screen(app, 20, 3);
  CHECK(screen.Text() == "+ menu\na\nb last\n");
  app->open = true;
  screen.Input("");
  CHECK(screen.Text() == "- menu\na\nb last\n");
}

class Misplaced : public Component<Misplaced> {
 public:
  std::string_view view = R"html(
    <div>
      <div class="a">a</div>
      <div class="b">b</div>
      <div class="c">c</div>
    </div>
    <style>
      .a { content: "nope"; }
      .b::before { content: nope; }
      .c::before:hover { content: "nope"; }
    </style>
  )html";
};

TEST_CASE("content that cannot generate anything is reported",
          "[component][generated]") {
  DiagnosticRecorder diagnostics;
  CHECK(RenderToString(Ref<Misplaced>::New(), 10, 3) == "a\nb\nc\n");
  CHECK(diagnostics.Has("only applies in a ::before or ::after rule"));
  CHECK(diagnostics.Has("invalid 'content: nope'"));
}

TEST_CASE("::before and ::after parse as pseudo-elements", "[css][generated]") {
  auto sheet = css::Parse(
      ".a::before { content: 'x'; } .b:hover::after { content: 'y'; } "
      ".c:before { content: 'z'; } .d::before:hover { content: 'w'; }");
  REQUIRE(sheet);
  const auto& rules = sheet.value();
  REQUIRE(rules.size() == 4);
  CHECK(rules[0].parsed_selector.pseudo_element == "before");
  CHECK(rules[0].parsed_selector.pseudo_classes.empty());
  CHECK(rules[1].parsed_selector.pseudo_element == "after");
  CHECK(rules[1].parsed_selector.pseudo_classes ==
        std::vector<std::string>{"hover"});
  // The legacy one-colon spelling still names a pseudo-element.
  CHECK(rules[2].parsed_selector.pseudo_element == "before");
  // Not at the end, it is not one: the selector matches nothing.
  CHECK(rules[3].parsed_selector.pseudo_element.empty());
  // A pseudo-element weighs as a type.
  CHECK(rules[0].parsed_selector.specificity == css::MakeSpecificity(0, 1, 1));
}

}  // namespace
}  // namespace rtxui
