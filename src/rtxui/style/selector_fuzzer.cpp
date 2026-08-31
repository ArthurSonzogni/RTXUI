// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
//
// Fuzzes selector *matching*, not just parsing. Style.TestStyle stops after
// css::Parse and css::Print, so everything downstream of a parsed selector --
// the compound/pseudo split, the structural pseudo-classes, the An+B
// arithmetic, and the mutual recursion :not() sets up between the compound
// matcher and the pseudo-class matcher -- had no direct cover. This target
// pins the selector to the fuzzer's input and holds the DOM fixed, so a
// generated string spends its whole budget on the matcher rather than on
// producing a tree.
#include <fuzztest/fuzztest.h>
#include <gtest/gtest.h>

#include <string>
#include <string_view>
#include <tuple>
#include <vector>

#include "rtxui/internal/component.hpp"
#include "rtxui/internal/refcounted.hpp"
#include "rtxui/style/style.hpp"
#include "rtxui/xml/xml.hpp"

namespace {

// GetView() returns the `view` member when there is one, so pointing it at a
// string the fuzzer owns is enough to make the template runtime-supplied.
// Tags resolve through the global component registry, so no Import is needed.
class FuzzedSelector : public rtxui::Component<FuzzedSelector> {
 public:
  std::string source;
  std::string_view view;
};

// Deliberately varied: two tags interleaved so the -of-type family differs
// from the -child family, repeated classes and a unique id so a compound can
// match some siblings but not others, an attribute whose value contains the
// punctuation that used to break the selector scans, and an empty element for
// :empty.
constexpr std::string_view kDom =
    R"(<div id="root" class="x y">)"
    R"(<span id="s1" class="x" data-k="a:b">1</span>)"
    R"(<p id="p1" class="y">2</p>)"
    R"(<span id="s2" class="x y">3</span>)"
    R"(<p id="p2">4</p>)"
    R"(<span id="s3"></span>)"
    R"(</div>)";

}  // namespace

void TestSelector(const std::string& selector) {
  // A fuzzer-generated selector is malformed far more often than not, and the
  // default reaction is to print the error and exit -- which would end the run
  // on the first input. Swallowing them keeps the session alive to reach the
  // matcher, which is what this target is for.
  static const int install_handler = [] {
    rtxui::SetXmlErrorHandler([](const rtxui::XmlError&) {});
    rtxui::SetCssErrorHandler([](const rtxui::CssError&) {});
    return 0;
  }();
  (void)install_handler;

  auto component = rtxui::Ref<FuzzedSelector>::New();
  component->source = std::string(kDom) + "<style>" + selector +
                      " { padding-left: 1; }</style>";
  component->view = component->source;

  // Mount() renders and resolves base styles, which is what runs the selector
  // against every element in the tree.
  component->Mount();

  // Pseudo-classes that depend on element state are only consulted by the
  // second pass, so ask for it explicitly rather than leaving that half of the
  // matcher unreached.
  component->ResolveTargetStyles();
}

FUZZ_TEST(Selector, TestSelector)
    .WithSeeds(std::vector<std::tuple<std::string>>{
        {"span"},
        {"#s1"},
        {".x.y"},
        {"div span"},
        {"div > span"},
        {"span + p"},
        {"span ~ p"},
        {"self"},
        {"*"},
        {"[data-k]"},
        {R"([data-k="a:b"])"},
        {"span:hover"},
        {"span:focus:active"},
        {"span:first-child"},
        {"span:last-of-type"},
        {"span:only-child"},
        {"span:empty"},
        {"span:nth-child(2)"},
        {"span:nth-child(even)"},
        {"span:nth-child(2n+1)"},
        {"span:nth-child(-n+2)"},
        {"span:nth-last-of-type(3n)"},
        {"span:not(.x)"},
        {"span:not(:first-child)"},
        {"span:not(.x:first-child)"},
        {"span:not(.x, #s2)"},
        {"span:not(:not(.x))"},
        // Nested :not() recursed unbounded until b2b726f's successor capped
        // it; keep a deep one in the corpus so a regression segfaults here.
        {"span:not(:not(:not(:not(:not(:not(:not(:not(.x))))))))"},
        {"div:not(span):not(#root)"},
        {"span::part(inner)"},
        {"div.x span:nth-child(2n+1):not(.y)"},
    });
