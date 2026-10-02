// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include "rtxui/diagnostic.hpp"

#include <memory>
#include <string>
#include <vector>

#include "catch2/catch_test_macros.hpp"
#include "rtxui/component.hpp"
#include "rtxui/component/default_components_internal.hpp"
#include "rtxui/screen.hpp"
#include "rtxui/terminal/terminal_device.hpp"

namespace rtxui {
namespace {

// Collects the diagnostics reported while it is alive.
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

// Mounts, styles, lays out and paints `app` once, as a running app would.
template <typename T>
void Show(Ref<T> app) {
  auto device = std::make_shared<MockTerminalDevice>();
  device->TriggerResize(40, 10);
  Screen screen(app, device);
  screen.Draw();
}

class Valid : public Component<Valid> {
 public:
  std::string title = "hi";
  std::vector<std::string> items{"a", "b"};
  bool shown = true;
  void Go() {}
  void InitReflection() override {
    Bind(title);
    Bind(items);
    Bind(shown);
    Bind(Go);
    Import<div>();
    Import<button>();
  }
  std::string_view view = R"html(
    <div class="box" style="padding: 1">
      <h1 if="{shown}">{title}</h1>
      <for each="{items}" as="item"><div>{item} {$index}</div></for>
      <button onclick="Go">go</button>
      <label for="field">field</label>
      <table><thead><tr><th>h</th></tr></thead>
        <tbody><tr><td>c<br/>d</td></tr></tbody></table>
    </div>
    <style>
      .box { display: flex; flex-direction: column; color: red; }
    </style>
  )html";
};

TEST_CASE("A valid template reports nothing", "[diagnostic]") {
  DiagnosticRecorder recorder;
  Show(Ref<Valid>::New());
  CHECK(recorder.messages.empty());
}

class UnknownCss : public Component<UnknownCss> {
 public:
  void InitReflection() override { Import<div>(); }
  std::string_view view = R"html(
    <div class="box">x</div>
    <style>
      .box { colr: red; flex-direction: diagonal; color: blue; }
    </style>
  )html";
};

TEST_CASE("Unsupported CSS declarations are reported once", "[diagnostic]") {
  DiagnosticRecorder recorder;
  auto app = Ref<UnknownCss>::New();
  Show(app);
  Show(app);  // Restyling must not report the same mistake again.
  CHECK(recorder.Has("'colr: red'"));
  CHECK(recorder.Has("'flex-direction: diagonal'"));
  CHECK_FALSE(recorder.Has("color: blue"));
  CHECK(recorder.messages.size() == 2);
}

class UnboundName : public Component<UnboundName> {
 public:
  int count = 0;
  void InitReflection() override {
    Bind(count);
    Import<div>();
  }
  std::string_view view = R"html(
    <div>{titel} {count == 1}</div>
  )html";
};

TEST_CASE("Unbound names and expressions in {} are reported", "[diagnostic]") {
  DiagnosticRecorder recorder;
  Show(Ref<UnboundName>::New());
  CHECK(recorder.Has("'{titel}' in <UnboundName> is not a bound name"));
  CHECK(
      recorder.Has("'{count == 1}' in <UnboundName>: templates do not "
                   "evaluate expressions"));
}

class ForeignSyntax : public Component<ForeignSyntax> {
 public:
  void InitReflection() override { Import<div>(); }
  std::string_view view = R"html(
    <div v-if="shown" className="box" onClick="Go">
      <div for="item in items">x</div>
    </div>
  )html";
};

TEST_CASE("Other frameworks' attribute syntax is reported", "[diagnostic]") {
  DiagnosticRecorder recorder;
  Show(Ref<ForeignSyntax>::New());
  CHECK(recorder.Has("'v-if'"));
  CHECK(recorder.Has("'className'"));
  CHECK(recorder.Has("'onClick'"));
  CHECK(recorder.Has("'for' on <div>"));
}

class Card : public Component<Card> {
 public:
  std::string_view view = R"html(<div>card</div>)html";
};

class UnknownTags : public Component<UnknownTags> {
 public:
  void InitReflection() override { Import<div>(); }
  std::string_view view = R"html(
    <div>
      <View>x</View>
      <Card/>
      <panel>y</panel>
      <div><style>.a { color: red; }</style></div>
    </div>
  )html";
};

TEST_CASE("Unknown tags and nested <style> are reported", "[diagnostic]") {
  DiagnosticRecorder recorder;
  Show(Ref<UnknownTags>::New());
  CHECK(
      recorder.Has("unknown tag <View> in <UnknownTags>: if it is your "
                   "component, call Import<View>()"));
  CHECK(recorder.Has("unknown tag <Card>"));  // Defined, but not imported.
  CHECK(recorder.Has("unknown tag <panel> in <UnknownTags>: use a built-in"));
  CHECK(recorder.Has("<style> inside <div> in <UnknownTags> is ignored"));
}

class UnboundHandler : public Component<UnboundHandler> {
 public:
  void InitReflection() override { Import<div>(); }
  std::string_view view = R"html(
    <div onclick="Missing">click</div>
  )html";
};

TEST_CASE("Clicking a handler that is not bound is reported", "[diagnostic]") {
  DiagnosticRecorder recorder;
  auto app = Ref<UnboundHandler>::New();
  auto device = std::make_shared<MockTerminalDevice>();
  device->TriggerResize(40, 10);
  Screen screen(app, device);
  screen.Draw();
  device->PushInput("\x1b[<0;1;1M");  // Left press on the first cell.
  screen.Step();
  CHECK(recorder.Has("handler 'Missing' is not bound"));
}

}  // namespace
}  // namespace rtxui
