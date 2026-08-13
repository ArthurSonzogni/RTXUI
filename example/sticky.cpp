// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
//
// position: sticky.
//
// Section headers pin to the top of the scroll container while their section is
// on screen.
#include <rtxui/rtxui.hpp>
#include <string>
#include <vector>

using namespace rtxui;

class MonthSection : public Component<MonthSection> {
 public:
  std::string month_index;
  std::string name;
  std::string header_bg = "var(--accent)";
  std::vector<std::string> days;

  std::string_view view = R"html(
    <div class="month-container">
      <div class="sticky-header">{name}</div>
      <for each="{days}" as="day">
        <div class="item">{day}</div>
      </for>
    </div>

    <style>
      self {
        --accent: rgb(88, 166, 255);

        display: block;
      }
      .month-container {
        display: block;
      }
      .sticky-header {
        position: sticky;
        top: 0;
        background-color: {header_bg};
        color: white;
        border: solid;
        font-weight: bold;
        padding: 1 1;
        z-index: 10;
      }
      .item {
        display: block;
        padding: 0 2;
        background-color: rgb(30, 41, 59, 0.4);
      }
      .item:hover {
        background-color: rgb(30, 41, 59, 0.8);
      }
    </style>
  )html";

  void set_month_index(std::string value) {
    int idx = std::stoi(value);
    std::vector<std::string> months = {
        "January", "February", "March", "April", "May", "June",
        "July", "August", "September", "October", "November", "December"
    };
    std::vector<std::string> colors = {
        "rgb(248, 81, 73)",   // Jan: Red
        "rgb(249, 115, 22)",  // Feb: Orange
        "rgb(245, 158, 11)",  // Mar: Amber
        "rgb(63, 185, 80)",  // Apr: Emerald
        "rgb(20, 184, 166)",  // May: Teal
        "rgb(6, 182, 212)",   // Jun: Cyan
        "var(--accent)",  // Jul: Blue
        "rgb(99, 102, 241)",  // Aug: Indigo
        "rgb(139, 92, 246)",  // Sep: Violet
        "rgb(168, 85, 247)",  // Oct: Purple
        "rgb(236, 72, 153)",  // Nov: Pink
        "rgb(244, 63, 94)"    // Dec: Rose
    };
    std::vector<int> days_in_month = {
        31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
    };
    if (idx >= 0 && idx < 12) {
      name = months[idx];
      header_bg = colors[idx];
      days.clear();
      for (int d = 1; d <= days_in_month[idx]; ++d) {
        days.push_back("Day " + std::to_string(d));
      }
    }
  }

  // Registers a template variable by hand rather than through Bind(), which
  // is what you drop to when a value needs custom read/write plumbing.
  MonthSection() {
    EnableHotReload();
    auto get_value = [this]() { return month_index; };
    auto set_value = [this](std::string_view val) {
      month_index = std::string(val);
      set_month_index(month_index);
    };
    auto check_and_update = [this, last_val = std::string()]() mutable {
      if (month_index != last_val) {
        last_val = month_index;
        return true;
      }
      return false;
    };
    entries_.push_back({"month-index", get_value, check_and_update, set_value});

    Bind(name);
    Bind(header_bg);
    Bind(days);
  }
};

class StickyDemo : public Component<StickyDemo> {
 public:
  std::vector<std::string> month_indices = {
      "0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11"
  };

  std::string_view view = R"html(
    <div class="container">
      <h2>Sticky Calendar Demo</h2>
      <p class="description">
        Use your mouse wheel to scroll the calendar.
        Notice how month names stay pinned at the top until they are pushed out of the way.
      </p>

      <div class="scroll-window">
        <for each="{month_indices}" as="idx">
          <month-section month-index="{idx}" />
        </for>
      </div>
    </div>

    <style>
      self {
        display: block;
        padding: 1 2;
        background-color: rgb(13, 17, 23); /* Deep dark slate background */
        color: rgb(230, 237, 243);
      }
      h2 {
        color: var(--accent); /* Bright blue */
        margin-bottom: 0;
      }
      .description {
        color: rgb(139, 148, 158); /* Muted gray text */
        margin-bottom: 2;
      }
      .scroll-window {
        display: block;
        overflow-y: scroll;
        scroll-speed: 1;
        max-width: 45;
        max-height: 20;
        margin: auto;
      }
    </style>
  )html";

  StickyDemo() {
    Import<MonthSection>("month-section");
    Bind(month_indices);
  }
};

int main() {
  auto app = Ref<StickyDemo>::New();
  Screen screen(app);
  screen.Loop();
  return 0;
}
