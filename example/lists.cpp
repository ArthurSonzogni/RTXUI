// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
//
// <ul>, <ol> and <li>, including nesting, `start`/`reversed`, per-item `value`,
// and the list-style-type property.
#include <rtxui/rtxui.hpp>

using namespace rtxui;

class ListsDemo : public Component<ListsDemo> {
 public:
  std::string selected_style = "disc";

  std::string_view view = R"html(
      <div class="container">
        <div class="header">
          <span class="title">RTXUI List Elements Demo</span>
          <span class="desc">Showcases nested unordered lists (&lt;ul&gt;), ordered lists (&lt;ol&gt;), and dynamic styling.</span>
        </div>

        <div class="demo-grid">
          <!-- Sidebar Controls -->
          <div class="sidebar">
            <span class="section-title">Controls</span>
            <p class="control-label">Custom List Style:</p>
            <select value="{selected_style}">
              <option value="disc">disc (•)</option>
              <option value="circle">circle (○)</option>
              <option value="square">square (■)</option>
              <option value="decimal">decimal (1., 2.)</option>
              <option value="none">none</option>
            </select>
          </div>

          <!-- Live Preview -->
          <div class="preview">
            <span class="section-title">Interactive Preview</span>
            
            <p class="preview-label">1. Custom Styled List (Dynamic list-style-type)</p>
            <ul style="list-style-type: {selected_style};">
              <li>Dynamic list item A</li>
              <li>Dynamic list item B</li>
              <li>Dynamic list item C</li>
            </ul>

            <p class="preview-label">2. Default Nested List (Automatic bullet alternation)</p>
            <ul>
              <li>Level 1 (disc)
                <ul>
                  <li>Level 2 (circle)
                    <ul>
                      <li>Level 3 (square)</li>
                      <li>Level 3 item 2</li>
                    </ul>
                  </li>
                  <li>Level 2 item 2</li>
                </ul>
              </li>
              <li>Level 1 item 2</li>
            </ul>

            <p class="preview-label">3. Ordered List (Decimal numbering)</p>
            <ol>
              <li>First steps with RTXUI</li>
              <li>Writing XML-based components</li>
              <li>Applying custom CSS rules</li>
            </ol>
          </div>
        </div>
      </div>

      <style>
        self {

          display: block;
          padding: 1;
          background-color: rgb(13, 17, 23);
          color: white;
          width: 100%;
          height: 100%;
        }
        .container {
          display: block;
        }
        .header {
          display: block;
          margin-bottom: 2;
        }
        .title {
          display: block;
          font-weight: bold;
          color: rgb(56, 189, 248);
          margin-bottom: 1;
        }
        .desc {
          display: block;
          color: rgb(139, 148, 158);
        }
        .demo-grid {
          display: flex;
          flex-direction: row;
          gap: 4;
        }
        .sidebar {
          display: block;
          width: 30;
          border-right: solid;
          border-color: rgb(48, 54, 61);
          padding-right: 2;
        }
        .preview {
          display: block;
          flex: 1;
          padding-left: 2;
        }
        .section-title {
          display: block;
          font-weight: bold;
          color: rgb(244, 63, 94);
          margin-bottom: 2;
        }
        .control-label {
          display: block;
          color: rgb(203, 213, 225);
          margin-bottom: 1;
        }
        .preview-label {
          display: block;
          font-weight: bold;
          color: rgb(56, 189, 248);
          margin-top: 2;
          margin-bottom: 1;
        }
        ul, ol {
          margin-bottom: 1;
        }
      </style>
    )html";

  ListsDemo() {
    Bind(selected_style);
  }
};

int main() {
  auto app = Ref<ListsDemo>::New();
  Screen screen(app);
  screen.Loop();
  return 0;
}
