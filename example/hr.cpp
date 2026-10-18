// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
//
// The <hr> separator, and how border styles apply to it.
#include <rtxui/rtxui.hpp>
#include <string_view>

using namespace rtxui;

class HrDemo : public Component<HrDemo> {
 public:
  std::string_view view = R"html(
      <div class="card">
        <h1>Horizontal rule</h1>
        <p>&lt;hr&gt; draws a line across its container to separate sections.</p>

        <h2>Introduction</h2>
        <div>The first section, above the rule.</div>

        <hr />

        <h2>Details</h2>
        <div>The second section, below it.</div>
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
          width: 54;
        }
        h1 {
          color: var(--accent);
        }
        p {
          color: var(--muted);
        }
        h2 {
          margin-bottom: 0;
        }
      </style>
    )html";
};

int main() {
  auto app = Ref<HrDemo>::New();
  Screen screen(app);
  screen.Loop();
  return 0;
}
