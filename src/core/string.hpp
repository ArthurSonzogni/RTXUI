#ifndef STRING_HPP_
#define STRING_HPP_

#include <string>
#include <string_view>
#include <vector>

// Split a string by a delimiter, return a vector of string views.
auto Split(std::string_view text,
           char delimiter) -> std::vector<std::string_view>;
auto StripIndent(const std::string_view& text) -> std::string;
auto Repeat(std::string_view text, int count) -> std::string;

#endif  // STRING_HPP_
