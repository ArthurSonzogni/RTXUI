// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
//
// Reactive state and `{...}` interpolation.
//
// Bind(count) registers a member as reactive state: the DOM is patched whenever
// it changes. Bind() on a const method registers a computed value that is
// re-evaluated from that state, and Bind() on a plain method registers an
// `onclick` handler.
//
// Try it: click the buttons, or Tab to them and press Enter.
#include <rtxui/rtxui.hpp>

using namespace rtxui;

class Counter : public Component<Counter> {
 public:
  int count = 0;

  int double_count() const { return count * 2; }

  void Increment() { count++; }
  void Decrement() { count--; }

  std::string_view view = R"html(
      <div class="card">
        <h1>Counter</h1>

        <div class="readout">
          <div class="stat">
            <span class="label">Count</span>
            <span class="value">{count}</span>
          </div>
          <div class="stat">
            <span class="label">Doubled</span>
            <span class="value accent">{double_count}</span>
          </div>
        </div>

        <div class="actions">
          <button onclick="Decrement">-  Decrement</button>
          <button onclick="Increment">+  Increment</button>
        </div>
      </div>

      <style>
        self {
          --bg: rgb(13, 17, 23);
          --surface: rgb(22, 27, 34);
          --border: rgb(48, 54, 61);
          --text: rgb(230, 237, 243);
          --muted: rgb(139, 148, 158);
          --accent: rgb(88, 166, 255);

          display: flex;
          align-items: center;
          justify-content: center;
          width: 100%;
          height: 100%;
          background-color: var(--bg);
          color: var(--text);
        }
        .card {
          border: tall;
          border-color: var(--border);
          background-color: var(--surface);
          padding: 1 3;
          width: 46;
        }
        h1 {
          color: var(--accent);
          font-weight: bold;
          margin-bottom: 1;
        }
        .readout {
          display: flex;
          gap: 3;
          margin-bottom: 1;
        }
        .stat {
          display: flex;
          flex-direction: column;
          flex-grow: 1;
        }
        .label {
          color: var(--muted);
        }
        .value {
          font-weight: bold;
        }
        .value.accent {
          color: var(--accent);
        }
        .actions {
          display: flex;
          gap: 2;
        }
        button {
          flex-grow: 1;
          border: tall;
          border-color: var(--border);
          background-color: var(--bg);
          color: var(--text);
          padding: 0 1;
          text-align: center;
          transition: background-color 0.15s ease, border-color 0.15s ease;
        }
        button:hover, button:focus {
          border-color: var(--accent);
          color: var(--accent);
        }
        button:active {
          background-color: var(--accent);
          color: var(--bg);
        }
      </style>
    )html";

  Counter() {
    Bind(count);
    Bind(double_count);
    Bind(Increment);
    Bind(Decrement);
  }
};

int main() {
  auto app = Ref<Counter>::New();
  Screen screen(app);
  screen.Loop();
  return 0;
}
