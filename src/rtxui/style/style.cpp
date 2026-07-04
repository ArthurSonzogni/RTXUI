// Copyright 2024 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include "rtxui/style/style.hpp"

#include <string_view>
#include <vector>

#include "rtxui/base/expected.hpp"

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
  auto ParseRuleset(const std::vector<std::string>& parent_selectors,
                    std::string_view media_query)
      -> Expected<std::vector<Ruleset>, Error>;
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

namespace {
SelectorPart ParseSinglePart(std::string_view current) {
  SelectorPart part;
  std::string working(current);
  size_t attr_start;
  while ((attr_start = working.find('[')) != std::string::npos) {
    size_t attr_end = working.find(']', attr_start);
    if (attr_end == std::string::npos) break;

    std::string attr_content = working.substr(attr_start + 1, attr_end - attr_start - 1);
    AttributeSelector attr;
    size_t eq = attr_content.find('=');
    if (eq != std::string::npos) {
      attr.name = attr_content.substr(0, eq);
      attr.value = attr_content.substr(eq + 1);
      attr.has_value = true;
      if (!attr.value.empty() && (attr.value.front() == '"' || attr.value.front() == '\'')) {
        attr.value = attr.value.substr(1);
      }
      if (!attr.value.empty() && (attr.value.back() == '"' || attr.value.back() == '\'')) {
        attr.value.pop_back();
      }
    } else {
      attr.name = attr_content;
      attr.has_value = false;
    }
    part.attributes.push_back(std::move(attr));
    working.erase(attr_start, attr_end - attr_start + 1);
  }

  std::string_view remaining(working);
  while (!remaining.empty() && IsWhiteSpace(remaining.front())) remaining.remove_prefix(1);
  while (!remaining.empty() && IsWhiteSpace(remaining.back())) remaining.remove_suffix(1);

  size_t dot = remaining.find('.');
  size_t hash = remaining.find('#');
  size_t split_pos = std::min(dot, hash);

  if (split_pos == std::string_view::npos) {
    part.base = std::string(remaining);
  } else {
    part.base = std::string(remaining.substr(0, split_pos));
    std::string_view rest_part = remaining.substr(split_pos);
    while (!rest_part.empty()) {
      if (rest_part.front() == '.') {
        rest_part.remove_prefix(1);
        size_t next = rest_part.find_first_of(".#");
        part.classes.push_back(std::string(rest_part.substr(0, next)));
        if (next == std::string_view::npos) break;
        rest_part = rest_part.substr(next);
      } else if (rest_part.front() == '#') {
        rest_part.remove_prefix(1);
        size_t next = rest_part.find_first_of(".#");
        part.id = std::string(rest_part.substr(0, next));
        if (next == std::string_view::npos) break;
        rest_part = rest_part.substr(next);
      } else {
        break;
      }
    }
  }
  return part;
}
} // namespace

auto ParseSelectorString(std::string_view current) -> ParsedSelector {
  while (!current.empty() && IsWhiteSpace(current.front())) {
    current.remove_prefix(1);
  }
  while (!current.empty() && IsWhiteSpace(current.back())) {
    current.remove_suffix(1);
  }

  std::vector<std::string_view> parts;
  std::vector<char> combinators;
  
  std::string_view rest = current;
  while (!rest.empty()) {
    while (!rest.empty() && IsWhiteSpace(rest.front())) {
      rest.remove_prefix(1);
    }
    if (rest.empty()) break;
    
    size_t next_space = rest.find_first_of(" \n\r\t>+~");
    if (next_space == std::string_view::npos) {
      parts.push_back(rest);
      break;
    }

    parts.push_back(rest.substr(0, next_space));
    char comb = rest[next_space];
    rest.remove_prefix(next_space);

    if (IsWhiteSpace(comb)) {
      while (!rest.empty() && IsWhiteSpace(rest.front())) {
        rest.remove_prefix(1);
      }
      if (!rest.empty() && Contains(rest.front(), {'>', '+', '~'})) {
        comb = rest.front();
        rest.remove_prefix(1);
      } else {
        comb = ' ';
      }
    } else if (Contains(comb, {'>', '+', '~'})) {
      rest.remove_prefix(1);
    }

    combinators.push_back(comb);
  }

  ParsedSelector parsed;
  if (parts.empty()) return parsed;
  
  std::string_view target_str = parts.back();
  size_t colon = target_str.find(':');
  std::string_view target_without_pseudos = target_str;
  if (colon != std::string_view::npos) {
    target_without_pseudos = target_str.substr(0, colon);
  }
  
  SelectorPart target_part = ParseSinglePart(target_without_pseudos);
  parsed.base = std::move(target_part.base);
  parsed.id = std::move(target_part.id);
  parsed.classes = std::move(target_part.classes);
  parsed.attributes = std::move(target_part.attributes);
  
  if (colon != std::string_view::npos) {
    std::string_view rest_pseudos = target_str.substr(colon);
    while (!rest_pseudos.empty() && rest_pseudos.front() == ':') {
      rest_pseudos.remove_prefix(1);
      size_t next_colon = rest_pseudos.find(':');
      std::string_view pseudo = rest_pseudos.substr(0, next_colon);
      while (!pseudo.empty() && IsWhiteSpace(pseudo.front())) {
        pseudo.remove_prefix(1);
      }
      while (!pseudo.empty() && IsWhiteSpace(pseudo.back())) {
        pseudo.remove_suffix(1);
      }
      parsed.pseudo_classes.push_back(std::string(pseudo));
      if (next_colon == std::string_view::npos) {
        break;
      }
      rest_pseudos = rest_pseudos.substr(next_colon);
    }
  }
  
  for (int i = static_cast<int>(parts.size()) - 2; i >= 0; --i) {
    SelectorPart parent_part = ParseSinglePart(parts[i]);
    parent_part.combinator = combinators[i];
    parsed.parents.push_back(std::move(parent_part));
  }
  
  return parsed;
}

std::string CombineSelectors(const std::string& parent, std::string_view child) {
  if (parent.empty()) return std::string(child);
  std::string result;
  size_t pos = child.find('&');
  if (pos == std::string_view::npos) {
    return parent + " " + std::string(child);
  }

  std::string_view rest = child;
  while (true) {
    size_t ampersand = rest.find('&');
    if (ampersand == std::string_view::npos) {
      result += rest;
      break;
    }
    result += rest.substr(0, ampersand);
    result += parent;
    rest.remove_prefix(ampersand + 1);
  }
  return result;
}

auto Parser::ParseRuleset() -> Expected<std::vector<Ruleset>, Error> {
  return ParseRuleset({}, "");
}

auto Parser::ParseRuleset(const std::vector<std::string>& parent_selectors,
                          std::string_view media_query)
    -> Expected<std::vector<Ruleset>, Error> {
  ParseWhiteSpaces();
  auto selector_full = ParseSelector();
  if (!selector_full) {
    return selector_full.error();
  }

  // Generate current selectors
  std::vector<std::string> current_selectors;
  std::string_view rest_selectors = selector_full.value();
  while (!rest_selectors.empty()) {
    size_t comma = rest_selectors.find(',');
    std::string_view part = (comma == std::string_view::npos)
                                ? rest_selectors
                                : rest_selectors.substr(0, comma);
    while (!part.empty() && IsWhiteSpace(part.front())) part.remove_prefix(1);
    while (!part.empty() && IsWhiteSpace(part.back())) part.remove_suffix(1);

    if (parent_selectors.empty()) {
      current_selectors.push_back(std::string(part));
    } else {
      for (const auto& parent : parent_selectors) {
        current_selectors.push_back(CombineSelectors(parent, part));
      }
    }

    if (comma == std::string_view::npos) break;
    rest_selectors = rest_selectors.substr(comma + 1);
  }

  ParseWhiteSpaces();
  if (Get() != '{') {
    return MakeErrorExpected("'{'");
  }
  Advance();  // Skip '{'

  std::vector<Ruleset> rulesets;
  std::vector<Declaration> declarations;
  std::vector<Ruleset> nested_rulesets_all;

  while (true) {
    ParseWhiteSpaces();
    if (Get() == '}' || Get() == '\0') {
      break;
    }

    // Heuristic: determine if we should try parsing as a declaration.
    bool has_lbrace = false;
    size_t i = pos_;
    while (i < css_.size() && css_[i] != ';' && css_[i] != '}') {
      if (css_[i] == '{') {
        has_lbrace = true;
        break;
      }
      i++;
    }

    bool is_declaration = false;
    size_t saved_pos = pos_;

    if (!has_lbrace) {
      // No '{' before ';' or '}'. It MUST be a declaration (or a syntax error in one).
      is_declaration = true;
    } else {
      // It has a '{'. Try parsing as a declaration to see if it's a binding.
      auto decl = ParseDeclaration();
      if (decl) {
        std::string_view val = decl.value().value;
        if (val.find('{') != std::string_view::npos) {
          if (val.front() == '{' && val.back() == '}' && val.find(';') == std::string_view::npos) {
            is_declaration = true;
          }
        } else {
          is_declaration = true;
        }
      }
    }
    
    pos_ = saved_pos; // Restore to branch accordingly

    if (!is_declaration) {
      auto nested_rules = ParseRuleset(current_selectors, media_query);
      if (!nested_rules) return nested_rules.error();
      for (auto& r : nested_rules.value()) {
        nested_rulesets_all.push_back(std::move(r));
      }
    } else {
      auto decl = ParseDeclaration(); // Re-parse to consume and get value
      if (!decl) return decl.error();
      declarations.push_back(decl.value());
    }
  }

  if (Get() != '}') {
    return MakeErrorExpected("'}'");
  }
  Advance();  // Skip '}'

  // If we have declarations, add rulesets for the current selectors.
  if (!declarations.empty()) {
    for (const auto& sel : current_selectors) {
      rulesets.push_back(Ruleset{sel, declarations, std::string(media_query), ParsedSelector{}});
      rulesets.back().parsed_selector = ParseSelectorString(rulesets.back().selector);
    }
  }

  for (auto& r : nested_rulesets_all) {
    rulesets.push_back(std::move(r));
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
    auto [ptr, ec] = std::from_chars(val_str.data(), val_str.data() + val_str.size(), val);
    if (ec != std::errc() || ptr != val_str.data() + val_str.size()) {
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
      result += "@media " + ruleset.media_query + " {\n  ";
    }
    result += ruleset.selector + " {\n";
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

auto SubstituteVars(std::string_view value, const CustomProperties& properties)
    -> std::optional<std::string> {
  auto trim = [](std::string_view s) {
    while (!s.empty() && (s.front() == ' ' || s.front() == '\t')) {
      s.remove_prefix(1);
    }
    while (!s.empty() && (s.back() == ' ' || s.back() == '\t')) {
      s.remove_suffix(1);
    }
    return s;
  };

  std::string result(value);
  // Each substitution may itself introduce new var() references (from a
  // property value or a fallback); bound the total number of expansions so
  // self-referential variables cannot loop forever.
  for (int budget = 0; budget < 16; ++budget) {
    size_t pos = result.find("var(");
    if (pos == std::string::npos) {
      return result;
    }

    // Find the matching closing parenthesis (fallbacks may nest var()).
    size_t end = pos + 4;
    int nesting = 1;
    while (end < result.size() && nesting > 0) {
      if (result[end] == '(') {
        ++nesting;
      } else if (result[end] == ')') {
        --nesting;
      }
      ++end;
    }
    if (nesting != 0) {
      return std::nullopt;  // Unbalanced parentheses.
    }

    std::string_view inner =
        std::string_view(result).substr(pos + 4, end - 1 - (pos + 4));
    std::string_view name = inner;
    std::string_view fallback;
    bool has_fallback = false;
    size_t comma = inner.find(',');
    if (comma != std::string_view::npos) {
      name = inner.substr(0, comma);
      fallback = inner.substr(comma + 1);
      has_fallback = true;
    }
    name = trim(name);
    fallback = trim(fallback);

    std::string replacement;
    auto it = properties.find(name);
    if (it != properties.end()) {
      replacement = it->second;
    } else if (has_fallback) {
      replacement = std::string(fallback);
    } else {
      return std::nullopt;  // Undefined variable without fallback.
    }

    result = result.substr(0, pos) + replacement + result.substr(end);
  }
  return std::nullopt;  // Expansion did not terminate.
}

}  // namespace css
