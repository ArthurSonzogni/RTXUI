// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include "rtxui/markdown/markdown.hpp"

#include <catch2/catch_test_macros.hpp>
#include <string>

namespace rtxui {

TEST_CASE("Markdown: Headings", "[markdown]") {
  REQUIRE(MarkdownToHtml("# Heading 1") == "<h1>Heading 1</h1>\n");
  REQUIRE(MarkdownToHtml("## Heading 2") == "<h2>Heading 2</h2>\n");
  REQUIRE(MarkdownToHtml("### Heading 3") == "<h3>Heading 3</h3>\n");
  REQUIRE(MarkdownToHtml("#### Heading 4") == "<h4>Heading 4</h4>\n");
  REQUIRE(MarkdownToHtml("##### Heading 5") == "<h5>Heading 5</h5>\n");
  REQUIRE(MarkdownToHtml("###### Heading 6") == "<h6>Heading 6</h6>\n");

  // Incorrect headings should fall back to paragraph
  REQUIRE(MarkdownToHtml("#NoSpace") == "<p>#NoSpace</p>\n");
  REQUIRE(MarkdownToHtml("####### Too many hashes") ==
          "<p>####### Too many hashes</p>\n");
}

TEST_CASE("Markdown: Setext headings", "[markdown]") {
  REQUIRE(MarkdownToHtml("title\n=====") == "<h1>title</h1>\n");
  REQUIRE(MarkdownToHtml("subtitle\n--------") == "<h2>subtitle</h2>\n");

  // A single underline character is enough.
  REQUIRE(MarkdownToHtml("title\n=") == "<h1>title</h1>\n");
  REQUIRE(MarkdownToHtml("subtitle\n-") == "<h2>subtitle</h2>\n");

  // Inline formatting still applies to the heading text.
  REQUIRE(MarkdownToHtml("**bold** title\n=====") ==
          "<h1><strong>bold</strong> title</h1>\n");

  // A multi-line paragraph is joined into a single heading, same as a
  // regular paragraph would be.
  REQUIRE(MarkdownToHtml("line 1\nline 2\n=====") ==
          "<h1>line 1 line 2</h1>\n");

  // Without a preceding paragraph, '=' or '-' only lines are plain text.
  REQUIRE(MarkdownToHtml("=====") == "<p>=====</p>\n");
  REQUIRE(MarkdownToHtml("-----") == "<p>-----</p>\n");

  // A blank line breaks the paragraph, so the underline no longer applies.
  REQUIRE(MarkdownToHtml("title\n\n=====") ==
          "<p>title</p>\n<p>=====</p>\n");
}

TEST_CASE("Markdown: Paragraphs and line breaks", "[markdown]") {
  REQUIRE(MarkdownToHtml("Hello world") == "<p>Hello world</p>\n");

  // Consecutive lines are one paragraph, but the newline between them is a
  // line break rather than a space: a terminal document is read as it was
  // written, and there is no viewport to rewrap it against.
  REQUIRE(MarkdownToHtml("Line 1\nLine 2") == "<p>Line 1<br />Line 2</p>\n");

  // Double newlines split paragraphs
  REQUIRE(MarkdownToHtml("Line 1\n\nLine 2") ==
          "<p>Line 1</p>\n<p>Line 2</p>\n");
}

TEST_CASE("Markdown: Unordered Lists", "[markdown]") {
  const std::string input_dash = R"(- Item 1
- Item 2
- Item 3)";
  const std::string expected = R"(<ul>
<li>Item 1</li>
<li>Item 2</li>
<li>Item 3</li>
</ul>
)";
  REQUIRE(MarkdownToHtml(input_dash) == expected);

  const std::string input_star = R"(* Item 1
* Item 2)";
  const std::string expected_star = R"(<ul>
<li>Item 1</li>
<li>Item 2</li>
</ul>
)";
  REQUIRE(MarkdownToHtml(input_star) == expected_star);
}

TEST_CASE("Markdown: Ordered Lists", "[markdown]") {
  const std::string input = R"(1. First item
2. Second item
10. Tenth item)";
  const std::string expected = R"(<ol>
<li>First item</li>
<li>Second item</li>
<li>Tenth item</li>
</ol>
)";
  REQUIRE(MarkdownToHtml(input) == expected);
}

TEST_CASE("Markdown: Blockquotes", "[markdown]") {
  const std::string input = R"(> Hello blockquote
> Second line of blockquote)";
  const std::string expected = R"(<blockquote>
<p>Hello blockquote<br />Second line of blockquote</p>
</blockquote>
)";
  REQUIRE(MarkdownToHtml(input) == expected);
}

TEST_CASE("Markdown: Deeply nested blockquotes do not overflow the stack",
          "[markdown]") {
  // Regression: each '>' recursed one level deeper into MarkdownToHtml;
  // 50000 of them (trivial to produce, e.g. in pasted chat/comment text)
  // used to crash the whole process with a stack overflow.
  std::string input(50000, '>');
  input += " deep";
  REQUIRE_NOTHROW(MarkdownToHtml(input));
}

TEST_CASE("Markdown: Deeply nested emphasis markers do not overflow the "
          "stack",
          "[markdown]") {
  // Regression: bold/italic/link content recursed one level deeper into
  // ProcessInlineMixed per nesting level, with no cap. Found by fuzzing
  // (excessive stack usage on an adversarial input using many nested
  // markers); this constructs a similar pattern directly.
  int n = 100000;
  std::string input;
  for (int i = 0; i < n; ++i) {
    input += (i % 2 == 0) ? "_*" : "*_";
  }
  input += "x";
  for (int i = 0; i < n; ++i) {
    input += (i % 2 == 0) ? "*_" : "_*";
  }
  REQUIRE_NOTHROW(MarkdownToHtml(input));
}

TEST_CASE("Markdown: Unterminated fenced code block still closes its tags",
          "[markdown]") {
  // Found by fuzzing: a ``` with no closing fence (truncated/streamed
  // input, or a user still mid-edit) left <pre><code> open with no closing
  // tags, producing HTML that failed to parse as XML.
  REQUIRE(MarkdownToHtml("```") == "<pre><code></code></pre>\n");
  REQUIRE(MarkdownToHtml("```\nint x = 1;") ==
          "<pre><code>int x = 1;\n</code></pre>\n");
}

TEST_CASE("Markdown: Fenced Code Blocks", "[markdown]") {
  const std::string input = R"(```
int x = 42;
if (x < 50) {
  print("hello & welcome");
}
```)";
  const std::string expected = R"(<pre><code>int x = 42;
if (x &lt; 50) {
  print("hello &amp; welcome");
}
</code></pre>
)";
  REQUIRE(MarkdownToHtml(input) == expected);
}

TEST_CASE("Markdown: Inline formatting", "[markdown]") {
  REQUIRE(MarkdownToHtml("**bold text**") ==
          "<p><strong>bold text</strong></p>\n");
  REQUIRE(MarkdownToHtml("__bold text__") ==
          "<p><strong>bold text</strong></p>\n");
  REQUIRE(MarkdownToHtml("*italic text*") == "<p><em>italic text</em></p>\n");
  REQUIRE(MarkdownToHtml("_italic text_") == "<p><em>italic text</em></p>\n");

  // Mixed / nested formatting
  REQUIRE(MarkdownToHtml("**bold *and italic***") ==
          "<p><strong>bold <em>and italic</em></strong></p>\n");

  // Inline code
  REQUIRE(MarkdownToHtml("Use `std::vector<int>` for arrays") ==
          "<p>Use <code>std::vector&lt;int&gt;</code> for arrays</p>\n");
  // Formattings inside inline code must be preserved verbatim
  REQUIRE(MarkdownToHtml("`**not bold**`") ==
          "<p><code>**not bold**</code></p>\n");

  // Links
  REQUIRE(MarkdownToHtml("Go to [Google](https://google.com)") ==
          "<p>Go to <a href=\"https://google.com\">Google</a></p>\n");
  // Formatting inside link text
  REQUIRE(MarkdownToHtml("Go to [**Google**](https://google.com)") ==
          "<p>Go to <a "
          "href=\"https://google.com\"><strong>Google</strong></a></p>\n");
}

TEST_CASE("Markdown: HTML Escaping", "[markdown]") {
  REQUIRE(MarkdownToHtml("a < b & c > d") ==
          "<p>a &lt; b &amp; c &gt; d</p>\n");
}

TEST_CASE("Markdown: NUL and other C0 control characters are stripped",
          "[markdown]") {
  // Found by fuzzing: a NUL byte was passed straight through into the
  // generated HTML, which then failed to parse as XML (the XML parser
  // can't tell an embedded NUL apart from end-of-input, and no XML
  // character reference can represent NUL anyway).
  REQUIRE(MarkdownToHtml(std::string("a") + '\0' + "b") == "<p>ab</p>\n");
  REQUIRE(MarkdownToHtml(std::string("a\x01\x1B" "b")) == "<p>ab</p>\n");
  // Tab, LF and CR must be preserved.
  REQUIRE(MarkdownToHtml("a\tb") == "<p>a\tb</p>\n");
}

TEST_CASE("Markdown: Link URL with a quote does not break out of the "
          "href attribute",
          "[markdown]") {
  // A '"' in the URL must not terminate the href attribute early: doing so
  // let markdown content inject arbitrary attributes (e.g. onclick=) into
  // the generated DOM, and could also make the output fail to parse as XML.
  REQUIRE(MarkdownToHtml("[click](\" onclick=\"Evil)") ==
          "<p><a href=\"&quot; onclick=&quot;Evil\">click</a></p>\n");
}

TEST_CASE("Markdown: Tables", "[markdown]") {
  const std::string input = R"(| Header 1 | Header 2 |
| --- | --- |
| Cell 1 | Cell 2 |
| Cell 3 | Cell 4 |)";
  const std::string expected = R"(<table>
<thead>
<tr>
<th>Header 1</th>
<th>Header 2</th>
</tr>
</thead>
<tbody>
<tr>
<td>Cell 1</td>
<td>Cell 2</td>
</tr>
<tr>
<td>Cell 3</td>
<td>Cell 4</td>
</tr>
</tbody>
</table>
)";
  REQUIRE(MarkdownToHtml(input) == expected);

  // Test escaping of pipes in cell content
  const std::string escaped_input = R"(| A \| B | C |
| --- | --- |
| D | E |)";
  const std::string expected_escaped = R"(<table>
<thead>
<tr>
<th>A | B</th>
<th>C</th>
</tr>
</thead>
<tbody>
<tr>
<td>D</td>
<td>E</td>
</tr>
</tbody>
</table>
)";
  REQUIRE(MarkdownToHtml(escaped_input) == expected_escaped);
}


TEST_CASE("Markdown: hard line breaks", "[markdown]") {
  // CommonMark's two hard-break spellings. Both were being dropped: the
  // trailing spaces survived into the text and the backslash rendered
  // literally, and neither produced a break.
  CHECK(MarkdownToHtml("Line 1  \nLine 2") == "<p>Line 1<br />Line 2</p>\n");
  CHECK(MarkdownToHtml("Line 1\\\nLine 2") == "<p>Line 1<br />Line 2</p>\n");

  // An escaped backslash is content, not a break marker, and keeps its pair.
  CHECK(MarkdownToHtml("Line 1\\\\\nLine 2") ==
        "<p>Line 1\\\\<br />Line 2</p>\n");
}

TEST_CASE("Markdown: a heading spans its source lines as one line",
          "[markdown]") {
  // A setext heading is a single line of text however it was written, so its
  // newlines join with a space instead of becoming breaks.
  CHECK(MarkdownToHtml("line 1\nline 2\n=====") == "<h1>line 1 line 2</h1>\n");
}

TEST_CASE("Markdown: inline formatting survives a line break", "[markdown]") {
  // The break is applied after inline parsing, so emphasis opened on one
  // line and closed on the next still resolves.
  CHECK(MarkdownToHtml("**bold\ntext**") ==
        "<p><strong>bold<br />text</strong></p>\n");
}

}  // namespace rtxui
