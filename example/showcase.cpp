// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
//
// A tour of RTXUI in one application.
//
// Four pages behind a sidebar: a live dashboard laid out by a CSS grid that
// reflows with the terminal, a gallery of transitions and keyframe
// animations, form widgets bound to C++ state, and rich text. The data on the
// dashboard is fed by a timer; everything that moves on screen is CSS.
//
// Try it: resize the terminal on the Overview page, hover the tiles on
// Motion, drag the slider on Widgets, select text with the mouse on Text.
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <rtxui/rtxui.hpp>
#include <string>
#include <string_view>
#include <vector>

using namespace rtxui;

namespace {

struct Metric {
  std::string name;
  std::string unit;
  std::string color;
  double value = 0;
  double scale = 1;  // How far one step of the random walk may move it.
  std::vector<double> history;
};

struct Bar {
  int height = 0;
  std::string label;
};

struct Page {
  std::string icon;
  std::string name;
};

// A tiny deterministic random source, so every run of the tour looks alike.
double Noise(unsigned& state) {
  state = state * 1664525u + 1013904223u;
  return static_cast<double>(state >> 8) / static_cast<double>(1u << 24);
}

std::string Fixed(double value, int decimals) {
  char text[32];
  std::snprintf(text, sizeof(text), "%.*f", decimals, value);
  return text;
}

// The last values of `history` as a row of block characters.
std::string Sparkline(const std::vector<double>& history) {
  static constexpr std::string_view kBlocks[] = {"▁", "▂", "▃", "▄",
                                                 "▅", "▆", "▇", "█"};
  if (history.empty()) {
    return "";
  }
  const auto [low, high] = std::minmax_element(history.begin(), history.end());
  const double range = std::max(*high - *low, 1e-9);
  std::string line;
  for (double value : history) {
    const int index = static_cast<int>(std::lround((value - *low) / range * 7));
    line += kBlocks[std::clamp(index, 0, 7)];
  }
  return line;
}

}  // namespace

class Showcase : public Component<Showcase> {
 public:
  std::vector<Page> pages = {
      {"◆", "Overview"},
      {"✦", "Motion"},
      {"▣", "Widgets"},
      {"¶", "Text"},
  };
  int page = 0;

  std::vector<Metric> metrics = {
      {"Requests", "k/s", "rgb(88, 166, 255)", 48.2, 2.5, {}},
      {"Latency", "ms", "rgb(210, 153, 34)", 21.0, 1.6, {}},
      {"Errors", "%", "rgb(248, 81, 73)", 0.42, 0.06, {}},
      {"CPU", "%", "rgb(63, 185, 80)", 37.0, 3.0, {}},
      {"Memory", "GB", "rgb(188, 140, 255)", 11.4, 0.3, {}},
      {"Users", "", "rgb(57, 197, 207)", 1284, 22, {}},
  };
  std::vector<Bar> bars;

  // Widgets.
  std::string name = "Ada";
  std::string theme = "midnight";
  bool notifications = true;
  bool telemetry = false;
  int volume = 60;
  std::string plan = "pro";

  // Motion.
  bool toast_open = false;
  int toasts = 0;

  // Text.
  std::string markdown = R"md(# Rich text

Templates are **HTML**, styles are **CSS**, and long-form content can be
*Markdown*. Text wraps, aligns and clips like it does on the web.

- `flex`, `grid`, `position`, `overflow`
- transitions, `@keyframes`, `:hover`, `:focus`
- borders, scrollbars, opacity, `translate`

> Unicode is laid out by display width: 日本語のテキスト, café, Ελληνικά.
)md";

  std::string markdown_style = R"css(
    h1 { color: rgb(88, 166, 255); font-weight: bold; text-decoration: none; }
    strong { color: rgb(230, 237, 243); }
    code { color: rgb(255, 166, 87); }
    blockquote {
      color: rgb(139, 148, 158);
      border-left: tall;
      border-color: rgb(88, 166, 255);
      padding-left: 1;
    }
  )css";

  bool on_overview() const { return page == 0; }
  bool on_motion() const { return page == 1; }
  bool on_widgets() const { return page == 2; }
  bool on_text() const { return page == 3; }
  std::string title() const { return pages[page].name; }
  std::string subtitle() const {
    switch (page) {
      case 0:
        return "A live dashboard. The grid reflows as the terminal resizes.";
      case 1:
        return "Transitions and keyframe animations, all declared in CSS.";
      case 2:
        return "Form controls bound to plain C++ members.";
      default:
        return "Markdown, Unicode, and text you can select with the mouse.";
    }
  }
  std::string greeting() const {
    return name.empty() ? "Hello!" : "Hello, " + name + "!";
  }
  std::string volume_text() const { return std::to_string(volume) + "%"; }
  std::string summary() const {
    return theme + " theme, " + (notifications ? "notifications on" : "muted") +
           ", " + plan + " plan";
  }
  std::string toast_text() const {
    return "✔ Saved. Toast number " + std::to_string(toasts) + ".";
  }

  void Go(const std::string& index) { page = std::stoi(index) % 4; }
  void ShowToast() {
    ++toasts;
    toast_open = true;
  }

  Showcase() {
    for (int i = 0; i < 24; ++i) {
      Step();
    }
    Bind(pages, &Showcase::PageFields);
    Bind(metrics, &Showcase::MetricFields);
    Bind(bars, &Showcase::BarFields);
    Bind(page);
    Bind(name);
    Bind(theme);
    Bind(notifications);
    Bind(telemetry);
    Bind(volume);
    Bind(plan);
    Bind(toast_open);
    Bind(toasts);
    Bind(markdown);
    Bind(markdown_style);
    Bind(on_overview);
    Bind(on_motion);
    Bind(on_widgets);
    Bind(on_text);
    Bind(title);
    Bind(subtitle);
    Bind(greeting);
    Bind(volume_text);
    Bind(summary);
    Bind(toast_text);
    Bind(ShowToast);
    Import("Go", [this](std::string index) { Go(index); });
    // Save this file with a different template or style, and the running
    // application picks it up without a rebuild.
    EnableHotReload();
  }

  // Starts the timer feeding the dashboard.
  void Start() {
    PostDelayedTask(
        [this] {
          Step();
          Start();
        },
        std::chrono::milliseconds(700));
  }

  void PageFields(const Page& item, FieldSink& out) const {
    const bool active = &item == &pages[page];
    out("icon", item.icon);
    out("name", item.name);
    out("class", active ? "nav active" : "nav");
  }

  void MetricFields(const Metric& metric, FieldSink& out) const {
    const double previous = metric.history[metric.history.size() - 2];
    const double change = (metric.value - previous) / previous * 100;
    const int decimals = metric.value < 10 ? 2 : metric.value < 100 ? 1 : 0;
    out("name", metric.name);
    out("value", Fixed(metric.value, decimals));
    out("unit", metric.unit);
    out("color", metric.color);
    out("spark", Sparkline(metric.history));
    out("delta", (change >= 0 ? "▲ " : "▼ ") + Fixed(std::abs(change), 1) + "%");
    out("trend", change >= 0 ? "up" : "down");
  }

  void BarFields(const Bar& bar, FieldSink& out) const {
    out("height", std::to_string(bar.height));
    out("label", bar.label);
  }

  std::string_view view = R"html(
      <div class="app">
        <div class="sidebar">
          <div class="brand"><span class="logo">◆</span><span class="label"> RTXUI</span></div>
          <for each="{pages}" as="item">
            <div class="{item.class}" onclick="Go({$index})">
              <span>{item.icon}</span><span class="label">{item.name}</span>
            </div>
          </for>
          <div class="grow"></div>
          <div class="hint label">C++ / HTML / CSS</div>
        </div>

        <div class="main">
          <div class="header">
            <h1>{title}</h1>
            <p>{subtitle}</p>
          </div>

          <if condition="{on_overview}">
            <div class="page">
              <div class="grid">
                <for each="{metrics}" as="m">
                  <div class="metric">
                    <div class="metric-name">{m.name}</div>
                    <div class="metric-value" style="color: {m.color};">{m.value}<span class="unit"> {m.unit}</span></div>
                    <div class="spark" style="color: {m.color};">{m.spark}</div>
                    <div class="delta {m.trend}">{m.delta}</div>
                  </div>
                </for>
              </div>
              <div class="chart">
                <div class="chart-title">Throughput, last 24 ticks</div>
                <div class="bars">
                  <for each="{bars}" as="b">
                    <div class="bar" style="height: {b.height};"></div>
                  </for>
                </div>
              </div>
            </div>
          </if>
          <elif condition="{on_motion}">
            <div class="page">
              <div class="tiles">
                <div class="tile">
                  <div class="tile-title">Keyframes</div>
                  <div class="wave">
                    <span class="w1">●</span><span class="w2">●</span><span class="w3">●</span><span class="w4">●</span><span class="w5">●</span>
                  </div>
                </div>
                <div class="tile">
                  <div class="tile-title">Easing</div>
                  <div class="track"><div class="ball">●</div></div>
                </div>
                <div class="tile">
                  <div class="tile-title">Loading</div>
                  <div class="load"><div class="fill"></div></div>
                </div>
                <div class="tile lift">
                  <div class="tile-title">Hover me</div>
                  <div class="tile-body">translate + color</div>
                </div>
                <div class="tile lift">
                  <div class="tile-title">And me</div>
                  <div class="tile-body">transition: 200ms</div>
                </div>
                <div class="tile hue">
                  <div class="tile-title">Color</div>
                  <div class="tile-body">background keyframes</div>
                </div>
              </div>
              <button onclick="ShowToast">Show a toast</button>
            </div>
          </elif>
          <elif condition="{on_widgets}">
            <div class="page form">
              <div class="field">
                <label>Name</label>
                <input value="{name}" placeholder="Your name" />
                <span class="echo">{greeting}</span>
              </div>
              <div class="field">
                <label>Theme</label>
                <select value="{theme}">
                  <option value="midnight">midnight</option>
                  <option value="daylight">daylight</option>
                  <option value="solarized">solarized</option>
                </select>
              </div>
              <div class="field">
                <label>Volume</label>
                <slider value="{volume}" min="0" max="100" step="5" width="30" />
                <span class="echo">{volume_text}</span>
              </div>
              <div class="field">
                <label>Level</label>
                <progress value="{volume}" max="100" width="30" />
              </div>
              <div class="field">
                <label>Options</label>
                <checkbox checked="{notifications}">Notifications</checkbox>
                <checkbox checked="{telemetry}">Telemetry</checkbox>
              </div>
              <tabs value="{plan}">
                <tab-pane label="Free" name="free"><p class="pane">Everything, for one user.</p></tab-pane>
                <tab-pane label="Pro" name="pro"><p class="pane">Everything, for the whole team.</p></tab-pane>
                <tab-pane label="Enterprise" name="enterprise"><p class="pane">Everything, with a contract.</p></tab-pane>
              </tabs>
              <div class="summary">{summary}</div>
            </div>
          </elif>
          <else>
            <div class="page text">
              <markdown content="{markdown}" stylesheet="{markdown_style}" />
            </div>
          </else>
        </div>
      </div>

      <toast open="{toast_open}" duration="2500">{toast_text}</toast>

      <style>
        @keyframes enter {
          from { opacity: 0; translate: 0 1; }
          to { opacity: 1; translate: 0 0; }
        }
        @keyframes wave {
          from { color: rgb(48, 54, 61); translate: 0 0; }
          to { color: rgb(88, 166, 255); translate: 0 -1; }
        }
        @keyframes slide {
          from { translate: 0; }
          to { translate: 20; }
        }
        @keyframes load {
          from { width: 0; }
          to { width: 100%; }
        }
        @keyframes hue {
          0% { background-color: rgb(88, 166, 255); }
          33% { background-color: rgb(188, 140, 255); }
          66% { background-color: rgb(248, 81, 73); }
          100% { background-color: rgb(63, 185, 80); }
        }

        self {
          --bg: rgb(13, 17, 23);
          --surface: rgb(22, 27, 34);
          --raised: rgb(31, 38, 47);
          --border: rgb(48, 54, 61);
          --muted: rgb(139, 148, 158);
          --text: rgb(230, 237, 243);
          --accent: rgb(88, 166, 255);
          --up: rgb(63, 185, 80);
          --down: rgb(248, 81, 73);

          display: block;
          width: 100%;
          height: 100%;
          background-color: var(--bg);
          color: var(--text);
        }
        .app {
          display: flex;
          height: 100%;
        }

        .sidebar {
          display: flex;
          flex-direction: column;
          width: 22;
          background-color: var(--surface);
          border-right: tall;
          border-color: var(--border);
          padding: 1 1;
        }
        .brand {
          font-weight: bold;
          margin: 0 1 1 1;
        }
        .logo {
          color: var(--accent);
        }
        .nav {
          display: flex;
          gap: 1;
          padding: 0 1;
          color: var(--muted);
          border-left: tall;
          border-color: var(--surface);
          transition: background-color 150ms, color 150ms, border-color 150ms;
        }
        .nav:hover {
          background-color: var(--raised);
          color: var(--text);
        }
        .nav.active {
          color: var(--accent);
          border-color: var(--accent);
          background-color: var(--raised);
          font-weight: bold;
        }
        .grow {
          flex-grow: 1;
        }
        .hint {
          color: var(--muted);
          padding: 0 1;
        }

        .main {
          display: flex;
          flex-direction: column;
          flex-grow: 1;
          padding: 1 2;
        }
        h1 {
          color: var(--text);
          font-weight: bold;
          text-decoration: none;
        }
        .header p {
          color: var(--muted);
          margin-bottom: 1;
        }
        .page {
          display: flex;
          flex-direction: column;
          gap: 1;
          animation: enter 300ms ease-out;
        }

        .grid {
          display: grid;
          grid-template-columns: repeat(auto-fill, minmax(24, 1fr));
          gap: 1 2;
        }
        .metric {
          border: tall;
          border-color: var(--border);
          background-color: var(--surface);
          padding: 0 2;
          transition: border-color 200ms, translate 200ms ease-out;
        }
        .metric:hover {
          border-color: var(--accent);
          translate: 0 -1;
        }
        .metric-name {
          color: var(--muted);
        }
        .metric-value {
          font-weight: bold;
        }
        .unit {
          color: var(--muted);
          font-weight: normal;
        }
        .delta.up {
          color: var(--up);
        }
        .delta.down {
          color: var(--down);
        }

        .chart {
          border: tall;
          border-color: var(--border);
          background-color: var(--surface);
          padding: 0 2;
        }
        .chart-title {
          color: var(--muted);
        }
        .bars {
          display: flex;
          align-items: flex-end;
          gap: 1;
          height: 8;
        }
        .bar {
          flex-grow: 1;
          background-color: var(--accent);
          transition: height 400ms ease-out;
        }

        .tiles {
          display: grid;
          grid-template-columns: repeat(auto-fill, minmax(26, 1fr));
          gap: 1 2;
        }
        .tile {
          border: tall;
          border-color: var(--border);
          background-color: var(--surface);
          padding: 1 2;
          height: 7;
        }
        .tile-title {
          color: var(--muted);
          margin-bottom: 1;
        }
        .tile-body {
          color: var(--text);
        }
        .wave span {
          margin-right: 1;
          animation: wave 500ms ease-in-out infinite alternate;
        }
        .w2 { animation-delay: 100ms; }
        .w3 { animation-delay: 200ms; }
        .w4 { animation-delay: 300ms; }
        .w5 { animation-delay: 400ms; }
        .track {
          width: 21;
          border-bottom: solid;
          border-color: var(--border);
        }
        .ball {
          width: 1;
          color: rgb(210, 153, 34);
          animation: slide 1.2s ease-in-out infinite alternate;
        }
        .load {
          width: 100%;
          background-color: var(--raised);
        }
        .fill {
          height: 1;
          width: 0;
          background-color: var(--up);
          animation: load 2s ease-in-out infinite;
        }
        .lift {
          transition: translate 200ms ease-out, border-color 200ms, background-color 200ms;
        }
        .lift:hover {
          translate: 0 -1;
          border-color: var(--accent);
          background-color: var(--raised);
        }
        .hue {
          animation: hue 3s linear infinite alternate;
        }
        .hue .tile-title, .hue .tile-body {
          color: var(--bg);
        }

        button {
          border: tall;
          border-color: var(--border);
          background-color: var(--bg);
          padding: 0 2;
          width: 24;
          text-align: center;
          transition: border-color 150ms, color 150ms;
        }
        button:hover, button:focus {
          border-color: var(--accent);
          color: var(--accent);
        }

        .form {
          width: 70;
        }
        .field {
          display: flex;
          align-items: center;
          gap: 2;
        }
        label {
          width: 8;
          color: var(--muted);
        }
        input {
          width: 30;
          border: tall;
          border-color: var(--border);
          background-color: var(--surface);
          transition: border-color 150ms;
        }
        input:focus {
          border-color: var(--accent);
        }
        select {
          width: 30;
        }
        slider, progress {
          color: var(--accent);
          background-color: var(--raised);
        }
        .echo {
          color: var(--accent);
        }
        .pane {
          color: var(--muted);
        }
        .summary {
          color: var(--muted);
          border-top: solid;
          border-color: var(--border);
        }

        .text {
          width: 72;
        }
        /* Small terminals: the sidebar keeps only its icons, then the chart
           gives its rows to the cards. */
        @media (max-width: 99) {
          .sidebar {
            width: 8;
          }
          .label {
            display: none;
          }
        }
        @media (max-width: 79) {
          .chart {
            display: none;
          }
        }
        @media (max-height: 29) {
          .chart {
            display: none;
          }
        }

        toast {
          border: tall;
          border-color: var(--up);
          background-color: var(--surface);
          color: var(--up);
          padding: 0 2;
        }
      </style>
    )html";

 private:
  // One tick of the dashboard: every metric takes a step of a random walk.
  void Step() {
    for (Metric& metric : metrics) {
      metric.value += (Noise(seed_) - 0.47) * metric.scale;
      metric.value = std::max(metric.value, metric.scale);
      metric.history.push_back(metric.value);
      if (metric.history.size() > 18) {
        metric.history.erase(metric.history.begin());
      }
    }
    if (metrics[0].history.size() < 2) {
      metrics[0].history.push_back(metrics[0].value);
    }
    bars.push_back({static_cast<int>(1 + Noise(seed_) * 7), ""});
    if (bars.size() > 24) {
      bars.erase(bars.begin());
    }
  }

  unsigned seed_ = 7;
};

int main() {
  auto app = Ref<Showcase>::New();
  Screen screen(app);
  app->Start();
  screen.Loop();
  return 0;
}
