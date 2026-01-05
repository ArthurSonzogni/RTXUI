// Copyright 2024 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#ifndef STRING_HPP_
#define STRING_HPP_

#include <string>
#include <string_view>
#include <vector>
#include <cstdint>

// Split a string by a delimiter, return a vector of string views.
auto Split(std::string_view text,
           char delimiter) -> std::vector<std::string_view>;
auto Split(std::string_view text,
           std::string_view delimiter) -> std::vector<std::string_view>;
auto Join(const std::vector<std::string_view>& parts,
          std::string_view delimiter) -> std::string;
auto Join(const std::vector<std::string>& parts,
          std::string_view delimiter) -> std::string;
auto StripIndent(const std::string_view& text) -> std::string;
auto Repeat(std::string_view text, int count) -> std::string;
auto CodePointToString(uint32_t codepoint) -> std::string;
auto EatCodePoint(std::string_view input,
                  size_t start,
                  size_t* end,
                  uint32_t* ucs) -> bool;

#endif  // STRING_HPP_
