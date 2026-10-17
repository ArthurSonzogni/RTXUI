// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
//
// Scroll-into-view on keyboard focus.
//
// Tabbing to an element that is outside its scroll container scrolls it into
// view automatically.
//
// Try it: hold Tab and watch the list follow the focus ring.
#include <rtxui/rtxui.hpp>
#include <string>
#include <string_view>
#include <vector>

using namespace rtxui;

class FocusScrollDemo : public Component<FocusScrollDemo> {
 public:
  std::vector<std::string> items = {
      "Inbox", "Drafts", "Sent",   "Archive",  "Spam",        "Trash",
      "Work",  "Family", "Travel", "Receipts", "Newsletters", "Later",
  };

  std::string_view view = R"html(
      <div class="card">
        <h1>Focus scrolls into view</h1>
        <p>Press Tab or Shift+Tab: the list scrolls to keep the focused item visible.</p>

        <div class="list">
          <for each="{items}" as="item">
            <div class="item" tabindex="0">{item}</div>
          </for>
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
          height: 100%;
          background-color: rgb(13, 17, 23);
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
        .list {
          height: 8;
          overflow-y: scroll;
          border: round;
          border-color: var(--border);
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
      </style>
    )html";

  FocusScrollDemo() { Bind(items); }
};

int main() {
  auto app = Ref<FocusScrollDemo>::New();
  Screen screen(app);
  screen.Loop();
  return 0;
}
