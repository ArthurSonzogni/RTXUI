// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
//
// <canvas>: the application draws the cells and reads the pointer. Braille
// curves animated from a timer, and a half-block surface to paint on.
#include <charconv>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <rtxui/color.hpp>
#include <rtxui/rtxui.hpp>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

using namespace rtxui;

struct Swatch {
  std::string css;
  Color color;
};

// A segment of a stroke, in the paint canvas's dots.
struct Segment {
  int x0, y0, x1, y1;
  int swatch;
};

class CanvasDemo : public Component<CanvasDemo> {
 public:
  std::vector<Swatch> swatches = {
      {"rgb(88, 166, 255)", Color::RGB(88, 166, 255)},
      {"rgb(63, 185, 80)", Color::RGB(63, 185, 80)},
      {"rgb(210, 153, 34)", Color::RGB(210, 153, 34)},
      {"rgb(248, 81, 73)", Color::RGB(248, 81, 73)},
      {"rgb(188, 140, 255)", Color::RGB(188, 140, 255)},
      {"rgb(230, 237, 243)", Color::RGB(230, 237, 243)},
  };
  int swatch = 0;
  std::vector<Segment> segments;
  int segment_count = 0;
  int last_x = 0;
  int last_y = 0;
  std::string pointer = "no pointer yet";

  float phase = 0;
  bool playing = false;

  std::string_view view = R"html(
      <div class="card">
        <div class="title">
          <h1>Canvas</h1>
          <span>{pointer} · {segment_count} segments</span>
        </div>

        <canvas class="wave" ondraw="DrawWave"/>
        <canvas class="paint" ondraw="DrawPaint" onpointer="Paint"/>

        <div class="actions">
          <button onclick="Toggle">{play_label}</button>
          <div class="swatches">
            <for each="{swatches}" as="s">
              <button class="swatch {s.class}" style="color: {s.css};" onclick="Pick({$index})">██</button>
            </for>
          </div>
          <button onclick="Clear">Clear</button>
        </div>
      </div>

      <style>
        self {
          --bg: rgb(13, 17, 23);
          --border: rgb(48, 54, 61);
          --muted: rgb(139, 148, 158);
          --accent: rgb(88, 166, 255);

          display: flex;
          align-items: center;
          justify-content: center;
          width: 100%;
          color: rgb(230, 237, 243);
        }
        .card {
          border: tall;
          border-color: var(--border);
          background-color: rgb(22, 27, 34);
          padding: 1 3;
          width: 64;
        }
        .title {
          display: flex;
          justify-content: space-between;
          color: var(--muted);
        }
        h1 {
          color: var(--accent);
          margin-bottom: 0;
        }
        canvas {
          display: block;
          width: 100%;
        }
        .wave {
          height: 5;
          color: var(--muted);
        }
        .paint {
          height: 6;
          background-color: var(--bg);
          color: var(--muted);
        }
        .actions {
          display: flex;
          justify-content: space-between;
          margin-top: 1;
        }
        .swatches {
          display: flex;
        }
        button {
          border: tall;
          border-color: var(--border);
          background-color: var(--bg);
          padding: 0 1;
          transition: border-color 0.15s ease;
        }
        button:hover, button:focus {
          border-color: var(--accent);
        }
        .swatch.picked {
          border-color: rgb(230, 237, 243);
        }
      </style>
    )html";

  CanvasDemo() {
    Bind(swatches, &CanvasDemo::SwatchFields);
    Bind(swatch);
    Bind(segment_count);
    Bind(pointer);
    Bind(phase);
    Bind(playing);
    Bind(play_label);
    Bind(DrawWave);
    Bind(DrawPaint);
    Bind(Paint);
    Bind(Pick);
    Bind(Clear);
    Bind(Toggle);
    EnableHotReload();
  }

  void SwatchFields(const Swatch& s, FieldSink& out) const {
    out("css", s.css);
    out("class", &s == &swatches[static_cast<size_t>(swatch)] ? "picked" : "");
  }

  std::string play_label() const { return playing ? "Pause" : "Play"; }

  // Two curves on an axis, in braille dots: 2×4 a cell.
  void DrawWave(Canvas& c) {
    const int w = c.pixel_width();
    const int h = c.pixel_height();
    c.Line(0, h / 2, w - 1, h / 2);  // In the CSS color.
    const auto curve = [&](float frequency, float offset, Color color) {
      int previous = 0;
      for (int x = 0; x < w; ++x) {
        const float t = static_cast<float>(x) / static_cast<float>(w);
        const float y = std::sin(t * frequency * 6.2832f + phase + offset);
        const int py =
            static_cast<int>(std::lround((h - 1) * (0.5f - 0.45f * y)));
        if (x > 0) {
          c.Line(x - 1, previous, x, py, color);
        }
        previous = py;
      }
    };
    curve(2, 0, Color::RGB(88, 166, 255));
    curve(3, 1.5f, Color::RGB(210, 153, 34));
  }

  // What was painted, in half blocks: 1×2 dots a cell, a color each.
  void DrawPaint(Canvas& c) {
    if (segments.empty()) {
      const std::string_view hint = "drag here to paint";
      c.Text((c.width() - static_cast<int>(hint.size())) / 2, c.height() / 2,
             hint);  // In the CSS color.
      return;
    }
    c.SetPixels(Canvas::Pixels::HalfBlock);
    for (const Segment& s : segments) {
      c.Line(s.x0, s.y0, s.x1, s.y1,
             swatches[static_cast<size_t>(s.swatch)].color);
    }
  }

  void Paint(const PointerEvent& e) {
    pointer =
        "dot " + std::to_string(e.pixel_x) + ", " + std::to_string(e.pixel_y);
    if (e.mouse.button() != Event::Mouse::Left) {
      return;
    }
    if (e.mouse.motion() == Event::Mouse::Pressed) {
      last_x = e.pixel_x;
      last_y = e.pixel_y;
    }
    if (e.mouse.motion() != Event::Mouse::Released) {
      segments.push_back({last_x, last_y, e.pixel_x, e.pixel_y, swatch});
      segment_count = static_cast<int>(segments.size());
      last_x = e.pixel_x;
      last_y = e.pixel_y;
    }
  }

  void Pick(const std::string& index) {
    int i = 0;
    const auto [ptr, ec] =
        std::from_chars(index.data(), index.data() + index.size(), i);
    if (ec == std::errc() && i >= 0 && i < static_cast<int>(swatches.size())) {
      swatch = i;
    }
  }

  void Clear() {
    segments.clear();
    segment_count = 0;
  }

  void Toggle() {
    playing = !playing;
    if (playing) {
      Tick(Ref<CanvasDemo>(this));
    }
  }

 private:
  // A frame of the animation, every 50 ms while playing.
  static void Tick(Ref<CanvasDemo> self) {
    if (!self->playing) {
      return;
    }
    self->phase += 0.2f;
    PostDelayedTask([self] { Tick(self); }, std::chrono::milliseconds(50));
  }
};

int main() {
  auto app = Ref<CanvasDemo>::New();
  Screen screen(app);
  screen.Loop();
  return 0;
}
