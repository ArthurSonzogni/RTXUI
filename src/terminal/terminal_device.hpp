#ifndef RTXUI_TERMINAL_TERMINAL_DEVICE_HPP_
#define RTXUI_TERMINAL_TERMINAL_DEVICE_HPP_

#include <termios.h>
#include <signal.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <string_view>
#include <string>
#include <iostream>
#include <algorithm>

namespace rtxui {

class TerminalDevice {
 public:
  virtual ~TerminalDevice() = default;
  virtual bool IsAtty() = 0;
  virtual int Read(char* buf, int len) = 0;
  virtual void Write(std::string_view data) = 0;
  virtual bool GetSize(int& width, int& height) = 0;

  virtual void EnterRawMode(void (*sigwinch_handler)(int)) = 0;
  virtual void ExitRawMode() = 0;
};

class SystemTerminalDevice : public TerminalDevice {
 public:
  bool IsAtty() override {
    return isatty(STDIN_FILENO);
  }

  int Read(char* buf, int len) override {
    return read(STDIN_FILENO, buf, len);
  }

  void Write(std::string_view data) override {
    std::cout << data << std::flush;
  }

  bool GetSize(int& width, int& height) override {
    struct winsize w;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0 && w.ws_col > 0 && w.ws_row > 0) {
      width = w.ws_col;
      height = w.ws_row;
      return true;
    }
    return false;
  }

  void EnterRawMode(void (*sigwinch_handler)(int)) override {
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

    terminal.c_cc[VMIN] = 1;
    terminal.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &terminal);

    struct sigaction sa;
    sa.sa_handler = sigwinch_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGWINCH, &sa, &previous_sigaction_);

    Write("\x1b[?1049h\x1b[?7l\x1b[?25l\x1b[?1000h\x1b[?1006h");
  }

  void ExitRawMode() override {
    Write("\x1b[?1000l\x1b[?1006l\x1b[?25h\x1b[?7h\x1b[?1049l");
    sigaction(SIGWINCH, &previous_sigaction_, nullptr);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &previous_termios_);
  }

 private:
  termios previous_termios_;
  struct sigaction previous_sigaction_;
};

class MockTerminalDevice : public TerminalDevice {
 public:
  bool IsAtty() override { return true; }

  int Read(char* buf, int len) override {
    if (input_buffer_.empty()) {
      return 0;
    }
    int to_read = std::min(len, static_cast<int>(input_buffer_.size()));
    for (int i = 0; i < to_read; ++i) {
      buf[i] = input_buffer_[i];
    }
    input_buffer_.erase(0, to_read);
    return to_read;
  }

  void Write(std::string_view data) override {
    output_buffer_ += data;
  }

  bool GetSize(int& width, int& height) override {
    width = mock_width_;
    height = mock_height_;
    return true;
  }

  void EnterRawMode(void (*sigwinch_handler)(int)) override {
    is_raw_ = true;
    sigwinch_handler_ = sigwinch_handler;
  }

  void ExitRawMode() override {
    is_raw_ = false;
  }

  void PushInput(std::string_view data) {
    input_buffer_ += data;
  }

  void TriggerResize(int new_width, int new_height) {
    mock_width_ = new_width;
    mock_height_ = new_height;
    if (sigwinch_handler_) {
      sigwinch_handler_(SIGWINCH);
    }
  }

  const std::string& GetOutput() const { return output_buffer_; }
  void ClearOutput() { output_buffer_.clear(); }
  bool IsRaw() const { return is_raw_; }

 private:
  std::string input_buffer_;
  std::string output_buffer_;
  int mock_width_ = 80;
  int mock_height_ = 24;
  bool is_raw_ = false;
  void (*sigwinch_handler_)(int) = nullptr;
};

} // namespace rtxui

#endif // RTXUI_TERMINAL_TERMINAL_DEVICE_HPP_
