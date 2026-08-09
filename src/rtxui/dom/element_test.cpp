// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include "rtxui/dom/element.hpp"

#include <catch2/catch_test_macros.hpp>

#include <string>
#include <vector>

namespace rtxui {
namespace {

Ref<Element> MakeElement(std::string tag) {
  auto element = Ref<Element>::New();
  element->SetTag(std::move(tag));
  return element;
}

// Every tree edit has to leave parent pointers consistent: reconciliation walks
// upwards from an element (to bubble events, to find a scroll container), so a
// child still pointing at a parent that dropped it is a dangling read.
TEST_CASE("Element tracks parents across tree edits", "[dom][element]") {
  auto root = MakeElement("div");

  SECTION("AddChild adopts the child") {
    auto child = MakeElement("span");
    Element* raw = child.get();
    root->AddChild(child);

    REQUIRE(root->ChildCount() == 1);
    CHECK(root->ChildAt(0) == raw);
    CHECK(raw->Parent() == root.get());
  }

  SECTION("RemoveChildren disowns every child") {
    auto a = MakeElement("a");
    auto b = MakeElement("b");
    Element* raw_a = a.get();
    Element* raw_b = b.get();
    root->AddChild(a);
    root->AddChild(b);

    root->RemoveChildren();

    CHECK(root->ChildCount() == 0);
    CHECK(raw_a->Parent() == nullptr);
    CHECK(raw_b->Parent() == nullptr);
  }

  SECTION("ReplaceChild disowns the old child and adopts the new one") {
    auto old_child = MakeElement("old");
    auto new_child = MakeElement("new");
    Element* raw_old = old_child.get();
    Element* raw_new = new_child.get();
    root->AddChild(old_child);

    root->ReplaceChild(0, new_child);

    REQUIRE(root->ChildCount() == 1);
    CHECK(root->ChildAt(0) == raw_new);
    CHECK(raw_new->Parent() == root.get());
    CHECK(raw_old->Parent() == nullptr);
  }

  SECTION("TruncateChildren disowns only what it drops") {
    // Hold owning references: TruncateChildren resizes the child vector, which
    // drops the last reference to the removed elements. Keeping only raw
    // pointers here would read freed memory the moment they are truncated.
    std::vector<Ref<Element>> kept;
    for (int i = 0; i < 4; ++i) {
      auto child = MakeElement("c" + std::to_string(i));
      kept.push_back(child);
      root->AddChild(child);
    }

    root->TruncateChildren(2);

    REQUIRE(root->ChildCount() == 2);
    CHECK(kept[0]->Parent() == root.get());
    CHECK(kept[1]->Parent() == root.get());
    CHECK(kept[2]->Parent() == nullptr);
    CHECK(kept[3]->Parent() == nullptr);
  }

  SECTION("TruncateChildren beyond the size is a no-op") {
    root->AddChild(MakeElement("only"));
    root->TruncateChildren(5);
    CHECK(root->ChildCount() == 1);
  }
}

TEST_CASE("Element::MoveChild reorders without losing parents",
          "[dom][element]") {
  auto root = MakeElement("div");
  std::vector<Ref<Element>> kept;
  std::vector<Element*> raw;
  for (int i = 0; i < 4; ++i) {
    auto child = MakeElement("c" + std::to_string(i));
    kept.push_back(child);
    raw.push_back(child.get());
    root->AddChild(child);
  }

  SECTION("moving forwards") {
    root->MoveChild(0, 2);
    CHECK(root->ChildAt(0) == raw[1]);
    CHECK(root->ChildAt(1) == raw[2]);
    CHECK(root->ChildAt(2) == raw[0]);
    CHECK(root->ChildAt(3) == raw[3]);
  }

  SECTION("moving backwards") {
    root->MoveChild(3, 1);
    CHECK(root->ChildAt(0) == raw[0]);
    CHECK(root->ChildAt(1) == raw[3]);
    CHECK(root->ChildAt(2) == raw[1]);
    CHECK(root->ChildAt(3) == raw[2]);
  }

  SECTION("moving onto itself changes nothing") {
    root->MoveChild(2, 2);
    for (size_t i = 0; i < raw.size(); ++i) {
      CHECK(root->ChildAt(i) == raw[i]);
    }
  }

  SECTION("every child keeps its parent") {
    root->MoveChild(0, 3);
    CHECK(root->ChildCount() == 4);
    for (Element* child : raw) {
      CHECK(child->Parent() == root.get());
    }
  }
}

TEST_CASE("Element::Visit walks the whole subtree in document order",
          "[dom][element]") {
  auto root = MakeElement("root");
  auto a = MakeElement("a");
  auto b = MakeElement("b");
  auto a1 = MakeElement("a1");
  a->AddChild(a1);
  root->AddChild(a);
  root->AddChild(b);

  std::vector<std::string> seen;
  root->Visit([&seen](Element& element) {
    seen.emplace_back(element.tag());
  });

  CHECK(seen == std::vector<std::string>{"root", "a", "a1", "b"});
}

TEST_CASE("Element::QuerySelector resolves by id, class and tag",
          "[dom][element]") {
  auto root = MakeElement("root");
  auto child = MakeElement("span");
  child->id = "target";
  child->classes = {"alpha", "beta"};
  auto grandchild = MakeElement("em");
  child->AddChild(grandchild);
  root->AddChild(child);

  SECTION("by id") {
    CHECK(root->QuerySelector("#target") == child.get());
  }
  SECTION("by class, including a non-first class") {
    CHECK(root->QuerySelector(".alpha") == child.get());
    CHECK(root->QuerySelector(".beta") == child.get());
  }
  SECTION("by tag, at any depth") {
    CHECK(root->QuerySelector("em") == grandchild.get());
  }
  SECTION("the element can match itself") {
    CHECK(root->QuerySelector("root") == root.get());
  }
  SECTION("no match yields nullptr") {
    CHECK(root->QuerySelector("#absent") == nullptr);
    CHECK(root->QuerySelector(".absent") == nullptr);
    CHECK(root->QuerySelector("absent") == nullptr);
  }
  SECTION("an empty selector matches nothing") {
    CHECK(root->QuerySelector("") == nullptr);
  }
}

TEST_CASE("Element::QuerySelector returns the first match in document order",
          "[dom][element]") {
  auto root = MakeElement("root");
  auto first = MakeElement("span");
  auto second = MakeElement("span");
  root->AddChild(first);
  root->AddChild(second);

  CHECK(root->QuerySelector("span") == first.get());
}

TEST_CASE("Element attributes can be set, overwritten and removed",
          "[dom][element]") {
  auto element = MakeElement("div");

  element->SetAttribute("value", "one");
  REQUIRE(element->Attributes().count("value") == 1);
  CHECK(element->Attributes().at("value") == "one");

  element->SetAttribute("value", "two");
  CHECK(element->Attributes().at("value") == "two");
  CHECK(element->Attributes().size() == 1);

  element->SetAttribute("other", "x");
  element->RemoveAttribute("value");
  CHECK(element->Attributes().count("value") == 0);
  CHECK(element->Attributes().count("other") == 1);

  element->ClearAttributes();
  CHECK(element->Attributes().empty());

  // Removing something absent is harmless.
  element->RemoveAttribute("never-there");
  CHECK(element->Attributes().empty());
}

TEST_CASE("Element scroll offsets follow the documented clamping contract",
          "[dom][element]") {
  auto element = MakeElement("div");

  SECTION("the setter stores what it is given, including out-of-range values") {
    // Deliberate: set_scroll_* is the low-level setter and only ClampScrollY
    // enforces the upper bound. The lower bound is the caller's job -- every
    // call site in screen.cpp wraps the value in std::max(0, ...). Worth
    // knowing before relying on the setter to sanitise anything.
    element->set_scroll_y(-50);
    CHECK(element->scroll_y() == -50);
    element->set_scroll_x(-50);
    CHECK(element->scroll_x() == -50);
  }

  SECTION("a plain set lands exactly on the requested offset") {
    element->set_scroll_height(100);
    element->set_scroll_y(25);
    CHECK(element->scroll_y() == 25);
    CHECK(element->target_scroll_y() == 25);
  }

  SECTION("ClampScrollY pulls an out-of-range offset back") {
    element->set_scroll_height(100);
    element->set_scroll_y(80);
    CHECK(element->scroll_y() == 80);

    // The content shrank: anything past the new maximum has to come back.
    element->ClampScrollY(30);
    CHECK(element->scroll_y() <= 30);
  }
}

TEST_CASE("Element interaction flags default to false", "[dom][element]") {
  auto element = MakeElement("div");
  CHECK_FALSE(element->focused());
  CHECK_FALSE(element->disabled());

  element->set_focused(true);
  element->set_disabled(true);
  CHECK(element->focused());
  CHECK(element->disabled());
}

}  // namespace
}  // namespace rtxui
