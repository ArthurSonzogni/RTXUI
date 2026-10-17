// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
//
// Focus styling: :focus-within, :focus-visible and currentColor.
//
// A group lights up while any field inside it has the focus (:focus-within).
// The buttons draw a focus ring when Tab reaches them, but not when they are
// clicked (:focus-visible). Every border is `currentColor`, so it follows the
// text color through each state and transition without repeating it.
//
// Try it: Tab through the fields and buttons, then click a button.
#include <rtxui/rtxui.hpp>
#include <string>
#include <string_view>

using namespace rtxui;

class FocusStatesDemo : public Component<FocusStatesDemo> {
 public:
  std::string name = "Ada";
  std::string email = "ada@example.com";
  std::string city = "London";
  std::string country = "England";

  FocusStatesDemo() {
    Bind(name);
    Bind(email);
    Bind(city);
    Bind(country);
  }

  std::string_view view = R"html(
      <div class="card">
        <h1>Focus states</h1>
        <p>Tab through the fields, then click a button.</p>

        <div class="groups">
          <div class="group">
            <span class="legend">Account</span>
            <input value="{name}" />
            <input value="{email}" />
          </div>
          <div class="group">
            <span class="legend">Address</span>
            <input value="{city}" />
            <input value="{country}" />
          </div>
        </div>

        <div class="row">
          <button>Save</button>
          <button>Cancel</button>
        </div>

        <p class="hint">A click leaves no ring; a key brings it back.</p>
      </div>

      <style>
        self {
          --bg: rgb(13, 17, 23);
          --muted: rgb(139, 148, 158);
          --text: rgb(230, 237, 243);
          --accent: rgb(88, 166, 255);
          --ring: rgb(163, 113, 247);

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
          border-color: rgb(48, 54, 61);
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
        .groups {
          display: flex;
          gap: 2;
          margin-bottom: 1;
        }
        .group {
          display: flex;
          flex-direction: column;
          width: 26;
          border: rounded;
          border-color: currentColor;
          color: var(--muted);
          padding: 0 1;
          transition: color 0.2s ease;

          &:focus-within {
            color: var(--accent);
          }
        }
        .legend {
          font-weight: bold;
        }
        input {
          color: var(--text);
        }
        .row {
          display: flex;
          gap: 2;
        }
        button {
          border: rounded;
          border-color: currentColor;
          color: var(--muted);
          padding: 0 2;
          transition: color 0.2s ease;

          &:hover {
            color: var(--text);
          }

          &:focus-visible {
            color: var(--ring);
          }
        }
        .hint {
          margin-top: 1;
          margin-bottom: 0;
        }
      </style>
    )html";
};

int main() {
  auto app = Ref<FocusStatesDemo>::New();
  Screen screen(app);
  screen.Loop();
  return 0;
}
