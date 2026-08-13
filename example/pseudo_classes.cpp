// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
//
// The interactive pseudo-classes: :hover, :focus and :active.
//
// Rules can be nested inside their parent with `&`, exactly as in modern CSS,
// so an element's interactive states live next to its base declarations.
//
// Try it: move the mouse over the buttons, Tab between them, and hold the
// mouse button down to see all three states.
#include <rtxui/rtxui.hpp>

using namespace rtxui;

class PseudoClassesDemo : public Component<PseudoClassesDemo> {
 public:
  std::string_view view = R"html(
      <div class="card">
        <h1>Pseudo-Classes</h1>
        <p>Hover, Tab to focus, or hold down the mouse button.</p>

        <div class="row">
          <div class="btn" tabindex="0">Hover me</div>
          <div class="btn" tabindex="0">Focus me</div>
          <div class="btn" tabindex="0">Press me</div>
        </div>

        <div class="legend">
          <span class="swatch hover"> </span><span>:hover</span>
          <span class="swatch focus"> </span><span>:focus</span>
          <span class="swatch active"> </span><span>:active</span>
        </div>
      </div>

      <style>
        self {
          --bg: rgb(13, 17, 23);
          --border: rgb(48, 54, 61);
          --text: rgb(230, 237, 243);
          --muted: rgb(139, 148, 158);
          --accent: rgb(88, 166, 255);
          --focus: rgb(163, 113, 247);
          --active: rgb(63, 185, 80);

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
          background-color: rgb(22, 27, 34);
          padding: 1 3;
        }
        h1 {
          color: var(--accent);
          font-weight: bold;
        }
        p {
          color: var(--muted);
          margin-bottom: 1;
        }
        .row {
          display: flex;
          gap: 2;
          margin-bottom: 1;
        }
        .btn {
          border: tall;
          border-color: var(--border);
          background-color: var(--bg);
          color: var(--text);
          padding: 0 2;
          text-align: center;
          transition: background-color 0.2s ease, border-color 0.2s ease,
                      color 0.2s ease;

          &:hover {
            border-color: var(--accent);
            color: var(--accent);
          }

          &:focus {
            border-color: var(--focus);
            color: var(--focus);
          }

          &:active {
            background-color: var(--active);
            border-color: var(--active);
            color: var(--bg);
          }
        }
        .legend {
          display: flex;
          gap: 1;
          color: var(--muted);
        }
        .swatch {
          width: 2;
        }
        .swatch.hover { background-color: var(--accent); }
        .swatch.focus { background-color: var(--focus); }
        .swatch.active { background-color: var(--active); }
      </style>
    )html";
};

int main() {
  auto app = Ref<PseudoClassesDemo>::New();
  Screen screen(app);
  screen.Loop();
  return 0;
}
