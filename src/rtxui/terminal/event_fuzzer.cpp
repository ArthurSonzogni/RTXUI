// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
//
// Fuzzes what the terminal does with input, not how it decodes it.
// TerminalInputParser.Fuzz stops at the parser: it turns bytes into Events and
// throws them away. Everything downstream -- hit-testing a click against the
// fragment tree, scrollbar and thumb dragging, focus movement, text editing,
// and the digest/redraw each of those triggers -- had no fuzz cover, and it is
// the part of the engine that holds pointers into the layout arena across
// frames. The one memory-corruption bug this project has had (fragments copied
// out of a scrollbar click path and released into the next frame's reused
// arena) lived exactly here.
#include <fuzztest/fuzztest.h>
#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>

#include "rtxui/dom/element.hpp"
#include "rtxui/internal/component.hpp"
#include "rtxui/internal/refcounted.hpp"
#include "rtxui/internal/screen.hpp"
#include "rtxui/terminal/terminal_device.hpp"
#include "rtxui/xml/xml.hpp"

namespace {

// Fixed on purpose: the input is the fuzzer's budget, so the interface has to
// be worth aiming at rather than something the fuzzer also has to invent. It
// carries the features whose event handling reaches furthest into the engine
// -- two scroll containers (one of each axis, for the scrollbar and thumb
// paths), focusable and editable fields, a checkbox and a select whose Digest()
// rebuilds elements, and a click handler that changes the tree.
class EventTargetApp : public rtxui::Component<EventTargetApp> {
 public:
  int clicks = 0;
  bool shown = true;
  bool dialog_open = false;
  std::string text;
  std::string tab = "one";

  void Toggle() { shown = !shown; }
  void Bump() { ++clicks; }
  void OpenDialog() { dialog_open = true; }
  void CloseDialog() { dialog_open = false; }
  std::string Count() const { return std::to_string(clicks); }

  void InitReflection() override {
    Bind(clicks);
    Bind(shown);
    Bind(dialog_open);
    Bind(text);
    Bind(tab);
    Bind(Toggle);
    Bind(Bump);
    Bind(OpenDialog);
    Bind(CloseDialog);
    Bind(Count);
    rtxui::Component<EventTargetApp>::InitReflection();
  }

  std::string_view view = R"(
    <div class="root">
      <button onclick="Bump">bump {Count}</button>
      <button onclick="Toggle">toggle</button>
      <div class="scroll-y">
        <p>one</p><p>two</p><p>three</p><p>four</p><p>five</p>
        <p>six</p><p>seven</p><p>eight</p><p>nine</p><p>ten</p>
      </div>
      <div class="scroll-x">
        <p>a very wide line that will not fit inside this container at all</p>
      </div>
      <input value="{text}" />
      <textarea value="{text}"></textarea>
      <input type="checkbox" />
      <select>
        <option>alpha</option>
        <option>beta</option>
      </select>
      <slider value="30" min="0" max="100" step="5" width="10" />
      <!-- <details> and <fieldset> hoist their summary/legend out of the
           projected content, which has crashed before when the hoisted child
           was conditional. -->
      <details>
        <summary>more</summary>
        <if condition="{shown}"><p>detail body</p></if>
      </details>
      <fieldset>
        <if condition="{shown}"><legend>group</legend></if>
        <p>grouped</p>
      </fieldset>
      <!-- <tabs> rebuilds its header strip from Digest(). -->
      <tabs value="{tab}">
        <tab-pane label="one" name="one"><p>pane one</p></tab-pane>
        <tab-pane label="two" name="two"><p>pane two</p></tab-pane>
      </tabs>
      <a href="#anchor">jump</a>
      <button onclick="OpenDialog">open</button>
      <if condition="{shown}"><p id="conditional">here</p></if>
      <p id="anchor">anchor</p>
      <!-- position: fixed, so it goes through CollectFixedFragments -- the
           function the first use-after-free was found in. -->
      <dialog open="{dialog_open}" title="confirm">
        <p>are you sure</p>
        <button onclick="CloseDialog">close</button>
      </dialog>
    </div>
    <style>
      .root { display: flex; flex-direction: column; }
      .scroll-y { height: 4; overflow-y: scroll; }
      .scroll-x { width: 12; overflow-x: scroll; white-space: nowrap; }
      button:hover { background-color: blue; }
      button:active { background-color: red; }
      input:focus { border: tall; }
    </style>
  )";
};

}  // namespace

void TestEvents(const std::string& input) {
  // A fuzzed byte stream is mostly malformed escape sequences, and the default
  // reaction to a template/stylesheet error is to print and exit. Nothing here
  // fuzzes the template, but the handlers keep an unrelated diagnostic from
  // ending the run.
  static const int install_handler = [] {
    rtxui::SetXmlErrorHandler([](const rtxui::XmlError&) {});
    rtxui::SetCssErrorHandler([](const rtxui::CssError&) {});
    return 0;
  }();
  (void)install_handler;

  auto app = rtxui::Ref<EventTargetApp>::New();
  auto device = std::make_shared<rtxui::MockTerminalDevice>();
  // Big enough that the whole interface is on screen. At 40x20 most of it sat
  // below the viewport, where nothing lays out and no click can reach it --
  // the fuzzer then spends its budget on coordinates that hit nothing.
  device->TriggerResize(60, 44);
  rtxui::Screen screen(app, device);
  screen.Draw();

  device->PushInput(input);

  // Step several times rather than once. Step() drains everything buffered in
  // one go, but a handler can post a task or leave a transition running, and
  // the layout arena is double-buffered -- a fragment held one frame too long
  // only corrupts on the frame after next. A single step could not see that.
  for (int i = 0; i < 8; ++i) {
    screen.Step();
  }
}

FUZZ_TEST(Events, TestEvents)
    .WithSeeds(std::vector<std::tuple<std::string>>{
        // Keyboard: navigation, editing, and the keys that move focus.
        {"\t"},
        {"\t\t\t\t"},
        {"\x1b[Z"},
        {"hello"},
        {"\r"},
        {" "},
        {"\x7f"},
        {"\x1b[A\x1b[B\x1b[C\x1b[D"},
        {"\x1b[5~\x1b[6~"},
        {"\x1b[H\x1b[F"},
        {"\x1b"},
        // SGR mouse: press, release, drag and wheel over the interface. The
        // scroll containers and their scrollbars sit in the middle rows.
        {"\x1b[<0;5;2M\x1b[<0;5;2m"},
        {"\x1b[<0;38;6M\x1b[<0;38;8m"},
        {"\x1b[<32;38;7M"},
        {"\x1b[<0;38;6M\x1b[<32;38;9M\x1b[<32;38;12M\x1b[<0;38;12m"},
        {"\x1b[<64;10;6M"},
        {"\x1b[<65;10;6M"},
        {"\x1b[<2;10;6M\x1b[<2;10;6m"},
        {"\x1b[<35;20;10M"},
        // A burst, which is what makes Step() suppress intermediate draws.
        {"\x1b[<32;10;5M\x1b[<32;11;5M\x1b[<32;12;5M\x1b[<32;13;5M"},
        // A press on every row in one burst. Every control gets clicked, and
        // each click reconciles under the batch flag -- which is the shape
        // that dangled the fragment tree. Also saves the mutator from having
        // to discover where the controls are.
        {[] {
          std::string s;
          for (int row = 1; row <= 44; ++row) {
            s += "\x1b[<0;3;" + std::to_string(row) + "M";
            s += "\x1b[<0;3;" + std::to_string(row) + "m";
          }
          return s;
        }()},
        // Bracketed paste.
        {"\x1b[200~pasted text\x1b[201~"},
        // Resize mid-stream, and a terminal reply the parser must swallow.
        {"\x1b[8;30;100t"},
        {"\x1b]11;rgb:1c1c/1c1c/1c1c\x07"},
    });
