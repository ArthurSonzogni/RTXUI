// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include "terminal/screen.hpp"

#include <sys/ioctl.h>
#include <unistd.h>
#include <iostream>

#include "layout/layout.hpp"
#include "layout/layout_tree_builder.hpp"
#include "paint/paint.hpp"
#include "paint/texture.hpp"
#include "terminal/terminal_input_parser.hpp"

namespace rtxui {

Screen::Screen(Ref<ComponentBase> component)
    : component_(std::move(component)) {
  raw_terminal_ = std::make_unique<RawTerminal>();
  UpdateSize();
  component_->Mount();
  Draw();
}

Screen::~Screen() {
  raw_terminal_.reset();
}

void Screen::Loop() {
  TerminalInputParser parser;
  while (true) {
    char c;
    if (read(STDIN_FILENO, &c, 1) != 1) {
      continue;
    }

    UpdateSize();
    parser.Add(c);

    while (auto event = parser.GetEvent()) {
      if (component_->OnEvent(*event)) {
        DigestAndDraw();
        continue;
      }
      if (*event == Event::Escape() || *event == Event::CtrlC()) {
        return;
      }
    }
  }
}

void Screen::Draw() {
  auto root = component_->Root();
  auto root_box = LayoutTreeBuilder::Build(root);

  LayoutConstraints viewport = {
      {width_, MeasureMode::Exactly},
      {height_, MeasureMode::Exactly},
  };
  auto root_fragment = RunLayout({root_box.get()}, viewport);

  Texture texture(width_, height_);
  Paint(root_fragment.get(), texture);

  std::string new_output = texture.Render();

  if (has_drawn_ && last_height_ > 0) {
    // Reset cursor up by the number of printed lines
    std::cout << "\x1b[" << last_height_ << "A";
  }

  std::cout << new_output << std::flush;

  last_height_ = 0;
  for (char ch : new_output) {
    if (ch == '\n') {
      last_height_++;
    }
  }
  has_drawn_ = true;
}

void Screen::UpdateSize() {
  struct winsize w;
  if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0 && w.ws_col > 0 && w.ws_row > 0) {
    if (w.ws_col != width_ || w.ws_row != height_) {
      width_ = w.ws_col;
      height_ = w.ws_row;
      if (has_drawn_) {
        component_->Render();
        Draw();
      }
    }
  }
}

void Screen::DigestAndDraw() {
  if (component_->Digest()) {
    Draw();
  }
}

// --- RawTerminal RAII Implementation ---

Screen::RawTerminal::RawTerminal() {
  if (isatty(STDIN_FILENO)) {
    tcgetattr(STDIN_FILENO, &previous_termios_);
    termios terminal = previous_termios_;

    terminal.c_iflag &= ~IGNBRK;
    terminal.c_iflag &= ~BRKINT;
    terminal.c_iflag &= ~PARMRK;
    terminal.c_iflag &= ~ISTRIP;
    terminal.c_iflag &= ~INLCR;
    terminal.c_iflag &= ~IGNCR;
    terminal.c_iflag &= ~ICRNL;
    terminal.c_iflag &= ~IXON;

    terminal.c_lflag &= ~ECHO;
    terminal.c_lflag &= ~ECHONL;
    terminal.c_lflag &= ~ICANON;
    terminal.c_lflag &= ~ISIG;
    terminal.c_lflag &= ~IEXTEN;
    terminal.c_cflag |= CS8;

    terminal.c_cc[VMIN] = 1;  // Block until at least 1 character is available
    terminal.c_cc[VTIME] = 0; // No timeout
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &terminal);

    std::cout << "\x1b[?7l";  // Disable line wrapping
    std::cout << "\x1b[?25l" << std::flush; // Hide cursor
  }
}

Screen::RawTerminal::~RawTerminal() {
  if (isatty(STDIN_FILENO)) {
    std::cout << "\x1b[?7h";  // Enable line wrapping
    std::cout << "\x1b[?25h" << std::flush; // Show cursor
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &previous_termios_);
  }
}

} // namespace rtxui
