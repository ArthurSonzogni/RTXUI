// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include <catch2/catch_test_macros.hpp>
#include <string>
#include <vector>

#include "rtxui/component.hpp"
#include "rtxui/diagnostic.hpp"
#include "rtxui/dom/element.hpp"
#include "rtxui/dom/text_element.hpp"
#include "rtxui/headless.hpp"

namespace rtxui {
namespace {

class Log : public Component<Log> {
 public:
  std::vector<std::string> lines;
  explicit Log(int count = 1000) {
    for (int i = 0; i < count; ++i) {
      lines.push_back("line " + std::to_string(i));
    }
    Bind(lines);
  }
  std::string_view view = R"html(
    <div class="log">
      <for each="{lines}" as="line" virtual="">
        <div class="row">{$index}: {line}</div>
      </for>
    </div>
    <style>
      .log { height: 5; overflow-y: scroll; scrollbar-width: none; }
    </style>
  )html";
};

int CountRows(Element* root) {
  int rows = 0;
  root->Visit([&](Element& element) {
    for (const auto& name : element.classes) {
      rows += name == "row";
    }
  });
  return rows;
}

TEST_CASE("A virtual loop renders only the items around the view",
          "[component][virtual]") {
  auto app = Ref<Log>::New();
  HeadlessScreen screen(app, 20, 5);
  Element* log = app->Root()->QuerySelector(".log");
  REQUIRE(log != nullptr);
  // Five rows show; as many again on either side are rendered, not 1000.
  CHECK(CountRows(app->Root()) <= 12);
  // The spacers keep the scroll range the whole list's.
  CHECK(log->scroll_height() == 1000);
}

TEST_CASE("Scrolling a virtual loop renders the items scrolled to",
          "[component][virtual]") {
  auto app = Ref<Log>::New();
  HeadlessScreen screen(app, 20, 5);
  CHECK(screen.Text().starts_with("0: line 0\n"));

  // Jump far down, as a scrollbar drag would.
  app->Root()->QuerySelector(".log")->set_scroll_y(500);
  screen.Input("");
  CHECK(screen.Text() ==
        "500: line 500\n501: line 501\n502: line 502\n503: line 503\n"
        "504: line 504\n");
  CHECK(CountRows(app->Root()) <= 16);

  // The mouse wheel, three notches down: scrolling by less than the overscan
  // shows rows already rendered.
  screen.Input("\x1b[<65;3;3M\x1b[<65;3;3M\x1b[<65;3;3M");
  INFO(screen.Text());
  CHECK(screen.Text().starts_with("503: line 503\n"));

  // To the very end.
  app->Root()->QuerySelector(".log")->set_scroll_y(995);
  screen.Input("");
  CHECK(screen.Text().ends_with("999: line 999\n"));
}

TEST_CASE("Wheeling past the rendered items renders more",
          "[component][virtual]") {
  auto app = Ref<Log>::New();
  HeadlessScreen screen(app, 20, 5);
  std::string notches;
  for (int i = 0; i < 40; ++i) {
    notches += "\x1b[<65;3;3M";
  }
  screen.Input(notches);
  const int top = app->Root()->QuerySelector(".log")->scroll_y();
  CHECK(top > 15);  // Past the 5 + 5 rows rendered at first.
  CHECK(screen.Text().starts_with(std::to_string(top) + ": line " +
                                  std::to_string(top) + "\n"));
}

class Focusable : public Component<Focusable> {
 public:
  std::vector<std::string> lines;
  Focusable() {
    for (int i = 0; i < 100; ++i) {
      lines.push_back("line " + std::to_string(i));
    }
    Bind(lines);
  }
  std::string_view view = R"html(
    <div class="log">
      <for each="{lines}" as="line" virtual="">
        <div class="row" tabindex="0">{line}</div>
      </for>
    </div>
    <style>
      .log { height: 5; overflow-y: scroll; scrollbar-width: none; }
    </style>
  )html";
};

std::string FocusedText(Element* root) {
  std::string text;
  root->Visit([&](Element& element) {
    if (element.focused()) {
      element.Visit([&](Element& inner) {
        if (inner.is_text()) {
          text += static_cast<TextElement&>(inner).text();
        }
      });
    }
  });
  return text;
}

TEST_CASE("Focus stays on its item when a virtual loop's window moves",
          "[component][virtual]") {
  auto app = Ref<Focusable>::New();
  HeadlessScreen screen(app, 20, 5);
  screen.Input("\t\t\t\t");  // The fourth row.
  CHECK(FocusedText(app->Root()) == "line 3");
  // Seven notches down: the window moves, and the rows shift in the DOM.
  std::string notches;
  for (int i = 0; i < 7; ++i) {
    notches += "\x1b[<65;3;3M";
  }
  screen.Input(notches);
  CHECK(FocusedText(app->Root()) == "line 3");
}

class Tall : public Component<Tall> {
 public:
  std::vector<std::string> items{"a", "b", "c", "d", "e", "f", "g", "h"};
  Tall() { Bind(items); }
  std::string_view view = R"html(
    <div class="list">
      <for each="{items}" as="item" virtual="" item-height="2">
        <div class="card">{item}</div>
      </for>
    </div>
    <style>
      .list { height: 4; overflow-y: scroll; scrollbar-width: none; }
      .card { height: 2; }
    </style>
  )html";
};

TEST_CASE("item-height sets how tall each virtual item is",
          "[component][virtual]") {
  auto app = Ref<Tall>::New();
  HeadlessScreen screen(app, 10, 4);
  Element* list = app->Root()->QuerySelector(".list");
  REQUIRE(list != nullptr);
  CHECK(list->scroll_height() == 16);
  list->set_scroll_y(12);
  screen.Input("");
  CHECK(screen.Text() == "g\n\nh\n\n");
}

class Unscrolled : public Component<Unscrolled> {
 public:
  std::vector<std::string> items{"a", "b"};
  Unscrolled() { Bind(items); }
  std::string_view view = R"html(
    <div><for each="{items}" as="item" virtual=""><div>{item}</div></for></div>
  )html";
};

TEST_CASE("A virtual loop outside a scroll container is reported",
          "[component][virtual]") {
  std::vector<std::string> messages;
  SetDiagnosticHandler(
      [&](const Diagnostic& d) { messages.push_back(d.message); });
  CHECK(RenderToString(Ref<Unscrolled>::New(), 10, 3) == "a\nb\n\n");
  SetDiagnosticHandler(nullptr);
  REQUIRE_FALSE(messages.empty());
  CHECK(messages[0].find("not inside a scroll container") != std::string::npos);
}

}  // namespace
}  // namespace rtxui
