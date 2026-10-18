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
#include <string_view>
#include <vector>

using namespace rtxui;

struct Month {
  std::string name;
  int days = 0;
  bool operator==(const Month& other) const = default;
};

// {month.name}, and {month.days}: the day numbers, a week per line.
void Fields(const Month& month, FieldSink& out) {
  out("name", month.name);
  std::string days;
  for (int day = 1; day <= month.days; ++day) {
    days += (day < 10 ? "  " : " ") + std::to_string(day);
    if (day % 7 == 0) {
      days += "\n";
    }
  }
  out("days", days);
}

class StickyDemo : public Component<StickyDemo> {
 public:
  std::vector<Month> months = {
      {"January", 31},   {"February", 28}, {"March", 31},    {"April", 30},
      {"May", 31},       {"June", 30},     {"July", 31},     {"August", 31},
      {"September", 30}, {"October", 31},  {"November", 30}, {"December", 31},
  };

  std::string_view view = R"html(
      <div class="card">
        <h1>Sticky</h1>
        <p>Scroll: each month's name stays at the top until the next one pushes it away.</p>

        <div class="calendar">
          <for each="{months}" as="month">
            <div class="month">
              <div class="name">{month.name}</div>
              <div class="days">{month.days}</div>
            </div>
          </for>
        </div>
      </div>

      <style>
        self {
          display: flex;
          align-items: center;
          justify-content: center;
          width: 100%;
          color: rgb(230, 237, 243);
        }
        .card {
          border: tall;
          border-color: rgb(48, 54, 61);
          background-color: rgb(22, 27, 34);
          padding: 1 3;
          width: 40;
        }
        h1 {
          color: rgb(88, 166, 255);
        }
        p {
          color: rgb(139, 148, 158);
        }
        .calendar {
          height: 14;
          overflow-y: scroll;
          border: round;
          border-color: rgb(48, 54, 61);
        }
        .name {
          position: sticky;
          top: 0;
          background-color: rgb(22, 45, 77);
          color: rgb(88, 166, 255);
          font-weight: bold;
          padding: 0 1;
        }
        .days {
          white-space: pre;
          padding: 0 1 1 1;
        }
      </style>
    )html";

  StickyDemo() { Bind(months); }
};

int main() {
  auto app = Ref<StickyDemo>::New();
  Screen screen(app);
  screen.Loop();
  return 0;
}
