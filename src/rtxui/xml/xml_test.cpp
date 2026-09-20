// Copyright 2024 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include "rtxui/xml/xml.hpp"

#include <catch2/catch_test_macros.hpp>
#include <string>

#include "rtxui/base/string.hpp"

TEST_CASE("XML parser works correctly", "[xml]") {
  const std::string input = R"(
    <root>
      Hello from here!
      <element attribute="value">
        <child/>
      </element>
      Hello from here too!
    </root>
  )";

  auto nodes = xml::Parse(input);
  if (!nodes) {
    FAIL(nodes.error().message + " at line " +
         std::to_string(nodes.error().line) + " column " +
         std::to_string(nodes.error().column));
  }

  CHECK(nodes.value().size() == 1);

  CHECK(xml::Print(nodes.value()[0]) == StripIndent(input));
}

TEST_CASE("XML nesting is bounded", "[xml]") {
  // The parser recurses once per level, so deep nesting used to overflow the
  // stack rather than report an error. A template is not always written by the
  // developer: hot reload re-reads one at runtime, and the playground lets one
  // be typed.
  const auto nested = [](int depth, bool close) {
    std::string xml;
    for (int i = 0; i < depth; ++i) {
      xml += "<div>";
    }
    xml += "x";
    if (close) {
      for (int i = 0; i < depth; ++i) {
        xml += "</div>";
      }
    }
    return xml;
  };

  SECTION("ordinary nesting still parses") {
    // Far deeper than anything hand-written, and still accepted.
    auto result = xml::Parse(nested(200, true));
    CHECK(result.has_value());
  }

  SECTION("excessive nesting is an error, not a crash") {
    auto result = xml::Parse(nested(50000, true));
    CHECK_FALSE(result.has_value());
  }

  SECTION("excessive unclosed nesting is an error too") {
    // The unclosed form recurses just as deep before it can fail.
    auto result = xml::Parse(nested(50000, false));
    CHECK_FALSE(result.has_value());
  }
}

TEST_CASE("XML with multiple roots", "[xml]") {
  const std::string input = R"(
    <root1>
      <sub/>
    </root1>
    <root2>
      <sub/>
    </root2>
  )";

  auto nodes = xml::Parse(input);
  if (!nodes) {
    FAIL(nodes.error().message + " at line " +
         std::to_string(nodes.error().line) + " column " +
         std::to_string(nodes.error().column));
  }

  CHECK(nodes.value().size() == 2);
  CHECK(nodes.value()[0].tag == "root1");
  CHECK(nodes.value()[1].tag == "root2");
}

TEST_CASE("XML with unique text", "[xml]") {
  const std::string input = "Hello World!";

  auto nodes = xml::Parse(input);
  if (!nodes) {
    FAIL(nodes.error().message + " at line " +
         std::to_string(nodes.error().line) + " column " +
         std::to_string(nodes.error().column));
  }

  CHECK(nodes.value().size() == 1);
  CHECK(nodes.value()[0].type == xml::Node::kText);
  CHECK(nodes.value()[0].text == "Hello World!");
  CHECK(xml::Print(nodes.value()[0]) == "Hello World!\n");
}

TEST_CASE("XML attribute containing spaces", "[xml]") {
  const std::string input = R"(
    <root>
      <sub key="value"/>
      <sub key ="value"/>
      <sub key= "value"/>
      <sub key = "value"/>
      <sub key = "value"/>
      <sub key1="value"     key2="value"/>
      <sub
        key1="value"
        key2="value"
      />
    </root>
  )";

  const std::string output = R"(
    <root>
      <sub key="value"/>
      <sub key="value"/>
      <sub key="value"/>
      <sub key="value"/>
      <sub key="value"/>
      <sub key1="value" key2="value"/>
      <sub key1="value" key2="value"/>
    </root>
  )";

  auto nodes = xml::Parse(input);
  if (!nodes) {
    FAIL(nodes.error().message + " at line " +
         std::to_string(nodes.error().line) + " column " +
         std::to_string(nodes.error().column));
  }

  CHECK(nodes.value().size() == 1);

  CHECK(xml::Print(nodes.value()[0]) == StripIndent(output));
}

TEST_CASE("XML quoted string with spaces", "[xml]") {
  const std::string input = R"(
    <root>
      <sub key="value with spaces"/>
    </root>
  )";

  auto nodes = xml::Parse(input);
  if (!nodes) {
    FAIL(nodes.error().message + " at line " +
         std::to_string(nodes.error().line) + " column " +
         std::to_string(nodes.error().column));
  }

  CHECK(nodes.value().size() == 1);
  CHECK(xml::Print(nodes.value()[0]) == StripIndent(input));
}

TEST_CASE("XML comment", "[xml]") {
  const std::string input = R"(
    <root>
      <!-- This is a comment -->
      <sub/>
    </root>
  )";

  auto nodes = xml::Parse(input);
  if (!nodes) {
    FAIL(nodes.error().message + " at line " +
         std::to_string(nodes.error().line) + " column " +
         std::to_string(nodes.error().column));
  }

  CHECK(nodes.value().size() == 1);
  CHECK(xml::Print(nodes.value()[0]) == StripIndent(input));
}

TEST_CASE("XML empty tag", "[xml]") {
  const std::string input = R"(
    <>
      <sub/>
    </>
  )";

  auto nodes = xml::Parse(input);
  CHECK(nodes.value().size() == 1);
  CHECK(xml::Print(nodes.value()[0]) == StripIndent(input));
}

TEST_CASE("XML reactive", "[xml]") {
  const std::string input = R"(
    <root>
      <!-- This demonstrates the usage of interpolation -->
      <paragraph>
        Hello, World!
        input = {{input}}
        state = {{state}}
      </paragraph>

      <!-- This demonstrates event handling -->
      <button @click.left="onClick">
        Click me!
      </button>

      <output if="state">
        This is a conditional text!
      </output>

      <SubComponent></SubComponent>

      <SubComponent prop="value"></SubComponent>

      <SubComponent>
        <template name="namedslot_1">Hello from slot 1</template>
        <template name="namedslot_2">Hello from slot 2</template>
      </SubComponent>

      <slot></slot>
    </root>
  )";

  const std::string output = R"(
    <root>
      <!-- This demonstrates the usage of interpolation -->
      <paragraph>
        Hello, World!
            input = {{input}}
            state = {{state}}
      </paragraph>
      <!-- This demonstrates event handling -->
      <button @click.left="onClick">
        Click me!
      </button>
      <output if="state">
        This is a conditional text!
      </output>
      <SubComponent/>
      <SubComponent prop="value"/>
      <SubComponent>
        <template name="namedslot_1">
          Hello from slot 1
        </template>
        <template name="namedslot_2">
          Hello from slot 2
        </template>
      </SubComponent>
      <slot/>
    </root>
  )";

  auto nodes = xml::Parse(input);
  if (!nodes) {
    FAIL(nodes.error().message + " at line " +
         std::to_string(nodes.error().line) + " column " +
         std::to_string(nodes.error().column));
  }

  CHECK(nodes.value().size() == 1);
  CHECK(xml::Print(nodes.value()[0]) == StripIndent(output));
}

TEST_CASE("XML.EscapedCharacters", "[xml]") {
  std::string_view input = R"xml(
    <div attr="&lt;hello&gt; &amp; &quot;world&quot; &apos;!&#x1F600;">
      &lt;escaped&gt; &amp; &quot;text&quot; &apos;! &#60; &#x3c; &#x3C; &#x1F600;
    </div>
  )xml";

  auto nodes = xml::Parse(input);
  REQUIRE(nodes.has_value());
  REQUIRE(nodes.value().size() == 1);

  const auto& div = nodes.value()[0];
  CHECK(div.tag == "div");
  CHECK(div.attributes.at("attr") == "<hello> & \"world\" '!😀");

  REQUIRE(div.children.size() == 1);
  CHECK(div.children[0].type == xml::Node::kText);
  CHECK(div.children[0].text == "<escaped> & \"text\" '! < < < 😀");
}

TEST_CASE("XML.EmbeddedNulByteIsRejectedNotMisreadAsEndOfFile", "[xml]") {
  // Parser::Get() returns '\0' both for a real NUL byte and for "past the
  // end of input". An embedded NUL used to be silently misread as an early
  // end of file, truncating the parse instead of failing clearly.
  std::string input = "<div>";
  input += '\0';
  input += "</div>";

  auto nodes = xml::Parse(input);
  REQUIRE_FALSE(nodes.has_value());
  CHECK(nodes.error().message == "Invalid NUL character in input");
}

TEST_CASE("XML.StyleContentIsRawText", "[xml]") {
  // <style> holds CSS, not markup. The parser used to descend into it looking
  // for tags, so any '<' in a CSS comment or selector was mistaken for an
  // element and aborted the parse -- e.g. mentioning a tag name in a comment.
  SECTION("a tag name inside a CSS comment") {
    auto nodes = xml::Parse(
        "<div><style>/* like the <dialog> root */ .a { color: red; }"
        "</style></div>");
    REQUIRE(nodes.has_value());
    REQUIRE(nodes.value().size() == 1);

    const auto& style = nodes.value()[0].children.at(0);
    REQUIRE(style.tag == "style");
    REQUIRE(style.children.size() == 1);
    CHECK(style.children[0].type == xml::Node::kText);
    CHECK(style.children[0].text ==
          "/* like the <dialog> root */ .a { color: red; }");
  }

  SECTION("a child combinator in a selector") {
    auto nodes = xml::Parse("<style>.a > .b { color: red; }</style>");
    REQUIRE(nodes.has_value());
    REQUIRE(nodes.value().size() == 1);
    REQUIRE(nodes.value()[0].children.size() == 1);
    CHECK(nodes.value()[0].children[0].text == ".a > .b { color: red; }");
  }

  SECTION("CSS is not entity-unescaped") {
    auto nodes =
        xml::Parse("<style>.a::before { content: \"&amp;\"; }</style>");
    REQUIRE(nodes.has_value());
    CHECK(nodes.value()[0].children[0].text ==
          ".a::before { content: \"&amp;\"; }");
  }

  SECTION("an unterminated style block is an error, not a truncated parse") {
    auto nodes = xml::Parse("<div><style>.a { color: red; }</div>");
    CHECK_FALSE(nodes.has_value());
  }

  SECTION("a mismatched closing tag is still an error") {
    // The scan for the end of the raw text looks for the "</style" prefix, so
    // a longer tag name starting with it must not be mistaken for the close.
    auto nodes = xml::Parse("<style>.a { color: red; }</stylesheet>");
    CHECK_FALSE(nodes.has_value());
  }
}
