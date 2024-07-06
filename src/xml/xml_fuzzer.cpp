// Copyright 2024 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include "xml/xml.hpp"

#include <fuzztest/fuzztest.h>
#include <gtest/gtest.h>

void TestXML(const std::string& input) {
  auto nodes = xml::Parse(input);
  return;
  // if (!nodes || nodes.value().size() != 1) {
  // return;
  //}
  // auto node0 = nodes.value()[0];
  // auto node0_str = xml::Print(node0);

  // auto nodes0 = xml::Parse(node0_str);
  // EXPECT_TRUE(nodes0);
  // EXPECT_EQ(nodes0.value().size(), 1);
  // auto node0_str2 = xml::Print(nodes0.value()[0]);
  // EXPECT_EQ(node0_str, node0_str2);
}

FUZZ_TEST(XML, TestXML);
