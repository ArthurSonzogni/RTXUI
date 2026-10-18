// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
//
// Anchor navigation.
//
// An <a href="#id"> scrolls its target into view inside the nearest scroll
// container.
//
// Try it: click the links in the index to jump between sections.
#include <rtxui/rtxui.hpp>
#include <string_view>

using namespace rtxui;

class AnchorDemo : public Component<AnchorDemo> {
 public:
  std::string_view view = R"html(
      <div class="card">
        <h1>Anchors</h1>
        <p>A link to "#id" scrolls that element into view. Click one, or Tab to it and press Enter.</p>

        <div class="workspace">
          <div class="index">
            <a href="#intro">Introduction</a>
            <a href="#install">Installation</a>
            <a href="#usage">Usage</a>
            <a href="#styling">Styling</a>
            <a href="#license">License</a>
          </div>

          <div class="pane">
            <div id="intro" class="section">
              <h2>Introduction</h2>
              RTXUI builds terminal interfaces from HTML templates and CSS, bound to C++ members.
            </div>
            <div id="install" class="section">
              <h2>Installation</h2>
              Add it with CMake's FetchContent, then link your target with rtxui::rtxui.
            </div>
            <div id="usage" class="section">
              <h2>Usage</h2>
              Derive a component, give it a view, bind its members, and run a Screen.
            </div>
            <div id="styling" class="section">
              <h2>Styling</h2>
              A style block in the view holds the CSS: flexbox, grid, borders, transitions.
            </div>
            <div id="license" class="section">
              <h2>License</h2>
              MIT.
            </div>
          </div>
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
          width: 70;
        }
        h1 {
          color: var(--accent);
        }
        p {
          color: var(--muted);
        }
        .workspace {
          display: flex;
          gap: 2;
        }
        .index {
          display: flex;
          flex-direction: column;
          width: 14;
          flex-shrink: 0;
        }
        a {
          color: var(--accent);
          text-decoration: none;
        }
        a:hover, a:focus {
          text-decoration: underline;
        }
        .pane {
          flex-grow: 1;
          height: 12;
          overflow-y: scroll;
          scroll-behavior: smooth;
          border-left: solid;
          border-color: var(--border);
          padding-left: 2;
        }
        .section {
          margin-bottom: 4;
        }
        h2 {
          color: var(--accent);
          margin-bottom: 0;
        }
      </style>
    )html";
};

int main() {
  auto app = Ref<AnchorDemo>::New();
  Screen screen(app);
  screen.Loop();
  return 0;
}
