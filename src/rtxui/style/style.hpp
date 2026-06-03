// Copyright 2024 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#ifndef CSS_HPP_
#define CSS_HPP_

#include <string>
#include <string_view>
#include <vector>

#include "rtxui/core/expected.hpp"

namespace css {

/// A single CSS declaration (property: value).
struct Declaration {
  std::string_view property;
  std::string_view value;
};

/// A CSS ruleset (selector { declarations }).
struct Ruleset {
  /// The selector name (e.g., "div", ".class", "#id").
  std::string_view selector;
  /// The list of declarations inside the ruleset.
  std::vector<Declaration> declarations;
  /// The media query condition (e.g., "(max-width: 80)"), empty if none.
  std::string_view media_query;
};

/// A stylesheet is a collection of rulesets.
using StyleSheet = std::vector<Ruleset>;

extern thread_local int g_terminal_width;
extern thread_local int g_terminal_height;

/// Evaluate the given media query condition against the current terminal size.
auto EvaluateMediaQuery(std::string_view query) -> bool;

/// The error object, which contains the error message, line, and column.
struct Error {
  /// The error message.
  std::string message;

  /// The line where the error occurred. 0-based.
  int line;

  /// The column where the error occurred. 0-based.
  int column;
};

/// Parse the given CSS string and return the stylesheet.
/// If the CSS is invalid, return an error.
auto Parse(std::string_view css) -> Expected<StyleSheet, Error>;

/// Parse the given inline CSS style string and return the declarations.
/// If the CSS is invalid, return an error.
auto ParseDeclarations(std::string_view css) -> Expected<std::vector<Declaration>, Error>;

/// Print the stylesheet (useful for debugging).
auto Print(const StyleSheet& stylesheet) -> std::string;

}  // namespace css

#endif  // CSS_HPP_
