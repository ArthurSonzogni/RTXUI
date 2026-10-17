// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
//
// <fieldset> and <legend> grouping.
//
// The legend is nested into the top border of the group it captions.
#include <rtxui/rtxui.hpp>
#include <string_view>

using namespace rtxui;

class FieldsetDemo : public Component<FieldsetDemo> {
 public:
  std::string_view view = R"html(
      <div class="card">
        <h1>Fieldset</h1>
        <p>The legend sits in the top border of the group it captions.</p>

        <fieldset>
          <legend>Author</legend>
          <div><span class="key">Name</span>Arthur Sonzogni</div>
          <div><span class="key">Role</span>Maintainer</div>
        </fieldset>

        <fieldset>
          <legend>Project</legend>
          <div><span class="key">Name</span>RTXUI</div>
          <div><span class="key">Language</span>C++23</div>
        </fieldset>
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
        fieldset {
          margin-bottom: 0;
        }
        fieldset::part(legend-line) {
          color: var(--accent);
          background-color: rgb(22, 27, 34);
        }
        fieldset::part(fieldset-body) {
          border-color: var(--border);
          padding: 1 2;
        }
        .key {
          display: inline-block;
          width: 10;
          color: var(--muted);
        }
      </style>
    )html";
};

int main() {
  auto app = Ref<FieldsetDemo>::New();
  Screen screen(app);
  screen.Loop();
  return 0;
}
