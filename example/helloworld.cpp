// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
//
// The smallest possible RTXUI application.
//
// A component is a class deriving from Component<Derived> whose `view` member
// holds an HTML template plus a <style> block. `self` selects the component's
// own root element, and custom properties declared there (--bg, --accent, ...)
// inherit into every descendant, so a single palette styles the whole tree.
#include <rtxui/rtxui.hpp>

using namespace rtxui;

class HelloWorldApp : public Component<HelloWorldApp> {
 public:
  std::string_view view = R"html(
      <div class="card">
        <h1>Hello, RTXUI</h1>
        <p>A reactive terminal UI, styled with CSS.</p>
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
          text-align: center;
        }
        h1 {
          color: var(--accent);
          font-weight: bold;
        }
        p {
          color: var(--muted);
          margin-top: 1;
        }
      </style>
    )html";
};

int main() {
  auto app = Ref<HelloWorldApp>::New();
  Screen screen(app);
  screen.Loop();
  return 0;
}
