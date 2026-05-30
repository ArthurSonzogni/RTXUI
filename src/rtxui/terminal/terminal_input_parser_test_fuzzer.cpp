// Copyright 2021 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include <fuzztest/fuzztest.h>
#include <gtest/gtest.h>

#include "rtxui/internal/event.hpp"
#include "rtxui/terminal/terminal_input_parser.hpp"

void Fuzz(const std::string& s) {
  auto parser = TerminalInputParser();
  for (const char c : s) {
    parser.Add(c);

    while(auto event = parser.GetEvent()) {
      // Do nothing.
    }
  }
}
FUZZ_TEST(TerminalInputParser, Fuzz);
