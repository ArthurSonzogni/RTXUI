// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
//
// The built-in <dialog> element.
//
// `open` is bound to a bool; the dialog renders centered over the rest of the
// UI with a dimmed backdrop and closes on Escape.
//
// Try it: open the dialog, then dismiss it with either button or Escape.
#include <rtxui/rtxui.hpp>

using namespace rtxui;

class DialogDemo : public Component<DialogDemo> {
 public:
  bool is_modal_open = false;
  std::string last_action = "Nothing yet.";

  void ShowDialog() { is_modal_open = true; }

  void Proceed() {
    last_action = "Proceeded.";
    is_modal_open = false;
  }

  void Cancel() {
    last_action = "Cancelled.";
    is_modal_open = false;
  }

  std::string_view view = R"html(
      <div class="card">
        <h1>Overlays &amp; Dialogs</h1>
        <p>The dialog renders above everything else, on a dimmed backdrop.</p>

        <button onclick="ShowDialog">Open dialog</button>

        <div class="status">
          <span class="label">Last action</span>
          <span class="value">{last_action}</span>
        </div>

        <dialog open="{is_modal_open}" title="Confirm destruction">
          <p class="dialog-msg">This action is permanent and cannot be undone.</p>
          <div class="actions">
            <button class="danger" onclick="Proceed">Proceed</button>
            <button onclick="Cancel">Cancel</button>
          </div>
        </dialog>
      </div>

      <style>
        self {
          --bg: rgb(13, 17, 23);
          --surface: rgb(22, 27, 34);
          --border: rgb(48, 54, 61);
          --text: rgb(230, 237, 243);
          --muted: rgb(139, 148, 158);
          --accent: rgb(88, 166, 255);
          --danger: rgb(248, 81, 73);

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
          width: 54;
        }
        h1 {
          color: var(--accent);
          font-weight: bold;
        }
        p {
          color: var(--muted);
          margin-bottom: 1;
        }

        /* Every colour is stated outright: a button that inherits the
           terminal's default foreground can end up invisible against its own
           background on a light-themed terminal. */
        button {
          border: tall;
          border-color: var(--border);
          background-color: var(--bg);
          color: var(--text);
          padding: 0 2;
          text-align: center;
          transition: background-color 0.15s ease, border-color 0.15s ease,
                      color 0.15s ease;
        }
        button:hover, button:focus {
          border-color: var(--accent);
          color: var(--accent);
        }
        button:active {
          background-color: var(--accent);
          color: var(--bg);
        }

        .status {
          display: flex;
          justify-content: space-between;
          width: 100%;
          border-top: solid;
          border-color: var(--border);
          margin-top: 1;
          padding-top: 1;
        }
        .label {
          color: var(--muted);
        }
        .value {
          color: var(--accent);
          font-weight: bold;
        }

        .dialog-msg {
          color: var(--danger);
          margin-bottom: 1;
        }
        .actions {
          display: flex;
          gap: 2;
          justify-content: flex-end;
        }
        .actions button {
          /* Without this the two buttons shrink to fit and their labels wrap
             mid-word inside the dialog's max-width. */
          flex-shrink: 0;
        }
        .actions button.danger {
          border-color: var(--danger);
          color: var(--danger);
        }
        .actions button.danger:hover, .actions button.danger:focus {
          background-color: var(--danger);
          color: var(--bg);
        }
      </style>
    )html";

  DialogDemo() {
    Bind(is_modal_open);
    Bind(last_action);
    Bind(ShowDialog);
    Bind(Proceed);
    Bind(Cancel);
  }
};

int main() {
  auto app = Ref<DialogDemo>::New();
  Screen screen(app);
  screen.Loop();
  return 0;
}
