// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
//
// An inline application: a package installer drawn below the command that
// started it, as `apt install` is, rather than over the whole terminal.
//
// Screen::SetInline() keeps the shell's lines above the interface, which is
// only as tall as its content. Screen::Print() writes each installed package
// above it, where it stays in the terminal's history. Once done, the interface
// is a one-line summary, left behind for the shell to go on below.
//
// Try it: press p to pause, q to cancel, or click them.
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <functional>
#include <rtxui/rtxui.hpp>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

using namespace rtxui;

struct Package {
  std::string_view name;
  std::string_view version;
  double megabytes;
};

constexpr Package kPackages[] = {
    {"libc6", "2.39-0ubuntu8", 3.2},
    {"zlib1g", "1:1.3.dfsg-3.1", 0.09},
    {"libssl3t64", "3.0.13-0ubuntu3", 1.9},
    {"libbrotli1", "1.1.0-2build2", 0.34},
    {"libnghttp2-14", "1.59.0-1build4", 0.07},
    {"libpsl5t64", "0.21.2-1.1build1", 0.06},
    {"librtmp1", "2.4+20151223", 0.06},
    {"libssh-4", "0.10.6-2build2", 0.19},
    {"ca-certificates", "20240203", 0.16},
    {"libcurl4t64", "8.5.0-2ubuntu10", 0.34},
    {"openssl", "3.0.13-0ubuntu3", 1.2},
    {"curl", "8.5.0-2ubuntu10", 0.23},
};
constexpr int kPackageCount = sizeof(kPackages) / sizeof(kPackages[0]);
constexpr int kParallelDownloads = 3;
constexpr double kTickSeconds = 0.05;
constexpr std::string_view kSpinner[] = {"⠋", "⠙", "⠹", "⠸", "⠼",
                                         "⠴", "⠦", "⠧", "⠇", "⠏"};

std::string Megabytes(double megabytes) {
  char text[32];
  if (megabytes < 1) {
    std::snprintf(text, sizeof(text), "%.0f kB", megabytes * 1000);
  } else {
    std::snprintf(text, sizeof(text), "%.1f MB", megabytes);
  }
  return text;
}

// A bar `width` cells wide, `fraction` full, as its full and empty parts.
std::pair<std::string, std::string> Bar(double fraction, int width) {
  const int halves =
      static_cast<int>(std::clamp(fraction, 0.0, 1.0) * width * 2);
  std::string full;
  std::string empty;
  for (int i = 0; i < width; ++i) {
    if (2 * i + 1 < halves) {
      full += "━";
    } else if (2 * i < halves) {
      full += "╸";
    } else {
      empty += "━";
    }
  }
  return {full, empty};
}

// A package being downloaded or unpacked, as a row shows it.
struct Row {
  std::string spinner;
  std::string phase;
  std::string name;
  std::string full;
  std::string empty;
  std::string percent;
  std::string detail;
  bool operator==(const Row&) const = default;
};

void Fields(const Row& row, FieldSink& out) {
  out("spinner", row.spinner);
  out("phase", row.phase);
  out("name", row.name);
  out("full", row.full);
  out("empty", row.empty);
  out("percent", row.percent);
  out("detail", row.detail);
}

// The line Screen::Print() writes above the interface for each package done.
class Installed : public Component<Installed> {
 public:
  std::string name;
  std::string version;
  std::string size;

  explicit Installed(const Package& package)
      : name(package.name),
        version(package.version),
        size(Megabytes(package.megabytes)) {
    Bind(name);
    Bind(version);
    Bind(size);
  }

  std::string_view view = R"html(
    <div class="line">
      <span class="check">✔</span>
      <span class="name">{name}</span>
      <span class="version">{version}</span>
      <span class="size">{size}</span>
    </div>
    <style>
      .line { display: flex; gap: 1; padding-left: 1; }
      .check { color: rgb(63, 185, 80); }
      .name { width: 16; font-weight: bold; }
      .version { width: 18; color: rgb(139, 148, 158); }
      .size { color: rgb(139, 148, 158); }
    </style>
  )html";
};

class Installer : public Component<Installer> {
 public:
  // Called with each package installed, and once all are, or once cancelled.
  std::function<void(const Package&)> on_installed;
  std::function<void()> on_done;

  std::vector<Row> rows;
  std::string overall_full;
  std::string overall_empty;
  std::string overall_percent;
  std::string counts;
  std::string eta;
  std::string pause_label = "pause";
  bool finished = false;
  std::string summary;

  Installer() {
    Bind(rows);
    Bind(overall_full);
    Bind(overall_empty);
    Bind(overall_percent);
    Bind(counts);
    Bind(eta);
    Bind(pause_label);
    Bind(finished);
    Bind(summary);
    Bind(TogglePause);
    Bind(Cancel);
    StartDownloads();
    Update();
  }

  // Starts the clock that moves the installation on.
  void Start() {
    PostDelayedTask(
        [this] { Tick(); },
        std::chrono::milliseconds(static_cast<int>(kTickSeconds * 1000)));
  }

  void TogglePause() {
    paused_ = !paused_;
    pause_label = paused_ ? "resume" : "pause";
    Update();
  }

  void Cancel() {
    if (finished) {
      return;
    }
    Finish("✘ Cancelled: " + std::to_string(installed_) + " of " +
           std::to_string(kPackageCount) + " packages installed");
  }

  bool OnEvent(Event event) override {
    if (event == Event(Event::Keyboard::From('p'))) {
      TogglePause();
      return true;
    }
    if (event == Event(Event::Keyboard::From('q')) ||
        event == Event::Escape()) {
      Cancel();
      return true;
    }
    return Component<Installer>::OnEvent(event);
  }

  std::string_view view = R"html(
    <if condition="{finished}">
      <div class="summary">{summary}</div>
    </if>
    <else>
      <for each="{rows}" as="r">
        <div class="row">
          <span class="spinner">{r.spinner}</span>
          <span class="phase">{r.phase}</span>
          <span class="name">{r.name}</span>
          <span class="bar"><span class="full">{r.full}</span><span class="empty">{r.empty}</span></span>
          <span class="percent">{r.percent}</span>
          <span class="detail">{r.detail}</span>
        </div>
      </for>
      <div class="row overall">
        <span class="spinner"> </span>
        <span class="phase">Total</span>
        <span class="name">{counts}</span>
        <span class="bar"><span class="full">{overall_full}</span><span class="empty">{overall_empty}</span></span>
        <span class="percent">{overall_percent}</span>
        <span class="detail">{eta}</span>
      </div>
      <div class="keys">
        <span onclick="TogglePause"><span class="key">p</span> {pause_label}</span>
        <span onclick="Cancel"><span class="key">q</span> cancel</span>
      </div>
    </else>

    <style>
      self {
        --accent: rgb(88, 166, 255);
        --muted: rgb(110, 118, 129);
        --ok: rgb(63, 185, 80);

        display: block;
        padding-left: 1;
      }
      .row { display: flex; gap: 1; }
      .spinner { color: var(--accent); }
      .phase { width: 11; color: var(--accent); }
      .name { width: 16; font-weight: bold; }
      .bar { width: 24; }
      .full { color: var(--accent); }
      .empty { color: rgb(48, 54, 61); }
      .percent { width: 4; text-align: right; }
      .detail { color: var(--muted); }
      .overall { margin-top: 1; }
      .overall .phase, .overall .name { color: var(--ok); }
      .overall .full { color: var(--ok); }
      .keys { display: flex; gap: 3; color: var(--muted); margin-top: 1; }
      .key { color: var(--accent); font-weight: bold; }
      .summary { color: var(--ok); font-weight: bold; }
    </style>
  )html";

 private:
  struct Download {
    int package = 0;
    double megabytes = 0;
  };

  void Tick() {
    if (finished) {
      return;
    }
    if (!paused_) {
      ++ticks_;
      Advance();
    }
    if (installed_ == kPackageCount) {
      const std::chrono::duration<double> elapsed =
          std::chrono::steady_clock::now() - started_;
      char text[96];
      std::snprintf(text, sizeof(text), "✔ Installed %d packages (%s) in %.1fs",
                    kPackageCount, Megabytes(TotalMegabytes()).c_str(),
                    elapsed.count());
      Finish(text);
      return;
    }
    Update();
    Start();
  }

  // One tick of work: downloads share the bandwidth, then each package is
  // unpacked and set up in turn.
  void StartDownloads() {
    while (static_cast<int>(downloads_.size()) < kParallelDownloads &&
           next_ < kPackageCount) {
      downloads_.push_back({next_++, 0});
    }
  }

  void Advance() {
    StartDownloads();
    // A link that wobbles between 4 and 8 MB/s.
    rate_ = 6 + 2 * std::sin(ticks_ * 0.13);
    const double share =
        rate_ * kTickSeconds / std::max<size_t>(1, downloads_.size());
    for (Download& download : downloads_) {
      download.megabytes = std::min(download.megabytes + share,
                                    kPackages[download.package].megabytes);
    }
    for (auto it = downloads_.begin(); it != downloads_.end();) {
      if (it->megabytes >= kPackages[it->package].megabytes) {
        downloaded_megabytes_ += it->megabytes;
        unpack_queue_.push_back(it->package);
        it = downloads_.erase(it);
      } else {
        ++it;
      }
    }
    if (unpacking_ >= 0) {
      unpack_seconds_ += kTickSeconds;
      if (unpack_seconds_ >= UnpackSeconds(unpacking_)) {
        ++installed_;
        if (on_installed) {
          on_installed(kPackages[unpacking_]);
        }
        unpacking_ = -1;
      }
    }
    // The next one starts right away: its row takes the place of the last.
    if (unpacking_ < 0 && !unpack_queue_.empty()) {
      unpacking_ = unpack_queue_.front();
      unpack_queue_.erase(unpack_queue_.begin());
      unpack_seconds_ = 0;
    }
  }

  static double UnpackSeconds(int package) {
    return 0.25 + kPackages[package].megabytes * 0.1;
  }

  static double TotalMegabytes() {
    double total = 0;
    for (const Package& package : kPackages) {
      total += package.megabytes;
    }
    return total;
  }

  // Shows the state of the installation.
  void Update() {
    const std::string spinner(paused_ ? "⏸"
                                      : kSpinner[ticks_ % std::size(kSpinner)]);
    rows.clear();
    for (const Download& download : downloads_) {
      const Package& package = kPackages[download.package];
      const double fraction = download.megabytes / package.megabytes;
      Row row = {spinner, "Downloading", std::string(package.name)};
      std::tie(row.full, row.empty) = Bar(fraction, 24);
      row.percent = std::to_string(static_cast<int>(fraction * 100)) + "%";
      row.detail =
          Megabytes(download.megabytes) + " / " + Megabytes(package.megabytes);
      rows.push_back(row);
    }
    if (unpacking_ >= 0) {
      const Package& package = kPackages[unpacking_];
      const double fraction = unpack_seconds_ / UnpackSeconds(unpacking_);
      const bool setting_up = fraction > 0.6;
      Row row = {spinner, setting_up ? "Setting up" : "Unpacking",
                 std::string(package.name)};
      std::tie(row.full, row.empty) = Bar(fraction, 24);
      row.percent = std::to_string(static_cast<int>(fraction * 100)) + "%";
      row.detail = std::string(package.version);
      rows.push_back(row);
    }

    const double total = TotalMegabytes();
    double done = downloaded_megabytes_;
    for (const Download& download : downloads_) {
      done += download.megabytes;
    }
    // Downloading is the first half of the work, installing the second.
    const double fraction =
        0.5 * done / total + 0.5 * installed_ / kPackageCount;
    std::tie(overall_full, overall_empty) = Bar(fraction, 24);
    overall_percent = std::to_string(static_cast<int>(fraction * 100)) + "%";
    counts = std::to_string(installed_) + " / " + std::to_string(kPackageCount);
    if (paused_) {
      eta = "paused";
    } else if (rate_ > 0) {
      const int seconds = static_cast<int>(std::ceil(
          (total - done) / rate_ + 0.4 * (kPackageCount - installed_)));
      char text[32];
      std::snprintf(text, sizeof(text), "%.1f MB/s  eta %d:%02d", rate_,
                    seconds / 60, seconds % 60);
      eta = text;
    } else {
      eta = "starting";
    }
  }

  void Finish(std::string text) {
    finished = true;
    summary = std::move(text);
    if (on_done) {
      on_done();
    }
  }

  std::chrono::steady_clock::time_point started_ =
      std::chrono::steady_clock::now();
  bool paused_ = false;
  int ticks_ = 0;
  double rate_ = 0;
  int next_ = 0;
  std::vector<Download> downloads_;
  double downloaded_megabytes_ = 0;
  std::vector<int> unpack_queue_;
  int unpacking_ = -1;
  double unpack_seconds_ = 0;
  int installed_ = 0;
};

int main() {
  auto app = Ref<Installer>::New();
  Screen screen(app);
  screen.SetInline(true);
  app->on_installed = [&screen](const Package& package) {
    screen.Print(Ref<Installed>::New(package));
  };
  app->on_done = [&screen] { screen.Exit(); };
  app->Start();
  screen.Loop();
  return 0;
}
