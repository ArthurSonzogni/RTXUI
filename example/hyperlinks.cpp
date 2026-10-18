// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
//
// Terminal hyperlinks: an <a href> to a URL becomes a clickable link.
//
// RTXUI marks the link's text with the OSC 8 escape sequence. kitty, WezTerm,
// iTerm2, foot, GNOME Terminal and Windows Terminal open it the way each does
// for links, usually Ctrl- or Cmd-click (plus Shift in some, since the app
// reads the mouse). Other terminals show the text as usual.
//
// Try it: Ctrl-click a link, or Tab to one and look at the focus style.
#include <rtxui/rtxui.hpp>
#include <string_view>

using namespace rtxui;

class HyperlinksDemo : public Component<HyperlinksDemo> {
 public:
  std::string_view view = R"html(
      <div class="card">
        <h1>Hyperlinks</h1>
        <p>Ctrl-click (or Cmd-click) a link in a terminal that supports it.</p>

        <div class="links">
          <div class="link-row">
            <span class="kind">https</span>
            <a href="https://github.com/ArthurSonzogni/RTXUI">RTXUI on GitHub</a>
          </div>
          <div class="link-row">
            <span class="kind">https</span>
            <a href="https://arthursonzogni.github.io/RTXUI/">The documentation</a>
          </div>
          <div class="link-row">
            <span class="kind">mailto</span>
            <a href="mailto:someone@example.com">someone@example.com</a>
          </div>
          <div class="link-row">
            <span class="kind">file</span>
            <a href="file:///etc/hosts">/etc/hosts</a>
          </div>
        </div>

        <p class="hint">A link that wraps stays one link: <a href="https://en.wikipedia.org/wiki/ANSI_escape_code">the escape codes behind it are on Wikipedia</a>.</p>
      </div>

      <style>
        self {
          --bg: rgb(13, 17, 23);
          --muted: rgb(139, 148, 158);
          --text: rgb(230, 237, 243);
          --accent: rgb(88, 166, 255);

          display: flex;
          align-items: center;
          justify-content: center;
          width: 100%;
          color: var(--text);
        }
        .card {
          width: 60;
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
        .links {
          margin-bottom: 1;
        }
        .link-row {
          display: flex;
          gap: 2;
        }
        .kind {
          width: 7;
          color: var(--muted);
        }
        a {
          color: var(--accent);

          &:focus {
            background-color: var(--accent);
            color: var(--bg);
          }
        }
        .hint {
          margin-bottom: 0;
        }
      </style>
    )html";
};

int main() {
  auto app = Ref<HyperlinksDemo>::New();
  Screen screen(app);
  screen.Loop();
  return 0;
}
