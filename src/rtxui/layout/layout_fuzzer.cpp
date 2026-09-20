// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
//
// Drives a whole frame -- parse, mount, style, layout, paint -- from a fuzzed
// template. The other fuzzers stop at their parser; this one is the only cover
// for the stages after it, which are also the ones that do arithmetic on sizes
// the template controls.
#include "rtxui/layout/layout.hpp"

#include <fuzztest/fuzztest.h>
#include <gtest/gtest.h>

#include <string>
#include <string_view>
#include <tuple>
#include <vector>

#include "rtxui/internal/component.hpp"
#include "rtxui/internal/refcounted.hpp"
#include "rtxui/layout/layout_tree_builder.hpp"
#include "rtxui/paint/paint.hpp"
#include "rtxui/paint/texture.hpp"

namespace {

// GetView() returns the `view` member when there is one, so pointing it at a
// string the fuzzer owns is enough to make the template runtime-supplied.
// Tags resolve through the global component registry, so no Import is needed.
class FuzzedView : public rtxui::Component<FuzzedView> {
 public:
  std::string source;
  std::string_view view;
};

}  // namespace

void TestLayout(const std::string& html, uint8_t width, uint8_t height) {
  // Most fuzzer-generated templates are malformed, and the default reaction to
  // that is to print the error and exit -- which would end the run on the first
  // input. Swallowing them here keeps the session alive to reach layout, which
  // is what this target is actually for.
  static const int install_handler = [] {
    rtxui::SetXmlErrorHandler([](const rtxui::XmlError&) {});
    rtxui::SetCssErrorHandler([](const rtxui::CssError&) {});
    return 0;
  }();
  (void)install_handler;

  auto component = rtxui::Ref<FuzzedView>::New();
  component->source = html;
  component->view = component->source;
  component->Mount();

  auto box = rtxui::LayoutTreeBuilder::Build(component->Root());
  if (!box) {
    return;
  }

  rtxui::LayoutConstraints constraints;
  constraints.width = {width, rtxui::MeasureMode::Exactly};
  constraints.height = {height, rtxui::MeasureMode::Exactly};
  auto fragment = rtxui::RunLayout({box.get()}, constraints);
  if (!fragment) {
    return;
  }

  Texture texture(width, height);
  rtxui::Paint(fragment.get(), texture);
  // Serialization walks every cell, so it catches a fragment placed outside
  // the texture that painting itself clipped away silently.
  (void)texture.Render();
}

// Random bytes almost never parse as a template, so unseeded runs spend their
// budget in the XML parser and barely reach layout at all. These give the
// mutator a working example of each layout mode to take apart.
std::vector<std::tuple<std::string, uint8_t, uint8_t>> LayoutSeeds() {
  const char* templates[] = {
      R"(<div><span>hello</span> <span>world</span></div>)",
      R"(<div class="f"><div>a</div><div>b</div></div>
         <style>.f { display: flex; gap: 1; }
                .f > div { flex-grow: 1; }</style>)",
      R"(<div class="g"><div>1</div><div>2</div><div>3</div></div>
         <style>.g { display: grid; grid-template-columns: 4 4 4; }</style>)",
      R"(<div class="s">a b c d e f g h i j k l m n o p</div>
         <style>.s { width: 6; overflow-y: scroll; word-break: break-all; }</style>)",
      R"(<div class="b">boxed</div>
         <style>.b { border: tall; padding: 1; margin: 1; width: 50%; }</style>)",
      R"(<div class="p"><div class="a">x</div></div>
         <style>.p { position: relative; height: 5; }
                .a { position: absolute; top: 2; right: 0; }</style>)",
      R"(<ul><li>one</li><li>two</li></ul>)",
      R"(<div><input value="v" /><button>ok</button></div>)",
  };
  std::vector<std::tuple<std::string, uint8_t, uint8_t>> seeds;
  for (const char* t : templates) {
    seeds.emplace_back(t, 20, 10);
    // The degenerate viewport reaches the size arithmetic that a comfortable
    // one never does.
    seeds.emplace_back(t, 1, 1);
  }
  return seeds;
}

FUZZ_TEST(Layout, TestLayout)
    .WithDomains(fuzztest::Arbitrary<std::string>(),
                 // 0 is worth reaching: an empty viewport is where size
                 // arithmetic is most likely to divide or index by zero.
                 fuzztest::InRange<uint8_t>(0, 40),
                 fuzztest::InRange<uint8_t>(0, 40))
    .WithSeeds(LayoutSeeds());
