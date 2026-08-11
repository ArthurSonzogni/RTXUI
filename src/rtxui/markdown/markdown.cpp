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
  for (unsigned char c : text) {
    if (c == '&') {
      result += "&amp;";
    } else if (c == '<') {
      result += "&lt;";
    } else if (c == '>') {
      result += "&gt;";
    } else if (c < 0x20 && c != '\t' && c != '\n' && c != '\r') {
      // These control characters (most notably NUL) are outright forbidden
      // by the XML spec and can't be represented via a character reference
      // either; drop them rather than hand the parser unrepresentable
      // input. `unsigned char` matters here: UTF-8 continuation bytes are
      // >= 0x80 and must not be mistaken for control characters when `char`
      // is signed.
    } else {
      result += static_cast<char>(c);
    }
  }
  return result;
}

// Escapes '"' in a link URL so it's safe to place inside a double-quoted XML
// attribute value. `link_url` is a substring of `s`, which ProcessInlineMixed
// only ever receives already passed through EscapeHtml (see ParseInline), so
// '&', '<', '>' are already encoded here; only the quote is still raw.
// Without this, a URL containing a '"' breaks out of the attribute and can
// inject arbitrary attributes (including rtxui event handlers like onclick=)
// into the DOM.
std::string EscapeAttributeQuotes(std::string_view text) {
  std::string result;
  result.reserve(text.size());
  for (char c : text) {
    if (c == '"') {
      result += "&quot;";
    } else {
      result += c;
    }
  }
  return result;
}

// Bold/italic/link content recurses into ProcessInlineMixed once per
// nesting level. Adversarial input with many nested markers (e.g.
// "_*_*_*..._*_*_") can nest arbitrarily deep; cap it and fall back to the
// (already HTML-escaped) content as-is once too deep, rather than
// overflowing the stack.
constexpr int kMaxInlineNestingDepth = 32;

std::string ProcessInlineMixed(std::string_view s, int depth = 0) {
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
          result += "<a href=\"" + EscapeAttributeQuotes(link_url) + "\">" +
                    (depth < kMaxInlineNestingDepth
                         ? ProcessInlineMixed(link_text, depth + 1)
                         : std::string(link_text)) +
                    "</a>";
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
        result += "<strong>" +
                  (depth < kMaxInlineNestingDepth
                       ? ProcessInlineMixed(content, depth + 1)
                       : std::string(content)) +
                  "</strong>";
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
        result += "<em>" +
                  (depth < kMaxInlineNestingDepth
                       ? ProcessInlineMixed(content, depth + 1)
                       : std::string(content)) +
                  "</em>";
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

// Drops the trailing whitespace a line carries into a paragraph, and the
// trailing backslash of CommonMark's other hard-break spelling. Both exist
// only to request the break that the newline itself now always produces, so
// neither should survive into the text.
std::string_view TrimLineBreakMarkers(std::string_view line) {
  while (!line.empty() && (line.back() == ' ' || line.back() == '\t')) {
    line.remove_suffix(1);
  }
  size_t backslashes = 0;
  while (backslashes < line.size() &&
         line[line.size() - 1 - backslashes] == '\\') {
    ++backslashes;
  }
  // An odd count means the last one is a break marker rather than an escaped
  // backslash of its own.
  if (backslashes % 2 == 1) {
    line.remove_suffix(1);
  }
  return line;
}

// Turns the newlines inside one paragraph's text into line breaks.
//
// CommonMark renders a soft break as a space and leaves rewrapping to the
// viewport. A terminal document is written to be read as it was laid out --
// and a hard break, which CommonMark does honour, was being dropped here
// entirely -- so every newline inside a paragraph becomes a break.
std::string BreakLines(std::string html) {
  std::string result;
  result.reserve(html.size());
  for (char c : html) {
    if (c == '\n') {
      result += "<br />";
    } else {
      result += c;
    }
  }
  return result;
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

std::string_view TrimTrailingSpaces(std::string_view line) {
  while (!line.empty() && (line.back() == ' ' || line.back() == '\t')) {
    line.remove_suffix(1);
  }
  return line;
}

// Setext heading underline: a line made up of only '=' (h1) or only '-'
// (h2) characters, at least one, ignoring trailing whitespace.
bool IsSetextUnderline(std::string_view line, char marker) {
  line = TrimTrailingSpaces(line);
  return !line.empty() &&
         std::all_of(line.begin(), line.end(),
                      [marker](char c) { return c == marker; });
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

// Blockquotes recurse into MarkdownToHtmlImpl once per nesting level (one
// '>' peeled off per level). Without a cap, a single line of many '>'
// characters (trivial to produce, e.g. in user-submitted chat/comment text)
// recurses once per character and overflows the stack.
constexpr int kMaxBlockquoteDepth = 32;

std::string MarkdownToHtmlImpl(std::string_view markdown, int depth);

}  // namespace


std::string MarkdownToHtml(std::string_view markdown) {
  return MarkdownToHtmlImpl(markdown, 0);
}

namespace {

std::string MarkdownToHtmlImpl(std::string_view markdown, int depth) {
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
      html += "<p>" + BreakLines(ParseInline(current_paragraph)) + "</p>\n";
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
      html += "<blockquote>\n";
      if (depth < kMaxBlockquoteDepth) {
        html += MarkdownToHtmlImpl(current_blockquote, depth + 1);
      } else {
        // Max nesting reached: stop recursing and render the remainder as
        // plain escaped text instead of overflowing the stack.
        html += "<p>" + ParseInline(current_blockquote) + "</p>\n";
      }
      html += "</blockquote>\n";
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

    // Setext-style heading: a paragraph line immediately followed by a line
    // of only '=' (h1) or only '-' (h2) characters turns that paragraph
    // into a heading instead.
    if (in_paragraph) {
      bool is_h1_underline = IsSetextUnderline(line, '=');
      bool is_h2_underline = !is_h1_underline && IsSetextUnderline(line, '-');
      if (is_h1_underline || is_h2_underline) {
        std::string tag = is_h1_underline ? "h1" : "h2";
        // A heading is one line of text however many source lines it was
        // written across, so its newlines join with a space rather than
        // becoming breaks the way a paragraph's do.
        std::string text = current_paragraph;
        std::replace(text.begin(), text.end(), '\n', ' ');
        html += "<" + tag + ">" + ParseInline(text) + "</" + tag + ">\n";
        current_paragraph.clear();
        in_paragraph = false;
        continue;
      }
    }

    // Regular line (paragraph)
    close_list();
    close_table();
    if (in_paragraph) {
      current_paragraph += "\n";
      current_paragraph += TrimLineBreakMarkers(line);
    } else {
      current_paragraph = TrimLineBreakMarkers(line);
      in_paragraph = true;
    }
  }

  // Close any remaining active blocks. An unterminated fenced code block
  // (no closing ``` line, e.g. truncated/streamed input) must still close
  // its <pre><code>, or the output is unbalanced HTML that fails to parse.
  if (in_code_block) {
    html += "</code></pre>\n";
  }
  close_paragraph();
  close_list();
  close_blockquote();
  close_table();

  return html;
}

}  // namespace

}  // namespace rtxui
