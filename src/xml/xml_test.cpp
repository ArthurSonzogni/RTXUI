// Copyright 2024 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include "xml/xml.hpp"

#include <catch2/catch_test_macros.hpp>
#include <string>

#include "core/string.hpp"

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
