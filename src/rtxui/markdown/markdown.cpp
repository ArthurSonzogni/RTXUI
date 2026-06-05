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

bool IsTableDivider(std::string_view line) {
  if (line.empty()) {
    return false;
  }
  bool has_pipe = false;
  bool has_dash = false;
  for (char c : line) {
    if (c == '|') {
      has_pipe = true;
    } else if (c == '-') {
      has_dash = true;
    } else if (c == ':' || c == ' ' || c == '\t' || c == '\r') {
      // Allowed characters
    } else {
      return false;
    }
  }
  return has_pipe && has_dash;
}

std::vector<std::string> SplitTableCells(std::string_view line) {
  std::vector<std::string> cells;
  bool starts_with_pipe = line.starts_with('|');
  bool ends_with_pipe = line.ends_with('|');
  
  size_t start = 0;
  if (starts_with_pipe) {
    start = 1;
  }
  
  size_t end_limit = line.size();
  if (ends_with_pipe && line.size() > 1) {
    if (line[line.size() - 2] != '\\') {
      end_limit = line.size() - 1;
    }
  }
  
  size_t pos = start;
  std::string current_cell;
  while (pos < end_limit) {
    if (line[pos] == '\\' && pos + 1 < end_limit && line[pos + 1] == '|') {
      current_cell += '|';
      pos += 2;
    } else if (line[pos] == '|') {
      cells.push_back(current_cell);
      current_cell.clear();
      pos++;
    } else {
      current_cell += line[pos];
      pos++;
    }
  }
  cells.push_back(current_cell);
  
  // Trim whitespace from each cell
  for (auto& cell : cells) {
    size_t l = 0;
    while (l < cell.size() && std::isspace(static_cast<unsigned char>(cell[l]))) {
      l++;
    }
    cell.erase(0, l);
    while (!cell.empty() && std::isspace(static_cast<unsigned char>(cell.back()))) {
      cell.pop_back();
    }
  }
  
  return cells;
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

  bool in_table = false;

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

  auto close_table = [&]() {
    if (in_table) {
      html += "</tbody>\n</table>\n";
      in_table = false;
    }
  };

  for (size_t line_idx = 0; line_idx < lines.size(); ++line_idx) {
    auto line = TrimTrailingCr(lines[line_idx]);

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
      close_table();
      continue;
    }

    // Handle table row if already in table
    if (in_table) {
      bool is_unordered_item = (line.starts_with("- ") ||
                                line.starts_with("* ") || line.starts_with("+ "));
      bool is_ordered_item = IsOrderedListItem(line);
      
      if (line.find('|') == std::string_view::npos ||
          line.starts_with("```") || line.starts_with("#") ||
          is_blockquote_line || is_unordered_item || is_ordered_item) {
        close_table();
        // Do not continue, process this line normally below
      } else {
        html += "<tr>\n";
        auto cells = SplitTableCells(line);
        for (const auto& cell : cells) {
          html += "<td>" + ParseInline(cell) + "</td>\n";
        }
        html += "</tr>\n";
        continue;
      }
    }

    // Handle table start detection
    bool can_start_table = false;
    if (!in_table && line.find('|') != std::string_view::npos) {
      if (line_idx + 1 < lines.size()) {
        if (IsTableDivider(TrimTrailingCr(lines[line_idx + 1]))) {
          can_start_table = true;
        }
      }
    }

    if (can_start_table) {
      close_paragraph();
      close_list();
      close_blockquote();
      
      in_table = true;
      html += "<table>\n<thead>\n<tr>\n";
      auto cells = SplitTableCells(line);
      for (const auto& cell : cells) {
        html += "<th>" + ParseInline(cell) + "</th>\n";
      }
      html += "</tr>\n</thead>\n<tbody>\n";
      // Skip the divider line
      line_idx++;
      continue;
    }

    // Handle fenced code block start
    if (line.starts_with("```")) {
      close_paragraph();
      close_list();
      close_blockquote();
      close_table();
      html += "<pre><code>";
      in_code_block = true;
      continue;
    }

    // Handle blockquote continuation / start
    if (is_blockquote_line) {
      close_paragraph();
      close_list();
      close_table();

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
        close_table();
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
      close_table();

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
    close_table();
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
  close_table();

  return html;
}

}  // namespace rtxui
