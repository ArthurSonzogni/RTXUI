// Copyright 2024 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.

#include <catch2/catch_test_macros.hpp>
#include <cctype>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

#include "catch2/catch_message.hpp"
#include "rtxui/component.hpp"
#include "rtxui/diagnostic.hpp"
#include "rtxui/dom/element.hpp"
#include "rtxui/dom/text_element.hpp"
#include "rtxui/headless.hpp"
#include "rtxui/layout/style.hpp"
#include "rtxui/paint/texture.hpp"
#include "rtxui/refcounted.hpp"
#include "rtxui/screen.hpp"
#include "rtxui/terminal/terminal_device.hpp"
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

class MarkdownTestContainer : public rtxui::Component<MarkdownTestContainer> {
 public:
  std::string_view view = R"(
    <markdown id="md" content="# Title" stylesheet="h1 { color: red; }"></markdown>
  )";
};

namespace {

class ChangingMarkdownComponent
    : public rtxui::Component<ChangingMarkdownComponent> {
 public:
  std::string text = "# First";
  ChangingMarkdownComponent() { Bind(text); }
  std::string_view view = R"(<markdown id="md" content="{text}"></markdown>)";
};

}  // namespace

TEST_CASE("A component whose view changes is not served a stale template",
          "[component][markdown]") {
  // Stripping a template's indentation is cached by its text, since every
  // instance of a component shares one. <markdown> is the component that
  // breaks the assumption behind that: it generates a fresh view from its
  // content, so the same component asks for a different template on every
  // keystroke and must get the new one back.
  auto app = rtxui::Ref<ChangingMarkdownComponent>::New();
  app->Mount();
  app->Digest();  // The markdown component builds its view from Digest().

  auto heading_text = [&]() -> std::string {
    auto* md = app->Root()->QuerySelector("#md");
    REQUIRE(md != nullptr);
    std::string out;
    md->Visit([&out](rtxui::Element& element) {
      if (element.is_text()) {
        out += static_cast<rtxui::TextElement&>(element).text();
      }
    });
    return out;
  };

  CHECK(heading_text().find("First") != std::string::npos);

  app->text = "# Second";
  app->Digest();
  CHECK(heading_text().find("Second") != std::string::npos);
  CHECK(heading_text().find("First") == std::string::npos);

  // Back to the original text, which the cache has seen before -- a stale
  // entry would show here as well as a missing one.
  app->text = "# First";
  app->Digest();
  CHECK(heading_text().find("First") != std::string::npos);
  CHECK(heading_text().find("Second") == std::string::npos);
}

TEST_CASE("Markdown Component rendering", "[component][markdown]") {
  auto container = rtxui::Ref<MarkdownTestContainer>::New();
  container->Mount();
  container->Digest();

  auto* root = container->Root();
  auto* md_el = root->QuerySelector("#md");
  REQUIRE(md_el != nullptr);

  auto* h1_el = root->QuerySelector("h1");
  REQUIRE(h1_el != nullptr);

  // The stylesheet inside the markdown component should apply to the h1
  // since the markdown component's GetView prepends the <style> block.
  CHECK(h1_el->style.foreground_color.has_value());
}

class MarkdownLinkInjectionTestContainer
    : public rtxui::Component<MarkdownLinkInjectionTestContainer> {
 public:
  std::string content = "[click](\" onclick=\"Evil)";
  MarkdownLinkInjectionTestContainer() { Bind(content); }

  std::string_view view = R"(
    <markdown id="md" content="{content}"></markdown>
  )";
};

TEST_CASE(
    "Markdown link URL with a quote does not crash Mount or inject "
    "attributes",
    "[component][markdown]") {
  // Regression: a link URL containing '"' used to produce HTML that failed
  // to parse as XML (crashing the whole process on Mount, since that path
  // is not recoverable like Digest()'s), or, when it happened to still
  // parse, injected an extra "onclick" attribute into the <a> element.
  auto container = rtxui::Ref<MarkdownLinkInjectionTestContainer>::New();
  CHECK_NOTHROW(container->Mount());
  CHECK_NOTHROW(container->Digest());

  auto* link_el = container->Root()->QuerySelector("a");
  REQUIRE(link_el != nullptr);
  CHECK(link_el->GetAttribute("onclick") == nullptr);
}

class MarkdownListTestContainer
    : public rtxui::Component<MarkdownListTestContainer> {
 public:
  std::string_view view = R"(
    <markdown id="md" content="- list item 1\n- list item 2"></markdown>
  )";
};

TEST_CASE("Markdown List rendering", "[component][markdown][list]") {
  auto container = rtxui::Ref<MarkdownListTestContainer>::New();
  container->Mount();
  container->Digest();

  auto* root = container->Root();
  auto* md_el = root->QuerySelector("#md");
  REQUIRE(md_el != nullptr);

  auto* ul_el = root->QuerySelector("ul");
  REQUIRE(ul_el != nullptr);

  auto* li_el = root->QuerySelector("li");
  REQUIRE(li_el != nullptr);

  auto print = ul_el->Print();
  CHECK(RemoveWhitespace(print).find("•</span>listitem1") != std::string::npos);
}

class MarkdownCodeBlockTestContainer
    : public rtxui::Component<MarkdownCodeBlockTestContainer> {
 public:
  std::string content = "```\nint main() {\n  return 0;\n}\n```";
  MarkdownCodeBlockTestContainer() { Bind(content); }

  std::string_view view = R"(
    <markdown id="md" content="{content}"></markdown>
  )";
};

TEST_CASE("Markdown code block newlines preserved",
          "[component][markdown][pre]") {
  auto container = rtxui::Ref<MarkdownCodeBlockTestContainer>::New();
  container->Mount();
  container->Digest();

  auto texture = PaintComponent(*container, 80, 24);

  std::string rendered = texture.Render();
  auto pos_main = rendered.find("int main()");
  auto pos_return = rendered.find("return 0;");
  REQUIRE(pos_main != std::string::npos);
  REQUIRE(pos_return != std::string::npos);
  CHECK(rendered.find('\n', pos_main) < pos_return);
}

// Mirrors example/markdown.cpp: a fenced block with a language tag, inside a
// list item, rendered in a scrolling flex pane with <pre> and <code> restyled.
class MarkdownStyledCodeBlockTestContainer
    : public rtxui::Component<MarkdownStyledCodeBlockTestContainer> {
 public:
  std::string content =
      "- item:\n\n```cpp\n#include <rtxui/rtxui.hpp>\nint main() {\n"
      "  return 0;\n}\n```";
  std::string stylesheet =
      "code { color: #94a3b8; }\n"
      "pre { background-color: #1e293b; border: tall; display: block; }";
  MarkdownStyledCodeBlockTestContainer() {
    Bind(content);
    Bind(stylesheet);
  }

  std::string_view view = R"html(
    <div class="app">
      <div class="pane">Editor</div>
      <div class="pane">
        <div class="preview-frame">
          <markdown content="{content}" stylesheet="{stylesheet}" />
        </div>
      </div>
    </div>
    <style>
      self { display: block; width: 100%; height: 100%; }
      .app { display: flex; flex-direction: row; width: 100%; height: 100%; }
      .pane { display: flex; flex-direction: column; width: 50%; padding: 1; }
      .preview-frame { flex-grow: 1; width: 100%; overflow-y: scroll; }
    </style>
  )html";
};

TEST_CASE("Markdown styled code block newlines preserved",
          "[component][markdown][pre]") {
  auto device = std::make_shared<rtxui::MockTerminalDevice>();
  device->TriggerResize(80, 24);
  auto container = rtxui::Ref<MarkdownStyledCodeBlockTestContainer>::New();
  rtxui::Screen screen(container, device);
  screen.Draw();

  // Each source line of the block keeps its own row.
  std::string text = screen.Text();
  INFO(text);
  auto main_pos = text.find("int main() {");
  REQUIRE(main_pos != std::string::npos);
  auto line_end = text.find('\n', main_pos);
  CHECK(text.find("return 0;") > line_end);
  CHECK(text.find("int main() {   return 0;") == std::string::npos);
}

class MarkdownStylesheetErrorTestContainer
    : public rtxui::Component<MarkdownStylesheetErrorTestContainer> {
 public:
  std::string stylesheet = "h1 { color: red; }";
  MarkdownStylesheetErrorTestContainer() { Bind(stylesheet); }

  std::string_view view = R"(
    <markdown id="md" content="# Title" stylesheet="{stylesheet}"></markdown>
  )";
};

TEST_CASE(
    "Markdown Component reports XML parse errors from a malformed "
    "stylesheet",
    "[component][markdown]") {
  // Regression: markdown::Digest() used to silently drop xml::Parse()
  // failures on its generated document (stylesheet + rendered Markdown
  // body), leaving the preview stuck on stale content with no way for a
  // host app (e.g. the Markdown playground) to surface the error to the
  // user.
  auto container = rtxui::Ref<MarkdownStylesheetErrorTestContainer>::New();
  container->Mount();
  container->Digest();

  std::optional<rtxui::Diagnostic> reported_error;
  rtxui::SetDiagnosticHandler([&](const rtxui::Diagnostic& error) {
    if (error.kind == rtxui::Diagnostic::Kind::XmlSyntax) {
      reported_error = error;
    }
  });

  // A stylesheet that closes its own <style> element leaves the rest of the
  // generated document unbalanced, so xml::Parse() fails and markdown must
  // surface that rather than silently keeping stale content.
  container->stylesheet = "h1 { color: red; } </style> <unclosed>";
  container->Digest();

  rtxui::SetDiagnosticHandler(nullptr);

  REQUIRE(reported_error.has_value());
  CHECK_FALSE(reported_error->message.empty());
}

TEST_CASE("Markdown Component accepts tag-like sequences in its stylesheet",
          "[component][markdown]") {
  // <style> is raw text, so a tag name written inside a CSS comment is just
  // CSS. This used to abort the XML parse of the generated document, which is
  // why playground.cpp once needed a workaround for it.
  auto container = rtxui::Ref<MarkdownStylesheetErrorTestContainer>::New();
  container->Mount();
  container->Digest();

  std::optional<rtxui::Diagnostic> reported_error;
  rtxui::SetDiagnosticHandler([&](const rtxui::Diagnostic& error) {
    if (error.kind == rtxui::Diagnostic::Kind::XmlSyntax) {
      reported_error = error;
    }
  });

  container->stylesheet = "/* <textarea> */ h1 { color: red; }";
  container->Digest();

  rtxui::SetDiagnosticHandler(nullptr);

  CHECK_FALSE(reported_error.has_value());
}

}  // namespace
