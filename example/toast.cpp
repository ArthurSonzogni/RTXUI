// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
//
// Toast notifications.
//
// A <toast> shows over everything else in a corner of the screen, and closes
// itself after `duration` milliseconds, or when clicked. Its `open` attribute
// is two-way bound: the application raises its flag to show it, and the flag
// falls back to false when the toast closes.
//
// Try it: press Save or Fail, then wait, or click a toast to dismiss it.
#include <rtxui/rtxui.hpp>

using namespace rtxui;

class ToastDemo : public Component<ToastDemo> {
 public:
  bool saved = false;
  bool failed = false;
  int saves = 0;

  void Save() {
    saves++;
    saved = true;
  }
  void Fail() { failed = true; }

  ToastDemo() {
    Bind(saved);
    Bind(failed);
    Bind(saves);
    Bind(Save);
    Bind(Fail);
  }

  std::string_view view = R"html(
      <div class="card">
        <h1>Toasts</h1>
        <p>Saved {saves} times.</p>
        <div class="row">
          <button onclick="Save">Save</button>
          <button onclick="Fail">Fail</button>
        </div>
      </div>

      <toast open="{saved}" duration="2000">✔ Saved</toast>
      <toast class="error" open="{failed}" duration="4000" placement="top-right">
        ✘ Could not reach the server
      </toast>

      <style>
        self {
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
          border-color: rgb(48, 54, 61);
          background-color: rgb(22, 27, 34);
          padding: 1 3;
        }
        h1 {
          color: rgb(88, 166, 255);
          font-weight: bold;
        }
        .row {
          display: flex;
          gap: 2;
          margin-top: 1;
        }
        .error::part(toast) {
          border-color: rgb(248, 81, 73);
          color: rgb(255, 161, 152);
        }
      </style>
    )html";
};

int main() {
  auto app = Ref<ToastDemo>::New();
  Screen screen(app);
  screen.Loop();
  return 0;
}
