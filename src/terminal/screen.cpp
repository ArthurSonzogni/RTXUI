// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include "terminal/screen.hpp"

#include <sys/ioctl.h>
#include <unistd.h>
#include <iostream>

#include "dom/element.hpp"
#include "layout/layout.hpp"
#include "layout/layout_tree_builder.hpp"
#include "layout/physical_fragment.hpp"
#include "paint/paint.hpp"
#include "paint/texture.hpp"
#include "terminal/terminal_input_parser.hpp"

namespace rtxui {

namespace {

Element* FindElementAt(const std::shared_ptr<PhysicalFragment>& fragment, int target_x, int target_y) {
  if (!fragment) {
    return nullptr;
  }
  if (target_x < 0 || target_y < 0 || target_x >= fragment->width || target_y >= fragment->height) {
    return nullptr;
  }
  // Traverse children in reverse order (top-most elements first)
  for (auto it = fragment->children.rbegin(); it != fragment->children.rend(); ++it) {
    int rel_x = target_x - it->x;
    int rel_y = target_y - it->y;
    if (auto* found = FindElementAt(it->fragment, rel_x, rel_y)) {
      return found;
    }
  }
  if (fragment->dom_node) {
    return fragment->dom_node;
  }
  return nullptr;
}

ComponentBase* GetOwningComponent(Element* element) {
  while (element) {
    if (element->component()) {
      return const_cast<ComponentBase*>(element->component());
    }
    element = element->Parent();
  }
  return nullptr;
}

} // namespace

Screen::Screen(Ref<ComponentBase> component)
    : component_(std::move(component)) {
  UpdateSize();
  component_->Mount();
  Draw();
}

Screen::~Screen() {}

void Screen::Loop() {
  RawTerminal raw_terminal;
  Draw();

  TerminalInputParser parser;
  while (true) {
    char c;
    if (read(STDIN_FILENO, &c, 1) != 1) {
      continue;
    }

    UpdateSize();
    parser.Add(c);

    while (auto event = parser.GetEvent()) {
      if (event->is<Event::Mouse>()) {
        auto mouse = event->get<Event::Mouse>();
        if (mouse.button == Event::Mouse::Button::Left &&
            mouse.motion == Event::Mouse::Motion::Pressed) {
          if (root_fragment_) {
            int tx = mouse.x - 1;
            int ty = mouse.y - 1;
            if (auto* clicked_element = FindElementAt(root_fragment_, tx, ty)) {
              std::string action;
              const auto& attrs = clicked_element->Attributes();
              if (attrs.count("onclick")) {
                action = attrs.at("onclick");
              } else if (attrs.count("@click.left")) {
                action = attrs.at("@click.left");
              } else if (attrs.count("@click")) {
                action = attrs.at("@click");
              }

              if (!action.empty()) {
                if (auto* comp = GetOwningComponent(clicked_element)) {
                  if (comp->RunCallback(action)) {
                    DigestAndDraw();
                    continue;
                  }
                }
              }
            }
          }
        }
      }

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
  root_fragment_ = root_fragment;

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

    std::cout << "\x1b[?1049h"; // Enter alternate screen buffer
    std::cout << "\x1b[?7l";  // Disable line wrapping
    std::cout << "\x1b[?25l"; // Hide cursor
    std::cout << "\x1b[?1000h\x1b[?1006h" << std::flush; // Enable mouse tracking in SGR mode
  }
}

Screen::RawTerminal::~RawTerminal() {
  if (isatty(STDIN_FILENO)) {
    std::cout << "\x1b[?1000l\x1b[?1006l"; // Disable mouse tracking
    std::cout << "\x1b[?25h";  // Show cursor
    std::cout << "\x1b[?7h";   // Enable line wrapping
    std::cout << "\x1b[?1049l" << std::flush; // Exit alternate screen buffer
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &previous_termios_);
  }
}

} // namespace rtxui
