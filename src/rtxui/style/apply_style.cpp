#include "rtxui/style/apply_style.hpp"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <cstdlib>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>

#include "rtxui/paint/color.hpp"

namespace rtxui {
namespace {

float StoF(std::string_view s) {
  while (!s.empty() && std::isspace(static_cast<unsigned char>(s.front()))) {
    s.remove_prefix(1);
  }
  while (!s.empty() && std::isspace(static_cast<unsigned char>(s.back()))) {
    s.remove_suffix(1);
  }
  if (s.empty()) {
    return 0.0f;
  }
  float value = 0.0f;
  auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), value);
  if (ec == std::errc()) {
    return value;
  }
  return 0.0f;
}
int StoI(std::string_view s) {
  while (!s.empty() && std::isspace(static_cast<unsigned char>(s.front()))) {
    s.remove_prefix(1);
  }
  while (!s.empty() && std::isspace(static_cast<unsigned char>(s.back()))) {
    s.remove_suffix(1);
  }
  int value = 0;
  auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), value);
  if (ec == std::errc()) {
    return value;
  }
  return 0;
}

std::vector<std::string_view> SplitWords(std::string_view s) {
  std::vector<std::string_view> words;
  while (!s.empty()) {
    while (!s.empty() && std::isspace(static_cast<unsigned char>(s.front()))) {
      s.remove_prefix(1);
    }
    if (s.empty()) {
      break;
    }
    // A word runs to the next whitespace, but whitespace inside balanced
    // parentheses (e.g. "calc(100% - 4)") does not split.
    size_t end = 0;
    int paren_depth = 0;
    while (end < s.size() &&
           (paren_depth > 0 ||
            !std::isspace(static_cast<unsigned char>(s[end])))) {
      if (s[end] == '(') {
        ++paren_depth;
      } else if (s[end] == ')' && paren_depth > 0) {
        --paren_depth;
      }
      ++end;
    }
    words.push_back(s.substr(0, end));
    s.remove_prefix(end);
  }
  return words;
}

Spacing ParseSpacingShorthand(std::string_view value) {
  auto parts = SplitWords(value);
  Spacing result;
  if (parts.size() == 1) {
    int val = StoI(parts[0]);
    result = {val, val, val, val};
  } else if (parts.size() == 2) {
    int v_val = StoI(parts[0]);
    int h_val = StoI(parts[1]);
    result = {v_val, h_val, v_val, h_val};
  } else if (parts.size() == 3) {
    result.top = StoI(parts[0]);
    result.right = StoI(parts[1]);
    result.left = StoI(parts[1]);
    result.bottom = StoI(parts[2]);
  } else if (parts.size() >= 4) {
    result.top = StoI(parts[0]);
    result.right = StoI(parts[1]);
    result.bottom = StoI(parts[2]);
    result.left = StoI(parts[3]);
  }
  return result;
}

std::optional<Color> ParseColor(std::string_view value) {
  if (value.empty()) {
    return std::nullopt;
  }

  // Parse hex colors: #RGB, #RGBA, #RRGGBB, #RRGGBBAA
  if (value.front() == '#') {
    std::string_view hex = value.substr(1);
    auto hex_val = [](char c) -> std::optional<uint8_t> {
      if (c >= '0' && c <= '9') {
        return c - '0';
      }
      if (c >= 'a' && c <= 'f') {
        return c - 'a' + 10;
      }
      if (c >= 'A' && c <= 'F') {
        return c - 'A' + 10;
      }
      return std::nullopt;
    };

    if (hex.length() == 3 || hex.length() == 4) {
      auto r_digit = hex_val(hex[0]);
      auto g_digit = hex_val(hex[1]);
      auto b_digit = hex_val(hex[2]);
      auto a_digit =
          (hex.length() == 4) ? hex_val(hex[3]) : std::optional<uint8_t>(15);
      if (r_digit && g_digit && b_digit && a_digit) {
        uint8_t r = (*r_digit << 4) | *r_digit;
        uint8_t g = (*g_digit << 4) | *g_digit;
        uint8_t b = (*b_digit << 4) | *b_digit;
        uint8_t a = (*a_digit << 4) | *a_digit;
        return Color::RGBA(r, g, b, a);
      }
    } else if (hex.length() == 6 || hex.length() == 8) {
      bool valid = true;
      uint8_t vals[8] = {0};
      for (size_t i = 0; i < hex.length(); ++i) {
        if (auto digit = hex_val(hex[i])) {
          vals[i] = *digit;
        } else {
          valid = false;
          break;
        }
      }
      if (valid) {
        uint8_t r = (vals[0] << 4) | vals[1];
        uint8_t g = (vals[2] << 4) | vals[3];
        uint8_t b = (vals[4] << 4) | vals[5];
        uint8_t a = (hex.length() == 8) ? ((vals[6] << 4) | vals[7]) : 255;
        return Color::RGBA(r, g, b, a);
      }
    }
    return std::nullopt;
  }

  // Parse rgb(r, g, b)
  if (value.substr(0, 4) == "rgb(" && value.back() == ')') {
    value.remove_prefix(4);
    value.remove_suffix(1);
    size_t first_comma = value.find(',');
    if (first_comma == std::string_view::npos) {
      return std::nullopt;
    }
    size_t second_comma = value.find(',', first_comma + 1);
    if (second_comma == std::string_view::npos) {
      return std::nullopt;
    }
    std::string_view r_str = value.substr(0, first_comma);
    std::string_view g_str =
        value.substr(first_comma + 1, second_comma - first_comma - 1);
    std::string_view b_str = value.substr(second_comma + 1);
    auto trim = [](std::string_view sv) {
      while (!sv.empty() && std::isspace(static_cast<unsigned char>(sv.front()))) {
        sv.remove_prefix(1);
      }
      while (!sv.empty() && std::isspace(static_cast<unsigned char>(sv.back()))) {
        sv.remove_suffix(1);
      }
      return sv;
    };
    r_str = trim(r_str);
    g_str = trim(g_str);
    b_str = trim(b_str);
    int r = 0, g = 0, b = 0;
    auto [r_ptr, r_ec] = std::from_chars(r_str.data(), r_str.data() + r_str.size(), r);
    auto [g_ptr, g_ec] = std::from_chars(g_str.data(), g_str.data() + g_str.size(), g);
    auto [b_ptr, b_ec] = std::from_chars(b_str.data(), b_str.data() + b_str.size(), b);
    if (r_ec == std::errc() && r_ptr == r_str.data() + r_str.size() &&
        g_ec == std::errc() && g_ptr == g_str.data() + g_str.size() &&
        b_ec == std::errc() && b_ptr == b_str.data() + b_str.size()) {
      return Color::RGB(r, g, b);
    }
    return std::nullopt;
  }

  // Parse rgba(r, g, b, a)
  if (value.substr(0, 5) == "rgba(" && value.back() == ')') {
    value.remove_prefix(5);
    value.remove_suffix(1);
    size_t first_comma = value.find(',');
    if (first_comma == std::string_view::npos) {
      return std::nullopt;
    }
    size_t second_comma = value.find(',', first_comma + 1);
    if (second_comma == std::string_view::npos) {
      return std::nullopt;
    }
    size_t third_comma = value.find(',', second_comma + 1);
    if (third_comma == std::string_view::npos) {
      return std::nullopt;
    }
    std::string_view r_str = value.substr(0, first_comma);
    std::string_view g_str =
        value.substr(first_comma + 1, second_comma - first_comma - 1);
    std::string_view b_str =
        value.substr(second_comma + 1, third_comma - second_comma - 1);
    std::string_view a_str = value.substr(third_comma + 1);
    auto trim = [](std::string_view sv) {
      while (!sv.empty() && std::isspace(static_cast<unsigned char>(sv.front()))) {
        sv.remove_prefix(1);
      }
      while (!sv.empty() && std::isspace(static_cast<unsigned char>(sv.back()))) {
        sv.remove_suffix(1);
      }
      return sv;
    };
    r_str = trim(r_str);
    g_str = trim(g_str);
    b_str = trim(b_str);
    a_str = trim(a_str);
    int r = 0, g = 0, b = 0;
    float a = 0.0f;
    auto [r_ptr, r_ec] = std::from_chars(r_str.data(), r_str.data() + r_str.size(), r);
    auto [g_ptr, g_ec] = std::from_chars(g_str.data(), g_str.data() + g_str.size(), g);
    auto [b_ptr, b_ec] = std::from_chars(b_str.data(), b_str.data() + b_str.size(), b);
    auto [a_ptr, a_ec] = std::from_chars(a_str.data(), a_str.data() + a_str.size(), a);
    if (r_ec == std::errc() && r_ptr == r_str.data() + r_str.size() &&
        g_ec == std::errc() && g_ptr == g_str.data() + g_str.size() &&
        b_ec == std::errc() && b_ptr == b_str.data() + b_str.size() &&
        a_ec == std::errc() && a_ptr == a_str.data() + a_str.size()) {
      return Color::RGBA(r, g, b, a * 255.f);
    }
    return std::nullopt;
  }

  if (value == "red") {
    return Color::RGB(255, 0, 0);
  }
  if (value == "white") {
    return Color::RGB(255, 255, 255);
  }
  if (value == "blue") {
    return Color::RGB(0, 0, 255);
  }
  if (value == "yellow") {
    return Color::RGB(255, 255, 0);
  }
  if (value == "green" || value == "lime") {
    return Color::RGB(0, 255, 0);
  }
  if (value == "black") {
    return Color::RGB(0, 0, 0);
  }
  if (value == "gray" || value == "grey") {
    return Color::RGB(128, 128, 128);
  }
  if (value == "cyan" || value == "aqua") {
    return Color::RGB(0, 255, 255);
  }
  if (value == "magenta" || value == "fuchsia") {
    return Color::RGB(255, 0, 255);
  }
  if (value == "silver") {
    return Color::RGB(192, 192, 192);
  }
  if (value == "maroon") {
    return Color::RGB(128, 0, 0);
  }
  if (value == "purple") {
    return Color::RGB(128, 0, 128);
  }
  if (value == "olive") {
    return Color::RGB(128, 128, 0);
  }
  if (value == "navy") {
    return Color::RGB(0, 0, 128);
  }
  if (value == "teal") {
    return Color::RGB(0, 128, 128);
  }
  return std::nullopt;
}

// Mixes `amount` of `target` into `base`, which is what both lighten() and
// darken() mean: `color-mix(in srgb, target amount, base)`.
//
// With no base color there is nothing to mix into yet, so the mix is deferred
// to paint time as `target` at `amount` alpha. Compositing that over whatever
// ends up behind gives the same result, which is why the two branches agree.
Color MixToward(const std::optional<Color>& base, Color target, float amount) {
  amount = std::clamp(amount, 0.0f, 1.0f);
  if (!base.has_value()) {
    return Color::RGBA(target.r, target.g, target.b,
                       static_cast<uint8_t>(amount * 255.0f));
  }
  const Color c = *base;
  auto mix = [&](uint8_t from, uint8_t to) {
    return static_cast<uint8_t>(from + amount * (static_cast<float>(to) -
                                                 static_cast<float>(from)));
  };
  return Color::RGBA(mix(c.r, target.r), mix(c.g, target.g),
                     mix(c.b, target.b), c.a);
}

std::optional<Color> TransformColor(std::optional<Color> current,
                                    std::string_view v) {
  while (!v.empty() && std::isspace(static_cast<unsigned char>(v.front()))) {
    v.remove_prefix(1);
  }
  while (!v.empty() && std::isspace(static_cast<unsigned char>(v.back()))) {
    v.remove_suffix(1);
  }

  if (v.starts_with("lighten(") && v.back() == ')') {
    std::string_view amount_str = v.substr(8, v.size() - 9);
    float amount = 0.0f;
    if (amount_str.ends_with('%')) {
      amount = StoF(amount_str.substr(0, amount_str.size() - 1)) / 100.0f;
    } else {
      amount = StoF(amount_str);
    }
    // Mix `amount` of white in, rather than adding a flat `255 * amount` to
    // each channel. The two differ -- the flat form shifts a bright color as
    // far as a dark one and clips -- and the no-base branch has always been
    // the mixing kind, since compositing white at `amount` alpha over the
    // backdrop *is* the mix. Using the flat form whenever a color happened to
    // resolve meant one declaration produced two different results.
    return MixToward(current, Color::RGB(255, 255, 255), amount);
  }

  if (v.starts_with("darken(") && v.back() == ')') {
    std::string_view amount_str = v.substr(7, v.size() - 8);
    float amount = 0.0f;
    if (amount_str.ends_with('%')) {
      amount = StoF(amount_str.substr(0, amount_str.size() - 1)) / 100.0f;
    } else {
      amount = StoF(amount_str);
    }
    return MixToward(current, Color::RGB(0, 0, 0), amount);
  }

  if (v.starts_with("alpha(") && v.back() == ')') {
    std::string_view amount_str = v.substr(6, v.size() - 7);
    float amount = 0.0f;
    if (amount_str.ends_with('%')) {
      amount = StoF(amount_str.substr(0, amount_str.size() - 1)) / 100.0f;
    } else {
      amount = StoF(amount_str);
    }
    if (current.has_value()) {
      Color c = *current;
      uint8_t alpha = std::clamp(amount * 255.0f, 0.0f, 255.0f);
      return Color::RGBA(c.r, c.g, c.b, alpha);
    } else {
      uint8_t alpha = std::clamp(amount * 255.0f, 0.0f, 255.0f);
      return Color::RGBA(255, 255, 255, alpha);
    }
  }

  return ParseColor(v);
}

// calc() expressions are folded at parse time into the linear form
// `cells + percent% of basis [+ ref_coef * minmax(ref)]`. Multiplication and
// division require one side to be a plain number (no percent component),
// matching CSS. `ref` points at an interned MinMaxExpr for nested
// min()/max()/clamp(); at most one such term per expression.
struct CalcLinear {
  float cells = 0;
  float percent = 0;
  int ref = -1;
  float ref_coef = 0;
};

std::optional<CalcLinear> ParseMinMaxLinear(std::string_view value);

class CalcParser {
 public:
  explicit CalcParser(std::string_view s) : s_(s) {}

  std::optional<CalcLinear> Parse() {
    auto result = ParseExpr();
    SkipWhiteSpace();
    if (result && pos_ != s_.size()) {
      return std::nullopt;  // Trailing garbage.
    }
    return result;
  }

 private:
  void SkipWhiteSpace() {
    while (pos_ < s_.size() && (s_[pos_] == ' ' || s_[pos_] == '\t')) {
      ++pos_;
    }
  }

  char Peek() { return pos_ < s_.size() ? s_[pos_] : '\0'; }

  std::optional<CalcLinear> ParseExpr() {
    auto lhs = ParseTerm();
    if (!lhs) {
      return std::nullopt;
    }
    while (true) {
      SkipWhiteSpace();
      char op = Peek();
      if (op != '+' && op != '-') {
        return lhs;
      }
      ++pos_;
      auto rhs = ParseTerm();
      if (!rhs) {
        return std::nullopt;
      }
      // A linear expression carries at most one nested min/max term.
      if (lhs->ref != -1 && rhs->ref != -1) {
        return std::nullopt;
      }
      float sign = (op == '+') ? 1.0f : -1.0f;
      lhs->cells += sign * rhs->cells;
      lhs->percent += sign * rhs->percent;
      if (rhs->ref != -1) {
        lhs->ref = rhs->ref;
        lhs->ref_coef = sign * rhs->ref_coef;
      }
    }
  }

  std::optional<CalcLinear> ParseTerm() {
    auto lhs = ParseFactor();
    if (!lhs) {
      return std::nullopt;
    }
    while (true) {
      SkipWhiteSpace();
      char op = Peek();
      if (op != '*' && op != '/') {
        return lhs;
      }
      ++pos_;
      auto rhs = ParseFactor();
      if (!rhs) {
        return std::nullopt;
      }
      if (op == '*') {
        // A side carrying a nested min/max term may only be scaled by a
        // plain number; otherwise at most one side may carry a percent.
        if (lhs->ref != -1 || rhs->ref != -1) {
          if (lhs->ref != -1 && (rhs->percent != 0 || rhs->ref != -1)) {
            return std::nullopt;
          }
          if (rhs->ref != -1 && (lhs->percent != 0 || lhs->ref != -1)) {
            return std::nullopt;
          }
        } else if (lhs->percent != 0 && rhs->percent != 0) {
          return std::nullopt;
        }
        lhs = CalcLinear{
            lhs->cells * rhs->cells,
            lhs->percent * rhs->cells + rhs->percent * lhs->cells,
            lhs->ref != -1 ? lhs->ref : rhs->ref,
            lhs->ref != -1 ? lhs->ref_coef * rhs->cells
                           : rhs->ref_coef * lhs->cells};
      } else {
        // The divisor must be a plain non-zero number.
        if (rhs->percent != 0 || rhs->ref != -1 || rhs->cells == 0) {
          return std::nullopt;
        }
        lhs->cells /= rhs->cells;
        lhs->percent /= rhs->cells;
        lhs->ref_coef /= rhs->cells;
      }
    }
  }

  std::optional<CalcLinear> ParseFactor() {
    SkipWhiteSpace();
    if (Peek() == '(') {
      ++pos_;
      auto inner = ParseExpr();
      SkipWhiteSpace();
      if (!inner || Peek() != ')') {
        return std::nullopt;
      }
      ++pos_;
      return inner;
    }
    if (s_.substr(pos_).starts_with("calc(")) {
      pos_ += 4;  // Nested calc( behaves like a parenthesis.
      return ParseFactor();
    }
    if (s_.substr(pos_).starts_with("min(") ||
        s_.substr(pos_).starts_with("max(") ||
        s_.substr(pos_).starts_with("clamp(")) {
      // Consume the whole call up to its matching parenthesis and hand it to
      // the min/max parser.
      size_t open = s_.find('(', pos_);
      int depth = 0;
      size_t end = open;
      for (; end < s_.size(); ++end) {
        if (s_[end] == '(') {
          ++depth;
        } else if (s_[end] == ')' && --depth == 0) {
          break;
        }
      }
      if (end == s_.size()) {
        return std::nullopt;  // Unbalanced parentheses.
      }
      auto inner = ParseMinMaxLinear(s_.substr(pos_, end + 1 - pos_));
      if (!inner) {
        return std::nullopt;
      }
      pos_ = end + 1;
      return inner;
    }

    size_t start = pos_;
    if (Peek() == '+' || Peek() == '-') {
      ++pos_;
    }
    bool has_digits = false;
    while (pos_ < s_.size() &&
           ((s_[pos_] >= '0' && s_[pos_] <= '9') || s_[pos_] == '.')) {
      has_digits = true;
      ++pos_;
    }
    if (!has_digits) {
      return std::nullopt;
    }
    float number = StoF(s_.substr(start, pos_ - start));
    if (Peek() == '%') {
      ++pos_;
      return CalcLinear{0, number};
    }
    return CalcLinear{number, 0};
  }

  std::string_view s_;
  size_t pos_ = 0;
};

// Parses "min(a, b)", "max(a, b)", or "clamp(lo, mid, hi)" where each
// argument is a calc-style linear expression (which may itself contain
// nested min()/max()/clamp()). Constant expressions fold to plain cells;
// basis-dependent ones are interned and returned as a reference term.
std::optional<CalcLinear> ParseMinMaxLinear(std::string_view value) {
  MinMaxExpr::Op op;
  size_t name_len;
  if (value.starts_with("min(")) {
    op = MinMaxExpr::Op::Min;
    name_len = 4;
  } else if (value.starts_with("max(")) {
    op = MinMaxExpr::Op::Max;
    name_len = 4;
  } else {
    op = MinMaxExpr::Op::Clamp;
    name_len = 6;
  }
  value.remove_prefix(name_len);
  if (value.empty() || value.back() != ')') {
    return std::nullopt;
  }
  value.remove_suffix(1);

  // Split arguments on top-level commas.
  std::vector<std::string_view> args;
  size_t start = 0;
  int depth = 0;
  for (size_t i = 0; i < value.size(); ++i) {
    if (value[i] == '(') {
      ++depth;
    } else if (value[i] == ')') {
      --depth;
    } else if (value[i] == ',' && depth == 0) {
      args.push_back(value.substr(start, i - start));
      start = i + 1;
    }
  }
  args.push_back(value.substr(start));

  size_t expected = (op == MinMaxExpr::Op::Clamp) ? 3 : 2;
  if (args.size() != expected) {
    return std::nullopt;
  }

  CalcLinear parsed[3] = {};
  for (size_t i = 0; i < expected; ++i) {
    auto linear = CalcParser(args[i]).Parse();
    if (!linear) {
      return std::nullopt;
    }
    parsed[i] = *linear;
  }

  MinMaxExpr expr;
  expr.op = op;
  expr.a_cells = parsed[0].cells;
  expr.a_percent = parsed[0].percent;
  expr.a_ref = parsed[0].ref;
  expr.a_ref_coef = parsed[0].ref_coef;
  expr.b_cells = parsed[1].cells;
  expr.b_percent = parsed[1].percent;
  expr.b_ref = parsed[1].ref;
  expr.b_ref_coef = parsed[1].ref_coef;
  if (op == MinMaxExpr::Op::Clamp) {
    expr.c_cells = parsed[2].cells;
    expr.c_percent = parsed[2].percent;
    expr.c_ref = parsed[2].ref;
    expr.c_ref_coef = parsed[2].ref_coef;
  }

  if (!expr.DependsOnBasis()) {
    return CalcLinear{static_cast<float>(expr.Evaluate(0)), 0};
  }
  return CalcLinear{0, 0, RegisterMinMaxExpr(expr), 1};
}

// Converts a parsed linear expression to a Length. A basis-dependent
// min/max term is wrapped in an interned expression; min(x, x) == x serves
// as the identity when the term has linear companions.
Length LinearToLength(const CalcLinear& linear) {
  if (linear.ref == -1) {
    if (linear.percent == 0) {
      return Length::Cells(linear.cells);
    }
    if (linear.cells == 0) {
      return Length::Pct(linear.percent);
    }
    return Length::MakeCalc(linear.cells, linear.percent);
  }
  if (linear.cells == 0 && linear.percent == 0 && linear.ref_coef == 1) {
    return Length::MakeMinMax(linear.ref);
  }
  MinMaxExpr wrap;
  wrap.op = MinMaxExpr::Op::Min;
  wrap.a_cells = wrap.b_cells = linear.cells;
  wrap.a_percent = wrap.b_percent = linear.percent;
  wrap.a_ref = wrap.b_ref = linear.ref;
  wrap.a_ref_coef = wrap.b_ref_coef = linear.ref_coef;
  return Length::MakeMinMax(RegisterMinMaxExpr(wrap));
}

Length ParseCalc(std::string_view value) {
  // `value` is the full "calc(...)" token; parse the inner expression.
  value.remove_prefix(5);
  if (value.empty() || value.back() != ')') {
    return Length::Auto();
  }
  value.remove_suffix(1);

  auto linear = CalcParser(value).Parse();
  if (!linear) {
    return Length::Auto();  // Invalid expression: treated as unset.
  }
  return LinearToLength(*linear);
}

Length ParseLength(std::string_view value) {
  if (value == "auto" || value.empty()) {
    return Length::Auto();
  }
  if (value.starts_with("calc(")) {
    return ParseCalc(value);
  }
  if (value.starts_with("min(") || value.starts_with("max(") ||
      value.starts_with("clamp(")) {
    auto linear = ParseMinMaxLinear(value);
    if (!linear) {
      return Length::Auto();
    }
    return LinearToLength(*linear);
  }
  if (value.back() == '%') {
    value.remove_suffix(1);
    return Length::Pct(StoF(value));
  }
  if (value.size() >= 2 && value.substr(value.size() - 2) == "fr") {
    value.remove_suffix(2);
    return Length::Fr(StoF(value));
  }
  return Length::Cells(StoF(value));
}

std::vector<Length> ParseGridTemplate(std::string_view v) {
  std::vector<Length> tracks;
  std::vector<std::string_view> tokens;
  size_t start = 0;
  size_t i = 0;
  int paren_depth = 0;
  while (i < v.size()) {
    if (v[i] == '(') {
      paren_depth++;
      i++;
    } else if (v[i] == ')') {
      paren_depth--;
      i++;
    } else if (std::isspace(static_cast<unsigned char>(v[i]))) {
      if (paren_depth == 0) {
        if (i > start) {
          tokens.push_back(v.substr(start, i - start));
        }
        while (i < v.size() && std::isspace(static_cast<unsigned char>(v[i]))) {
          i++;
        }
        start = i;
      } else {
        i++;
      }
    } else {
      i++;
    }
  }
  if (i > start) {
    tokens.push_back(v.substr(start, i - start));
  }

  for (std::string_view token : tokens) {
    if (token.empty() || token == "none") continue;
    if (token.starts_with("repeat(") && token.back() == ')') {
      std::string_view inner = token.substr(7, token.size() - 8);
      size_t comma = inner.find(',');
      if (comma != std::string_view::npos) {
        std::string_view count_str = inner.substr(0, comma);
        std::string_view pattern_str = inner.substr(comma + 1);
        while (!count_str.empty() && std::isspace(static_cast<unsigned char>(count_str.front()))) count_str.remove_prefix(1);
        while (!count_str.empty() && std::isspace(static_cast<unsigned char>(count_str.back()))) count_str.remove_suffix(1);
        while (!pattern_str.empty() && std::isspace(static_cast<unsigned char>(pattern_str.front()))) pattern_str.remove_prefix(1);
        while (!pattern_str.empty() && std::isspace(static_cast<unsigned char>(pattern_str.back()))) pattern_str.remove_suffix(1);

        int count = 0;
        auto [ptr, ec] = std::from_chars(count_str.data(), count_str.data() + count_str.size(), count);
        if (ec != std::errc() || ptr != count_str.data() + count_str.size()) {
          continue;
        }

        std::vector<Length> sub_tracks = ParseGridTemplate(pattern_str);
        for (int c = 0; c < count; ++c) {
          tracks.insert(tracks.end(), sub_tracks.begin(), sub_tracks.end());
        }
      }
    } else {
      tracks.push_back(ParseLength(token));
    }
  }
  return tracks;
}

std::optional<BorderStyle> ParseBorderStyle(std::string_view v) {
  if (v == "none") {
    return BorderStyle::None;
  }
  if (v == "ascii") {
    return BorderStyle::Ascii;
  }
  if (v == "blank") {
    return BorderStyle::Blank;
  }
  if (v == "dashed") {
    return BorderStyle::Dashed;
  }
  if (v == "double") {
    return BorderStyle::Double;
  }
  if (v == "heavy") {
    return BorderStyle::Heavy;
  }
  if (v == "hkey") {
    return BorderStyle::HKey;
  }
  if (v == "inner") {
    return BorderStyle::Inner;
  }
  if (v == "outer") {
    return BorderStyle::Outer;
  }
  if (v == "panel") {
    return BorderStyle::Panel;
  }
  if (v == "round" || v == "rounded") {
    return BorderStyle::Round;
  }
  if (v == "solid") {
    return BorderStyle::Solid;
  }
  if (v == "tall") {
    return BorderStyle::Tall;
  }
  if (v == "thick") {
    return BorderStyle::Thick;
  }
  if (v == "vkey") {
    return BorderStyle::VKey;
  }
  if (v == "wide") {
    return BorderStyle::Wide;
  }
  if (v == "dotted") {
    return BorderStyle::Dotted;
  }
  if (v == "double-horizontal") {
    return BorderStyle::DoubleHorizontal;
  }
  if (v == "double-vertical") {
    return BorderStyle::DoubleVertical;
  }
  if (v == "shadow" || v == "3d") {
    return BorderStyle::Shadow;
  }
  if (v == "shade-light") {
    return BorderStyle::ShadeLight;
  }
  if (v == "shade-medium") {
    return BorderStyle::ShadeMedium;
  }
  if (v == "shade-dark") {
    return BorderStyle::ShadeDark;
  }
  if (v == "squiggle" || v == "wave") {
    return BorderStyle::Squiggle;
  }
  if (v == "block") {
    return BorderStyle::Block;
  }
  if (v == "tab") {
    return BorderStyle::Tab;
  }
  // Textual spells "no border" three ways; they all mean the same here.
  if (v == "hidden") {
    return BorderStyle::None;
  }
  return std::nullopt;
}

std::optional<Overflow> ParseOverflow(std::string_view v) {
  if (v == "visible") {
    return Overflow::Visible;
  }
  if (v == "hidden") {
    return Overflow::Hidden;
  }
  if (v == "scroll" || v == "auto") {
    return Overflow::Scroll;
  }
  return std::nullopt;
}

std::optional<ScrollbarWidth> ParseScrollbarWidth(std::string_view v) {
  if (v == "auto") {
    return ScrollbarWidth::Auto;
  }
  if (v == "none") {
    return ScrollbarWidth::None;
  }
  return std::nullopt;
}

std::pair<std::string_view, std::string_view> SplitScrollbarColors(
    std::string_view v) {
  while (!v.empty() && std::isspace(static_cast<unsigned char>(v.front()))) {
    v.remove_prefix(1);
  }
  if (v.empty()) {
    return {{}, {}};
  }

  size_t idx = 0;
  if (v.starts_with("rgb(") || v.starts_with("rgba(") ||
      v.starts_with("lighten(") || v.starts_with("darken(")) {
    size_t open_paren = v.find('(');
    size_t parens = 1;
    idx = open_paren + 1;
    while (idx < v.size() && parens > 0) {
      if (v[idx] == '(') {
        parens++;
      } else if (v[idx] == ')') {
        parens--;
      }
      idx++;
    }
  } else {
    while (idx < v.size() &&
           !std::isspace(static_cast<unsigned char>(v[idx]))) {
      idx++;
    }
  }

  std::string_view first = v.substr(0, idx);
  std::string_view rest = v.substr(idx);

  while (!rest.empty() &&
         std::isspace(static_cast<unsigned char>(rest.front()))) {
    rest.remove_prefix(1);
  }

  return {first, rest};
}

}  // namespace

void ApplyStyle(ComputedStyle& style, const css::Declaration& declaration) {
  auto p = declaration.property;
  auto v = declaration.value;

  if (p == "transition") {
    std::string_view value_view = v;
    style.transitions.reset();
    while (!value_view.empty()) {
      size_t comma = value_view.find(',');
      std::string_view token = (comma == std::string_view::npos)
                                   ? value_view
                                   : value_view.substr(0, comma);
      while (!token.empty() &&
             std::isspace(static_cast<unsigned char>(token.front()))) {
        token.remove_prefix(1);
      }
      while (!token.empty() &&
             std::isspace(static_cast<unsigned char>(token.back()))) {
        token.remove_suffix(1);
      }
      if (!token.empty()) {
        size_t space1 = token.find(' ');
        if (space1 != std::string_view::npos) {
          std::string_view prop_name = token.substr(0, space1);
          std::string_view remaining = token.substr(space1 + 1);
          while (!remaining.empty() &&
                 std::isspace(static_cast<unsigned char>(remaining.front()))) {
            remaining.remove_prefix(1);
          }
          size_t space2 = remaining.find(' ');
          std::string_view dur_str = (space2 == std::string_view::npos)
                                         ? remaining
                                         : remaining.substr(0, space2);
          float dur = 0.0f;
          if (dur_str.ends_with("ms")) {
            dur = StoF(dur_str.substr(0, dur_str.size() - 2)) / 1000.0f;
          } else if (dur_str.ends_with("s")) {
            dur = StoF(dur_str.substr(0, dur_str.size() - 1));
          } else {
            dur = StoF(dur_str);
          }
          std::string_view timing = "ease";
          if (space2 != std::string_view::npos) {
            std::string_view remaining2 = remaining.substr(space2 + 1);
            while (
                !remaining2.empty() &&
                std::isspace(static_cast<unsigned char>(remaining2.front()))) {
              remaining2.remove_prefix(1);
            }
            size_t space3 = remaining2.find(' ');
            timing = (space3 == std::string_view::npos)
                         ? remaining2
                         : remaining2.substr(0, space3);
          }
          if (!style.transitions) {
            style.transitions =
                std::make_unique<std::vector<TransitionConfig>>();
          }
          style.transitions->push_back(
              {std::string(prop_name), dur, 0.0f, std::string(timing)});
        }
      }
      if (comma == std::string_view::npos) {
        break;
      }
      value_view = value_view.substr(comma + 1);
    }
    return;
  }

  if (p == "background-color") {
    style.background_color = TransformColor(style.background_color, v);
    return;
  }

  if (p == "color" || p == "foreground-color") {
    style.foreground_color = TransformColor(style.foreground_color, v);
    return;
  }

  if (p == "font-weight") {
    if (v == "bold" || v == "bolder") {
      style.bold = true;
      style.dim = false;
      return;
    }
    if (v == "lighter") {
      style.bold = false;
      style.dim = true;
      return;
    }
    // Numeric weights: 100-300 render dim, >=600 render bold.
    int weight = 0;
    auto [ptr, ec] = std::from_chars(v.data(), v.data() + v.size(), weight);
    if (ec == std::errc() && ptr == v.data() + v.size()) {
      style.bold = (weight >= 600);
      style.dim = (weight <= 300);
      return;
    }
    // "normal" and anything unrecognized.
    style.bold = false;
    style.dim = false;
    return;
  }

  if (p == "font-style") {
    style.italic = (v == "italic" || v == "oblique");
    return;
  }

  if (p == "text-decoration") {
    if (v == "none") {
      style.underlined = false;
      style.underlined_double = false;
      style.strikethrough = false;
      style.overlined = false;
      style.blink = false;
      return;
    }
    style.underlined = ((v.find("underline") != std::string_view::npos ||
                         v.find("underlined") != std::string_view::npos) &&
                        v.find("double") == std::string_view::npos);
    style.underlined_double =
        (v.find("double-underline") != std::string_view::npos ||
         v.find("underlined-double") != std::string_view::npos ||
         ((v.find("underline") != std::string_view::npos ||
           v.find("underlined") != std::string_view::npos) &&
          v.find("double") != std::string_view::npos));
    style.strikethrough = (v.find("line-through") != std::string_view::npos ||
                           v.find("strikethrough") != std::string_view::npos);
    // "overline" must not also match the "underline" tests above; it does not,
    // since neither word is a substring of the other.
    style.overlined = (v.find("overline") != std::string_view::npos);
    style.blink = (v.find("blink") != std::string_view::npos);
    return;
  }

  if (p == "margin") {
    auto parts = SplitWords(v);
    if (parts.size() == 1) {
      if (parts[0] == "auto") {
        style.margin_left_auto = true;
        style.margin_right_auto = true;
        style.margin_top_auto = true;
        style.margin_bottom_auto = true;
        style.margin = {0, 0, 0, 0};
      } else {
        int m = StoI(parts[0]);
        style.margin_left_auto = false;
        style.margin_right_auto = false;
        style.margin_top_auto = false;
        style.margin_bottom_auto = false;
        style.margin = {m, m, m, m};
      }
    } else if (parts.size() == 2) {
      if (parts[0] == "auto") {
        style.margin_top_auto = true;
        style.margin_bottom_auto = true;
        style.margin.top = 0;
        style.margin.bottom = 0;
      } else {
        int v_val = StoI(parts[0]);
        style.margin_top_auto = false;
        style.margin_bottom_auto = false;
        style.margin.top = v_val;
        style.margin.bottom = v_val;
      }
      if (parts[1] == "auto") {
        style.margin_left_auto = true;
        style.margin_right_auto = true;
        style.margin.left = 0;
        style.margin.right = 0;
      } else {
        int h_val = StoI(parts[1]);
        style.margin_left_auto = false;
        style.margin_right_auto = false;
        style.margin.left = h_val;
        style.margin.right = h_val;
      }
    } else if (parts.size() == 3) {
      if (parts[0] == "auto") {
        style.margin_top_auto = true;
        style.margin.top = 0;
      } else {
        style.margin_top_auto = false;
        style.margin.top = StoI(parts[0]);
      }
      if (parts[2] == "auto") {
        style.margin_bottom_auto = true;
        style.margin.bottom = 0;
      } else {
        style.margin_bottom_auto = false;
        style.margin.bottom = StoI(parts[2]);
      }
      if (parts[1] == "auto") {
        style.margin_left_auto = true;
        style.margin_right_auto = true;
        style.margin.left = 0;
        style.margin.right = 0;
      } else {
        int h_val = StoI(parts[1]);
        style.margin_left_auto = false;
        style.margin_right_auto = false;
        style.margin.left = h_val;
        style.margin.right = h_val;
      }
    } else if (parts.size() >= 4) {
      if (parts[0] == "auto") {
        style.margin_top_auto = true;
        style.margin.top = 0;
      } else {
        style.margin_top_auto = false;
        style.margin.top = StoI(parts[0]);
      }
      if (parts[2] == "auto") {
        style.margin_bottom_auto = true;
        style.margin.bottom = 0;
      } else {
        style.margin_bottom_auto = false;
        style.margin.bottom = StoI(parts[2]);
      }
      if (parts[1] == "auto") {
        style.margin_right_auto = true;
        style.margin.right = 0;
      } else {
        style.margin_right_auto = false;
        style.margin.right = StoI(parts[1]);
      }
      if (parts[3] == "auto") {
        style.margin_left_auto = true;
        style.margin.left = 0;
      } else {
        style.margin_left_auto = false;
        style.margin.left = StoI(parts[3]);
      }
    }
    return;
  }

  if (p == "margin-top") {
    if (v == "auto") {
      style.margin_top_auto = true;
      style.margin.top = 0;
    } else {
      style.margin_top_auto = false;
      style.margin.top = StoI(v);
    }
    return;
  }

  if (p == "margin-bottom") {
    if (v == "auto") {
      style.margin_bottom_auto = true;
      style.margin.bottom = 0;
    } else {
      style.margin_bottom_auto = false;
      style.margin.bottom = StoI(v);
    }
    return;
  }

  if (p == "margin-left") {
    if (v == "auto") {
      style.margin_left_auto = true;
      style.margin.left = 0;
    } else {
      style.margin_left_auto = false;
      style.margin.left = StoI(v);
    }
    return;
  }

  if (p == "margin-right") {
    if (v == "auto") {
      style.margin_right_auto = true;
      style.margin.right = 0;
    } else {
      style.margin_right_auto = false;
      style.margin.right = StoI(v);
    }
    return;
  }

  if (p == "padding") {
    style.padding = ParseSpacingShorthand(v);
    return;
  }

  if (p == "padding-top") {
    int p = StoI(v);
    style.padding.top = p;
    return;
  }

  if (p == "padding-bottom") {
    int p = StoI(v);
    style.padding.bottom = p;
    return;
  }

  if (p == "padding-left") {
    int p = StoI(v);
    style.padding.left = p;
    return;
  }

  if (p == "padding-right") {
    int p = StoI(v);
    style.padding.right = p;
    return;
  }

  if (p == "border-width") {
    style.border = ParseSpacingShorthand(v);
    return;
  }

  if (p == "border") {
    if (auto style_opt = ParseBorderStyle(v)) {
      style.border_style = *style_opt;
      if (style.border_style != BorderStyle::None &&
          style.border.top == 0 && style.border.bottom == 0 &&
          style.border.left == 0 && style.border.right == 0) {
        style.border = {1, 1, 1, 1};
      }
      return;
    }
    // Try to parse as a number for width
    int bw = 0;
    std::string_view s = v;
    while (!s.empty() && std::isspace(static_cast<unsigned char>(s.front()))) {
      s.remove_prefix(1);
    }
    while (!s.empty() && std::isspace(static_cast<unsigned char>(s.back()))) {
      s.remove_suffix(1);
    }
    if (!s.empty()) {
      auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), bw);
      if (ec == std::errc() && ptr == s.data() + s.size()) {
        style.border = {bw, bw, bw, bw};
        style.border_style = BorderStyle::Solid;
        return;
      }
    }
    return;
  }

  // Like the `border` shorthand above, a single side also accepts a
  // border-style keyword: it selects the frame's character set and gives that
  // side a thickness of one cell. Without this, `border-bottom: solid` parsed
  // as the integer 0 and silently drew nothing.
  auto ApplyBorderSide = [&style, &v](int& side) {
    if (auto style_opt = ParseBorderStyle(v)) {
      style.border_style = *style_opt;
      side = (*style_opt == BorderStyle::None) ? 0 : 1;
      return;
    }
    side = StoI(v);
  };

  if (p == "border-top") {
    ApplyBorderSide(style.border.top);
    return;
  }

  if (p == "border-bottom") {
    ApplyBorderSide(style.border.bottom);
    return;
  }

  if (p == "border-left") {
    ApplyBorderSide(style.border.left);
    return;
  }

  if (p == "border-right") {
    ApplyBorderSide(style.border.right);
    return;
  }

  if (p == "border-style") {
    if (auto style_opt = ParseBorderStyle(v)) {
      style.border_style = *style_opt;
    }
    return;
  }

  if (p == "border-color") {
    style.border_color_top = TransformColor(style.border_color_top, v);
    style.border_color_right = TransformColor(style.border_color_right, v);
    style.border_color_bottom = TransformColor(style.border_color_bottom, v);
    style.border_color_left = TransformColor(style.border_color_left, v);
    return;
  }

  if (p == "border-color-top") {
    style.border_color_top = TransformColor(style.border_color_top, v);
    return;
  }

  if (p == "border-color-right") {
    style.border_color_right = TransformColor(style.border_color_right, v);
    return;
  }

  if (p == "border-color-bottom") {
    style.border_color_bottom = TransformColor(style.border_color_bottom, v);
    return;
  }

  if (p == "border-color-left") {
    style.border_color_left = TransformColor(style.border_color_left, v);
    return;
  }

  if (p == "opacity") {
    float val = StoF(v);
    style.opacity = val < 0.0f ? 0.0f : (val > 1.0f ? 1.0f : val);
    return;
  }

  if (p == "flex-grow") {
    style.flex_grow = StoF(v);
    return;
  }

  if (p == "flex-shrink") {
    style.flex_shrink = StoF(v);
    return;
  }

  if (p == "flex-basis") {
    style.flex_basis = ParseLength(v);
    return;
  }

  if (p == "order") {
    style.order = StoI(v);
    return;
  }

  if (p == "flex") {
    if (v == "none") {
      style.flex_grow = 0.0f;
      style.flex_shrink = 0.0f;
      style.flex_basis = Length::Auto();
      return;
    }
    if (v == "auto") {
      style.flex_grow = 1.0f;
      style.flex_shrink = 1.0f;
      style.flex_basis = Length::Auto();
      return;
    }
    auto parts = SplitWords(v);
    if (parts.size() == 1) {
      if (parts[0] == "auto") {
        style.flex_basis = Length::Auto();
      } else if (parts[0].back() == '%' ||
                 std::isdigit(static_cast<unsigned char>(parts[0].back())) ==
                     0) {
        style.flex_basis = ParseLength(parts[0]);
      } else {
        float val = StoF(parts[0]);
        style.flex_grow = val;
        style.flex_shrink = 1.0f;
        style.flex_basis = Length::Cells(0.0f);
      }
    } else if (parts.size() == 2) {
      if (parts[1] == "auto" || parts[1].back() == '%') {
        style.flex_grow = StoF(parts[0]);
        style.flex_shrink = 1.0f;
        style.flex_basis = ParseLength(parts[1]);
      } else {
        style.flex_grow = StoF(parts[0]);
        style.flex_shrink = StoF(parts[1]);
        style.flex_basis = Length::Cells(0.0f);
      }
    } else if (parts.size() >= 3) {
      style.flex_grow = StoF(parts[0]);
      style.flex_shrink = StoF(parts[1]);
      style.flex_basis = ParseLength(parts[2]);
    }
    return;
  }

  if (p == "flex-direction") {
    if (v == "row") {
      style.flex_direction = Direction::Row;
      return;
    }
    if (v == "row-reverse") {
      style.flex_direction = Direction::RowReverse;
      return;
    }
    if (v == "column") {
      style.flex_direction = Direction::Column;
      return;
    }
    if (v == "column-reverse") {
      style.flex_direction = Direction::ColumnReverse;
      return;
    }
  }

  if (p == "flex-wrap") {
    if (v == "nowrap") {
      style.flex_wrap = FlexWrap::NoWrap;
      return;
    }
    if (v == "wrap") {
      style.flex_wrap = FlexWrap::Wrap;
      return;
    }
    if (v == "wrap-reverse") {
      style.flex_wrap = FlexWrap::WrapReverse;
      return;
    }
  }

  if (p == "aspect-ratio") {
    if (v == "auto") {
      style.aspect_ratio = 0;
      return;
    }
    // "<width> / <height>" or a single ratio number. Ratios are in cells;
    // terminal cells are roughly twice as tall as wide, so a visually square
    // box is approximately "2 / 1".
    auto parts = SplitWords(v);
    float w = 0;
    float h = 1;
    if (parts.size() == 1) {
      size_t slash = parts[0].find('/');
      if (slash != std::string_view::npos) {
        w = StoF(parts[0].substr(0, slash));
        h = StoF(parts[0].substr(slash + 1));
      } else {
        w = StoF(parts[0]);
      }
    } else if (parts.size() == 3 && parts[1] == "/") {
      w = StoF(parts[0]);
      h = StoF(parts[2]);
    } else if (parts.size() == 2) {
      // "W/ H" or "W /H" split into two tokens.
      std::string joined = std::string(parts[0]) + std::string(parts[1]);
      size_t slash = joined.find('/');
      if (slash != std::string::npos) {
        w = StoF(std::string_view(joined).substr(0, slash));
        h = StoF(std::string_view(joined).substr(slash + 1));
      }
    }
    style.aspect_ratio = (w > 0 && h > 0) ? (w / h) : 0;
    return;
  }

  if (p == "inset") {
    auto parts = SplitWords(v);
    if (parts.size() == 1) {
      Length len = ParseLength(parts[0]);
      style.top = len;
      style.right = len;
      style.bottom = len;
      style.left = len;
    } else if (parts.size() == 2) {
      Length vertical = ParseLength(parts[0]);
      Length horizontal = ParseLength(parts[1]);
      style.top = vertical;
      style.bottom = vertical;
      style.left = horizontal;
      style.right = horizontal;
    } else if (parts.size() == 3) {
      style.top = ParseLength(parts[0]);
      Length horizontal = ParseLength(parts[1]);
      style.left = horizontal;
      style.right = horizontal;
      style.bottom = ParseLength(parts[2]);
    } else if (parts.size() >= 4) {
      style.top = ParseLength(parts[0]);
      style.right = ParseLength(parts[1]);
      style.bottom = ParseLength(parts[2]);
      style.left = ParseLength(parts[3]);
    }
    return;
  }

  if (p == "position") {
    if (v == "static") {
      style.position = PositionType::Static;
      return;
    }
    if (v == "relative") {
      style.position = PositionType::Relative;
      return;
    }
    if (v == "absolute") {
      style.position = PositionType::Absolute;
      return;
    }
    if (v == "fixed") {
      style.position = PositionType::Fixed;
      return;
    }
    if (v == "sticky") {
      style.position = PositionType::Sticky;
      return;
    }
  }

  if (p == "top") {
    style.top = ParseLength(v);
    return;
  }
  if (p == "right") {
    style.right = ParseLength(v);
    return;
  }
  if (p == "bottom") {
    style.bottom = ParseLength(v);
    return;
  }
  if (p == "left") {
    style.left = ParseLength(v);
    return;
  }

  if (p == "z-index") {
    if (v == "auto") {
      style.z_index = std::nullopt;
    } else {
      style.z_index = StoI(v);
    }
    return;
  }

  if (p == "width") {
    style.width = ParseLength(v);
    return;
  }

  if (p == "min-width") {
    style.min_width = ParseLength(v);
    return;
  }

  if (p == "min-height") {
    style.min_height = ParseLength(v);
    return;
  }

  if (p == "max-width") {
    style.max_width = ParseLength(v);
    return;
  }

  if (p == "max-height") {
    style.max_height = ParseLength(v);
    return;
  }

  if (p == "height") {
    style.height = ParseLength(v);
    return;
  }

  if (p == "gap" || p == "grid-gap") {
    auto parts = SplitWords(v);
    if (parts.size() == 1) {
      Length len = ParseLength(parts[0]);
      style.row_gap = len;
      style.column_gap = len;
    } else if (parts.size() >= 2) {
      style.row_gap = ParseLength(parts[0]);
      style.column_gap = ParseLength(parts[1]);
    }
    return;
  }

  if (p == "row-gap" || p == "grid-row-gap") {
    style.row_gap = ParseLength(v);
    return;
  }

  if (p == "column-gap" || p == "grid-column-gap") {
    style.column_gap = ParseLength(v);
    return;
  }

  if (p == "grid-template-columns") {
    style.grid_template_columns = ParseGridTemplate(v);
    return;
  }

  if (p == "grid-template-rows") {
    style.grid_template_rows = ParseGridTemplate(v);
    return;
  }

  if (p == "grid-template") {
    std::string_view val = v;
    while (!val.empty() && std::isspace(static_cast<unsigned char>(val.front()))) {
      val.remove_prefix(1);
    }
    while (!val.empty() && std::isspace(static_cast<unsigned char>(val.back()))) {
      val.remove_suffix(1);
    }
    if (val == "none") {
      style.grid_template_rows.clear();
      style.grid_template_columns.clear();
      return;
    }
    size_t slash = val.find('/');
    if (slash != std::string_view::npos) {
      style.grid_template_rows = ParseGridTemplate(val.substr(0, slash));
      style.grid_template_columns = ParseGridTemplate(val.substr(slash + 1));
    }
    return;
  }

  if (p == "grid-column" || p == "grid-column-end") {
    int span = 1;
    if (v.find("span") != std::string_view::npos) {
      auto parts = SplitWords(v);
      for (size_t i = 0; i < parts.size(); ++i) {
        if (parts[i] == "span" && i + 1 < parts.size()) {
          std::string_view s = parts[i + 1];
          int val = 1;
          auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), val);
          if (ec == std::errc() && ptr == s.data() + s.size()) {
            span = val;
          }
          break;
        }
      }
    } else {
      std::string_view s = v;
      while (!s.empty() && std::isspace(static_cast<unsigned char>(s.front()))) {
        s.remove_prefix(1);
      }
      while (!s.empty() && std::isspace(static_cast<unsigned char>(s.back()))) {
        s.remove_suffix(1);
      }
      if (!s.empty()) {
        int val = 1;
        auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), val);
        if (ec == std::errc() && ptr == s.data() + s.size()) {
          span = val;
        }
      }
    }
    style.grid_column_span = std::max(1, span);
    return;
  }

  if (p == "grid-row" || p == "grid-row-end") {
    int span = 1;
    if (v.find("span") != std::string_view::npos) {
      auto parts = SplitWords(v);
      for (size_t i = 0; i < parts.size(); ++i) {
        if (parts[i] == "span" && i + 1 < parts.size()) {
          std::string_view s = parts[i + 1];
          int val = 1;
          auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), val);
          if (ec == std::errc() && ptr == s.data() + s.size()) {
            span = val;
          }
          break;
        }
      }
    } else {
      std::string_view s = v;
      while (!s.empty() && std::isspace(static_cast<unsigned char>(s.front()))) {
        s.remove_prefix(1);
      }
      while (!s.empty() && std::isspace(static_cast<unsigned char>(s.back()))) {
        s.remove_suffix(1);
      }
      if (!s.empty()) {
        int val = 1;
        auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), val);
        if (ec == std::errc() && ptr == s.data() + s.size()) {
          span = val;
        }
      }
    }
    style.grid_row_span = std::max(1, span);
    return;
  }

  // `start`/`end` are accepted alongside `flex-start`/`flex-end`, matching
  // parse_align_items below: CSS box alignment renamed them when it outgrew
  // flexbox, and both spellings are in current use.
  auto parse_justify_content =
      [](std::string_view kw) -> std::optional<JustifyContent> {
    if (kw == "flex-start" || kw == "start") return JustifyContent::FlexStart;
    if (kw == "flex-end" || kw == "end") return JustifyContent::FlexEnd;
    if (kw == "center") return JustifyContent::Center;
    if (kw == "space-between") return JustifyContent::SpaceBetween;
    if (kw == "space-around") return JustifyContent::SpaceAround;
    if (kw == "space-evenly") return JustifyContent::SpaceEvenly;
    return std::nullopt;
  };
  auto parse_align_content =
      [](std::string_view kw) -> std::optional<AlignContent> {
    if (kw == "stretch") return AlignContent::Stretch;
    if (kw == "flex-start" || kw == "start") return AlignContent::FlexStart;
    if (kw == "flex-end" || kw == "end") return AlignContent::FlexEnd;
    if (kw == "center") return AlignContent::Center;
    if (kw == "space-between") return AlignContent::SpaceBetween;
    if (kw == "space-around") return AlignContent::SpaceAround;
    if (kw == "space-evenly") return AlignContent::SpaceEvenly;
    return std::nullopt;
  };

  if (p == "justify-content") {
    if (auto parsed = parse_justify_content(v)) {
      style.justify_content = *parsed;
    }
    return;
  }

  auto parse_align_items = [](std::string_view kw) -> std::optional<AlignItems> {
    if (kw == "stretch") return AlignItems::Stretch;
    if (kw == "flex-start" || kw == "start") return AlignItems::FlexStart;
    if (kw == "flex-end" || kw == "end") return AlignItems::FlexEnd;
    if (kw == "center") return AlignItems::Center;
    if (kw == "baseline") return AlignItems::Baseline;
    return std::nullopt;
  };
  auto parse_align_self = [](std::string_view kw) -> std::optional<AlignSelf> {
    if (kw == "auto") return AlignSelf::Auto;
    if (kw == "stretch") return AlignSelf::Stretch;
    if (kw == "flex-start" || kw == "start") return AlignSelf::FlexStart;
    if (kw == "flex-end" || kw == "end") return AlignSelf::FlexEnd;
    if (kw == "center") return AlignSelf::Center;
    if (kw == "baseline") return AlignSelf::Baseline;
    return std::nullopt;
  };

  if (p == "align-items") {
    if (auto parsed = parse_align_items(v)) {
      style.align_items = *parsed;
    }
    return;
  }

  if (p == "align-self") {
    if (auto parsed = parse_align_self(v)) {
      style.align_self = *parsed;
    }
    return;
  }

  if (p == "justify-items") {
    if (auto parsed = parse_align_items(v)) {
      style.justify_items = *parsed;
    }
    return;
  }

  if (p == "justify-self") {
    if (auto parsed = parse_align_self(v)) {
      style.justify_self = *parsed;
    }
    return;
  }

  if (p == "place-items") {
    auto parts = SplitWords(v);
    if (parts.size() == 1) {
      if (auto parsed = parse_align_items(parts[0])) {
        style.align_items = *parsed;
        style.justify_items = *parsed;
      }
    } else if (parts.size() >= 2) {
      if (auto parsed = parse_align_items(parts[0])) {
        style.align_items = *parsed;
      }
      if (auto parsed = parse_align_items(parts[1])) {
        style.justify_items = *parsed;
      }
    }
    return;
  }

  if (p == "place-self") {
    auto parts = SplitWords(v);
    if (parts.size() == 1) {
      if (auto parsed = parse_align_self(parts[0])) {
        style.align_self = *parsed;
        style.justify_self = *parsed;
      }
    } else if (parts.size() >= 2) {
      if (auto parsed = parse_align_self(parts[0])) {
        style.align_self = *parsed;
      }
      if (auto parsed = parse_align_self(parts[1])) {
        style.justify_self = *parsed;
      }
    }
    return;
  }

  if (p == "align-content") {
    if (auto parsed = parse_align_content(v)) {
      style.align_content = *parsed;
    }
    return;
  }

  // place-content: <align-content> <justify-content>?, the block-axis value
  // first. A single value sets both axes, as with place-items/place-self.
  if (p == "place-content") {
    auto parts = SplitWords(v);
    if (parts.empty()) {
      return;
    }
    if (auto parsed = parse_align_content(parts[0])) {
      style.align_content = *parsed;
    }
    std::string_view justify = parts.size() >= 2 ? parts[1] : parts[0];
    if (auto parsed = parse_justify_content(justify)) {
      style.justify_content = *parsed;
    }
    return;
  }

  if (p == "cursor") {
    if (v == "pointer") {
      style.cursor = Cursor::Pointer;
    } else if (v == "text") {
      style.cursor = Cursor::Text;
    } else if (v == "wait") {
      style.cursor = Cursor::Wait;
    } else if (v == "help") {
      style.cursor = Cursor::Help;
    } else if (v == "default") {
      style.cursor = Cursor::Default;
    } else {
      style.cursor = Cursor::Auto;
    }
    return;
  }

  if (p == "visibility") {
    if (v == "hidden") {
      style.visibility = Visibility::Hidden;
    } else {
      style.visibility = Visibility::Visible;
    }
    return;
  }

  if (p == "text-overflow") {
    if (v == "ellipsis") {
      style.text_overflow = TextOverflow::Ellipsis;
    } else {
      style.text_overflow = TextOverflow::Clip;
    }
    return;
  }

  if (p == "text-align") {
    if (v == "left") {
      style.text_align = TextAlign::Left;
      return;
    }
    if (v == "right") {
      style.text_align = TextAlign::Right;
      return;
    }
    if (v == "center") {
      style.text_align = TextAlign::Center;
      return;
    }
    if (v == "justify") {
      style.text_align = TextAlign::Justify;
      return;
    }
  }

  if (p == "text-transform") {
    if (v == "none") {
      style.text_transform = TextTransform::None;
      return;
    }
    if (v == "uppercase") {
      style.text_transform = TextTransform::Uppercase;
      return;
    }
    if (v == "lowercase") {
      style.text_transform = TextTransform::Lowercase;
      return;
    }
    if (v == "capitalize") {
      style.text_transform = TextTransform::Capitalize;
      return;
    }
  }

  if (p == "letter-spacing") {
    // Whole cells only; a terminal cannot render fractional or negative
    // spacing, so those clamp to zero.
    style.letter_spacing = (v == "normal") ? 0 : std::max(0, StoI(v));
    return;
  }

  if (p == "tab-size") {
    // Zero is legal and means a tab advances nothing; negative is not, and
    // clamps. `tab-size` in CSS also accepts a length, which here would be the
    // same thing as a count of cells, so there is nothing extra to parse.
    style.tab_size = std::max(0, StoI(v));
    return;
  }

  if (p == "line-height") {
    // Whole rows only: the value is the minimum height of each line box.
    style.line_height = (v == "normal") ? 1 : std::max(1, StoI(v));
    return;
  }

  // word-wrap is the legacy alias of overflow-wrap.
  if (p == "overflow-wrap" || p == "word-wrap") {
    if (v == "normal") {
      style.overflow_wrap = OverflowWrap::Normal;
      return;
    }
    if (v == "break-word" || v == "anywhere") {
      style.overflow_wrap = OverflowWrap::Anywhere;
      return;
    }
  }

  if (p == "word-break") {
    if (v == "normal") {
      style.word_break = WordBreak::Normal;
      return;
    }
    if (v == "break-all") {
      style.word_break = WordBreak::BreakAll;
      return;
    }
  }

  if (p == "white-space") {
    if (v == "normal") {
      style.white_space = WhiteSpace::Normal;
      return;
    }
    if (v == "nowrap") {
      style.white_space = WhiteSpace::Nowrap;
      return;
    }
    if (v == "pre") {
      style.white_space = WhiteSpace::Pre;
      return;
    }
    if (v == "pre-wrap" || v == "break-spaces") {
      style.white_space = WhiteSpace::PreWrap;
      return;
    }
    if (v == "pre-line") {
      style.white_space = WhiteSpace::PreLine;
      return;
    }
  }

  if (p == "list-style" || p == "list-style-type") {
    if (v == "disc") {
      style.list_style_type = ListStyleType::Disc;
      return;
    }
    if (v == "circle") {
      style.list_style_type = ListStyleType::Circle;
      return;
    }
    if (v == "square") {
      style.list_style_type = ListStyleType::Square;
      return;
    }
    if (v == "decimal") {
      style.list_style_type = ListStyleType::Decimal;
      return;
    }
    if (v == "none") {
      style.list_style_type = ListStyleType::None;
      return;
    }
  }

  if (p == "box-sizing") {
    if (v == "border-box") {
      style.box_sizing = BoxSizing::BorderBox;
    } else if (v == "content-box") {
      style.box_sizing = BoxSizing::ContentBox;
    }
    return;
  }

  if (p == "display") {
    // Parse combined display property (display-outside and display-inside)
    // For simplicity, handle common single-keyword values and assume default
    // display-inside: flow For two-keyword values, parse them as specified.

    // Split the value string by space
    std::string s_value(v.data(), v.size());

    if (s_value == "none") {
      style.display_none = true;
      return;
    }
    style.display_none = false;

    size_t space_pos = s_value.find(' ');
    if (space_pos == std::string::npos) {
      // Single keyword value
      if (s_value == "block") {
        style.display_outside = DisplayOutside::Block;
        style.display_inside = DisplayInside::Flow;
        return;
      }
      if (s_value == "inline") {
        style.display_outside = DisplayOutside::Inline;
        style.display_inside = DisplayInside::Flow;
        return;
      }
      if (s_value == "flex") {
        style.display_outside = DisplayOutside::Block;
        style.display_inside = DisplayInside::Flex;
        return;
      }
      if (s_value == "grid") {
        style.display_outside = DisplayOutside::Block;
        style.display_inside = DisplayInside::Grid;
        return;
      }
      if (s_value == "inline-block") {
        style.display_outside = DisplayOutside::Inline;
        style.display_inside = DisplayInside::FlowRoot;
        return;
      }
      if (s_value == "flow-root") {
        style.display_outside = DisplayOutside::Block;
        style.display_inside = DisplayInside::FlowRoot;
        return;
      }
      if (s_value == "inline-flex") {
        style.display_outside = DisplayOutside::Inline;
        style.display_inside = DisplayInside::Flex;
        return;
      }
      if (s_value == "grid" || s_value == "inline-grid") {
        style.display_outside = DisplayOutside::Inline;
        style.display_inside = DisplayInside::Grid;
        return;
      }

    } else {
      std::string outside = s_value.substr(0, space_pos);
      std::string inside = s_value.substr(space_pos + 1);

      if (outside == "block") {
        style.display_outside = DisplayOutside::Block;
      }

      if (outside == "inline") {
        style.display_outside = DisplayOutside::Inline;
      }

      if (inside == "flow") {
        style.display_inside = DisplayInside::Flow;
      }
      if (inside == "flow-root") {
        style.display_inside = DisplayInside::FlowRoot;
      }

      if (inside == "flex") {
        style.display_inside = DisplayInside::Flex;
      }
      if (inside == "grid") {
        style.display_inside = DisplayInside::Grid;
      }
      return;
    }
  }

  if (p == "overflow") {
    if (auto o = ParseOverflow(v)) {
      style.overflow_x = *o;
      style.overflow_y = *o;
    }
    return;
  }

  if (p == "overflow-x") {
    if (auto o = ParseOverflow(v)) {
      style.overflow_x = *o;
    }
    return;
  }

  if (p == "overflow-y") {
    if (auto o = ParseOverflow(v)) {
      style.overflow_y = *o;
    }
    return;
  }

  if (p == "scrollbar-width") {
    if (auto sw = ParseScrollbarWidth(v)) {
      style.scrollbar_width = *sw;
    }
    return;
  }

  if (p == "scrollbar-color") {
    if (v == "auto") {
      style.has_scrollbar_color_thumb = false;
      style.has_scrollbar_color_track = false;
      return;
    }
    auto [first, second] = SplitScrollbarColors(v);
    if (!first.empty()) {
      std::optional<Color> current =
          style.has_scrollbar_color_thumb
              ? std::optional<Color>(style.scrollbar_color_thumb)
              : std::nullopt;
      auto resolved = TransformColor(current, first);
      if (resolved) {
        style.has_scrollbar_color_thumb = true;
        style.scrollbar_color_thumb = *resolved;
      }
    }
    if (!second.empty()) {
      std::optional<Color> current =
          style.has_scrollbar_color_track
              ? std::optional<Color>(style.scrollbar_color_track)
              : std::nullopt;
      auto resolved = TransformColor(current, second);
      if (resolved) {
        style.has_scrollbar_color_track = true;
        style.scrollbar_color_track = *resolved;
      }
    }
    return;
  }

  if (p == "scroll-speed") {
    int val = StoI(v);
    style.scroll_speed_x = val;
    style.scroll_speed_y = val;
    return;
  }

  if (p == "scroll-speed-x") {
    style.scroll_speed_x = StoI(v);
    return;
  }

  if (p == "scroll-speed-y") {
    style.scroll_speed_y = StoI(v);
    return;
  }

  if (p == "scroll-behavior") {
    if (v == "smooth") {
      style.scroll_behavior = ScrollBehavior::Smooth;
    } else {
      style.scroll_behavior = ScrollBehavior::Auto;
    }
    return;
  }
}

}  // namespace rtxui
