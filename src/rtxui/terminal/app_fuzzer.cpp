// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
//
// Fuzzes a whole app over time: a fuzzed template and stylesheet, then fuzzed
// input, then the clock moving forward. Layout.TestLayout draws one frame of a
// fuzzed template and Events.TestEvents sends fuzzed input to a fixed one;
// neither lets the template decide what the input does, and neither moves the
// clock, so transitions, animations and the frames that reuse the last layout
// were never fuzzed.
//
// It also has an oracle, not just the sanitizers: RTXUI_VERIFY_LAYOUT_REUSE
// makes every frame that reuses a cached layout lay out again from scratch,
// and abort if that draws anything else. A stale cache draws a plausible
// frame, so without this check it would never show up as a crash.
#include <fuzztest/fuzztest.h>
#include <gtest/gtest.h>

#include <cstdint>
#include <cstdlib>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>

#include "rtxui/component.hpp"
#include "rtxui/diagnostic.hpp"
#include "rtxui/headless.hpp"
#include "rtxui/refcounted.hpp"

namespace {

// GetView() returns the `view` member when there is one, so pointing it at a
// string the fuzzer owns is enough to make the template runtime-supplied.
class FuzzedApp : public rtxui::Component<FuzzedApp> {
 public:
  std::string source;
  std::string_view view;
};

}  // namespace

void TestApp(const std::string& html,
             const std::string& input,
             const std::vector<uint16_t>& steps) {
  static const int setup = [] {
    // Most fuzzed templates are malformed, and the default reaction is to
    // print the error and exit, which would end the run on the first input.
    rtxui::SetDiagnosticHandler([](const rtxui::Diagnostic&) {});
    setenv("RTXUI_VERIFY_LAYOUT_REUSE", "1", /*overwrite=*/1);
    return 0;
  }();
  (void)setup;

  auto app = rtxui::Ref<FuzzedApp>::New();
  app->source = html;
  app->view = app->source;
  rtxui::TimelineScreen screen(app, 40, 16);

  // Half the input lands before the clock moves and half in the middle, so
  // an event can arrive while a transition it started is still running.
  const size_t half = input.size() / 2;
  screen.Input(std::string_view(input).substr(0, half));
  for (size_t i = 0; i < steps.size() && i < 16; ++i) {
    if (i == steps.size() / 2) {
      screen.Input(std::string_view(input).substr(half));
    }
    screen.Advance(steps[i]);
  }
}

namespace {

std::vector<std::tuple<std::string, std::string, std::vector<uint16_t>>>
AppSeeds() {
  const char* templates[] = {
      // A transition started by :hover, so the mouse moving in starts it.
      R"(<button class="b">hover me</button>
         <style>.b { background-color: blue; transition: background-color 300ms; }
                .b:hover { background-color: red; }</style>)",
      // A keyframe animation that changes the layout on every frame.
      R"(<div class="a">grow</div>
         <style>@keyframes g { from { width: 2; } to { width: 30; } }
                .a { animation: g 500ms infinite alternate; border: tall; }</style>)",
      // A paint-only animation: the frames that reuse the last layout.
      R"(<div class="o">fade</div><div class="t">slide</div>
         <style>@keyframes f { from { opacity: 0; } to { opacity: 1; } }
                @keyframes s { from { translate: 0; } to { translate: 10; } }
                .o { animation: f 200ms infinite; }
                .t { animation: s 400ms ease-in-out infinite; }</style>)",
      // Flex and grid, with a focusable control in each cell.
      R"(<div class="f"><input value="a" /><button>ok</button></div>
         <div class="g"><div>1</div><div>2</div><div>3</div><div>4</div></div>
         <style>.f { display: flex; gap: 1; } .f > * { flex-grow: 1; }
                .g { display: grid; grid-template-columns: repeat(auto-fill, minmax(6, 1fr)); }
                input:focus { border: tall; transition: width 100ms; width: 20; }</style>)",
      // A scroll container: wheel, scrollbar drag, text selection.
      R"(<div class="s"><p>one two three four five six seven eight nine ten</p>
         <p>eleven twelve thirteen fourteen fifteen sixteen</p></div>
         <style>.s { height: 3; width: 14; overflow-y: scroll; scroll-behavior: smooth; }</style>)",
      // Built-in components that rebuild their elements in Digest().
      R"(<tabs><tab-pane label="one"><p>a</p></tab-pane>
         <tab-pane label="two"><p>b</p></tab-pane></tabs>
         <details><summary>more</summary><p>body</p></details>
         <select><option>x</option><option>y</option></select>
         <textarea value="text"></textarea>)",
      // Markdown, through the template.
      "<markdown># Title\n\n- one\n- **two**\n\n`code`</markdown>",
  };
  const char* inputs[] = {
      "",
      // Mouse moves in, presses, drags, releases.
      "\x1b[<35;3;1M\x1b[<0;3;1M\x1b[<32;8;3M\x1b[<0;8;3m",
      // Focus the first control, type, move away.
      "\tabc\x7f\t\r",
      // Wheel over the top-left area.
      "\x1b[<65;3;2M\x1b[<65;3;2M\x1b[<64;3;2M",
      // Resize in the middle of the stream.
      "\x1b[<35;3;1M\x1b[8;10;20t\x1b[<35;30;1M",
  };
  std::vector<std::tuple<std::string, std::string, std::vector<uint16_t>>>
      seeds;
  for (const char* t : templates) {
    for (const char* in : inputs) {
      seeds.emplace_back(t, in, std::vector<uint16_t>{16, 16, 100, 250, 1000});
    }
  }
  return seeds;
}

}  // namespace

FUZZ_TEST(App, TestApp)
    .WithDomains(fuzztest::Arbitrary<std::string>(),
                 fuzztest::Arbitrary<std::string>(),
                 // Up to a few seconds per step: long enough to finish any
                 // seeded transition, short enough to land in the middle of
                 // one.
                 fuzztest::VectorOf(fuzztest::InRange<uint16_t>(0, 3000))
                     .WithMaxSize(16))
    .WithSeeds(AppSeeds());
