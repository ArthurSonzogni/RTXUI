// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
//
// <ul>, <ol> and <li>, including nesting, `start`/`reversed`, per-item `value`,
// and the list-style-type property.
#include <rtxui/rtxui.hpp>
#include <string>
#include <string_view>

using namespace rtxui;

class ListsDemo : public Component<ListsDemo> {
 public:
  std::string selected_style = "disc";

  std::string_view view = R"html(
      <div class="card">
        <h1>Lists</h1>
        <p>&lt;ul&gt; nests with a new bullet per level; &lt;ol&gt; counts, from start, or down when reversed.</p>

        <div class="style">
          <span>list-style-type</span>
          <select value="{selected_style}">
            <option value="disc">disc</option>
            <option value="circle">circle</option>
            <option value="square">square</option>
            <option value="decimal">decimal</option>
            <option value="none">none</option>
          </select>
        </div>

        <div class="columns">
          <ul class="styled">
            <li>Apples</li>
            <li>Pears</li>
            <li>Plums</li>
          </ul>
          <ul>
            <li>Fruit
              <ul>
                <li>Citrus
                  <ul>
                    <li>Lemon</li>
                  </ul>
                </li>
                <li>Berries</li>
              </ul>
            </li>
            <li>Vegetables</li>
          </ul>
          <ol start="3">
            <li>Clone</li>
            <li>Build</li>
            <li value="10">Ship</li>
          </ol>
          <ol reversed="true">
            <li>Three</li>
            <li>Two</li>
            <li>One</li>
          </ol>
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
        .style {
          display: flex;
          gap: 2;
          margin-bottom: 1;
          color: var(--muted);
        }
        select {
          width: 14;
          color: rgb(230, 237, 243);
        }
        select:focus {
          color: var(--accent);
        }
        .columns {
          display: flex;
          gap: 3;
        }
        .styled {
          list-style-type: {selected_style};
        }
      </style>
    )html";

  ListsDemo() { Bind(selected_style); }
};

int main() {
  auto app = Ref<ListsDemo>::New();
  Screen screen(app);
  screen.Loop();
  return 0;
}
