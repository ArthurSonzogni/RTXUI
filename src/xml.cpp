#include "xml.hpp"
#include <iostream>

namespace xml {

namespace {

class Parser {
 public:
  Parser(std::string_view xml) : xml_(xml) {}

  auto Get() -> char;
  auto Get(int) -> char;
  auto Advance() -> void;

  auto ParseNode() -> Expected<Node, Error>;
  auto ParseTag() -> Expected<std::string_view, Error>;
  auto ParseAttribute() -> Expected<Attributes, Error>;
  auto ParseWhiteSpaces() -> void;
  auto ParseComment() -> Expected<Node, Error>;

  auto MakeError(std::string message) -> Error;
  auto MakeErrorExpected(std::string expected) -> Error;

 private:
  std::string_view xml_;
  size_t pos_ = 0;
};

static std::string indent = "";

// Helper function to check if a character is Contained in a string.
template <size_t N>
bool Contains(char c, const char (&chars)[N]) {
  for (char g : chars) {
    if (c == g) {
      return true;
    }
  }
  return false;
}

auto Parser::Advance() -> void {
  ++pos_;
}
auto Parser::Get() -> char {
  return pos_ >= xml_.size() ? '\0' : xml_[pos_];
}

auto Parser::Get(int offset) -> char {
  return pos_ + offset >= xml_.size() ? '\0' : xml_[pos_ + offset];
}

auto Parser::ParseWhiteSpaces() -> void {
  indent += " ";
  while (Contains(Get(), " \n\r\t")) {
    Advance();  // Skip white spaces
  }
  indent.pop_back();
}

auto Parser::ParseTag() -> Expected<std::string_view, Error> {
  indent += " ";
  int start = pos_;
  while (!Contains(Get(), "> \n\t=\"/\0")) {
    Advance();
  }
  if (start == pos_) {
    return MakeErrorExpected("tag name");
  }
  std::string_view name = xml_.substr(start, pos_ - start);
  indent.pop_back();
  return name;
}

auto Parser::ParseAttribute() -> Expected<Attributes, Error> {
  indent += " ";
  Attributes attributes;
  while (true) {
    auto g = Get();
    if (Contains(g, ">/")) {
      indent.pop_back();
      return attributes;
    }

    auto key = ParseTag();
    if (!key) {
      return key.error();
    }
    ParseWhiteSpaces();
    if (Get() != '=') {
      return MakeErrorExpected("'='");
    }
    Advance();  // Skip '='.
    ParseWhiteSpaces();
    if (Get() != '"') {
      return MakeErrorExpected("'\"'");
    }
    Advance();  // Skip '"'.
    auto value_start = pos_;
    while (!Contains(Get(), "\"\0")) {
      Advance();
    }
    if (Get() == 0) {
      return MakeErrorExpected("'\"'");
    }
    Advance();  // Skip '"'.
    attributes[key.value()] = xml_.substr(value_start, pos_ - value_start - 1);

    ParseWhiteSpaces();
  }
}

auto Parser::ParseNode() -> Expected<Node, Error> {
  indent += " ";
  ParseWhiteSpaces();

  // Parse text node.
  if (Get() != '<') {
    int start = pos_;
    while (!Contains(Get(), "<\0")) {
      Advance();
    }

    if (Get() != '<') {
      return MakeErrorExpected("<");
    }

    int end = pos_ - 1;
    while (Contains(xml_[end], " \n\t")) {
      --end;
    }

    return Node{
        .type = Node::kText,
        .text = xml_.substr(start, end - start + 1),
        .attributes = {},
        .children = {},
    };
  }

  // Parse Comment:
  if (Get(1) == '!') {
    return ParseComment();
  }

  Advance();  // Skip '<'.

  ParseWhiteSpaces();
  auto tag_opening = ParseTag();
  if (!tag_opening) {
    return tag_opening.error();
  }
  ParseWhiteSpaces();
  auto attributes = ParseAttribute();
  if (!attributes) {
    return attributes.error();
  }

  // Self closing tag.
  if (Get() == '/') {
    Advance();  // Skip '/'.
    if (Get() != '>') {
      return MakeErrorExpected(">");
    }
    Advance();  // Skip '>'.
    indent.pop_back();

    return Node{
        .type = Node::kElement,
        .tag = tag_opening.value(),
        .attributes = attributes.value(),
    };
  }

  // Normally closing tag.
  if (Get() != '>') {
    return MakeErrorExpected(">");
  }
  Advance();  // Skip '>'.
  Nodes children;
  while (true) {
    ParseWhiteSpaces();
    if (Get() == '<' && Get(1) == '/') {
      break;
    }

    auto node = ParseNode();
    if (!node) {
      return node.error();
    }
    children.push_back(node.value());
  }

  Advance();  // Skip '<'.
  Advance();  // Skip '/'.
  ParseWhiteSpaces();
  auto tag_closing = ParseTag();
  if (!tag_closing) {
    return tag_closing.error();
  }
  ParseWhiteSpaces();
  if (Get() != '>') {
    return MakeErrorExpected(">");
  }
  Advance();  // Skip '>'.
  if (tag_opening.value() != tag_closing.value()) {
    return MakeError("Expected closing tag to match opening tag, got " +
                     std::string(tag_closing.value()) + " instead of " +
                     std::string(tag_opening.value()));
  }

  indent.pop_back();
  return Node{
      .type = Node::kElement,
      .tag = tag_opening.value(),
      .attributes = attributes.value(),
      .children = children,
  };
}

auto Parser::ParseComment() -> Expected<Node, Error> {
  indent += " ";
  if (Get() != '<' || Get(1) != '!' || Get(2) != '-' || Get(3) != '-') {
    return MakeErrorExpected("<!--");
  }
  Advance();  // Skip '<'.
  Advance();  // Skip '!'
  Advance();  // Skip '-'
  Advance();  // Skip '-'

  int start = pos_;
  while (true) {
    if (Get() == '-' && Get(1) == '-' && Get(2) == '>') {
      break;
    }
    Advance();
  }

  std::string_view comment = xml_.substr(start, pos_ - start);
  Advance();  // Skip '-'
  Advance();  // Skip '-'
  Advance();  // Skip '>'
  indent.pop_back();
  return Node{
      .type = Node::kComment,
      .text = comment,
      .attributes = {},
      .children = {},
  };
}

auto Parser::MakeError(std::string message) -> Error {
  int line = 0;
  int column = 0;
  for (size_t i = 0; i < pos_; ++i) {
    if (xml_[i] == '\n') {
      ++line;
      column = 0;
    } else {
      ++column;
    }
  }
  return Error{message, line, column};
}

auto Parser::MakeErrorExpected(std::string expected) -> Error {
  if (Get() == 0) {
    return MakeError("Expected " + expected + ", but got end of file");
  }
  return MakeError("Expected " + expected + ", but got " +
                   std::string(1, Get()));
}

}  // namespace

auto Parse(std::string_view xml) -> Expected<Nodes, Error> {
  std::vector<Node> nodes;
  Parser parser(xml);
  while (true) {
    auto node = parser.ParseNode();
    if (!node) {
      return node.error();
    }
    nodes.push_back(node.value());
    break;
  }
  return nodes;
}

std::string Print(const xml::Node& node, int level) {
  switch (node.type) {
    case xml::Node::kText:
      return std::string(level, ' ') + std::string(node.text) + "\n";

    case xml::Node::kComment:
      return std::string(level, ' ') +  //
             "<!--" +                   //
             std::string(node.text) +   //
             "-->\n";                   //

    case xml::Node::kElement:

      // Opening tag.
      std::string result;
      result += std::string(level, ' ') + "<" + std::string(node.tag);

      // Attributes:
      for (const auto& [key, value] : node.attributes) {
        result += " " + std::string(key) + "=\"" + std::string(value) + "\"";
      }

      // Self-closing tag.
      if (node.children.empty()) {
        result += "/>\n";
        return result;
      }

      result += ">\n";

      // Recursive print children:
      for (const auto& child : node.children) {
        result += Print(child, level + 2);
      }

      // Closing tag.
      result += std::string(level, ' ') + "</" + std::string(node.tag) + ">\n";
      return result;
  }
}

}  // namespace xml
