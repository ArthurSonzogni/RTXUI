#include "string.hpp"

#include <limits>

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

auto Repeat(std::string_view text, int count) -> std::string {
  std::string result;
  for (int i = 0; i < count; ++i) {
    result += text;
  }
  return result;
}
