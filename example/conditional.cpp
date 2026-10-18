// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
//
// Conditional rendering.
//
// <if condition="{computed}"> includes its children only when the bound value
// is true; the `if` attribute does the same inline on any element.
//
// Try it: pick a status to swap the rendered branch.
#include <rtxui/rtxui.hpp>
#include <string>
#include <string_view>

using namespace rtxui;

class ConditionalApp : public Component<ConditionalApp> {
 public:
  std::string status = "loading";

  bool is_loading() const { return status == "loading"; }
  bool is_error() const { return status == "error"; }

  void Set(std::string value) { status = value; }

  std::string_view view = R"html(
      <div class="card">
        <h1>Conditional rendering</h1>
        <p>An &lt;if&gt; / &lt;elif&gt; / &lt;else&gt; chain renders its first true branch.</p>

        <div class="buttons">
          <button onclick="Set(loading)">Loading</button>
          <button onclick="Set(error)">Error</button>
          <button onclick="Set(done)">Done</button>
        </div>

        <div class="result">
          <if condition="{is_loading}">
            <span class="loading">Fetching the profile...</span>
          </if>
          <elif condition="{is_error}">
            <span class="error">The profile could not be fetched.</span>
          </elif>
          <else>
            <span class="done">Profile loaded.</span>
          </else>
        </div>

        <div if="{is_error}" class="hint">The if attribute shows a single element, like this hint.</div>
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
          width: 56;
        }
        h1 {
          color: var(--accent);
        }
        p {
          color: var(--muted);
        }
        .buttons {
          display: flex;
          gap: 2;
        }
        button {
          border: tall;
          border-color: var(--border);
          background-color: rgb(13, 17, 23);
          padding: 0 1;
        }
        button:hover, button:focus {
          border-color: var(--accent);
          color: var(--accent);
        }
        .result {
          border: round;
          border-color: var(--border);
          padding: 0 1;
          margin-top: 1;
        }
        .loading {
          color: rgb(210, 153, 34);
        }
        .error {
          color: rgb(248, 81, 73);
        }
        .done {
          color: rgb(63, 185, 80);
        }
        .hint {
          margin-top: 1;
          color: var(--muted);
        }
      </style>
    )html";

  ConditionalApp() {
    Bind(status);
    Bind(is_loading);
    Bind(is_error);
    Bind(Set);
  }
};

int main() {
  auto app = Ref<ConditionalApp>::New();
  Screen screen(app);
  screen.Loop();
  return 0;
}
