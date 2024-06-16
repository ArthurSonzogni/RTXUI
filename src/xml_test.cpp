#include "./xml.hpp"
#include <catch2/catch_test_macros.hpp>
#include <string>

namespace {

// Split a string by a delimiter, return a vector of string views.
auto Split(std::string_view text,
           char delimiter) -> std::vector<std::string_view> {
  std::vector<std::string_view> result;
  size_t start = 0;
  for (size_t i = 0; i < text.size(); ++i) {
    if (text[i] == delimiter) {
      result.push_back(text.substr(start, i - start));
      start = i + 1;
    }
  }
  result.push_back(text.substr(start));
  return result;
}

// Strip the maximum indentation on a string. Every line should have the same
// amount of leading spaces removed.
std::string StripIndent(const std::string_view& text) {
  int min_indent = std::numeric_limits<int>::max();
  for (const auto& line : Split(text, '\n')) {
    // Skip empty lines:
    bool is_empty = true;
    for (char c : line) {
      if (c != ' ') {
        is_empty = false;
        break;
      }
    }
    if (is_empty) {
      continue;
    }

    int indent = 0;
    for (char c : line) {
      if (c == ' ') {
        ++indent;
      } else {
        break;
      }
    }
    min_indent = std::min(min_indent, indent);
  }

  std::string result;
  for (const auto& line : Split(text, '\n')) {
    // Skip empty lines:
    bool is_empty = true;
    for (char c : line) {
      if (c != ' ') {
        is_empty = false;
        break;
      }
    }
    if (is_empty) {
      continue;
    }
    result += std::string(line.substr(min_indent)) + "\n";
  }

  return result;
}

}  // namespace

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
  CHECK(!nodes);
  REQUIRE(nodes.error().message == "Expected tag name, but got >");
  REQUIRE(nodes.error().line == 1);
  REQUIRE(nodes.error().column == 5);
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
