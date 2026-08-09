// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
//
// Recipe: a confirmation dialog.
//
// The built-in <dialog> renders centered above the interface with a dimmed
// backdrop and closes on Escape. Bind its `open` attribute to a bool.
#include <rtxui/rtxui.hpp>

#include <string>

using namespace rtxui;

// Cookbook recipe: confirmation dialog.
//
// The built-in <dialog> element renders centered above the rest of the
// interface with a dimmed backdrop, and closes on Escape. Bind its `open`
// attribute to a boolean and flip that boolean from methods.
class ConfirmApp : public Component<ConfirmApp> {
 public:
  bool confirming = false;
  std::string result = "No action taken.";

  void Ask() { confirming = true; }
  void Confirm() {
    result = "Action confirmed.";
    confirming = false;
  }
  void Cancel() {
    result = "Action cancelled.";
    confirming = false;
  }

  ConfirmApp() {
    Bind(confirming);
    Bind(result);
    Bind(Ask);
    Bind(Confirm);
    Bind(Cancel);
  }

  std::string_view view = R"html(
    <div class="main">
      <div>{result}</div>
      <button onclick="Ask">Delete everything</button>

      <dialog open="{confirming}" title="Confirm">
        <p>Are you sure? This cannot be undone.</p>
        <div class="actions">
          <button class="danger" onclick="Confirm">Confirm</button>
          <button onclick="Cancel">Cancel</button>
        </div>
      </dialog>
    </div>

    <style>
      .main { padding: 1; width: 60; height: 10; }
      .actions { display: flex; flex-direction: row; justify-content: flex-end; gap: 1; margin-top: 1; }
      .danger { background-color: #ef4444; color: #fff; }
    </style>
  )html";
};

int main() {
  auto app = Ref<ConfirmApp>::New();
  Screen screen(app);
  screen.Loop();
  return 0;
}
