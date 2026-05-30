#ifndef RTXUI_TERMINAL_TERMINAL_DEVICE_HPP_
#define RTXUI_TERMINAL_TERMINAL_DEVICE_HPP_

#include <signal.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

#include <algorithm>
#include <iostream>
#include <string>
#include <string_view>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
inline int emscripten_get_char() {
  return EM_ASM_INT({
    return Asyncify.handleSleep(function(wakeUp) {
      if (window.rtxui_input_queue && window.rtxui_input_queue.length > 0) {
        wakeUp(window.rtxui_input_queue.shift());
        return;
      }
      let timer = null;
      if (window.rtxui_has_active_transitions) {
        timer = setTimeout(function() {
          window.rtxui_on_input = null;
          wakeUp(-1);
        }, 16);
      }
      window.rtxui_on_input = function(char_code) {
        if (timer) clearTimeout(timer);
        window.rtxui_on_input = null;
        wakeUp(char_code);
      };
    });
  });
}
#endif

namespace rtxui {

#ifdef __EMSCRIPTEN__
// Forward declaration or empty namespace content is fine as long as we define SystemTerminalDevice below.
#endif

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
  bool IsAtty() override { return isatty(STDIN_FILENO); }

  int Read(char* buf, int len) override {
#ifdef __EMSCRIPTEN__
    if (len <= 0) {
      return 0;
    }
    int c = emscripten_get_char();
    if (c == -1) {
      errno = EAGAIN;
      return -1;
    }
    buf[0] = static_cast<char>(c);
    return 1;
#else
    return read(STDIN_FILENO, buf, len);
#endif
  }

  void Write(std::string_view data) override {
#ifdef __EMSCRIPTEN__
    std::string str(data);
    EM_ASM(
        {
          let s = UTF8ToString($0);
          if (window.rtxui_on_output) {
            window.rtxui_on_output(s);
          } else {
            console.log(s);
          }
        },
        str.c_str());
#else
    std::cout << data << std::flush;
#endif
  }

  bool GetSize(int& width, int& height) override {
#ifdef __EMSCRIPTEN__
    width = EM_ASM_INT({ return window.rtxui_columns || 80; });
    height = EM_ASM_INT({ return window.rtxui_lines || 24; });
    return true;
#else
    struct winsize w;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0 && w.ws_col > 0 &&
        w.ws_row > 0) {
      width = w.ws_col;
      height = w.ws_row;
      return true;
    }
    return false;
#endif
  }

  void EnterRawMode(void (*sigwinch_handler)(int)) override {
#ifdef __EMSCRIPTEN__
    Write("\x1b[?1049h\x1b[?7l\x1b[?25l\x1b[?1003h\x1b[?1006h");
#else
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

    Write("\x1b[?1049h\x1b[?7l\x1b[?25l\x1b[?1003h\x1b[?1006h");
#endif
  }

  void ExitRawMode() override {
#ifdef __EMSCRIPTEN__
    Write("\x1b[?1003l\x1b[?1006l\x1b[?25h\x1b[?7h\x1b[?1049l");
#else
    Write("\x1b[?1003l\x1b[?1006l\x1b[?25h\x1b[?7h\x1b[?1049l");
    sigaction(SIGWINCH, &previous_sigaction_, nullptr);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &previous_termios_);
#endif
  }

 private:
#ifndef __EMSCRIPTEN__
  termios previous_termios_;
  struct sigaction previous_sigaction_;
#endif
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

  void Write(std::string_view data) override { output_buffer_ += data; }

  bool GetSize(int& width, int& height) override {
    width = mock_width_;
    height = mock_height_;
    return true;
  }

  void EnterRawMode(void (*sigwinch_handler)(int)) override {
    is_raw_ = true;
    sigwinch_handler_ = sigwinch_handler;
  }

  void ExitRawMode() override { is_raw_ = false; }

  void PushInput(std::string_view data) { input_buffer_ += data; }

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

}  // namespace rtxui

#endif  // RTXUI_TERMINAL_TERMINAL_DEVICE_HPP_
