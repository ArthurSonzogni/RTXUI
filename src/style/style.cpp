// Copyright 2024 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include "css/css.hpp"

#include <string_view>
#include <vector>

#include "core/expected.hpp"

namespace css {

namespace {

class Parser {
 public:
  Parser(std::string_view css) : css_(css) {}

  auto Get() -> char;
  auto Get(int offset) -> char;
  auto Advance() -> void;

  auto ParseStyleSheet() -> Expected<StyleSheet, Error>;
  auto ParseRuleset() -> Expected<Ruleset, Error>;
  auto ParseSelector() -> Expected<std::string_view, Error>;
  auto ParseDeclaration() -> Expected<Declaration, Error>;
  auto ParseValue() -> Expected<std::string_view, Error>;
  auto ParseWhiteSpaces() -> void;

  auto MakeError(std::string message) -> Error;
  auto MakeErrorExpected(std::string expected) -> Error;

 private:
  std::string_view css_;
  size_t pos_ = 0;
};

bool Contains(char c, const std::vector<char>& chars) {
  for (char x : chars) {
    if (c == x) {
      return true;
    }
  }
  return false;
}

auto Parser::Advance() -> void {
  ++pos_;
}

auto Parser::Get() -> char {
  return pos_ >= css_.size() ? '\0' : css_[pos_];
}

auto Parser::Get(int offset) -> char {
  return pos_ + offset >= css_.size() ? '\0' : css_[pos_ + offset];
}

auto Parser::ParseWhiteSpaces() -> void {
  while (Contains(Get(), {' ', '\n', '\r', '\t'})) {
    Advance();
  }
}

auto Parser::ParseSelector() -> Expected<std::string_view, Error> {
  int start = pos_;
  while (Get() != '\0' && !Contains(Get(), {'{', ' ', '\n', '\t'})) {
    Advance();
  }
  if (start == pos_) {
    return MakeErrorExpected("selector");
  }
  return css_.substr(start, pos_ - start);
}

auto Parser::ParseValue() -> Expected<std::string_view, Error> {
  int start = pos_;
  while (Get() != '\0' && !Contains(Get(), {';', '}'})) {
    Advance();
  }

  std::string_view val = css_.substr(start, pos_ - start);
  while (!val.empty() && Contains(val.back(), {' ', '\n', '\t', '\r'})) {
    val.remove_suffix(1);
  }
  return val;
}

auto Parser::ParseDeclaration() -> Expected<Declaration, Error> {
  ParseWhiteSpaces();
  int start = pos_;
  while (Get() != '\0' && !Contains(Get(), {':', ' ', '\n', '\t', '}'})) {
    Advance();
  }
  std::string_view property = css_.substr(start, pos_ - start);
  if (property.empty()) {
    return MakeErrorExpected("property");
  }

  ParseWhiteSpaces();
  if (Get() != ':') {
    return MakeErrorExpected("':'");
  }
  Advance();  // Skip ':'
  ParseWhiteSpaces();

  auto value = ParseValue();
  if (!value) {
    return value.error();
  }

  if (Get() == ';') {
    Advance();  // Skip ';'
  }

  return Declaration{property, value.value()};
}

auto Parser::ParseRuleset() -> Expected<Ruleset, Error> {
  ParseWhiteSpaces();
  auto selector = ParseSelector();
  if (!selector) {
    return selector.error();
  }

  ParseWhiteSpaces();
  if (Get() != '{') {
    return MakeErrorExpected("'{'");
  }
  Advance();  // Skip '{'

  std::vector<Declaration> declarations;
  while (true) {
    ParseWhiteSpaces();
    if (Get() == '}' || Get() == '\0') {
      break;
    }

    auto decl = ParseDeclaration();
    if (!decl) {
      return decl.error();
    }
    declarations.push_back(decl.value());
  }

  if (Get() != '}') {
    return MakeErrorExpected("'}'");
  }
  Advance();  // Skip '}'

  return Ruleset{selector.value(), declarations};
}

auto Parser::ParseStyleSheet() -> Expected<StyleSheet, Error> {
  StyleSheet stylesheet;
  while (true) {
    ParseWhiteSpaces();
    if (Get() == '\0') {
      break;
    }

    auto ruleset = ParseRuleset();
    if (!ruleset) {
      return ruleset.error();
    }
    stylesheet.push_back(ruleset.value());
  }
  return stylesheet;
}

auto Parser::MakeError(std::string message) -> Error {
  int line = 0;
  int column = 0;
  for (size_t i = 0; i < pos_; ++i) {
    if (css_[i] == '\n') {
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
  return MakeError("Expected " + expected + ", but got '" +
                   std::string(1, Get()) + "'");
}

}  // namespace

auto Parse(std::string_view css) -> Expected<StyleSheet, Error> {
  Parser parser(css);
  return parser.ParseStyleSheet();
}

}  // namespace css
