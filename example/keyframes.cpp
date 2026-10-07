// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
//
// CSS keyframe animations.
//
// `@keyframes` names a sequence of styles, and `animation` plays one on an
// element: for how long, how many times, in which direction, and with which
// easing. Unlike a transition, it needs no change of state to start, which
// suits indicators that move on their own: a pulsing status light, a loading
// bar, a blinking cursor.
//
// Try it: click the button, or Tab to it and press Enter, to pause and resume
// every animation.
#include <rtxui/rtxui.hpp>

using namespace rtxui;

class KeyframesDemo : public Component<KeyframesDemo> {
 public:
  bool paused = false;

  std::string state() const { return paused ? "paused" : "running"; }
  std::string label() const { return paused ? "Resume" : "Pause"; }
  void Toggle() { paused = !paused; }

  KeyframesDemo() {
    Bind(paused);
    Bind(state);
    Bind(label);
    Bind(Toggle);
  }

  std::string_view view = R"html(
      <div class="card {state}">
        <h1>Keyframe animations</h1>

        <div class="row">
          <span class="light">●</span>
          <span>Live: pulses forever, alternating direction.</span>
        </div>

        <div class="row">
          <div class="track"><div class="bar"></div></div>
          <span>Loading: steps(10), repeats.</span>
        </div>

        <div class="row">
          <span>Prompt&gt; </span><span class="cursor">█</span>
          <span> Cursor: step-end, blinks.</span>
        </div>

        <div class="row">
          <span class="fade">Faded in once, then held (forwards).</span>
        </div>

        <button onclick="Toggle">{label}</button>
      </div>

      <style>
        @keyframes pulse {
          from { color: rgb(35, 134, 54); }
          to { color: rgb(126, 231, 135); }
        }
        @keyframes load {
          from { width: 0; }
          to { width: 20; }
        }
        @keyframes blink {
          from { opacity: 1; }
          50% { opacity: 0; }
        }
        @keyframes fade-in {
          from { opacity: 0; }
          to { opacity: 1; }
        }

        self {
          display: flex;
          align-items: center;
          justify-content: center;
          width: 100%;
          height: 100%;
          background-color: rgb(13, 17, 23);
          color: rgb(230, 237, 243);
        }
        .card {
          border: tall;
          border-color: rgb(48, 54, 61);
          background-color: rgb(22, 27, 34);
          padding: 1 3;
          width: 60;
        }
        h1 {
          color: rgb(88, 166, 255);
          font-weight: bold;
          margin-bottom: 1;
        }
        .row {
          display: flex;
          margin-bottom: 1;
        }
        .light {
          margin-right: 1;
          animation: pulse 0.8s ease-in-out infinite alternate;
        }
        .track {
          width: 20;
          margin-right: 1;
          background-color: rgb(48, 54, 61);
        }
        .bar {
          width: 0;
          height: 1;
          background-color: rgb(88, 166, 255);
          animation: load 2s steps(10) infinite;
        }
        .cursor {
          animation: blink 1s step-end infinite;
        }
        .fade {
          color: rgb(139, 148, 158);
          animation: fade-in 1s ease-out forwards;
        }
        .paused .light, .paused .bar, .paused .cursor, .paused .fade {
          animation-play-state: paused;
        }
      </style>
    )html";
};

int main() {
  auto app = Ref<KeyframesDemo>::New();
  Screen screen(app);
  screen.Loop();
  return 0;
}
