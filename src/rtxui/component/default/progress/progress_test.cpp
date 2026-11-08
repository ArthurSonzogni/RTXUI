// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include <catch2/catch_message.hpp>
#include <catch2/catch_test_macros.hpp>
#include <string_view>

#include "rtxui/color.hpp"
#include "rtxui/component.hpp"
#include "rtxui/headless.hpp"
#include "rtxui/refcounted.hpp"

namespace rtxui {
namespace {

class HalfProgress : public Component<HalfProgress> {
 public:
  std::string_view view = R"(
    <progress value="5" max="10" width="10" />
    <style>
      progress { background-color: rgb(255, 0, 0); }
    </style>
  )";
};

// The empty part of the track is spaces, and they were collapsed into one as
// a paragraph's are: the background covered a single cell past the bar.
TEST_CASE("A progress bar's track takes its whole width", "[component]") {
  TimelineScreen screen(Ref<HalfProgress>::New(), 20, 1);
  screen.Advance(0);
  for (int x = 0; x < 10; ++x) {
    INFO("column " << x);
    CHECK(screen.BackgroundAt(x, 0).a > 0);
  }
  CHECK(screen.BackgroundAt(10, 0).a == 0);
}

}  // namespace
}  // namespace rtxui
