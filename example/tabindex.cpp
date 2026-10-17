// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
//
// Controlling focus order with tabindex.
#include <rtxui/rtxui.hpp>
#include <string_view>

using namespace rtxui;

class TabIndexDemo : public Component<TabIndexDemo> {
 public:
  std::string_view view = R"html(
      <div class="card">
        <h1>tabindex</h1>
        <p>Tab visits positive tabindex values first, in order, then 0 in document order. -1 is focusable by click only.</p>

        <div class="item" tabindex="2"><span class="tag">tabindex="2"</span>second</div>
        <div class="item" tabindex="0"><span class="tag">tabindex="0"</span>third</div>
        <div class="item" tabindex="1"><span class="tag">tabindex="1"</span>first</div>
        <div class="item" tabindex="-1"><span class="tag">tabindex="-1"</span>skipped by Tab</div>
        <div class="item" focusable="true"><span class="tag">focusable="true"</span>fourth, like tabindex="0"</div>
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
          height: 100%;
          background-color: rgb(13, 17, 23);
          color: rgb(230, 237, 243);
        }
        .card {
          border: tall;
          border-color: var(--border);
          background-color: rgb(22, 27, 34);
          padding: 1 3;
          width: 60;
        }
        h1 {
          color: var(--accent);
        }
        p {
          color: var(--muted);
        }
        .item {
          padding: 0 1;
        }
        .item:hover {
          background-color: rgb(33, 38, 45);
        }
        .item:focus {
          background-color: rgb(22, 45, 77);
          color: var(--accent);
        }
        .tag {
          display: inline-block;
          width: 20;
          color: var(--muted);
        }
      </style>
    )html";
};

int main() {
  auto app = Ref<TabIndexDemo>::New();
  Screen screen(app);
  screen.Loop();
  return 0;
}
