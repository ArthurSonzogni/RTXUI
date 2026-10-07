// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
//
// A virtual list: 100,000 log lines.
//
// `<for ... virtual="">` inside a scroll container renders only the lines
// around what the container shows, with two spacers standing in for the rest,
// and renders again as scrolling moves past them. A frame then costs what a
// screenful of lines does, however long the log.
//
// Try it: scroll with the mouse wheel, or Tab to the log and use the arrows,
// PageUp/PageDown and Home/End.
#include <rtxui/rtxui.hpp>
#include <string>
#include <vector>

using namespace rtxui;

class LogViewer : public Component<LogViewer> {
 public:
  struct Line {
    std::string level;
    std::string text;
    bool operator==(const Line&) const = default;
  };
  std::vector<Line> lines;

  LogViewer() {
    static constexpr const char* kLevels[] = {"INFO", "INFO",  "INFO", "WARN",
                                              "INFO", "DEBUG", "ERROR"};
    for (int i = 0; i < 100000; ++i) {
      lines.push_back(
          {kLevels[i % 7], "request " + std::to_string(i) + " served in " +
                               std::to_string(3 + (i * 37) % 250) + "ms"});
    }
    Bind(lines, [](const Line& line) {
      return std::make_shared<ManualStructVisitor>(
          std::map<std::string, std::string, std::less<>>{{"level", line.level},
                                                          {"text", line.text}});
    });
  }

  std::string_view view = R"html(
      <div class="app">
        <h1>Log viewer — 100,000 lines</h1>
        <div class="log" tabindex="0">
          <for each="{lines}" as="line" virtual="">
            <div class="line">
              <span class="index">{$index}</span>
              <span class="{line.level}">{line.level}</span>
              <span>{line.text}</span>
            </div>
          </for>
        </div>
      </div>

      <style>
        self {
          width: 100%;
          height: 100%;
          background-color: rgb(13, 17, 23);
          color: rgb(201, 209, 217);
        }
        .app {
          display: flex;
          flex-direction: column;
          height: 100%;
          padding: 0 1;
        }
        h1 {
          color: rgb(88, 166, 255);
          font-weight: bold;
        }
        .log {
          flex-grow: 1;
          overflow-y: scroll;
          border: round;
          border-color: rgb(48, 54, 61);
        }
        .log:focus {
          border-color: rgb(88, 166, 255);
        }
        .line {
          display: flex;
          gap: 1;
        }
        .index {
          width: 6;
          text-align: right;
          color: rgb(110, 118, 129);
        }
        .INFO { color: rgb(63, 185, 80); width: 5; }
        .DEBUG { color: rgb(139, 148, 158); width: 5; }
        .WARN { color: rgb(210, 153, 34); width: 5; }
        .ERROR { color: rgb(248, 81, 73); width: 5; font-weight: bold; }
      </style>
    )html";
};

int main() {
  auto app = Ref<LogViewer>::New();
  Screen screen(app);
  screen.Loop();
  return 0;
}
