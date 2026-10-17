// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
//
// The <tooltip> component, shown on hover in each of four directions.
#include <rtxui/rtxui.hpp>
#include <string>
#include <string_view>
#include <vector>

using namespace rtxui;

class TooltipDemo : public Component<TooltipDemo> {
 public:
  std::vector<std::string> placements = {
      "top-start",  "top",  "top-end",  "bottom-start", "bottom", "bottom-end",
      "left-start", "left", "left-end", "right-start",  "right",  "right-end",
  };
  std::string custom_text = "Edit the field above to change this.";

  std::string_view view = R"html(
      <div class="card">
        <h1>Tooltips</h1>
        <p>Hover a button to show its tooltip, on the side its placement names.</p>

        <div class="grid">
          <for each="{placements}" as="placement">
            <tooltip content="{placement}" placement="{placement}">
              <button>{placement}</button>
            </tooltip>
          </for>
        </div>

        <p>The content attribute can be bound, like any other:</p>
        <input value="{custom_text}" />
        <tooltip content="{custom_text}" placement="bottom">
          <button>Hover me</button>
        </tooltip>
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
          width: 66;
        }
        h1 {
          color: var(--accent);
        }
        p {
          color: var(--muted);
        }
        .grid {
          display: grid;
          grid-template-columns: repeat(3, 1fr);
          gap: 1 4;
          margin: 1 0;
        }
        button {
          text-align: center;
          background-color: rgb(13, 17, 23);
        }
        .grid button {
          width: 100%;
        }
        button:hover {
          background-color: rgb(22, 45, 77);
          color: var(--accent);
        }
        input {
          width: 100%;
          border: tall;
          border-color: var(--border);
          background-color: rgb(13, 17, 23);
          padding: 0 1;
          margin-bottom: 1;
        }
        input:focus {
          border-color: var(--accent);
        }
      </style>
    )html";

  TooltipDemo() {
    Bind(placements);
    Bind(custom_text);
  }
};

int main() {
  auto app = Ref<TooltipDemo>::New();
  Screen screen(app);
  screen.Loop();
  return 0;
}
