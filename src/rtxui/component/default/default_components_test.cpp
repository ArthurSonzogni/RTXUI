// Copyright 2024 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include <catch2/catch_test_macros.hpp>
#include <cctype>
#include <memory>
#include <string>
#include <string_view>
#include <type_traits>

#include "catch2/catch_message.hpp"
#include "rtxui/color.hpp"
#include "rtxui/component.hpp"
#include "rtxui/component/component_internal.hpp"
#include "rtxui/component/default/a/a.hpp"
#include "rtxui/component/default/b/b.hpp"
#include "rtxui/component/default/code/code.hpp"
#include "rtxui/component/default/div/div.hpp"
#include "rtxui/component/default/fieldset/fieldset.hpp"
#include "rtxui/component/default/h1/h1.hpp"
#include "rtxui/component/default/hr/hr.hpp"
#include "rtxui/component/default/kbd/kbd.hpp"
#include "rtxui/component/default/progress/progress.hpp"
#include "rtxui/component/default/radio/radio.hpp"
#include "rtxui/component/default/s/s.hpp"
#include "rtxui/component/default/strong/strong.hpp"
#include "rtxui/component/default/u/u.hpp"
#include "rtxui/dom/element.hpp"
#include "rtxui/event.hpp"
#include "rtxui/layout/style.hpp"
#include "rtxui/paint/texture.hpp"
#include "rtxui/refcounted.hpp"
#include "rtxui/screen.hpp"
#include "rtxui/terminal/screen_internal.hpp"
#include "rtxui/terminal/terminal_device.hpp"
#include "rtxui/testing/component_as.hpp"
#include "rtxui/testing/render.hpp"

using namespace rtxui;

namespace {

std::string RemoveWhitespace(std::string_view str) {
  std::string result;
  for (char c : str) {
    if (!std::isspace(static_cast<unsigned char>(c))) {
      result += c;
    }
  }
  return result;
}

class SemanticTestComponent : public rtxui::Component<SemanticTestComponent> {
 public:
  std::string_view view = R"(
    <header><span>Head</span></header>
    <nav><span>Nav</span></nav>
    <main>
      <section>
        <article>
          <aside><span>Aside</span></aside>
          <span>Article content</span>
          <kbd>Ctrl+C</kbd>
        </article>
      </section>
    </main>
    <footer><span>Foot</span></footer>
  )";
};

TEST_CASE("Semantic Tags and Kbd Component Rendering",
          "[component][semantic][kbd]") {
  auto container = rtxui::Ref<SemanticTestComponent>::New();
  container->Mount();

  CHECK(container->Root()->QuerySelector("header") != nullptr);
  CHECK(container->Root()->QuerySelector("footer") != nullptr);
  CHECK(container->Root()->QuerySelector("main") != nullptr);
  CHECK(container->Root()->QuerySelector("nav") != nullptr);
  CHECK(container->Root()->QuerySelector("aside") != nullptr);
  CHECK(container->Root()->QuerySelector("section") != nullptr);
  CHECK(container->Root()->QuerySelector("article") != nullptr);
  auto* kbd_el = container->Root()->QuerySelector("kbd");
  REQUIRE(kbd_el != nullptr);
  auto* kbd_comp = dynamic_cast<const rtxui::kbd*>(kbd_el->component());
  REQUIRE(kbd_comp != nullptr);
}

class ProgressTestComponent : public rtxui::Component<ProgressTestComponent> {
 public:
  double my_progress = 25.0;

  ProgressTestComponent() { Bind(my_progress); }
  std::string_view view = R"(
    <progress value="{my_progress}" max="100" width="10" />
  )";
};

TEST_CASE("Progress Component Basic Rendering", "[component][progress]") {
  auto container = rtxui::Ref<ProgressTestComponent>::New();
  container->Mount();
  container->Digest();

  auto* progress_el = container->Root()->QuerySelector("progress");
  REQUIRE(progress_el != nullptr);
  auto* progress_ptr = ComponentAs<rtxui::progress>(progress_el);
  REQUIRE(progress_ptr != nullptr);

  // Initial state (value = 25.0) -> width is 10, so 2.5 rounded to 3 characters
  // filled.
  CHECK(progress_ptr->value == 25.0);
  CHECK(progress_ptr->filled_track == "███");
  CHECK(progress_ptr->empty_track == "       ");  // 7 spaces

  // Update value
  container->my_progress = 70.0;
  container->Digest();

  // Value = 70.0 -> 7 characters filled.
  CHECK(progress_ptr->value == 70.0);
  CHECK(progress_ptr->filled_track == "███████");
  CHECK(progress_ptr->empty_track == "   ");  // 3 spaces
}

TEST_CASE(
    "Progress Component Exposes Filled/Empty Part Attributes For "
    "External ::part() Styling",
    "[component][progress][part]") {
  auto container = rtxui::Ref<ProgressTestComponent>::New();
  container->Mount();
  container->Digest();

  auto* progress_el = container->Root()->QuerySelector("progress");
  REQUIRE(progress_el != nullptr);

  auto* filled_el = progress_el->QuerySelector(".filled");
  REQUIRE(filled_el != nullptr);
  const std::string* filled_part = filled_el->GetAttribute("part");
  REQUIRE(filled_part != nullptr);
  CHECK(*filled_part == "filled");

  auto* empty_el = progress_el->QuerySelector(".empty");
  REQUIRE(empty_el != nullptr);
  const std::string* empty_part = empty_el->GetAttribute("part");
  REQUIRE(empty_part != nullptr);
  CHECK(*empty_part == "empty");
}

class HrTestComponent : public rtxui::Component<HrTestComponent> {
 public:
  std::string_view view = R"(
    <hr />
  )";
};

TEST_CASE("Horizontal Rule Component", "[component][hr]") {
  auto container = rtxui::Ref<HrTestComponent>::New();
  rtxui::Screen screen(container);
  screen.Draw();

  auto* hr_el = container->Root()->QuerySelector("hr");
  REQUIRE(hr_el != nullptr);
  auto* hr_ptr = ComponentAs<rtxui::hr>(hr_el);
  REQUIRE(hr_ptr != nullptr);

  container->Digest();
  screen.Draw();

  CHECK(hr_ptr->line_chars.size() > 0);
  CHECK(hr_ptr->line_chars.substr(0, 3) == "─");
}

TEST_CASE("Horizontal Rule paints on the first frame", "[component][hr]") {
  // Regression: hr computes its length from the laid-out width, so the first
  // digest runs before any layout. It used to return early on that change
  // without running the base digest, so the rule reached the DOM only on some
  // later digest and the first frame painted nothing at all.
  auto device = std::make_shared<rtxui::MockTerminalDevice>();
  device->TriggerResize(20, 3);

  auto container = rtxui::Ref<HrTestComponent>::New();
  rtxui::Screen screen = rtxui::ScreenInternals::Make(container, device);

  // The Screen constructor mounts, digests and draws once: that is frame one.
  std::string output = device->GetOutput();
  CHECK(output.find("─") != std::string::npos);
}

class BoldTestComponent : public rtxui::Component<BoldTestComponent> {
 public:
  std::string_view view = R"(
    <div>
      <b>Bold text</b>
      <strong>Strong text</strong>
    </div>
  )";
};

TEST_CASE("Bold and Strong Components", "[component][b][strong]") {
  auto container = rtxui::Ref<BoldTestComponent>::New();
  rtxui::Screen screen(container);
  screen.Draw();

  auto* b_el = container->Root()->QuerySelector("b");
  REQUIRE(b_el != nullptr);
  auto* b_ptr = ComponentAs<rtxui::b>(b_el);
  REQUIRE(b_ptr != nullptr);

  auto* strong_el = container->Root()->QuerySelector("strong");
  REQUIRE(strong_el != nullptr);
  auto* strong_ptr = ComponentAs<rtxui::strong>(strong_el);
  REQUIRE(strong_ptr != nullptr);
}

TEST_CASE("Default Components Registration", "[component]") {
  // Every built-in tag, including those that were once registered only by
  // a static initializer in their own .cpp file.
  for (std::string_view tag : {
           "a",        "article",  "aside",  "b",       "blockquote", "button",
           "checkbox", "code",     "del",    "details", "dialog",     "div",
           "em",       "fieldset", "footer", "h1",      "h2",         "h3",
           "h4",       "h5",       "h6",     "header",  "hr",         "i",
           "input",    "kbd",      "label",  "legend",  "li",         "main",
           "markdown", "nav",      "ol",     "option",  "p",          "pre",
           "progress", "radio",    "s",      "section", "select",     "slider",
           "span",     "strike",   "strong", "summary", "tab-pane",   "tabs",
           "textarea", "tooltip",  "u",      "ul",
       }) {
    CAPTURE(tag);
    CHECK(rtxui::GetGlobalComponentFactory(tag) != nullptr);
  }
}

class ListTestComponent : public rtxui::Component<ListTestComponent> {
 public:
  std::string_view view = R"(
      <div>
        <ul id="ul_disc">
          <li>item 1</li>
          <li>item 2</li>
        </ul>
        <ul id="ul_nested">
          <li>outer 1
            <ul>
              <li>inner 1</li>
              <li>inner 2</li>
            </ul>
          </li>
        </ul>
        <ol id="ol_decimal">
          <li>first</li>
          <li>second</li>
        </ol>
        <ol id="ol_start" start="5">
          <li>fifth</li>
          <li>sixth</li>
        </ol>
        <ol id="ol_value">
          <li>first</li>
          <li value="10">tenth</li>
          <li>eleventh</li>
        </ol>
        <ol id="ol_reversed" reversed="">
          <li>third</li>
          <li>second</li>
          <li>first</li>
        </ol>
        <ul id="ul_custom_square">
          <li>square item</li>
        </ul>
        <ul id="ul_custom_none">
          <li>none item</li>
        </ul>
      </div>

      <style>
        #ul_custom_square {
          list-style-type: square;
        }
        #ul_custom_none {
          list-style-type: none;
        }
      </style>
    )";
};

TEST_CASE("List Rendering - ul, ol, li, and CSS", "[component][list]") {
  auto comp = rtxui::Ref<ListTestComponent>::New();
  comp->Mount();
  comp->Digest();

  auto* root = comp->Root();
  REQUIRE(root != nullptr);

  // 1. Unordered list with default bullet (disc -> "• ")
  auto* ul_disc = root->QuerySelector("#ul_disc");
  REQUIRE(ul_disc != nullptr);
  auto print_disc = ul_disc->Print();
  CHECK(RemoveWhitespace(print_disc) ==
        "<ulid=\"ul_disc\"><li><span>•</span>item1</li><li><span>•</"
        "span>item2</li></ul>");

  // 2. Nested unordered list (outer disc -> "• ", inner circle -> "○ ")
  auto* ul_nested = root->QuerySelector("#ul_nested");
  REQUIRE(ul_nested != nullptr);
  auto print_nested = ul_nested->Print();
  CHECK(RemoveWhitespace(print_nested) ==
        "<ulid=\"ul_nested\"><li><span>•</span>outer1<ul><li><span>○</"
        "span>inner1</li><li><span>○</span>inner2</li></ul></li></ul>");

  // 3. Ordered list with decimal numbering ("1. ", "2. ")
  auto* ol_decimal = root->QuerySelector("#ol_decimal");
  REQUIRE(ol_decimal != nullptr);
  auto print_decimal = ol_decimal->Print();
  CHECK(RemoveWhitespace(print_decimal) ==
        "<olid=\"ol_decimal\"><li><span>1.</span>first</li><li><span>2.</"
        "span>second</li></ol>");

  // 3b. Ordered list honors the HTML start="" attribute ("5. ", "6. ")
  auto* ol_start = root->QuerySelector("#ol_start");
  REQUIRE(ol_start != nullptr);
  auto print_start = ol_start->Print();
  CHECK(RemoveWhitespace(print_start) ==
        "<olid=\"ol_start\"start=\"5\"><li><span>5.</span>fifth</li><li><span>"
        "6.</span>sixth</li></ol>");

  // 3c. A <li value=""> override also shifts every later item's number
  // ("1. ", "10. ", "11. ")
  auto* ol_value = root->QuerySelector("#ol_value");
  REQUIRE(ol_value != nullptr);
  auto print_value = ol_value->Print();
  CHECK(RemoveWhitespace(print_value) ==
        "<olid=\"ol_value\"><li><span>1.</span>first</li><livalue=\"10\">"
        "<span>10.</span>tenth</li><li><span>11.</span>eleventh</li></ol>");

  // 3d. The HTML boolean reversed="" attribute counts down from the item
  // count (3 items -> "3. ", "2. ", "1. ")
  auto* ol_reversed = root->QuerySelector("#ol_reversed");
  REQUIRE(ol_reversed != nullptr);
  auto print_reversed = ol_reversed->Print();
  CHECK(RemoveWhitespace(print_reversed) ==
        "<olid=\"ol_reversed\"reversed=\"\"><li><span>3.</span>third</li><li>"
        "<span>2.</span>second</li><li><span>1.</span>first</li></ol>");

  // 4. Custom list-style-type: square ("■ ")
  auto* ul_custom_square = root->QuerySelector("#ul_custom_square");
  REQUIRE(ul_custom_square != nullptr);
  auto print_square = ul_custom_square->Print();
  CHECK(RemoveWhitespace(print_square) ==
        "<ulid=\"ul_custom_square\"><li><span>■</span>squareitem</li></ul>");

  // 5. Custom list-style-type: none ("")
  auto* ul_custom_none = root->QuerySelector("#ul_custom_none");
  REQUIRE(ul_custom_none != nullptr);
  auto print_none = ul_custom_none->Print();
  CHECK(RemoveWhitespace(print_none) ==
        "<ulid=\"ul_custom_none\"><li><span></span>noneitem</li></ul>");
}

TEST_CASE("Heading Tags Rendering h1-h6", "[component]") {
  class HeadingsTestComponent : public rtxui::Component<HeadingsTestComponent> {
   public:
    std::string_view view = R"html(
      <div>
        <h1 id="heading1">H1</h1>
        <h2 id="heading2">H2</h2>
        <h3 id="heading3">H3</h3>
        <h4 id="heading4">H4</h4>
        <h5 id="heading5">H5</h5>
        <h6 id="heading6">H6</h6>
      </div>
    )html";

    HeadingsTestComponent() {
      Import<rtxui::div>();
      Import<rtxui::h1>();
      Import<rtxui::h2>();
      Import<rtxui::h3>();
      Import<rtxui::h4>();
      Import<rtxui::h5>();
      Import<rtxui::h6>();
    }
  };

  auto app = rtxui::Ref<HeadingsTestComponent>::New();
  app->Mount();
  app->Digest();

  auto* root = app->Root();
  REQUIRE(root != nullptr);

  auto* h1_el = root->QuerySelector("#heading1");
  auto* h2_el = root->QuerySelector("#heading2");
  auto* h3_el = root->QuerySelector("#heading3");
  auto* h4_el = root->QuerySelector("#heading4");
  auto* h5_el = root->QuerySelector("#heading5");
  auto* h6_el = root->QuerySelector("#heading6");

  REQUIRE(h1_el != nullptr);
  REQUIRE(h2_el != nullptr);
  REQUIRE(h3_el != nullptr);
  REQUIRE(h4_el != nullptr);
  REQUIRE(h5_el != nullptr);
  REQUIRE(h6_el != nullptr);

  // Check that default style block display is active
  CHECK(h1_el->style.display_outside == rtxui::DisplayOutside::Block);
  CHECK(h2_el->style.display_outside == rtxui::DisplayOutside::Block);
  CHECK(h3_el->style.display_outside == rtxui::DisplayOutside::Block);
  CHECK(h4_el->style.display_outside == rtxui::DisplayOutside::Block);
  CHECK(h5_el->style.display_outside == rtxui::DisplayOutside::Block);
  CHECK(h6_el->style.display_outside == rtxui::DisplayOutside::Block);
}

struct FormattingTestComponent
    : public rtxui::Component<FormattingTestComponent> {
  std::string_view view = R"(
    <div>
      <u>Underlined</u>
      <s>Strikethrough</s>
      <strike>Strike</strike>
      <del>Del</del>
      <code>inline code</code>
    </div>
  )";
};

}  // namespace

TEST_CASE("Underline, Strikethrough, and Code Components",
          "[component][u][s][code]") {
  auto container = rtxui::Ref<FormattingTestComponent>::New();
  rtxui::Screen screen(container);
  screen.Draw();

  auto* u_el = container->Root()->QuerySelector("u");
  REQUIRE(u_el != nullptr);
  auto* u_ptr = ComponentAs<rtxui::u>(u_el);
  REQUIRE(u_ptr != nullptr);

  auto* s_el = container->Root()->QuerySelector("s");
  REQUIRE(s_el != nullptr);
  auto* s_ptr = ComponentAs<rtxui::s>(s_el);
  REQUIRE(s_ptr != nullptr);

  auto* strike_el = container->Root()->QuerySelector("strike");
  REQUIRE(strike_el != nullptr);
  auto* strike_ptr = ComponentAs<rtxui::strike>(strike_el);
  REQUIRE(strike_ptr != nullptr);

  auto* del_el = container->Root()->QuerySelector("del");
  REQUIRE(del_el != nullptr);
  auto* del_ptr = ComponentAs<rtxui::del>(del_el);
  REQUIRE(del_ptr != nullptr);

  auto* code_el = container->Root()->QuerySelector("code");
  REQUIRE(code_el != nullptr);
  auto* code_ptr = ComponentAs<rtxui::code>(code_el);
  REQUIRE(code_ptr != nullptr);
}

namespace {

// Regression test: <a> was completely unregistered - it fell through to a
// plain, unstyled Element. This was already silently affecting real output:
// <markdown> generates <a href="..."> for markdown links (markdown.cpp), and
// screen.cpp already has dedicated click handling for <a href="#id"> anchor
// navigation (Screen.AnchorLinkScrolling) - both rendered with zero visual
// indication of being a link. <a> now gets the same default styling as
// every other simple inline tag here (underline, matching u/s/code above),
// plus a link-colored foreground.
struct AnchorTestComponent : public rtxui::Component<AnchorTestComponent> {
  std::string_view view = R"(
    <a href="https://example.com">Link</a>
  )";
};

}  // namespace

TEST_CASE("Anchor component registration and default styling",
          "[component][a]") {
  auto container = rtxui::Ref<AnchorTestComponent>::New();
  container->Mount();

  auto* a_el = container->Root()->QuerySelector("a");
  REQUIRE(a_el != nullptr);
  auto* a_ptr = ComponentAs<rtxui::a>(a_el);
  REQUIRE(a_ptr != nullptr);

  // href must still land on the underlying Element's generic attribute map -
  // that's what screen.cpp's anchor-click handler reads.
  const std::string* href = a_el->GetAttribute("href");
  REQUIRE(href != nullptr);
  CHECK(*href == "https://example.com");

  auto texture = PaintComponent(*container, 80, 24);

  bool found = false;
  for (int y = 0; y < texture.height(); ++y) {
    for (int x = 0; x < texture.width(); ++x) {
      const auto& cell = texture[x, y];
      if (cell.character == "L") {
        CHECK(cell.underlined);
        CHECK(cell.foreground_color == Color::RGB(0x3b, 0x82, 0xf6));
        found = true;
      }
    }
  }
  CHECK(found);
}

namespace {

// --- Fieldset & Legend ---
class FieldsetTestComponent : public rtxui::Component<FieldsetTestComponent> {
 public:
  std::string_view view = R"(
    <fieldset>
      <legend>User Profile</legend>
      <p>Fieldset content</p>
    </fieldset>
  )";
};

}  // namespace

TEST_CASE("Fieldset and Legend Components", "[component][fieldset]") {
  auto container = rtxui::Ref<FieldsetTestComponent>::New();
  rtxui::Screen screen(container);
  screen.Draw();

  auto* fieldset_el = container->Root()->QuerySelector("fieldset");
  REQUIRE(fieldset_el != nullptr);
  auto* fieldset_ptr = ComponentAs<rtxui::fieldset>(fieldset_el);
  REQUIRE(fieldset_ptr != nullptr);

  // The legend slot should contain a legend component
  auto legend_slot = rtxui::ComponentInternals::Slot(*fieldset_ptr, "legend");
  REQUIRE(legend_slot != nullptr);
  REQUIRE(legend_slot->ChildCount() > 0);

  auto* legend_el = legend_slot->ChildAt(0);
  CHECK(legend_el->tag() == "legend");
  CHECK(fieldset_ptr->legend_class == "has-legend");
}

TEST_CASE(
    "Fieldset Component Exposes Part Attributes For External "
    "::part() Styling",
    "[component][fieldset][part]") {
  auto container = rtxui::Ref<FieldsetTestComponent>::New();
  rtxui::Screen screen(container);
  screen.Draw();

  auto* fieldset_el = container->Root()->QuerySelector("fieldset");
  REQUIRE(fieldset_el != nullptr);

  auto* wrapper_el = fieldset_el->QuerySelector(".fieldset-wrapper");
  REQUIRE(wrapper_el != nullptr);
  const std::string* wrapper_part = wrapper_el->GetAttribute("part");
  REQUIRE(wrapper_part != nullptr);
  CHECK(*wrapper_part == "fieldset-wrapper");

  auto* legend_line_el = fieldset_el->QuerySelector(".legend-line");
  REQUIRE(legend_line_el != nullptr);
  const std::string* legend_line_part = legend_line_el->GetAttribute("part");
  REQUIRE(legend_line_part != nullptr);
  CHECK(*legend_line_part == "legend-line");

  auto* body_el = fieldset_el->QuerySelector(".fieldset-body");
  REQUIRE(body_el != nullptr);
  const std::string* body_part = body_el->GetAttribute("part");
  REQUIRE(body_part != nullptr);
  CHECK(*body_part == "fieldset-body");
}

namespace {

// --- Radio ---
class RadioTestComponent : public rtxui::Component<RadioTestComponent> {
 public:
  bool opt_a = false;
  bool opt_b = false;

  RadioTestComponent() {
    Bind(opt_a);
    Bind(opt_b);
  }
  std::string_view view = R"(
    <div>
      <radio id="radioA" name="my_group" checked="{opt_a}">Option A</radio>
      <radio id="radioB" name="my_group" checked="{opt_b}">Option B</radio>
    </div>
  )";
};

}  // namespace

TEST_CASE("Radio Component Grouping", "[component][radio]") {
  auto container = rtxui::Ref<RadioTestComponent>::New();
  rtxui::Screen screen(container);
  screen.Draw();

  auto* radioA_el = container->Root()->QuerySelector("#radioA");
  auto* radioB_el = container->Root()->QuerySelector("#radioB");
  REQUIRE(radioA_el != nullptr);
  REQUIRE(radioB_el != nullptr);

  auto* radioA_ptr = ComponentAs<rtxui::radio>(radioA_el);
  auto* radioB_ptr = ComponentAs<rtxui::radio>(radioB_el);
  REQUIRE(radioA_ptr != nullptr);
  REQUIRE(radioB_ptr != nullptr);

  // Initial State: neither is checked
  CHECK(radioA_ptr->checked == false);
  CHECK(radioB_ptr->checked == false);

  // Click on Radio A
  Event clickA = Event::Keyboard::From(' ');
  radioA_el->set_focused(true);
  CHECK(radioA_ptr->OnEvent(clickA) == true);
  container->Digest();
  screen.Draw();

  CHECK(radioA_ptr->checked == true);
  CHECK(radioB_ptr->checked == false);
  CHECK(container->opt_a == true);
  CHECK(container->opt_b == false);

  // Click on Radio B: A should automatically uncheck!
  radioA_el->set_focused(false);
  radioB_el->set_focused(true);
  Event clickB = Event::Keyboard::From(' ');
  CHECK(radioB_ptr->OnEvent(clickB) == true);
  container->Digest();
  screen.Draw();

  CHECK(radioA_ptr->checked == false);
  CHECK(radioB_ptr->checked == true);
  CHECK(container->opt_a == false);
  CHECK(container->opt_b == true);
}

TEST_CASE(
    "Radio Component Exposes Radio-Mark Part Attribute For External "
    "::part() Styling",
    "[component][radio][part]") {
  auto container = rtxui::Ref<RadioTestComponent>::New();
  rtxui::Screen screen(container);
  screen.Draw();

  auto* radioA_el = container->Root()->QuerySelector("#radioA");
  REQUIRE(radioA_el != nullptr);

  auto* mark_el = radioA_el->QuerySelector(".radio-mark");
  REQUIRE(mark_el != nullptr);
  const std::string* mark_part = mark_el->GetAttribute("part");
  REQUIRE(mark_part != nullptr);
  CHECK(*mark_part == "radio-mark");
}
