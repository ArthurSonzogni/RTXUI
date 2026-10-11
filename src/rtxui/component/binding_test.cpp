// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
//
// What bound members of each supported type look like on screen, and what a
// two-way binding writes back into them. The string and int cases are covered
// throughout the suite; these are the types only apps used.
#include <string>
#include <string_view>
#include <vector>

#include "catch2/catch_test_macros.hpp"
#include "rtxui/component.hpp"
#include "rtxui/component/default/div/div.hpp"
#include "rtxui/headless.hpp"
#include "rtxui/refcounted.hpp"

namespace rtxui {
namespace {

class Numbers : public Component<Numbers> {
 public:
  double price = 2.5;
  float ratio = 0.1f;
  std::vector<int> scores{3, 14, 15};
  Numbers() {
    Bind(price);
    Bind(ratio);
    Bind(scores);
  }
  std::string_view view = R"html(
    <div>
      <div>price {price}</div>
      <div>ratio {ratio}</div>
      <for each="{scores}" as="s"><div>score {s} at {$index}</div></for>
    </div>
  )html";
};

TEST_CASE("Floating-point and non-string collections render", "[binding]") {
  // The shortest text that reads back as the same value: 0.1f is "0.1", not
  // the "0.100000" std::to_string would give.
  std::string text = RenderToString(Ref<Numbers>::New(), 30, 6);
  CHECK(text.find("price 2.5\n") != std::string::npos);
  CHECK(text.find("ratio 0.1\n") != std::string::npos);
  CHECK(text.find("score 3 at 0\n") != std::string::npos);
  CHECK(text.find("score 15 at 2\n") != std::string::npos);
}

class NumberInputs : public Component<NumberInputs> {
 public:
  int count = 0;
  double amount = 0.0;
  NumberInputs() {
    Bind(count);
    Bind(amount);
  }
  std::string_view view = R"html(
    <div>
      <input value="{count}"/>
      <input value="{amount}"/>
    </div>
  )html";
};

TEST_CASE("Two-way binding parses into numeric members", "[binding]") {
  auto app = Ref<NumberInputs>::New();
  HeadlessScreen screen(app, 20, 4);

  SECTION("an int") {
    screen.Input("\t\x1b[F\x7f");  // Focus, End, delete the "0".
    screen.Input("42");
    CHECK(app->count == 42);
  }

  SECTION("a double") {
    screen.Input("\t\t\x1b[F\x7f");
    screen.Input("1.25");
    CHECK(app->amount == 1.25);
  }

  SECTION("a half-typed number keeps its text") {
    // "1." stores 1, which renders as "1". The input must keep showing what
    // was typed, or the dot is lost and "1.25" becomes 125.
    screen.Input("\t\t\x1b[F\x7f");
    screen.Input("1.");
    CHECK(app->amount == 1.0);
    CHECK(screen.Text().find("1.") != std::string::npos);
  }

  SECTION("a value set by the app still replaces the text") {
    screen.Input("\t\t\x1b[F\x7f");
    screen.Input("1.");
    app->amount = 7.5;
    screen.Input("");  // Let the change render.
    CHECK(screen.Text().find("7.5") != std::string::npos);
  }

  SECTION("text that is not a number reads as zero") {
    screen.Input("\t\t\x1b[F\x7f");
    screen.Input("abc");
    CHECK(app->amount == 0.0);
  }
}

struct Row {
  std::string name;
  int stock;
  // Bound collections are snapshotted and compared to detect changes.
  bool operator==(const Row&) const = default;
};

class Inventory : public Component<Inventory> {
 public:
  std::vector<Row> rows{{"bolts", 12}, {"nuts", 0}};
  Inventory() {
    BindCollection("rows", &rows, [](const Row& row, FieldSink& out) {
      out("name", row.name);
      out("stock", row.stock ? std::to_string(row.stock) : "out");
    });
    Import<div>();
  }
  std::string_view view = R"html(
    <div>
      <for each="{rows}" as="row"><div>{row.name}: {row.stock}{row.missing}</div></for>
    </div>
  )html";
};

TEST_CASE("A struct collection reads fields through its mapper", "[binding]") {
  // A field the mapper does not provide reads as empty.
  std::string text = RenderToString(Ref<Inventory>::New(), 20, 3);
  CHECK(text.find("bolts: 12\n") != std::string::npos);
  CHECK(text.find("nuts: out\n") != std::string::npos);
}

// Described once for the type, so binding a collection of it needs nothing
// more. No operator==: changes are found by comparing the fields.
struct Book {
  std::string title;
  int pages;
};

void Fields(const Book& book, FieldSink& out) {
  out("title", book.title);
  out("pages", book.pages);
  out("length", book.pages > 300 ? "long" : "short");
}

class Shelf : public Component<Shelf> {
 public:
  std::vector<Book> books{{"Dune", 412}, {"Ubik", 202}};
  Shelf() { Bind(books); }
  std::string_view view = R"html(
    <div>
      <for each="{books}" as="book"><div>{book.title} {book.pages} {book.length}</div></for>
    </div>
  )html";
};

TEST_CASE("A collection binds alone when its type describes its fields",
          "[binding]") {
  std::string text = RenderToString(Ref<Shelf>::New(), 30, 3);
  CHECK(text.find("Dune 412 long\n") != std::string::npos);
  CHECK(text.find("Ubik 202 short\n") != std::string::npos);
}

TEST_CASE("An item without operator== is seen to change", "[binding]") {
  auto app = Ref<Shelf>::New();
  HeadlessScreen screen(app, 30, 3);
  CHECK(screen.Text().find("Ubik 202 short") != std::string::npos);

  app->books[1].pages = 900;
  screen.Input("");  // Let the change render.
  CHECK(screen.Text().find("Ubik 900 long") != std::string::npos);
}

class ShelfByAuthor : public Component<ShelfByAuthor> {
 public:
  std::vector<Book> books{{"Dune", 412}};
  std::string unit = "p.";
  ShelfByAuthor() { Bind(books, &ShelfByAuthor::BookFields); }
  // The binding's own fields replace the type's.
  void BookFields(const Book& book, FieldSink& out) const {
    out("title", book.title);
    out("pages", std::to_string(book.pages) + " " + unit);
  }
  std::string_view view = R"html(
    <div>
      <for each="{books}" as="book"><div>{book.title}, {book.pages}{book.length}</div></for>
    </div>
  )html";
};

TEST_CASE("A binding's fields take precedence over the type's", "[binding]") {
  std::string text = RenderToString(Ref<ShelfByAuthor>::New(), 30, 3);
  CHECK(text.find("Dune, 412 p.\n") != std::string::npos);
}

class Tally : public Component<Tally> {
 public:
  std::vector<Row> rows{{"bolts", 12}};
  Tally() {
    Bind(rows, [](const Row& row, FieldSink& out) {
      out("name", row.name);
      out("stock", row.stock);
    });
  }
  std::string_view view = R"html(
    <div>
      <for each="{rows}" as="row"><div>{row.name}={row.stock}</div></for>
    </div>
  )html";
};

TEST_CASE("A collection's fields can come from a lambda", "[binding]") {
  std::string text = RenderToString(Ref<Tally>::New(), 30, 3);
  CHECK(text.find("bolts=12\n") != std::string::npos);
}

}  // namespace
}  // namespace rtxui
