#ifndef RTXUI_TERMINAL_TERMINAL_DEVICE_HPP_
#define RTXUI_TERMINAL_TERMINAL_DEVICE_HPP_

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <io.h>
#include <signal.h>
#elif !defined(__EMSCRIPTEN__)
#include <signal.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>
#endif

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

inline int emscripten_read(char* buf, int len) {
  if (len <= 0) {
    return 0;
  }
  // 1. If bytes are already queued in JS, drain them synchronously without
  // sleeping.
  int drained = EM_ASM_INT(
      {
        let queue = window.rtxui_input_queue;
        if (queue && queue.length > 0) {
          let max_len = $1;
          let to_copy = Math.min(queue.length, max_len);
          for (let i = 0; i < to_copy; ++i) {
            HEAPU8[$0 + i] = queue[i];
          }
          queue.splice(0, to_copy);
          return to_copy;
        }
        return 0;
      },
      buf, len);

  if (drained > 0) {
    return drained;
  }

  // 2. Queue was empty: sleep until input arrives or transition timer fires.
  // Crucially, char_code is returned as the function's return value from
  // wakeUp(), which Asyncify preserves in a register across the stack rewind.
  int c = emscripten_get_char();
  if (c == -1) {
    errno = EAGAIN;
    return -1;
  }

  // Written AFTER Asyncify finishes rewinding, so it cannot be clobbered!
  buf[0] = static_cast<char>(c);
  int total = 1;

  // 3. If that event burst queued additional bytes (e.g. mouse escape sequence
  // or pasted text), drain them now into buf + 1, since Asyncify rewind is
  // done.
  if (len > 1) {
    int extra = EM_ASM_INT(
        {
          let queue = window.rtxui_input_queue;
          if (queue && queue.length > 0) {
            let max_len = $1;
            let to_copy = Math.min(queue.length, max_len);
            for (let i = 0; i < to_copy; ++i) {
              HEAPU8[$0 + i] = queue[i];
            }
            queue.splice(0, to_copy);
            return to_copy;
          }
          return 0;
        },
        buf + 1, len - 1);
    total += extra;
  }

  return total;
}
#endif

namespace rtxui {

#if defined(_WIN32)
namespace internal {

inline constexpr char kExitRawModeSequence[] =
    "\x1b[?1003l\x1b[?1006l\x1b[?1016l\x1b[?2004l\x1b[?25h\x1b[?7h\x1b[?1049l";

inline DWORD g_saved_in_mode = 0;
inline DWORD g_saved_out_mode = 0;
inline UINT g_saved_in_cp = 0;
inline UINT g_saved_out_cp = 0;
inline volatile long g_raw_mode_active = 0;

inline BOOL WINAPI ConsoleCtrlHandler(DWORD ctrl_type) {
  (void)ctrl_type;
  if (g_raw_mode_active) {
    g_raw_mode_active = 0;
    HANDLE out_handle = GetStdHandle(STD_OUTPUT_HANDLE);
    HANDLE in_handle = GetStdHandle(STD_INPUT_HANDLE);
    DWORD written = 0;
    WriteFile(out_handle, kExitRawModeSequence,
              static_cast<DWORD>(sizeof(kExitRawModeSequence) - 1), &written,
              nullptr);
    SetConsoleMode(in_handle, g_saved_in_mode);
    SetConsoleMode(out_handle, g_saved_out_mode);
    SetConsoleCP(g_saved_in_cp);
    SetConsoleOutputCP(g_saved_out_cp);
  }
  return FALSE;
}

}  // namespace internal
#elif !defined(__EMSCRIPTEN__)
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
  virtual bool GetCellPixelSize(int& /*cell_width*/, int& /*cell_height*/) {
    return false;
  }

  virtual void EnterRawMode(void (*sigwinch_handler)(int)) = 0;
  virtual void ExitRawMode() = 0;
};

class SystemTerminalDevice : public TerminalDevice {
 public:
  bool IsAtty() override {
#if defined(_WIN32)
    HANDLE handle = GetStdHandle(STD_INPUT_HANDLE);
    DWORD mode = 0;
    return handle != INVALID_HANDLE_VALUE && GetConsoleMode(handle, &mode);
#elif defined(__EMSCRIPTEN__)
    return true;
#else
    return isatty(STDIN_FILENO);
#endif
  }

  int Read(char* buf, int len) override {
#if defined(_WIN32)
    if (len <= 0) {
      return 0;
    }
    HANDLE handle = GetStdHandle(STD_INPUT_HANDLE);
    DWORD bytes_read = 0;
    if (!ReadFile(handle, buf, static_cast<DWORD>(len), &bytes_read, nullptr)) {
      return -1;
    }
    return static_cast<int>(bytes_read);
#elif defined(__EMSCRIPTEN__)
    if (len <= 0) {
      return 0;
    }
    int bytes_read = emscripten_read(buf, len);
    if (bytes_read == -1) {
      errno = EAGAIN;
      return -1;
    }
    return bytes_read;
#else
    return read(STDIN_FILENO, buf, len);
#endif
  }

  void Write(std::string_view data) override {
#if defined(_WIN32)
    HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD written = 0;
    WriteFile(handle, data.data(), static_cast<DWORD>(data.size()), &written,
              nullptr);
#elif defined(__EMSCRIPTEN__)
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
#if defined(_WIN32)
    HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (GetConsoleScreenBufferInfo(handle, &csbi)) {
      width = csbi.srWindow.Right - csbi.srWindow.Left + 1;
      height = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
      return true;
    }
    return false;
#elif defined(__EMSCRIPTEN__)
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
#if defined(_WIN32)
    HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_FONT_INFO font_info;
    if (GetCurrentConsoleFont(handle, FALSE, &font_info)) {
      cell_width = font_info.dwFontSize.X;
      cell_height = font_info.dwFontSize.Y;
      return cell_width > 0 && cell_height > 0;
    }
    return false;
#elif defined(__EMSCRIPTEN__)
    (void)cell_width;
    (void)cell_height;
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
#if defined(_WIN32)
    (void)sigwinch_handler;
    HANDLE in_handle = GetStdHandle(STD_INPUT_HANDLE);
    HANDLE out_handle = GetStdHandle(STD_OUTPUT_HANDLE);

    GetConsoleMode(in_handle, &previous_in_mode_);
    GetConsoleMode(out_handle, &previous_out_mode_);
    previous_in_cp_ = GetConsoleCP();
    previous_out_cp_ = GetConsoleOutputCP();

    internal::g_saved_in_mode = previous_in_mode_;
    internal::g_saved_out_mode = previous_out_mode_;
    internal::g_saved_in_cp = previous_in_cp_;
    internal::g_saved_out_cp = previous_out_cp_;
    internal::g_raw_mode_active = 1;

    SetConsoleCtrlHandler(internal::ConsoleCtrlHandler, TRUE);

    SetConsoleCP(CP_UTF8);
    SetConsoleOutputCP(CP_UTF8);

    DWORD in_mode = previous_in_mode_;
    in_mode |= ENABLE_VIRTUAL_TERMINAL_INPUT;
    in_mode &=
        ~(ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT | ENABLE_PROCESSED_INPUT);
    SetConsoleMode(in_handle, in_mode);

    DWORD out_mode = previous_out_mode_;
    out_mode |=
        ENABLE_VIRTUAL_TERMINAL_PROCESSING | DISABLE_NEWLINE_AUTO_RETURN;
    SetConsoleMode(out_handle, out_mode);

    int cell_w = 0, cell_h = 0;
    if (GetCellPixelSize(cell_w, cell_h)) {
      Write("\x1b[?1049h\x1b[?7l\x1b[?25l\x1b[?1003h\x1b[?1006h\x1b[?1016h");
    } else {
      Write("\x1b[?1049h\x1b[?7l\x1b[?25l\x1b[?1003h\x1b[?1006h");
    }
#elif defined(__EMSCRIPTEN__)
    (void)sigwinch_handler;
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
#if defined(_WIN32)
    Write("\x1b[?1003l\x1b[?1006l\x1b[?1016l\x1b[?25h\x1b[?7h\x1b[?1049l");
    internal::g_raw_mode_active = 0;
    SetConsoleCtrlHandler(internal::ConsoleCtrlHandler, FALSE);

    HANDLE in_handle = GetStdHandle(STD_INPUT_HANDLE);
    HANDLE out_handle = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleMode(in_handle, previous_in_mode_);
    SetConsoleMode(out_handle, previous_out_mode_);
    SetConsoleCP(previous_in_cp_);
    SetConsoleOutputCP(previous_out_cp_);
#elif defined(__EMSCRIPTEN__)
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
#if defined(_WIN32)
  DWORD previous_in_mode_ = 0;
  DWORD previous_out_mode_ = 0;
  UINT previous_in_cp_ = 0;
  UINT previous_out_cp_ = 0;
#elif !defined(__EMSCRIPTEN__)
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
#if defined(SIGWINCH)
      sigwinch_handler_(SIGWINCH);
#else
      sigwinch_handler_(0);
#endif
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
