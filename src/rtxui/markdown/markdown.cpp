// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include "rtxui/markdown/markdown.hpp"

#include <algorithm>
#include <cctype>
#include <string>
#include <string_view>
#include <vector>

namespace rtxui {

namespace {

enum class ListType { None, Unordered, Ordered };

std::string EscapeHtml(std::string_view text) {
  std::string result;
  result.reserve(text.size());
  for (char c : text) {
    if (c == '&') {
      result += "&amp;";
    } else if (c == '<') {
      result += "&lt;";
    } else if (c == '>') {
      result += "&gt;";
    } else {
      result += c;
    }
  }
  return result;
}

std::string ProcessInlineMixed(std::string_view s) {
  std::string result;
  size_t i = 0;
  while (i < s.size()) {
    // Check for inline code first
    if (s[i] == '`') {
      size_t start = i + 1;
      size_t end = s.find('`', start);
      if (end != std::string_view::npos) {
        result +=
            "<code>" + std::string(s.substr(start, end - start)) + "</code>";
        i = end + 1;
        continue;
      }
    }

    // Check for Link: [text](url)
    if (s[i] == '[') {
      size_t text_end = s.find(']', i);
      if (text_end != std::string_view::npos && text_end + 1 < s.size() &&
          s[text_end + 1] == '(') {
        size_t url_end = s.find(')', text_end + 2);
        if (url_end != std::string_view::npos) {
          std::string_view link_text = s.substr(i + 1, text_end - (i + 1));
          std::string_view link_url =
              s.substr(text_end + 2, url_end - (text_end + 2));
          result += "<a href=\"" + std::string(link_url) + "\">" +
                    ProcessInlineMixed(link_text) + "</a>";
          i = url_end + 1;
          continue;
        }
      }
    }

    // Check for Bold/Italic ** or __
    if ((s[i] == '*' && i + 1 < s.size() && s[i + 1] == '*') ||
        (s[i] == '_' && i + 1 < s.size() && s[i + 1] == '_')) {
      char marker = s[i];
      size_t start = i + 2;
      size_t end = s.find(std::string(2, marker), start);
      if (end != std::string_view::npos) {
        size_t count = 0;
        while (end + count < s.size() && s[end + count] == marker) {
          count++;
        }
        if (count > 2) {
          end += (count - 2);
        }
        std::string_view content = s.substr(start, end - start);
        result += "<strong>" + ProcessInlineMixed(content) + "</strong>";
        i = end + 2;
        continue;
      }
    }

    // Check for Italic * or _
    if (s[i] == '*' || s[i] == '_') {
      char marker = s[i];
      size_t start = i + 1;
      size_t end = start;
      while (true) {
        end = s.find(marker, end);
        if (end == std::string_view::npos) {
          break;
        }
        if (end + 1 < s.size() && s[end + 1] == marker) {
          end += 2;
        } else {
          break;
        }
      }
      if (end != std::string_view::npos) {
        std::string_view content = s.substr(start, end - start);
        result += "<em>" + ProcessInlineMixed(content) + "</em>";
        i = end + 1;
        continue;
      }
    }

    result += s[i];
    i++;
  }
  return result;
}

std::string ParseInline(std::string_view text) {
  return ProcessInlineMixed(EscapeHtml(text));
}

std::vector<std::string_view> SplitLines(std::string_view text) {
  std::vector<std::string_view> lines;
  size_t start = 0;
  while (start < text.size()) {
    size_t pos = text.find('\n', start);
    if (pos == std::string_view::npos) {
      lines.push_back(text.substr(start));
      break;
    }
    lines.push_back(text.substr(start, pos - start));
    start = pos + 1;
  }
  return lines;
}

std::string_view TrimTrailingCr(std::string_view line) {
  if (!line.empty() && line.back() == '\r') {
    line.remove_suffix(1);
  }
  return line;
}

bool IsOrderedListItem(std::string_view line) {
  if (line.empty()) {
    return false;
  }
  size_t i = 0;
  while (i < line.size() && std::isdigit(static_cast<unsigned char>(line[i]))) {
    i++;
  }
  return (i > 0 && i + 1 < line.size() && line[i] == '.' && line[i + 1] == ' ');
}

}  // namespace

std::string MarkdownToHtml(std::string_view markdown) {
  std::string html;
  auto lines = SplitLines(markdown);

  bool in_code_block = false;
  bool in_paragraph = false;
  std::string current_paragraph;

  ListType current_list_type = ListType::None;

  bool in_blockquote = false;
  std::string current_blockquote;

  auto close_paragraph = [&]() {
    if (in_paragraph) {
      html += "<p>" + ParseInline(current_paragraph) + "</p>\n";
      current_paragraph.clear();
      in_paragraph = false;
    }
  };

  auto close_list = [&]() {
    if (current_list_type == ListType::Unordered) {
      html += "</ul>\n";
    } else if (current_list_type == ListType::Ordered) {
      html += "</ol>\n";
    }
    current_list_type = ListType::None;
  };

  auto close_blockquote = [&]() {
    if (in_blockquote) {
      html += "<blockquote>\n" + MarkdownToHtml(current_blockquote) +
              "</blockquote>\n";
      current_blockquote.clear();
      in_blockquote = false;
    }
  };

  for (auto raw_line : lines) {
    auto line = TrimTrailingCr(raw_line);

    // If in code block, we only look for the closing backticks
    if (in_code_block) {
      if (line.starts_with("```")) {
        html += "</code></pre>\n";
        in_code_block = false;
      } else {
        html += EscapeHtml(line) + "\n";
      }
      continue;
    }

    // Handle blockquote check
    bool is_blockquote_line = line.starts_with(">");
    if (!is_blockquote_line) {
      close_blockquote();
    }

    // Handle blank lines
    auto is_blank = [](std::string_view l) {
      return l.empty() || std::all_of(l.begin(), l.end(), [](char c) {
               return std::isspace(static_cast<unsigned char>(c));
             });
    };
    if (is_blank(line)) {
      close_paragraph();
      close_list();
      close_blockquote();
      continue;
    }

    // Handle fenced code block start
    if (line.starts_with("```")) {
      close_paragraph();
      close_list();
      close_blockquote();
      html += "<pre><code>";
      in_code_block = true;
      continue;
    }

    // Handle blockquote continuation / start
    if (is_blockquote_line) {
      close_paragraph();
      close_list();

      std::string_view content = line.substr(1);
      if (content.starts_with(" ")) {
        content.remove_prefix(1);
      }
      current_blockquote += content;
      current_blockquote += "\n";
      in_blockquote = true;
      continue;
    }

    // Handle headers
    if (line.starts_with("#")) {
      size_t hashes = 0;
      while (hashes < line.size() && line[hashes] == '#') {
        hashes++;
      }
      if (hashes >= 1 && hashes <= 6 && hashes < line.size() &&
          line[hashes] == ' ') {
        close_paragraph();
        close_list();
        std::string_view text = line.substr(hashes + 1);
        std::string tag = "h" + std::to_string(hashes);
        html += "<" + tag + ">" + ParseInline(text) + "</" + tag + ">\n";
        continue;
      }
    }

    // Handle list items
    bool is_unordered_item = (line.starts_with("- ") ||
                              line.starts_with("* ") || line.starts_with("+ "));
    bool is_ordered_item = IsOrderedListItem(line);

    if (is_unordered_item || is_ordered_item) {
      close_paragraph();

      ListType target_type =
          is_unordered_item ? ListType::Unordered : ListType::Ordered;
      if (current_list_type != target_type) {
        close_list();
        if (target_type == ListType::Unordered) {
          html += "<ul>\n";
        } else {
          html += "<ol>\n";
        }
        current_list_type = target_type;
      }

      std::string_view text;
      if (is_unordered_item) {
        text = line.substr(2);
      } else {
        size_t i = 0;
        while (i < line.size() &&
               std::isdigit(static_cast<unsigned char>(line[i]))) {
          i++;
        }
        text = line.substr(i + 2);
      }
      html += "<li>" + ParseInline(text) + "</li>\n";
      continue;
    }

    // Regular line (paragraph)
    close_list();
    if (in_paragraph) {
      current_paragraph += " ";
      current_paragraph += line;
    } else {
      current_paragraph = line;
      in_paragraph = true;
    }
  }

  // Close any remaining active blocks
  close_paragraph();
  close_list();
  close_blockquote();

  return html;
}

}  // namespace rtxui
