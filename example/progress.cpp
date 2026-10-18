// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
//
// The <progress> component driven by bound state.
#include <rtxui/rtxui.hpp>
#include <string_view>

using namespace rtxui;

class ProgressDemo : public Component<ProgressDemo> {
 public:
  int progress_val = 45;

  std::string_view view = R"html(
      <div class="card">
        <h1>Progress</h1>
        <p>The bar follows the slider's bound value.</p>

        <slider value="{progress_val}" min="0" max="100" width="40" />
        <progress value="{progress_val}" max="100" width="42" />

        <div class="readout">
          <span>completion</span>
          <span class="value">{progress_val}%</span>
        </div>
      </div>

      <style>
        self {
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
          width: 50;
        }
        h1 {
          color: var(--accent);
        }
        p {
          color: var(--muted);
        }
        slider:focus {
          color: var(--accent);
        }
        progress {
          margin-top: 1;
          color: var(--accent);
          background-color: var(--border);
        }
        .readout {
          display: flex;
          justify-content: space-between;
          border-top: solid;
          border-color: var(--border);
          margin-top: 1;
          padding-top: 1;
          color: var(--muted);
        }
        .value {
          color: var(--accent);
          font-weight: bold;
        }
      </style>
    )html";

  ProgressDemo() { Bind(progress_val); }
};

int main() {
  auto app = Ref<ProgressDemo>::New();
  Screen screen(app);
  screen.Loop();
  return 0;
}
