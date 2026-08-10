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
        timer = setTimeout(
            function() {
              window.rtxui_on_input = null;
              wakeUp(-1);
            },
            16);
      }
      window.rtxui_on_input = function(char_code) {
        if (timer) {
          clearTimeout(timer);
        }
        window.rtxui_on_input = null;
        wakeUp(char_code);
      };
    });
  });
}
#endif

namespace rtxui {

#ifdef __EMSCRIPTEN__
// Forward declaration or empty namespace content is fine as long as we define
// SystemTerminalDevice below.
#endif

#ifndef __EMSCRIPTEN__
namespace internal {

// Undoes everything EnterRawMode() turns on. Kept as a literal so the signal
// handler can write() it without allocating.
inline constexpr char kExitRawModeSequence[] =
    "\x1b[?1003l\x1b[?1006l\x1b[?1016l\x1b[?2004l\x1b[?25h\x1b[?7h\x1b[?1049l";

// Signals that would otherwise kill the process while the terminal is in raw
// mode, leaving the user's shell with no echo, no line editing and mouse
// reporting still on (which makes every mouse move spew escape sequences).
inline constexpr int kFatalSignals[] = {
    SIGHUP, SIGINT, SIGQUIT, SIGILL, SIGABRT, SIGFPE, SIGSEGV, SIGTERM, SIGBUS,
};
inline constexpr int kFatalSignalCount =
    sizeof(kFatalSignals) / sizeof(kFatalSignals[0]);

// A signal handler may only touch async-signal-safe state, so this is plain
// data: no allocation, no locks, no destructors.
inline termios g_saved_termios;
inline struct sigaction g_saved_signal_handlers[kFatalSignalCount];
inline volatile sig_atomic_t g_raw_mode_active = 0;

// write()/tcsetattr() are both async-signal-safe, so this is legal from a
// handler as well as from the normal teardown path.
inline void RestoreTerminalFromSignal() {
  if (g_raw_mode_active == 0) {
    return;
  }
  g_raw_mode_active = 0;
  ssize_t ignored = write(STDOUT_FILENO, kExitRawModeSequence,
                          sizeof(kExitRawModeSequence) - 1);
  (void)ignored;
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &g_saved_termios);
}

inline void FatalSignalHandler(int sig) {
  RestoreTerminalFromSignal();
  // SA_RESETHAND already put the default disposition back, so re-raising
  // kills the process the way it was going to die anyway: the exit status
  // stays correct and the crash signals still dump core. The signal is
  // blocked for the duration of the handler, so it lands on return.
  raise(sig);
}

inline void InstallFatalSignalHandlers() {
  struct sigaction sa;
  sa.sa_handler = FatalSignalHandler;
  sigemptyset(&sa.sa_mask);
  // SA_RESETHAND: restore the default disposition on delivery, so a second
  // fatal signal (a crash inside the handler, say) cannot recurse.
  sa.sa_flags = SA_RESETHAND;
  for (int i = 0; i < kFatalSignalCount; ++i) {
    sigaction(kFatalSignals[i], &sa, &g_saved_signal_handlers[i]);
  }
}

inline void RemoveFatalSignalHandlers() {
  for (int i = 0; i < kFatalSignalCount; ++i) {
    sigaction(kFatalSignals[i], &g_saved_signal_handlers[i], nullptr);
  }
}

}  // namespace internal
#endif

class TerminalDevice {
 public:
  virtual ~TerminalDevice() = default;
  virtual bool IsAtty() = 0;
  virtual int Read(char* buf, int len) = 0;
  virtual void Write(std::string_view data) = 0;
  virtual bool GetSize(int& width, int& height) = 0;
  virtual bool GetCellPixelSize(int& cell_width, int& cell_height) {
    return false;
  }

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

  bool GetCellPixelSize(int& cell_width, int& cell_height) override {
#ifdef __EMSCRIPTEN__
    return false;
#else
    struct winsize w;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0 && w.ws_col > 0 &&
        w.ws_row > 0 && w.ws_xpixel > 0 && w.ws_ypixel > 0) {
      cell_width = w.ws_xpixel / w.ws_col;
      cell_height = w.ws_ypixel / w.ws_row;
      return cell_width > 0 && cell_height > 0;
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

    // From here on a signal death would strand the terminal in raw mode with
    // mouse reporting on, so arm the restore path before writing the modes.
    internal::g_saved_termios = previous_termios_;
    internal::g_raw_mode_active = 1;
    internal::InstallFatalSignalHandlers();

    struct sigaction sa;
    sa.sa_handler = sigwinch_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGWINCH, &sa, &previous_sigaction_);

    int cell_w = 0, cell_h = 0;
    if (GetCellPixelSize(cell_w, cell_h)) {
      Write("\x1b[?1049h\x1b[?7l\x1b[?25l\x1b[?1003h\x1b[?1006h\x1b[?1016h");
    } else {
      Write("\x1b[?1049h\x1b[?7l\x1b[?25l\x1b[?1003h\x1b[?1006h");
    }
#endif
  }

  void ExitRawMode() override {
#ifdef __EMSCRIPTEN__
    Write("\x1b[?1003l\x1b[?1006l\x1b[?25h\x1b[?7h\x1b[?1049l");
#else
    Write("\x1b[?1003l\x1b[?1006l\x1b[?1016l\x1b[?25h\x1b[?7h\x1b[?1049l");
    internal::g_raw_mode_active = 0;
    internal::RemoveFatalSignalHandlers();
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

  bool GetCellPixelSize(int& cell_width, int& cell_height) override {
    if (mock_cell_width_ > 0 && mock_cell_height_ > 0) {
      cell_width = mock_cell_width_;
      cell_height = mock_cell_height_;
      return true;
    }
    return false;
  }

  void SetMockCellPixelSize(int width, int height) {
    mock_cell_width_ = width;
    mock_cell_height_ = height;
  }

  void EnterRawMode(void (*sigwinch_handler)(int)) override {
    is_raw_ = true;
    sigwinch_handler_ = sigwinch_handler;
    int cell_w = 0, cell_h = 0;
    if (GetCellPixelSize(cell_w, cell_h)) {
      Write("\x1b[?1049h\x1b[?7l\x1b[?25l\x1b[?1003h\x1b[?1006h\x1b[?1016h");
    } else {
      Write("\x1b[?1049h\x1b[?7l\x1b[?25l\x1b[?1003h\x1b[?1006h");
    }
  }

  void ExitRawMode() override {
    is_raw_ = false;
    Write("\x1b[?1003l\x1b[?1006l\x1b[?1016l\x1b[?25h\x1b[?7h\x1b[?1049l");
  }

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
  int mock_cell_width_ = 0;
  int mock_cell_height_ = 0;
  bool is_raw_ = false;
  void (*sigwinch_handler_)(int) = nullptr;
};

}  // namespace rtxui

#endif  // RTXUI_TERMINAL_TERMINAL_DEVICE_HPP_
