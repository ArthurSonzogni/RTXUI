// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#ifndef RTXUI_MARKDOWN_MARKDOWN_HPP_
#define RTXUI_MARKDOWN_MARKDOWN_HPP_

#include <string>
#include <string_view>

namespace rtxui {

/// Parses a Markdown string and returns its equivalent HTML representation.
/// Supported markdown:
///   - Headings (# through ######)
///   - Paragraphs (blank-line separated blocks)
///   - Blockquotes (>)
///   - Fenced Code Blocks (```)
///   - Ordered/Unordered Lists (-, *, +, 1., etc.)
///   - Inline code (`)
///   - Bold (**, __)
///   - Italic (*, _)
///   - Links ([text](url))
std::string MarkdownToHtml(std::string_view markdown);

}  // namespace rtxui

#endif  // RTXUI_MARKDOWN_MARKDOWN_HPP_
