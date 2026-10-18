// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
//
// An inline prompt that stays: pick dependencies to add, one after the other,
// below the command that started it, as `cargo add` or `npm install` would.
//
// The prompt is drawn inline, as a Screen is by default, as tall as the list
// under it, which grows and shrinks as the query filters it. Each pick is
// written above it with Screen::Print(), where it stays in the terminal's
// history, and the prompt is ready for the next one. Escape or Ctrl+C ends it
// with a summary line.
//
// Try it: type "re", choose with Up and Down or the mouse, press Enter. Again.
#include <algorithm>
#include <cctype>
#include <cstddef>
#include <functional>
#include <rtxui/rtxui.hpp>
#include <string>
#include <string_view>
#include <vector>

using namespace rtxui;

struct Package {
  std::string_view name;
  std::string_view version;
  std::string_view about;
};

constexpr Package kPackages[] = {
    {"serde", "1.0.210", "Serialize and deserialize data structures"},
    {"tokio", "1.40.0", "An asynchronous runtime"},
    {"reqwest", "0.12.8", "An HTTP client"},
    {"regex", "1.11.0", "Regular expressions"},
    {"rand", "0.8.5", "Random number generators"},
    {"clap", "4.5.20", "Command line argument parsing"},
    {"anyhow", "1.0.89", "Flexible error handling"},
    {"tracing", "0.1.40", "Application-level tracing"},
    {"rayon", "1.10.0", "Data parallelism"},
    {"chrono", "0.4.38", "Date and time"},
    {"itertools", "0.13.0", "Extra iterator adaptors"},
    {"thiserror", "1.0.64", "Derive macros for error types"},
};

struct Match {
  std::string name;
  std::string version;
  std::string about;
  std::string row_class;
  bool operator==(const Match&) const = default;
};

void Fields(const Match& match, FieldSink& out) {
  out("name", match.name);
  out("version", match.version);
  out("about", match.about);
  out("row_class", match.row_class);
}

// The line Screen::Print() writes above the prompt for each pick.
class Added : public Component<Added> {
 public:
  std::string name;
  std::string version;

  explicit Added(const Match& match)
      : name(match.name), version(match.version) {
    Bind(name);
    Bind(version);
  }

  std::string_view view = R"html(
    <div class="line">
      <span class="plus">+</span>
      <span class="name">{name}</span>
      <span class="version">v{version}</span>
    </div>
    <style>
      .line { display: flex; gap: 1; padding-left: 1; }
      .plus { color: rgb(63, 185, 80); font-weight: bold; }
      .name { font-weight: bold; }
      .version { color: rgb(139, 148, 158); }
    </style>
  )html";
};

class Prompt : public Component<Prompt> {
 public:
  // Called with each pick, and once the prompt is done.
  std::function<void(const Match&)> on_pick;
  std::function<void()> on_done;

  std::string query;
  std::vector<Match> matches;
  int selected = 0;
  bool done = false;
  std::string summary;
  std::string hint() const {
    return matches.empty() ? "no match" : "enter add · ↑↓ choose · esc done";
  }

  Prompt() {
    Bind(query);
    Bind(matches);
    Bind(done);
    Bind(summary);
    Bind(hint);
    Bind(Pick);
  }

  // Adds the match at `index` in `matches`; from a click, it arrives as text.
  void Pick(std::string index) {
    const size_t i = index.empty() ? 0 : std::stoul(index);
    if (i >= matches.size()) {
      return;
    }
    picked_.push_back(matches[i].name);
    if (on_pick) {
      on_pick(matches[i]);
    }
    query.clear();
    selected = 0;
  }

  // Keys the focused input does not use reach here.
  bool OnEvent(Event event) override {
    if (done) {
      return false;
    }
    if (event == Event::ArrowDown()) {
      selected++;
      return true;
    }
    if (event == Event::ArrowUp()) {
      selected = std::max(0, selected - 1);
      return true;
    }
    if (event == Event::Return()) {
      Pick(std::to_string(selected));
      return true;
    }
    if (event == Event::Escape() || event == Event::CtrlC()) {
      Finish();
      return true;
    }
    return Component<Prompt>::OnEvent(event);
  }

  // Recomputes the matches from the query before every render.
  bool Digest() override {
    matches.clear();
    for (const Package& package : kPackages) {
      if (FuzzyMatch(query, package.name) &&
          std::ranges::find(picked_, package.name) == picked_.end()) {
        matches.push_back({std::string(package.name),
                           std::string(package.version),
                           std::string(package.about), ""});
      }
    }
    if (matches.size() > 6) {
      matches.resize(6);
    }
    const int count = static_cast<int>(matches.size());
    selected = count == 0 ? 0 : std::clamp(selected, 0, count - 1);
    if (count > 0) {
      matches[selected].row_class = "selected";
    }
    return Component<Prompt>::Digest();
  }

  std::string_view view = R"html(
    <if condition="{done}">
      <div class="summary">{summary}</div>
    </if>
    <else>
      <div class="prompt">
        <span class="label">Add a dependency ›</span>
        <input value="{query}" placeholder="type to search" autofocus=""/>
      </div>
      <div class="list">
        <for each="{matches}" as="m">
          <div class="row {m.row_class}" onclick="Pick({$index})">
            <span class="name">{m.name}</span>
            <span class="version">{m.version}</span>
            <span class="about">{m.about}</span>
          </div>
        </for>
      </div>
      <div class="hint">{hint}</div>
    </else>

    <style>
      self {
        --accent: rgb(88, 166, 255);
        --muted: rgb(110, 118, 129);

        display: block;
        padding-left: 1;
      }
      .prompt { display: flex; gap: 1; }
      .label { color: var(--accent); font-weight: bold; }
      input { width: 30; background-color: rgba(128, 128, 128, 0.2); }
      .row { display: flex; gap: 1; padding-left: 2; }
      /* Translucent: they blend with the terminal's background, light or dark,
         and its own text color reads on them. */
      .row:hover { background-color: rgba(128, 128, 128, 0.2); }
      .selected { background-color: rgba(88, 166, 255, 0.3); }
      .name { width: 12; font-weight: bold; }
      .version { width: 8; color: var(--muted); }
      .about { color: var(--muted); }
      .hint { color: var(--muted); padding-left: 2; }
      .summary { color: rgb(63, 185, 80); font-weight: bold; }
    </style>
  )html";

 private:
  // Whether `pattern`'s letters appear in `text` in order, ignoring case.
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

  void Finish() {
    done = true;
    summary = picked_.empty() ? "Nothing added."
                              : "✔ Added " + std::to_string(picked_.size()) +
                                    (picked_.size() == 1 ? " dependency."
                                                         : " dependencies.");
    if (on_done) {
      on_done();
    }
  }

  std::vector<std::string> picked_;
};

int main() {
  auto app = Ref<Prompt>::New();
  Screen screen(app);
  app->on_pick = [&screen](const Match& match) {
    screen.Print(Ref<Added>::New(match));
  };
  app->on_done = [&screen] { screen.Exit(); };
  screen.Loop();
  return 0;
}
