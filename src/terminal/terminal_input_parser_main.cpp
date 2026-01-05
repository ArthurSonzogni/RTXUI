// Copyright 2020 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.

#include <termios.h>
#include <unistd.h>

#include <iostream>
#include <string>
#include <vector>

#include "terminal/event.hpp"
#include "terminal/terminal_input_parser.hpp"

// A simple RAII class to configure the terminal in raw mode.
struct RawTerminal {
  termios previous_termios_;
  RawTerminal() {
    tcgetattr(STDIN_FILENO, &previous_termios_);
    termios terminal = previous_termios_;
    terminal.c_iflag &= ~IGNBRK;  // Disable ignoring break condition
    terminal.c_iflag &= ~BRKINT;  // Disable break causing input and output to
                                  // be flushed
    terminal.c_iflag &= ~PARMRK;  // Disable marking parity errors.
    terminal.c_iflag &= ~ISTRIP;  // Disable striping 8th bit off characters.
    terminal.c_iflag &= ~INLCR;   // Disable mapping NL to CR.
    terminal.c_iflag &= ~IGNCR;   // Disable ignoring CR.
    terminal.c_iflag &= ~ICRNL;   // Disable mapping CR to NL.
    terminal.c_iflag &= ~IXON;    // Disable XON/XOFF flow control on output

    terminal.c_lflag &= ~ECHO;    // Disable echoing input characters.
    terminal.c_lflag &= ~ECHONL;  // Disable echoing new line characters.
    terminal.c_lflag &= ~ICANON;  // Disable Canonical mode.
    terminal.c_lflag &= ~ISIG;    // Disable sending signal when hitting:
                                  // -     => DSUSP
                                  // - C-Z => SUSP
                                  // - C-C => INTR
                                  // - C-d => QUIT
    terminal.c_lflag &= ~IEXTEN;  // Disable extended input processing
    terminal.c_cflag |= CS8;      // 8 bits per byte

    terminal.c_cc[VMIN] = 0;   // Minimum number of characters for non-canonical
                               // read.
    terminal.c_cc[VTIME] = 0;  // Timeout in deciseconds for non-canonical read.
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &terminal);

    std::cout << "\x1b[?1000h";  // Enable mouse
    std::cout << "\x1b[?1001h";  // Enable Hilite Mouse mode
    std::cout << "\x1b[?1002h";  // Enable Mouse Vt1000 mode
    std::cout << "\x1b[?1003h";  // Enable Mouse Motion events
    std::cout << "\x1b[?1004h";  // Enable Focus events
    std::cout << "\x1b[?1005h";  // Enable UTF-8 Mouse mode
    std::cout << "\x1b[?1006h";  // Enable SGR mouse mode
    std::cout << "\x1b[?1007h";  // Enable Extended Mouse mode
    std::cout << "\x1b[?1013h";  // Enable Any Event Mouse mode
    std::cout << "\x1b[?1011h";  // Enable Drag Event Mouse mode
    std::cout << "\x1b[?1010h";  // Enable X10 Mouse mode
    std::cout << "\x1b[?1012h";  // Enable Button Event Mouse mode
    std::cout << "\x1b[?1015h";  // Enable urxvt Mouse mode
    std::cout << "\x1b[?1049h";  // Enable alternative screen buffer
    std::cout << "\x1b[?7l";

    // Enable kitty keyboard protocol
    std::cout << "\x1b[>4;2m";

    std::cout << "\x1b[?25l"  << std::flush;

    // Enable cursor
  }
  ~RawTerminal() {
    std::cout << "\x1b[?1000l";  // Disable mouse
    std::cout << "\x1b[?1001l";  // Disable Hilite Mouse mode
    std::cout << "\x1b[?1002l";  // Disable Mouse Vt1000 mode
    std::cout << "\x1b[?1003l";  // Disable Mouse Motion events
    std::cout << "\x1b[?1004l";  // Disable Focus events
    std::cout << "\x1b[?1005l";  // Disable UTF-8 Mouse mode
    std::cout << "\x1b[?1006l";  // Disable SGR mouse mode
    std::cout << "\x1b[?1007l";  // Disable Extended Mouse mode
    std::cout << "\x1b[?1013l";  // Disable Any Event Mouse mode
    std::cout << "\x1b[?1011l";  // Disable Drag Event Mouse mode
    std::cout << "\x1b[?1010l";  // Disable X10 Mouse mode
    std::cout << "\x1b[?1012l";  // Disable Button Event Mouse mode
    std::cout << "\x1b[?1015l";  // Disable urxvt Mouse mode
    std::cout << "\x1b[?1049l";  // Disable alternative screen buffer

    std::cout << "\x1b[?25h"; // Enable cursor.

    std::cout << std::flush;

    tcsetattr(STDIN_FILENO, TCSAFLUSH, &previous_termios_);
  }
};

int main() {
  RawTerminal raw_terminal;
  TerminalInputParser parser;
  std::cout << "Reading keyboard input. Press ESC or Ctrl+C to quit." << '\r'
            << std::endl;

  while (true) {
    char c;
    if (read(STDIN_FILENO, &c, 1) != 1) {
      continue;
    }

    std::cout << static_cast<int>(c) << ' ' << std::flush;
    parser.Add(c);

    while (auto event = parser.GetEvent()) {
      std::cout << std::endl;
      std::cout << *event << "\r" << std::endl;
      if (*event == Event::Escape || *event == Event::CtrlC) {
        return 0;
      }
    }
  }

  return 0;
}
