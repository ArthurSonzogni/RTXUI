// Copyright 2024 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#ifndef CSS_HPP_
#define CSS_HPP_

#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "rtxui/base/expected.hpp"

namespace css {

/// A single CSS declaration (property: value).
struct Declaration {
  std::string_view property;
  std::string_view value;
  /// Set when the value carried an `!important` suffix. Important
  /// declarations win over normal ones regardless of rule order.
  bool important = false;
};

struct AttributeSelector {
  std::string name;
  std::string value;
  bool has_value = false;
};

struct SelectorPart {
  std::string base;
  std::string id;
  std::vector<std::string> classes;
  std::vector<AttributeSelector> attributes;
  char combinator = ' '; // ' ' or '>'
};

/// Parses one compound selector (`div.card#id[attr]`) with no combinators and
/// no pseudo-classes. Exposed for `:not()`, which has to match a selector from
/// inside the matcher.
SelectorPart ParseSinglePart(std::string_view text);

/// Parses one compound selector that may carry pseudo-classes
/// (`div.card:hover:not(.x)`), returning the non-pseudo part and appending each
/// pseudo-class -- argument included, e.g. `not(.x)` -- to `pseudo_classes`.
/// Colons nested inside `(...)` or `[...]` do not split a token, so
/// `:not(:first-child)` stays whole. Exposed for `:not()`, whose argument is
/// itself a compound selector with pseudo-classes.
SelectorPart ParseCompound(std::string_view text,
                           std::vector<std::string>& pseudo_classes);

struct ParsedSelector {
  std::string base;
  std::string id;
  std::vector<std::string> classes;
  std::vector<std::string> pseudo_classes;
  std::vector<AttributeSelector> attributes;
  std::vector<SelectorPart> parents; // Ancestors in right-to-left order
  // From a trailing `::part(name)`, e.g. `textarea::part(gutter)`. Empty
  // when the selector doesn't use ::part(). base/id/classes/attributes
  // above describe the *host* (the component instantiation site, e.g.
  // `textarea`), not the part element itself -- ::part() reaches into
  // another component's own template to select an element it has marked
  // with a matching `part="..."` attribute (see docs/guide/css/basics.md).
  std::string part;

  /// CSS specificity, packed so it compares as a single integer: ids weigh
  /// most, then classes/attributes/pseudo-classes, then element names. It is
  /// summed over the whole selector, ancestors included, so `.card .title`
  /// outranks `.title` and `.a.b` outranks `.a` -- which a scheme that only
  /// looked at the selector's rightmost part, or only at its "kind", cannot
  /// express. The `style` attribute outranks every selector and is applied
  /// separately, so it needs no value here.
  int specificity = 0;
};

/// Packs the CSS (id, class, type) counts into one comparable integer. The
/// per-bucket range is deliberately wide: a selector would need 1024 classes
/// before it could borrow from the id column.
constexpr int MakeSpecificity(int ids, int classes, int types) {
  return (ids << 20) | (classes << 10) | types;
}

/// A CSS ruleset (selector { declarations }).
struct Ruleset {
  /// The selector name (e.g., "div", ".class", "#id").
  std::string selector;
  /// The list of declarations inside the ruleset.
  std::vector<Declaration> declarations;
  /// The media query condition (e.g., "(max-width: 80)"), empty if none.
  std::string media_query;
  /// Pre-parsed selector representation.
  ParsedSelector parsed_selector;
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

/// Resolved CSS custom properties ("--name" -> value), e.g. per element.
using CustomProperties = std::map<std::string, std::string, std::less<>>;

/// Expand every `var(--name)` / `var(--name, fallback)` occurrence in the
/// given declaration value. Returns std::nullopt when a referenced variable
/// is undefined and has no fallback (the declaration is then invalid and
/// should be ignored), or when substitution does not terminate (cycles).
auto SubstituteVars(std::string_view value, const CustomProperties& properties)
    -> std::optional<std::string>;

}  // namespace css

#endif  // CSS_HPP_
