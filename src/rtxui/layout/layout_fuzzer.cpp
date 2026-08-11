// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
//
// Drives a whole frame -- parse, mount, style, layout, paint -- from a fuzzed
// template. The other fuzzers stop at their parser; this one is the only cover
// for the stages after it, which are also the ones that do arithmetic on sizes
// the template controls.
#include <fuzztest/fuzztest.h>
#include <gtest/gtest.h>

#include <string>
#include <string_view>

#include "rtxui/internal/component.hpp"
#include "rtxui/internal/refcounted.hpp"
#include "rtxui/layout/layout.hpp"
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

FUZZ_TEST(Layout, TestLayout)
    .WithDomains(fuzztest::Arbitrary<std::string>(),
                 // 0 is worth reaching: an empty viewport is where size
                 // arithmetic is most likely to divide or index by zero.
                 fuzztest::InRange<uint8_t>(0, 40),
                 fuzztest::InRange<uint8_t>(0, 40));
