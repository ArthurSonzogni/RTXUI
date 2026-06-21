// Copyright 2024 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include <rtxui/rtxui.hpp>

#include "rtxui/component/default_components_internal.hpp"

using namespace rtxui;

class DialogDemo : public Component<DialogDemo> {
 public:
  bool is_modal_open = false;

  void ShowDialog() {
    is_modal_open = true;
  }
  void CloseDialog() {
    is_modal_open = false;
  }

  std::string_view view = R"html(
      <div class="content">
        <h1>RTXUI Overlays &amp; Dialog Modals</h1>
        <p>Press the button to show the floating overlay dialog modal:</p>
        
        <button onclick="ShowDialog">Open Dialog</button>

        <dialog open="{is_modal_open}" title="Confirm Destruction">
          <p class="dialog-msg">Warning: This action is permanent and cannot be undone.</p>
          <div class="actions">
            <button onclick="CloseDialog">Proceed</button>
            <button onclick="CloseDialog">Cancel</button>
          </div>
        </dialog>
      </div>

      <style>
        self {
          display: block;
          padding: 1;
          background-color: rgb(18, 18, 18);
        }
        h1 {
          color: rgb(59, 130, 246);
          margin-bottom: 1;
        }
        p {
          margin-bottom: 1;
        }
        .dialog-msg {
          margin-bottom: 2;
          color: rgb(248, 113, 113);
        }
        .actions {
          display: flex;
          flex-direction: row;
          gap: 2;
          justify-content: flex-end;
        }
      </style>
    )html";

  void InitReflection() override {
    Bind(is_modal_open);
    Bind(ShowDialog);
    Bind(CloseDialog);
    Import<rtxui::dialog>();
    Import<rtxui::div>();
    Import<rtxui::p>();
    Import<rtxui::h1>();
    Import<rtxui::button>();
    Component<DialogDemo>::InitReflection();
  }
};

int main() {
  auto app = Ref<DialogDemo>::New();
  Screen screen(app);
  screen.Loop();
  return 0;
}
