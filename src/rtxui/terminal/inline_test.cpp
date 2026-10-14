// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
//
// Inline screens (Screen::SetInline), drawn on a terminal emulator that keeps
// what a real one keeps: the lines scrolled into its history, and, on a
// resize, lines rewrapped the way kitty and tmux do or cut the way xterm
// does. Each test checks everything left on it, history included: whatever
// an inline screen gets wrong stays there as garbage.
#include <stdlib.h>

#include <algorithm>
#include <catch2/catch_message.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <cstddef>
#include <cstdlib>
#include <deque>
#include <functional>
#include <iterator>
#include <memory>
#include <optional>
#include <ranges>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

#include "rtxui/component.hpp"
#include "rtxui/event.hpp"
#include "rtxui/refcounted.hpp"
#include "rtxui/screen.hpp"
#include "rtxui/terminal/terminal_device.hpp"

namespace rtxui {
namespace {

// What a terminal does with its lines when it gets narrower.
enum class Narrowing {
  // Lines are rewrapped, and so are the spaces written at their end (kitty).
  kRewrap,
  // Lines are rewrapped, without the spaces at their end (alacritty).
  kRewrapTrimmed,
  // Lines are rewrapped, and the last one stays on its row: the rows they
  // gained go into the history, the cursor's among them if they must, which
  // leaves the cursor on the top row (tmux).
  kRewrapKeepBottom,
  // Lines are cut (xterm).
  kCut,
};

class Terminal {
 public:
  Terminal(int width, int height, Narrowing narrowing = Narrowing::kRewrap)
      : width_(width), height_(height), narrowing_(narrowing) {
    screen_.assign(height_, Line(width_));
  }

  int width() const { return width_; }
  int height() const { return height_; }
  int cursor_x() const { return cx_; }
  int cursor_y() const { return cy_; }
  bool used_alternate_screen() const { return used_alternate_screen_; }
  int clipboard_writes() const { return clipboard_writes_; }

  void Write(std::string_view data) {
    pending_ += data;
    size_t i = 0;
    while (i < pending_.size()) {
      const size_t used = Consume(std::string_view(pending_).substr(i));
      if (used == 0) {
        break;  // An incomplete sequence: the rest comes later.
      }
      i += used;
    }
    pending_.erase(0, i);
  }

  // The replies to queries, as the terminal sends them back as input.
  std::string TakeReplies() { return std::exchange(replies_, ""); }

  void Resize(int width, int height) {
    if (narrowing_ == Narrowing::kCut) {
      Cut(width, height);
    } else {
      Rewrap(width, height);
    }
  }

  // The screen's rows, without trailing spaces or empty rows below.
  std::string Screen() const { return Text(screen_); }
  // The history, then the screen.
  std::string All() const {
    std::vector<Line> lines = history_;
    lines.insert(lines.end(), screen_.begin(), screen_.end());
    return Text(lines);
  }

 private:
  struct Line {
    explicit Line(int width) : cells(width) {}
    // "" for a cell never written to, or erased.
    std::vector<std::string> cells;
    // Whether it goes on to the next line: autowrap moved there.
    bool wrapped = false;
  };

  static std::string Text(const std::vector<Line>& lines) {
    std::vector<std::string> rows;
    for (const Line& line : lines) {
      std::string row;
      for (const std::string& cell : line.cells) {
        row += cell.empty() ? " " : cell;
      }
      row.erase(row.find_last_not_of(' ') + 1);
      rows.push_back(row);
    }
    while (!rows.empty() && rows.back().empty()) {
      rows.pop_back();
    }
    std::string text;
    for (const std::string& row : rows) {
      text += row + "\n";
    }
    return text;
  }

  // Handles the sequence or the character `data` starts with. Returns its
  // size, or 0 when it is incomplete.
  size_t Consume(std::string_view data) {
    const unsigned char c = data[0];
    if (c == '\x1b') {
      if (data.size() < 2) {
        return 0;
      }
      if (data[1] == '[') {
        return ConsumeCsi(data);
      }
      if (data[1] == ']') {
        // OSC, to BEL or ST. Only what is copied to the clipboard is kept.
        for (size_t i = 2; i < data.size(); ++i) {
          size_t end = 0;
          if (data[i] == '\x07') {
            end = i + 1;
          } else if (data[i] == '\x1b' && i + 1 < data.size() &&
                     data[i + 1] == '\\') {
            end = i + 2;
          }
          if (end != 0) {
            if (data.substr(2).starts_with("52;")) {
              ++clipboard_writes_;
            }
            return end;
          }
        }
        return 0;
      }
      return 2;
    }
    if (c == '\r') {
      cx_ = 0;
      pending_wrap_ = false;
      return 1;
    }
    if (c == '\n') {
      LineFeed();
      return 1;
    }
    if (c < 0x20) {
      return 1;
    }
    size_t size = c >= 0xf0 ? 4 : c >= 0xe0 ? 3 : c >= 0xc0 ? 2 : 1;
    if (data.size() < size) {
      return 0;
    }
    Put(std::string(data.substr(0, size)));
    return size;
  }

  size_t ConsumeCsi(std::string_view data) {
    size_t end = 2;
    while (end < data.size() && (data[end] < 0x40 || data[end] > 0x7e)) {
      ++end;
    }
    if (end == data.size()) {
      return 0;
    }
    const char command = data[end];
    std::string_view params = data.substr(2, end - 2);
    const bool is_private = !params.empty() && params[0] == '?';
    if (is_private) {
      params.remove_prefix(1);
    }
    std::vector<int> args;
    int arg = 0;
    bool has_arg = false;
    for (char p : params) {
      if (p >= '0' && p <= '9') {
        arg = arg * 10 + (p - '0');
        has_arg = true;
      } else if (p == ';') {
        args.push_back(has_arg ? arg : 0);
        arg = 0;
        has_arg = false;
      }
    }
    args.push_back(has_arg ? arg : 0);
    const int n = std::max(1, args[0]);
    switch (command) {
      case 'A':
        cy_ = std::max(0, cy_ - n);
        pending_wrap_ = false;
        break;
      case 'B':
        cy_ = std::min(height_ - 1, cy_ + n);
        pending_wrap_ = false;
        break;
      case 'C':
        cx_ = std::min(width_ - 1, cx_ + n);
        pending_wrap_ = false;
        break;
      case 'D':
        cx_ = std::max(0, cx_ - n);
        pending_wrap_ = false;
        break;
      case 'H':
        cy_ = std::clamp(n, 1, height_) - 1;
        cx_ =
            std::clamp(args.size() > 1 ? std::max(1, args[1]) : 1, 1, width_) -
            1;
        pending_wrap_ = false;
        break;
      case 'J':
        if (args[0] == 2) {
          screen_.assign(height_, Line(width_));
        } else {
          EraseLine();
          for (int y = cy_ + 1; y < height_; ++y) {
            screen_[y] = Line(width_);
          }
        }
        break;
      case 'K':
        EraseLine();
        break;
      case 'h':
      case 'l':
        if (is_private && args[0] == 7) {
          autowrap_ = command == 'h';
        }
        if (is_private && args[0] == 1049) {
          SwitchScreen(/*alternate=*/command == 'h');
        }
        break;
      case 'n':
        if (args[0] == 6) {
          replies_ += "\x1b[" + std::to_string(cy_ + 1) + ";" +
                      std::to_string(cx_ + 1) + "R";
        }
        break;
      default:
        break;  // Colors, cursor shape: nothing to keep.
    }
    return end + 1;
  }

  // The alternate screen has no history, and leaving it brings back the
  // main one as it was, cursor included.
  void SwitchScreen(bool alternate) {
    if (alternate == on_alternate_screen_) {
      return;
    }
    on_alternate_screen_ = alternate;
    if (alternate) {
      used_alternate_screen_ = true;
      main_screen_ = std::move(screen_);
      main_cursor_ = {cx_, cy_};
      screen_.assign(height_, Line(width_));
      cx_ = 0;
      cy_ = 0;
    } else {
      screen_ = std::move(main_screen_);
      std::tie(cx_, cy_) = main_cursor_;
    }
    pending_wrap_ = false;
  }

  void EraseLine() {
    for (int x = cx_; x < width_; ++x) {
      screen_[cy_].cells[x].clear();
    }
    screen_[cy_].wrapped = false;
  }

  void LineFeed() {
    pending_wrap_ = false;
    if (cy_ < height_ - 1) {
      ++cy_;
      return;
    }
    if (!on_alternate_screen_) {
      history_.push_back(std::move(screen_.front()));
    }
    screen_.erase(screen_.begin());
    screen_.emplace_back(width_);
  }

  void Put(std::string glyph) {
    if (pending_wrap_) {
      screen_[cy_].wrapped = true;
      cx_ = 0;
      LineFeed();
    }
    screen_[cy_].cells[cx_] = std::move(glyph);
    if (cx_ < width_ - 1) {
      ++cx_;
    } else {
      pending_wrap_ = autowrap_;
    }
  }

  // xterm: rows are cut or padded, and rows go to the history only when the
  // cursor would be below the screen.
  void Cut(int width, int height) {
    for (auto* lines : {&history_, &screen_}) {
      for (Line& line : *lines) {
        line.cells.resize(width);
        line.wrapped = false;
      }
    }
    while (static_cast<int>(screen_.size()) > height) {
      if (cy_ < static_cast<int>(screen_.size()) - 1) {
        screen_.pop_back();
      } else {
        history_.push_back(std::move(screen_.front()));
        screen_.erase(screen_.begin());
        --cy_;
      }
    }
    while (static_cast<int>(screen_.size()) < height) {
      screen_.emplace_back(width);
    }
    width_ = width;
    height_ = height;
    cx_ = std::min(cx_, width_ - 1);
    pending_wrap_ = false;
  }

  // kitty, tmux: the lines autowrap split are joined back, then split again
  // at the new width. The cursor stays on the glyph it was on.
  void Rewrap(int width, int height) {
    std::vector<Line> all = std::move(history_);
    all.insert(all.end(), screen_.begin(), screen_.end());
    const int cursor_line = static_cast<int>(all.size()) - height_ + cy_;
    // The screen row of the last one with anything on it, or the cursor's.
    int last_row = cy_;
    for (int y = 0; y < height_; ++y) {
      if (std::ranges::any_of(screen_[y].cells, [](const std::string& cell) {
            return !cell.empty();
          })) {
        last_row = std::max(last_row, y);
      }
    }

    struct Logical {
      std::vector<std::string> cells;
    };
    std::vector<Logical> logical;
    int cursor_logical = 0;
    int cursor_offset = 0;
    bool continues = false;
    for (int i = 0; i < static_cast<int>(all.size()); ++i) {
      if (!continues) {
        logical.emplace_back();
      }
      if (i == cursor_line) {
        cursor_logical = static_cast<int>(logical.size()) - 1;
        cursor_offset = static_cast<int>(logical.back().cells.size()) + cx_;
      }
      auto& cells = logical.back().cells;
      cells.insert(cells.end(), all[i].cells.begin(), all[i].cells.end());
      continues = all[i].wrapped;
    }
    for (Logical& line : logical) {
      while (!line.cells.empty() &&
             (line.cells.back().empty() ||
              (narrowing_ != Narrowing::kRewrap && line.cells.back() == " "))) {
        line.cells.pop_back();
      }
    }
    // The empty lines below the cursor are only the screen's bottom.
    while (static_cast<int>(logical.size()) > cursor_logical + 1 &&
           logical.back().cells.empty()) {
      logical.pop_back();
    }

    std::vector<Line> lines;
    int cursor_row = 0;
    for (int i = 0; i < static_cast<int>(logical.size()); ++i) {
      const auto& cells = logical[i].cells;
      int rows = std::max<int>(1, (cells.size() + width - 1) / width);
      if (i == cursor_logical) {
        rows = std::max(rows, cursor_offset / width + 1);
        cursor_row = static_cast<int>(lines.size()) + cursor_offset / width;
      }
      for (int r = 0; r < rows; ++r) {
        Line line(width);
        for (int x = 0; x < width; ++x) {
          const size_t at = static_cast<size_t>(r) * width + x;
          if (at < cells.size()) {
            line.cells[x] = cells[at];
          }
        }
        line.wrapped = r + 1 < rows;
        lines.push_back(std::move(line));
      }
    }
    // The last lines, but always the cursor's: lines below it are dropped
    // first.
    int top = std::min(std::max(0, static_cast<int>(lines.size()) - height),
                       cursor_row);
    if (narrowing_ == Narrowing::kRewrapKeepBottom) {
      int last_line = cursor_row;
      for (int i = 0; i < static_cast<int>(lines.size()); ++i) {
        if (std::ranges::any_of(lines[i].cells, [](const std::string& cell) {
              return !cell.empty();
            })) {
          last_line = std::max(last_line, i);
        }
      }
      top = std::max(0, last_line - last_row);
      if (cursor_row < top) {
        cursor_row = top;
        cursor_offset = 0;
      }
    }
    const int bottom = std::min(static_cast<int>(lines.size()), top + height);
    history_.assign(std::make_move_iterator(lines.begin()),
                    std::make_move_iterator(lines.begin() + top));
    screen_.assign(std::make_move_iterator(lines.begin() + top),
                   std::make_move_iterator(lines.begin() + bottom));
    while (static_cast<int>(screen_.size()) < height) {
      screen_.emplace_back(width);
    }
    width_ = width;
    height_ = height;
    cy_ = cursor_row - top;
    cx_ = cursor_offset % width;
    pending_wrap_ = false;
  }

  int width_;
  int height_;
  Narrowing narrowing_;
  std::vector<Line> history_;
  std::vector<Line> screen_;
  int cx_ = 0;
  int cy_ = 0;
  bool pending_wrap_ = false;
  bool autowrap_ = true;
  bool used_alternate_screen_ = false;
  int clipboard_writes_ = 0;
  bool on_alternate_screen_ = false;
  std::vector<Line> main_screen_;
  std::pair<int, int> main_cursor_;
  std::string pending_;
  std::string replies_;
};

// A TerminalDevice on a Terminal, fed with a script: each step runs when the
// screen has handled all the input of the previous ones. Loop() returns after
// the last.
class EmulatorDevice : public TerminalDevice {
 public:
  explicit EmulatorDevice(Terminal& terminal) : terminal_(terminal) {}

  void Then(std::function<void()> step) { script_.push_back(std::move(step)); }
  void Type(std::string bytes) {
    Then([this, bytes] { input_ += bytes; });
  }
  // A left click on the terminal's cell at column `x`, row `y`, 0-based.
  void Click(int x, int y) {
    const std::string at = std::to_string(x + 1) + ";" + std::to_string(y + 1);
    Type("\x1b[<0;" + at + "M\x1b[<0;" + at + "m");
  }
  // A left drag from column `x0`, row `y0` to `x1`, `y1` of the terminal.
  void Drag(int x0, int y0, int x1, int y1) {
    auto at = [](int x, int y) {
      return std::to_string(x + 1) + ";" + std::to_string(y + 1);
    };
    Type("\x1b[<0;" + at(x0, y0) + "M\x1b[<32;" + at(x1, y1) + "M\x1b[<0;" +
         at(x1, y1) + "m");
  }
  void Resize(int width, int height) {
    Then([this, width, height] {
      terminal_.Resize(width, height);
      // The screen sees the new size on its next read: something to read.
      input_ += "\x1b]99;resized\x07";
    });
  }

  bool IsAtty() override { return true; }
  int Read(char* buf, int len) override {
    while (input_.empty() && !script_.empty()) {
      auto step = std::move(script_.front());
      script_.pop_front();
      step();
    }
    const int count = std::min(len, static_cast<int>(input_.size()));
    std::copy_n(input_.begin(), count, buf);
    input_.erase(0, count);
    return count;
  }
  void Write(std::string_view data) override {
    // A Screen on a terminal draws nothing before Loop(); this one drew from
    // its constructor, as it does on a device that is no terminal.
    if (!started_) {
      return;
    }
    // The terminal driver turns each line feed into a carriage return and a
    // line feed (ONLCR), which raw mode leaves on.
    std::string translated;
    for (char c : data) {
      if (c == '\n') {
        translated += '\r';
      }
      translated += c;
    }
    terminal_.Write(translated);
    input_ += terminal_.TakeReplies();
  }
  bool GetSize(int& width, int& height) override {
    width = terminal_.width();
    height = terminal_.height();
    return true;
  }
  void EnterRawMode(void (* /*sigwinch_handler*/)(int),
                    bool alternate_screen) override {
    started_ = true;
    alternate_screen_ = alternate_screen;
    Write(std::string(alternate_screen ? "\x1b[?1049h" : "") +
          "\x1b[?7l\x1b[?25l\x1b[?1003h\x1b[?1006h");
  }
  void ExitRawMode() override {
    Write("\x1b[?1003l\x1b[?1006l\x1b[?25h\x1b[?7h" +
          std::string(alternate_screen_ ? "\x1b[?1049l" : ""));
  }

 private:
  Terminal& terminal_;
  std::deque<std::function<void()>> script_;
  std::string input_;
  bool started_ = false;
  bool alternate_screen_ = true;
};

// One line per string, which keys change: '+' adds one, '-' removes the
// last. Any other key goes to `on_key`.
class Lines : public Component<Lines> {
 public:
  std::vector<std::string> lines;
  std::function<void(char)> on_key;
  std::string_view view =
      R"(<for each="{lines}" as="line"><div>{line}</div></for>)";

  explicit Lines(std::vector<std::string> initial) : lines(std::move(initial)) {
    Bind(lines);
  }

  bool OnEvent(Event event) override {
    auto* key = event.get_if<Event::Keyboard>();
    if (!key || key->codepoint == 0) {
      return Component<Lines>::OnEvent(event);
    }
    const char c = static_cast<char>(key->codepoint);
    if (c == '+') {
      lines.push_back("line " + std::to_string(lines.size()));
    } else if (c == '-' && !lines.empty()) {
      lines.pop_back();
    } else if (on_key) {
      on_key(c);
    }
    return true;
  }
};

// Runs `app` inline on `terminal` through the script of `device`.
void RunInline(Ref<ComponentBase> app,
               std::shared_ptr<EmulatorDevice> device,
               const std::function<void(Screen&)>& setup = {}) {
  Screen screen(std::move(app), device);
  screen.SetInline(true);
  if (setup) {
    setup(screen);
  }
  screen.Loop();
}

std::vector<std::string> Numbered(std::string_view prefix, int count) {
  std::vector<std::string> lines;
  for (int i = 0; i < count; ++i) {
    lines.push_back(std::string(prefix) + std::to_string(i));
  }
  return lines;
}

// Sets environment variables for its lifetime, then puts them back.
class ScopedEnvironment {
 public:
  void Set(const char* name, const char* value) {
    const char* old = std::getenv(name);
    saved_.emplace_back(name,
                        old ? std::optional<std::string>(old) : std::nullopt);
    if (value) {
      setenv(name, value, /*overwrite=*/1);
    } else {
      unsetenv(name);
    }
  }
  ~ScopedEnvironment() {
    for (const auto& [name, value] : std::ranges::reverse_view(saved_)) {
      if (value) {
        setenv(name.c_str(), value->c_str(), /*overwrite=*/1);
      } else {
        unsetenv(name.c_str());
      }
    }
  }

 private:
  std::vector<std::pair<std::string, std::optional<std::string>>> saved_;
};

TEST_CASE("An inline screen draws below the shell, and leaves its last frame",
          "[inline]") {
  Terminal terminal(20, 10);
  terminal.Write("$ ls\r\nfile\r\n$ ./app\r\n");
  auto device = std::make_shared<EmulatorDevice>(terminal);
  RunInline(Ref<Lines>::New(Numbered("row ", 3)), device);

  CHECK(terminal.All() ==
        "$ ls\n"
        "file\n"
        "$ ./app\n"
        "row 0\n"
        "row 1\n"
        "row 2\n");
  // The shell goes on below.
  CHECK(terminal.cursor_x() == 0);
  CHECK(terminal.cursor_y() == 6);
  CHECK_FALSE(terminal.used_alternate_screen());
}

TEST_CASE("An inline screen starts on a line of its own", "[inline]") {
  Terminal terminal(20, 10);
  terminal.Write("$ printf no-newline\r\nno-newline");
  auto device = std::make_shared<EmulatorDevice>(terminal);
  RunInline(Ref<Lines>::New(Numbered("row ", 2)), device);

  CHECK(terminal.All() ==
        "$ printf no-newline\n"
        "no-newline\n"
        "row 0\n"
        "row 1\n");
}

TEST_CASE("An inline screen erases what it no longer draws", "[inline]") {
  Terminal terminal(20, 10);
  terminal.Write("$ ./app\r\n");
  auto device = std::make_shared<EmulatorDevice>(terminal);
  device->Type("++");
  device->Then([&] {
    CHECK(terminal.All() ==
          "$ ./app\n"
          "row 0\n"
          "line 1\n"
          "line 2\n");
  });
  device->Type("---");
  RunInline(Ref<Lines>::New(Numbered("row ", 1)), device);

  CHECK(terminal.All() == "$ ./app\n");
  CHECK(terminal.cursor_y() == 1);
}

TEST_CASE("An inline screen growing past the bottom scrolls the shell up",
          "[inline]") {
  Terminal terminal(20, 5);
  terminal.Write("one\r\ntwo\r\nthree\r\nfour\r\n");
  auto device = std::make_shared<EmulatorDevice>(terminal);
  device->Type("+++");
  RunInline(Ref<Lines>::New(Numbered("row ", 1)), device);

  // The last frame took the bottom 4 rows; the shell then got a row of its
  // own below it.
  CHECK(terminal.All() ==
        "one\n"
        "two\n"
        "three\n"
        "four\n"
        "row 0\n"
        "line 1\n"
        "line 2\n"
        "line 3\n");
  CHECK(terminal.Screen() ==
        "row 0\n"
        "line 1\n"
        "line 2\n"
        "line 3\n");
}

TEST_CASE("An inline screen is at most as tall as the terminal", "[inline]") {
  Terminal terminal(20, 4);
  terminal.Write("$ ./app\r\n");
  auto device = std::make_shared<EmulatorDevice>(terminal);
  device->Then([&] {
    // The rows past the terminal's height are not drawn: nothing of the
    // frame reaches the history, where it could never be erased.
    CHECK(terminal.All() ==
          "$ ./app\n"
          "row 0\n"
          "row 1\n"
          "row 2\n"
          "row 3\n");
  });
  device->Type("-------");
  RunInline(Ref<Lines>::New(Numbered("row ", 10)), device);

  CHECK(terminal.All() ==
        "$ ./app\n"
        "row 0\n"
        "row 1\n"
        "row 2\n");
}

TEST_CASE("An inline screen fills the terminal at height: 100%", "[inline]") {
  class Full : public Component<Full> {
   public:
    std::string_view view = R"(
      <div class="box">top</div>
      <style>
        .box { height: 100%; border: solid; }
      </style>)";
  };
  Terminal terminal(10, 4);
  terminal.Write("$ ./app\r\n");
  auto device = std::make_shared<EmulatorDevice>(terminal);
  device->Then([&] {
    CHECK(terminal.Screen() ==
          "┌────────┐\n"
          "│top     │\n"
          "│        │\n"
          "└────────┘\n");
  });
  RunInline(Ref<Full>::New(), device);

  // The shell's line below it scrolled it up.
  CHECK(terminal.All() ==
        "$ ./app\n"
        "┌────────┐\n"
        "│top     │\n"
        "│        │\n"
        "└────────┘\n");
}

class Clicks : public Component<Clicks> {
 public:
  int hits = 0;
  void Hit() { hits++; }
  std::string_view view =
      R"(<div>head</div><div onclick="Hit">hits {hits}</div>)";
  Clicks() {
    Bind(hits);
    Bind(Hit);
  }
};

TEST_CASE("An inline screen takes clicks on its own rows only", "[inline]") {
  Terminal terminal(20, 6);
  terminal.Write("$ ./app\r\n");
  auto device = std::make_shared<EmulatorDevice>(terminal);
  // The frame is on rows 1 and 2.
  device->Click(1, 2);
  device->Click(1, 2);
  // On the shell's line, and below the frame: nothing.
  device->Click(1, 0);
  device->Click(1, 4);
  RunInline(Ref<Clicks>::New(), device);

  CHECK(terminal.All() ==
        "$ ./app\n"
        "head\n"
        "hits 2\n");
}

TEST_CASE("An inline screen follows the shell scrolling up", "[inline]") {
  Terminal terminal(20, 4);
  terminal.Write("a\r\nb\r\nc\r\n$ ./app\r\n");
  auto device = std::make_shared<EmulatorDevice>(terminal);
  // Drawn at the bottom, the frame scrolled the terminal: it is on the last
  // two rows.
  device->Click(1, 3);
  RunInline(Ref<Clicks>::New(), device);

  CHECK(terminal.All() ==
        "a\n"
        "b\n"
        "c\n"
        "$ ./app\n"
        "head\n"
        "hits 1\n");
}

TEST_CASE("An inline screen follows itself scrolling the shell up",
          "[inline]") {
  // Rows added above a clickable one, past the terminal's bottom.
  class Growing : public Component<Growing> {
   public:
    std::vector<std::string> rows;
    int hits = 0;
    void Hit() { hits++; }
    std::string_view view = R"(<div onclick="Hit">hits {hits}</div>
      <for each="{rows}" as="row"><div>{row}</div></for>)";
    Growing() {
      Bind(rows);
      Bind(hits);
      Bind(Hit);
    }
    bool OnEvent(Event event) override {
      if (event == Event(Event::Keyboard::From('+'))) {
        rows.push_back("row");
        return true;
      }
      return Component<Growing>::OnEvent(event);
    }
  };
  Terminal terminal(20, 5);
  terminal.Write("a\r\nb\r\nc\r\n$ ./app\r\n");
  auto device = std::make_shared<EmulatorDevice>(terminal);
  device->Type("+");
  device->Type("+");
  device->Type("+");
  // The frame now takes the terminal's last 4 rows.
  device->Click(1, 1);
  RunInline(Ref<Growing>::New(), device);

  CHECK(terminal.All().ends_with("hits 1\nrow\nrow\nrow\n"));
}

TEST_CASE("A drag from the shell's lines selects nothing inline", "[inline]") {
  Terminal terminal(20, 6);
  terminal.Write("$ ./app\r\n");
  auto device = std::make_shared<EmulatorDevice>(terminal);
  // From the shell's line, into the frame on rows 1 and 2.
  device->Drag(0, 0, 5, 2);
  device->Then([&] { CHECK(terminal.clipboard_writes() == 0); });
  // The same drag, inside the frame, copies.
  device->Drag(0, 1, 5, 2);
  device->Then([&] { CHECK(terminal.clipboard_writes() == 1); });
  RunInline(Ref<Lines>::New(Numbered("row ", 2)), device);
}

TEST_CASE("Print writes lines above an inline screen", "[inline]") {
  Terminal terminal(20, 6);
  terminal.Write("$ ./app\r\n");
  auto device = std::make_shared<EmulatorDevice>(terminal);
  auto app = Ref<Lines>::New(Numbered("frame ", 2));
  Screen* screen = nullptr;
  app->on_key = [&](char c) { screen->Print(std::string("printed ") + c); };
  device->Type("a");
  device->Type("bc");
  // Longer than the terminal is wide, and two lines.
  device->Then([&] {
    screen->Print("a long line wrapped by the terminal\nand a second");
  });
  device->Type("+");
  RunInline(app, device, [&](Screen& s) { screen = &s; });

  CHECK(terminal.All() ==
        "$ ./app\n"
        "printed a\n"
        "printed b\n"
        "printed c\n"
        "a long line wrapped\n"
        "by the terminal\n"
        "and a second\n"
        "frame 0\n"
        "frame 1\n"
        "line 2\n");
}

TEST_CASE("Print before Loop() writes once Loop() draws", "[inline]") {
  Terminal terminal(20, 6);
  terminal.Write("$ ./app\r\n");
  auto device = std::make_shared<EmulatorDevice>(terminal);
  // A frame drawn before Loop() draws on no terminal yet (as a headless run
  // does): it must leave the line printed meanwhile for Loop() to write.
  RunInline(Ref<Lines>::New(Numbered("frame ", 1)), device, [](Screen& screen) {
    screen.Print("early");
    screen.Draw();
  });

  CHECK(terminal.All() ==
        "$ ./app\n"
        "early\n"
        "frame 0\n");
}

TEST_CASE("Print writes what a component draws", "[inline]") {
  class Done : public Component<Done> {
   public:
    std::string name;
    std::string_view view = R"(<div class="done">✔ {name}</div>
      <style>.done { padding-left: 2; }</style>)";
    explicit Done(std::string n) : name(std::move(n)) { Bind(name); }
  };
  Terminal terminal(20, 6);
  terminal.Write("$ ./app\r\n");
  auto device = std::make_shared<EmulatorDevice>(terminal);
  auto app = Ref<Lines>::New(Numbered("frame ", 1));
  Screen* screen = nullptr;
  app->on_key = [&](char c) {
    screen->Print(Ref<Done>::New(std::string("pkg-") + c));
  };
  device->Type("a");
  device->Type("b");
  RunInline(app, device, [&](Screen& s) { screen = &s; });

  CHECK(terminal.All() ==
        "$ ./app\n"
        "  ✔ pkg-a\n"
        "  ✔ pkg-b\n"
        "frame 0\n");
}

TEST_CASE("Print, on the alternate screen, writes once Loop() returns",
          "[inline]") {
  Terminal terminal(20, 6);
  terminal.Write("$ ./app\r\n");
  auto device = std::make_shared<EmulatorDevice>(terminal);
  auto app = Ref<Lines>::New(Numbered("frame ", 1));
  Screen screen(app, device);
  app->on_key = [&](char c) { screen.Print(std::string("done ") + c); };
  device->Type("a");
  screen.Loop();

  CHECK(terminal.used_alternate_screen());
  CHECK(terminal.All() ==
        "$ ./app\n"
        "done a\n");
}

TEST_CASE("An inline screen drawing nothing leaves nothing", "[inline]") {
  Terminal terminal(20, 6);
  terminal.Write("$ ./app\r\n");
  auto device = std::make_shared<EmulatorDevice>(terminal);
  device->Type("--");
  RunInline(Ref<Lines>::New(Numbered("row ", 2)), device);

  CHECK(terminal.All() == "$ ./app\n");
  CHECK(terminal.cursor_x() == 0);
  CHECK(terminal.cursor_y() == 1);
}

// A frame of full-width rows, the worst case for a terminal rewrapping them.
class Boxes : public Component<Boxes> {
 public:
  std::string_view view = R"(
    <div class="box">one</div>
    <div class="box">two</div>
    <style>.box { border: solid; }</style>)";
};

TEST_CASE("An inline screen redraws whole when the terminal is resized",
          "[inline]") {
  const auto narrowing =
      GENERATE(Narrowing::kRewrap, Narrowing::kRewrapTrimmed,
               Narrowing::kRewrapKeepBottom, Narrowing::kCut);
  ScopedEnvironment environment;
  environment.Set("TMUX", nullptr);
  environment.Set("XTERM_VERSION",
                  narrowing == Narrowing::kCut ? "XTerm(407)" : nullptr);
  Terminal terminal(20, 12, narrowing);
  terminal.Write("$ ./app\r\n");
  auto device = std::make_shared<EmulatorDevice>(terminal);
  // What is left once resized: nothing but the frame, below the shell's
  // line. A terminal pushing the rows rewrapping added into its history
  // leaves pieces of the old frame there, out of anyone's reach: then only
  // the screen is clean.
  auto check = [&](const std::string& frame) {
    if (narrowing == Narrowing::kRewrapKeepBottom) {
      const std::string screen = terminal.Screen();
      CAPTURE(screen);
      CHECK((screen == frame || screen == "$ ./app\n" + frame));
    } else {
      CHECK(terminal.All() == "$ ./app\n" + frame);
    }
  };
  device->Resize(12, 12);
  device->Then([&] {
    check(
        "┌──────────┐\n"
        "│one       │\n"
        "└──────────┘\n"
        "┌──────────┐\n"
        "│two       │\n"
        "└──────────┘\n");
  });
  device->Resize(16, 12);
  RunInline(Ref<Boxes>::New(), device);

  check(
      "┌──────────────┐\n"
      "│one           │\n"
      "└──────────────┘\n"
      "┌──────────────┐\n"
      "│two           │\n"
      "└──────────────┘\n");
}

// Full-width rows above an input, which keeps the terminal's cursor on its
// caret, where input methods show what is being composed.
class Form : public Component<Form> {
 public:
  std::string name;
  std::string_view view = R"(
    <div class="box">one</div>
    <div class="box">two</div>
    <input value="{name}" autofocus=""/>
    <style>.box { border: solid; }</style>)";
  Form() { Bind(name); }
};

TEST_CASE("An inline screen keeps the cursor on a focused input's caret",
          "[inline]") {
  Terminal terminal(20, 12);
  terminal.Write("$ ./app\r\n");
  auto device = std::make_shared<EmulatorDevice>(terminal);
  device->Type("abc");
  device->Then([&] {
    const std::string screen = terminal.Screen();
    REQUIRE(screen.find("abc") != std::string::npos);
    // On the input's row, after "abc".
    const int row = static_cast<int>(
        std::count(screen.begin(), screen.begin() + screen.find("abc"), '\n'));
    CHECK(terminal.cursor_y() == row);
    const size_t line_start = screen.rfind('\n', screen.find("abc")) + 1;
    CHECK(terminal.cursor_x() ==
          static_cast<int>(screen.find("abc") - line_start) + 3);
  });
  RunInline(Ref<Form>::New(), device);
}

TEST_CASE("An inline screen with a caret redraws whole when resized",
          "[inline]") {
  const auto narrowing =
      GENERATE(Narrowing::kRewrap, Narrowing::kRewrapTrimmed,
               Narrowing::kRewrapKeepBottom, Narrowing::kCut);
  ScopedEnvironment environment;
  environment.Set("TMUX", nullptr);
  environment.Set("XTERM_VERSION",
                  narrowing == Narrowing::kCut ? "XTerm(407)" : nullptr);
  Terminal terminal(20, 14, narrowing);
  terminal.Write("$ ./app\r\n");
  auto device = std::make_shared<EmulatorDevice>(terminal);
  device->Type("abc");
  device->Resize(12, 14);
  device->Resize(9, 14);
  device->Resize(16, 14);
  RunInline(Ref<Form>::New(), device);

  // As above: the screen only, when the history keeps pieces of the frame.
  const std::string text = narrowing == Narrowing::kRewrapKeepBottom
                               ? terminal.Screen()
                               : terminal.All();
  CAPTURE(text);
  const std::string frame =
      "┌──────────────┐\n"
      "│one           │\n"
      "└──────────────┘\n"
      "┌──────────────┐\n"
      "│two           │\n"
      "└──────────────┘\n";
  const size_t at = text.find(frame);
  REQUIRE(at != std::string::npos);
  CHECK((at == 0 || text.substr(0, at) == "$ ./app\n"));
  // The input, once, and nothing after it.
  CHECK(std::count(text.begin() + at, text.end(), '\n') == 7);
  CHECK(text.find("abc") != std::string::npos);
}

TEST_CASE("An inline screen survives a terminal getting shorter", "[inline]") {
  Terminal terminal(20, 10);
  terminal.Write("a\r\nb\r\n$ ./app\r\n");
  auto device = std::make_shared<EmulatorDevice>(terminal);
  device->Resize(20, 4);
  device->Then([&] {
    CHECK(terminal.Screen() ==
          "row 0\n"
          "row 1\n"
          "row 2\n"
          "row 3\n");
  });
  device->Resize(20, 10);
  RunInline(Ref<Lines>::New(Numbered("row ", 6)), device);

  const std::string all = terminal.All();
  CAPTURE(all);
  CHECK(
      all.ends_with("row 0\n"
                    "row 1\n"
                    "row 2\n"
                    "row 3\n"
                    "row 4\n"
                    "row 5\n"));
  CHECK(all.starts_with("a\nb\n$ ./app\nrow 0\n"));
}

TEST_CASE("A click on the shell's lines leaves the focus inline", "[inline]") {
  Terminal terminal(20, 12);
  terminal.Write("$ ./app\r\n");
  auto device = std::make_shared<EmulatorDevice>(terminal);
  device->Type("ab");
  device->Click(1, 0);
  device->Type("c");
  auto app = Ref<Form>::New();
  RunInline(app, device);

  CHECK(app->name == "abc");
}

TEST_CASE("An inline screen is as tall as what overflows it", "[inline]") {
  class Menu : public Component<Menu> {
   public:
    std::string_view view = R"(
      <div class="anchor">menu<div class="popup">item</div></div>
      <style>
        .anchor { position: relative; }
        .popup { position: absolute; top: 1; left: 2; }
      </style>)";
  };
  Terminal terminal(20, 6);
  terminal.Write("$ ./app\r\n");
  auto device = std::make_shared<EmulatorDevice>(terminal);
  RunInline(Ref<Menu>::New(), device);

  CHECK(terminal.All() ==
        "$ ./app\n"
        "menu\n"
        "  item\n");
}

TEST_CASE("A press on the shell's lines clicks nothing inline", "[inline]") {
  class Hidden : public Component<Hidden> {
   public:
    int hits = 0;
    void Hit() { hits++; }
    // Hanging above the frame, over the shell's line, where it is not drawn.
    std::string_view view = R"(
      <div class="anchor">frame<div class="ghost" onclick="Hit">ghost</div></div>
      <div onclick="Hit">button</div>
      <style>
        .anchor { position: relative; }
        .ghost { position: absolute; top: -1; }
      </style>)";
    Hidden() {
      Bind(hits);
      Bind(Hit);
    }
  };
  Terminal terminal(20, 6);
  terminal.Write("$ ./app\r\n");
  auto device = std::make_shared<EmulatorDevice>(terminal);
  auto app = Ref<Hidden>::New();
  device->Click(1, 0);
  // From the shell's line to the button: no click either.
  device->Drag(1, 0, 1, 2);
  RunInline(app, device);

  CHECK(app->hits == 0);
}

TEST_CASE("An inline application sees no mouse button outside it", "[inline]") {
  class Canvas : public Component<Canvas> {
   public:
    std::vector<std::string> buttons;
    std::string_view view = R"(<div>canvas</div>)";
    bool OnEvent(Event event) override {
      if (auto* mouse = event.get_if<Event::Mouse>();
          mouse && mouse->motion != Event::Mouse::Moved) {
        buttons.push_back(
            (mouse->motion == Event::Mouse::Pressed ? "press " : "release ") +
            std::to_string(mouse->y));
      }
      return Component<Canvas>::OnEvent(event);
    }
  };
  Terminal terminal(20, 6);
  terminal.Write("$ ./app\r\n");
  auto device = std::make_shared<EmulatorDevice>(terminal);
  device->Click(1, 0);       // On the shell's line.
  device->Drag(1, 0, 1, 1);  // From there into the frame.
  device->Click(1, 1);       // On the frame: row 1, 1-based.
  device->Drag(1, 1, 1, 3);  // From the frame to below it.
  auto app = Ref<Canvas>::New();
  RunInline(app, device);

  CHECK(app->buttons == std::vector<std::string>{"press 1", "release 1",
                                                 "press 1", "release 3"});
}

TEST_CASE("Print draws a component in the terminal's text color", "[inline]") {
  class Word : public Component<Word> {
   public:
    std::string_view view = R"(<div>word</div>)";
  };
  auto device = std::make_shared<MockTerminalDevice>();
  auto app = Ref<Lines>::New(Numbered("frame ", 1));
  Screen screen(app, device);
  screen.SetInline(true);
  app->on_key = [&](char) { screen.Print(Ref<Word>::New()); };
  device->PushInput("\x1b]10;rgb:2020/2020/2020\x1b\\a");
  device->ClearOutput();
  screen.Loop();

  const std::string output = device->GetOutput();
  const size_t word = output.find("word");
  REQUIRE(word != std::string::npos);
  // Printed with autowrap on, in the terminal's default color, as the frame
  // drawn since the terminal answered is.
  const size_t printed = output.rfind("\x1b[?7h", word);
  REQUIRE(printed != std::string::npos);
  CHECK(output.substr(printed, word - printed).find("38;2;255;255;255m") ==
        std::string::npos);
}

}  // namespace
}  // namespace rtxui
