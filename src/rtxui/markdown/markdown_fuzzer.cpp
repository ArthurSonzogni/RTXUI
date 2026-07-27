// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include "rtxui/markdown/markdown.hpp"

#include <fuzztest/fuzztest.h>
#include <gtest/gtest.h>

#include "rtxui/xml/xml.hpp"

void TestMarkdown(const std::string& input) {
  std::string html = rtxui::MarkdownToHtml(input);
  // The <markdown> component feeds this straight into the XML/template
  // parser, so it must always be well-formed.
  EXPECT_TRUE(xml::Parse(html)) << "MarkdownToHtml produced unparsable HTML";
}

FUZZ_TEST(Markdown, TestMarkdown);
