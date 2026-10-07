// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
//
// Recipe: a command palette.
//
// Ctrl+P opens a <dialog> holding an <input> that takes the focus through
// `autofocus`. Each keystroke filters the commands by fuzzy match; Up and Down
// move the highlight, Enter runs the highlighted command, Escape closes.
//
// Try it: press Ctrl+P (or click the button), type "th", then press Enter.
#include <algorithm>
#include <cctype>
#include <map>
#include <memory>
#include <rtxui/rtxui.hpp>
#include <string>
#include <vector>

using namespace rtxui;

class CommandPalette : public Component<CommandPalette> {
 public:
  struct Match {
    std::string name;
    std::string row_class;
    bool operator==(const Match&) const = default;
  };

  const std::vector<std::string> commands = {
      "Open file",     "Save file",           "Close tab",
      "Toggle theme",  "Toggle line numbers", "Go to line",
      "Find in files", "Rename symbol",       "Quit",
  };

  bool open = false;
  std::string query;
  std::vector<Match> matches;
  int selected = 0;
  std::string last = "nothing yet";

  void Open() {
    open = true;
    query.clear();
    selected = 0;
  }

  // Runs the command at `index` in `matches`; from a click, it arrives as
  // text.
  void Run(std::string index) {
    const size_t i = index.empty() ? 0 : std::stoul(index);
    if (i < matches.size()) {
      last = matches[i].name;
    }
    open = false;
  }

  CommandPalette() {
    Bind(open);
    Bind(query);
    Bind(last);
    Bind(Open);
    Bind(Run);
    Bind(matches, [](const Match& match) {
      return std::make_shared<ManualStructVisitor>(
          std::map<std::string, std::string, std::less<>>{
              {"name", match.name}, {"row_class", match.row_class}});
    });
  }

  // Whether `pattern`'s letters appear in `text` in order, ignoring case:
  // "tln" matches "Toggle line numbers".
  static bool FuzzyMatch(std::string_view pattern, std::string_view text) {
    size_t at = 0;
    for (const char c : pattern) {
      const auto lower = [](char x) {
        return std::tolower(static_cast<unsigned char>(x));
      };
      while (at < text.size() && lower(text[at]) != lower(c)) {
        ++at;
      }
      if (at == text.size()) {
        return false;
      }
      ++at;
    }
    return true;
  }

  // Recomputes the matches from the query before every render.
  bool Digest() override {
    matches.clear();
    for (const auto& command : commands) {
      if (FuzzyMatch(query, command)) {
        matches.push_back({command, ""});
      }
    }
    const int count = static_cast<int>(matches.size());
    selected = count == 0 ? 0 : std::clamp(selected, 0, count - 1);
    if (count > 0) {
      matches[selected].row_class = "selected";
    }
    return Component<CommandPalette>::Digest();
  }

  // Keys the focused input does not use reach here.
  bool OnEvent(Event event) override {
    if (event == Event::CtrlP()) {
      Open();
      return true;
    }
    if (open && event == Event::ArrowDown()) {
      selected++;
      return true;
    }
    if (open && event == Event::ArrowUp()) {
      selected = std::max(0, selected - 1);
      return true;
    }
    if (open && event == Event::Return()) {
      Run(std::to_string(selected));
      return true;
    }
    return Component<CommandPalette>::OnEvent(event);
  }

  std::string_view view = R"html(
    <div class="main">
      <p>Last command: {last}</p>
      <button onclick="Open">Command palette (Ctrl+P)</button>

      <dialog open="{open}" title="Run a command">
        <if condition="{open}">
          <input value="{query}" placeholder="Type to filter" autofocus=""/>
        </if>
        <div class="list">
          <for each="{matches}" as="match">
            <div class="{match.row_class}" onclick="Run({$index})">{match.name}</div>
          </for>
        </div>
      </dialog>
    </div>

    <style>
      .main { padding: 1; width: 60; height: 16; }
      .list { margin-top: 1; height: 9; }
      .selected { background-color: rgb(37, 99, 235); color: white; }
    </style>
  )html";
};

int main() {
  auto app = Ref<CommandPalette>::New();
  Screen screen(app);
  screen.Loop();
  return 0;
}
