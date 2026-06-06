// Copyright 2024 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include "rtxui/style/style.hpp"

#include <string_view>
#include <vector>

#include "rtxui/core/expected.hpp"

namespace css {

namespace {

class Parser {
 public:
  Parser(std::string_view css) : css_(css) {}

  auto Get() -> char;
  auto Get(int offset) -> char;
  auto Advance() -> void;

  auto ParseStyleSheet() -> Expected<StyleSheet, Error>;
  auto ParseRuleset() -> Expected<std::vector<Ruleset>, Error>;
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

bool IsWhiteSpace(char c) {
  return c == ' ' || c == '\n' || c == '\r' || c == '\t';
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
  while (true) {
    // 1. Skip standard whitespace.
    if (IsWhiteSpace(Get())) {
      Advance();
      continue;
    }

    // 2. Skip comments /* ... */
    if (Get() == '/' && Get(1) == '*') {
      Advance();  // Skip '/'
      Advance();  // Skip '*'
      while (Get() != '\0') {
        if (Get() == '*' && Get(1) == '/') {
          Advance();  // Skip '*'
          Advance();  // Skip '/'
          break;
        }
        Advance();
      }
      continue;
    }

    break;
  }
}

auto Parser::ParseSelector() -> Expected<std::string_view, Error> {
  int start = pos_;

  // Read until we hit the opening brace '{' or EOF.
  while (Get() != '\0' && Get() != '{') {
    Advance();
  }

  // Trim trailing whitespace.
  int end = pos_;
  while (end > start && IsWhiteSpace(css_[end - 1])) {
    end--;
  }

  if (start == end) {
    return MakeErrorExpected("selector");
  }

  return css_.substr(start, end - start);
}

auto Parser::ParseValue() -> Expected<std::string_view, Error> {
  int start = pos_;
  int brace_depth = 0;
  while (Get() != '\0') {
    char c = Get();
    if (brace_depth == 0 && Contains(c, {';', '}'})) {
      break;
    }

    if (c == '{') {
      brace_depth++;
    } else if (c == '}') {
      if (brace_depth > 0) {
        brace_depth--;
      }
    }
    Advance();
  }

  std::string_view val = css_.substr(start, pos_ - start);
  while (!val.empty() && IsWhiteSpace(val.back())) {
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

auto ParseSelectorString(std::string_view selector_str) -> ParsedSelector {
  std::string_view current = selector_str;
  while (!current.empty() && IsWhiteSpace(current.front())) {
    current.remove_prefix(1);
  }
  while (!current.empty() && IsWhiteSpace(current.back())) {
    current.remove_suffix(1);
  }

  ParsedSelector parsed;
  size_t colon = current.find(':');
  std::string_view base_and_classes;
  if (colon == std::string_view::npos) {
    base_and_classes = current;
  } else {
    base_and_classes = current.substr(0, colon);
  }

  while (!base_and_classes.empty() && IsWhiteSpace(base_and_classes.back())) {
    base_and_classes.remove_suffix(1);
  }

  size_t dot = base_and_classes.find('.');
  if (dot == std::string_view::npos) {
    parsed.base = base_and_classes;
  } else if (dot == 0) {
    parsed.base = base_and_classes;
  } else {
    parsed.base = base_and_classes.substr(0, dot);
    std::string_view remaining_classes = base_and_classes.substr(dot);
    while (!remaining_classes.empty() && remaining_classes.front() == '.') {
      remaining_classes.remove_prefix(1);
      size_t next_dot = remaining_classes.find('.');
      std::string_view cls = remaining_classes.substr(0, next_dot);
      parsed.classes.push_back(cls);
      if (next_dot == std::string_view::npos) break;
      remaining_classes = remaining_classes.substr(next_dot);
    }
  }

  if (colon == std::string_view::npos) return parsed;

  std::string_view rest = current.substr(colon);
  while (!rest.empty() && rest.front() == ':') {
    rest.remove_prefix(1);
    size_t next_colon = rest.find(':');
    std::string_view pseudo = rest.substr(0, next_colon);
    while (!pseudo.empty() && IsWhiteSpace(pseudo.front())) {
      pseudo.remove_prefix(1);
    }
    while (!pseudo.empty() && IsWhiteSpace(pseudo.back())) {
      pseudo.remove_suffix(1);
    }
    parsed.pseudo_classes.push_back(pseudo);
    if (next_colon == std::string_view::npos) {
      break;
    }
    rest = rest.substr(next_colon);
  }
  return parsed;
}

auto Parser::ParseRuleset() -> Expected<std::vector<Ruleset>, Error> {
  ParseWhiteSpaces();
  auto selector_full = ParseSelector();
  if (!selector_full) {
    return selector_full.error();
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

  std::vector<Ruleset> rulesets;
  std::string_view rest = selector_full.value();
  while (!rest.empty()) {
    size_t comma = rest.find(',');
    std::string_view part = (comma == std::string_view::npos) ? rest : rest.substr(0, comma);
    
    auto parsed_sel = ParseSelectorString(part);
    rulesets.push_back(Ruleset{part, declarations, "", std::move(parsed_sel)});

    if (comma == std::string_view::npos) break;
    rest = rest.substr(comma + 1);
  }

  return rulesets;
}

auto Parser::ParseStyleSheet() -> Expected<StyleSheet, Error> {
  StyleSheet stylesheet;
  while (true) {
    ParseWhiteSpaces();
    if (Get() == '\0') {
      break;
    }

    if (Get() == '@') {
      int start = pos_;
      while (Get() != '\0' && !IsWhiteSpace(Get()) && Get() != '(') {
        Advance();
      }
      std::string_view keyword = css_.substr(start, pos_ - start);
      if (keyword != "@media") {
        return MakeError("Unexpected directive: " + std::string(keyword));
      }
      ParseWhiteSpaces();

      int cond_start = pos_;
      while (Get() != '\0' && Get() != '{') {
        Advance();
      }
      std::string_view media_query = css_.substr(cond_start, pos_ - cond_start);
      // Trim whitespaces
      while (!media_query.empty() && IsWhiteSpace(media_query.front())) {
        media_query.remove_prefix(1);
      }
      while (!media_query.empty() && IsWhiteSpace(media_query.back())) {
        media_query.remove_suffix(1);
      }

      if (Get() != '{') {
        return MakeErrorExpected("'{'");
      }
      Advance();  // Skip '{'

      while (true) {
        ParseWhiteSpaces();
        if (Get() == '}' || Get() == '\0') {
          break;
        }
        auto rulesets = ParseRuleset();
        if (!rulesets) {
          return rulesets.error();
        }
        for (auto& ruleset_val : rulesets.value()) {
          ruleset_val.media_query = media_query;
          stylesheet.push_back(std::move(ruleset_val));
        }
      }

      ParseWhiteSpaces();
      if (Get() != '}') {
        return MakeErrorExpected("'}'");
      }
      Advance();  // Skip '}'
      continue;
    }

    auto rulesets = ParseRuleset();
    if (!rulesets) {
      return rulesets.error();
    }
    for (auto& ruleset_val : rulesets.value()) {
      stylesheet.push_back(std::move(ruleset_val));
    }
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

thread_local int g_terminal_width = 80;
thread_local int g_terminal_height = 24;

auto EvaluateMediaQuery(std::string_view query) -> bool {
  if (query.empty()) {
    return true;
  }

  std::vector<std::string_view> parts;
  size_t start = 0;
  while (true) {
    size_t pos = query.find("and", start);
    if (pos == std::string_view::npos) {
      parts.push_back(query.substr(start));
      break;
    }
    bool before_ok =
        (pos == 0 || IsWhiteSpace(query[pos - 1]) || query[pos - 1] == ')');
    bool after_ok = (pos + 3 >= query.size() || IsWhiteSpace(query[pos + 3]) ||
                     query[pos + 3] == '(');
    if (before_ok && after_ok) {
      parts.push_back(query.substr(start, pos - start));
      start = pos + 3;
    } else {
      start = pos + 3;
    }
  }

  for (auto part : parts) {
    while (!part.empty() &&
           (IsWhiteSpace(part.front()) || part.front() == '(')) {
      part.remove_prefix(1);
    }
    while (!part.empty() && (IsWhiteSpace(part.back()) || part.back() == ')')) {
      part.remove_suffix(1);
    }
    if (part.empty()) {
      continue;
    }

    size_t colon = part.find(':');
    if (colon == std::string_view::npos) {
      if (part == "screen" || part == "all") {
        continue;
      }
      return false;
    }

    std::string_view key = part.substr(0, colon);
    std::string_view val_str = part.substr(colon + 1);

    while (!key.empty() && IsWhiteSpace(key.front())) {
      key.remove_prefix(1);
    }
    while (!key.empty() && IsWhiteSpace(key.back())) {
      key.remove_suffix(1);
    }
    while (!val_str.empty() && IsWhiteSpace(val_str.front())) {
      val_str.remove_prefix(1);
    }
    while (!val_str.empty() && IsWhiteSpace(val_str.back())) {
      val_str.remove_suffix(1);
    }

    int val = 0;
    try {
      val = std::stoi(std::string(val_str));
    } catch (...) {
      return false;
    }

    if (key == "max-width") {
      if (g_terminal_width > val) {
        return false;
      }
    } else if (key == "min-width") {
      if (g_terminal_width < val) {
        return false;
      }
    } else if (key == "width") {
      if (g_terminal_width != val) {
        return false;
      }
    } else if (key == "max-height") {
      if (g_terminal_height > val) {
        return false;
      }
    } else if (key == "min-height") {
      if (g_terminal_height < val) {
        return false;
      }
    } else if (key == "height") {
      if (g_terminal_height != val) {
        return false;
      }
    } else {
      return false;
    }
  }

  return true;
}

auto Print(const StyleSheet& stylesheet) -> std::string {
  std::string result;
  for (const auto& ruleset : stylesheet) {
    if (!ruleset.media_query.empty()) {
      result += "@media " + std::string(ruleset.media_query) + " {\n  ";
    }
    result += std::string(ruleset.selector) + " {\n";
    for (const auto& decl : ruleset.declarations) {
      if (!ruleset.media_query.empty()) {
        result += "  ";
      }
      result += "  " + std::string(decl.property) + ": " +
                std::string(decl.value) + ";\n";
    }
    if (!ruleset.media_query.empty()) {
      result += "  }\n}\n";
    } else {
      result += "}\n\n";
    }
  }
  return result;
}

}  // namespace css
