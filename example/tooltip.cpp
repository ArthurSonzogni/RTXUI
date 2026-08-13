// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
//
// The <tooltip> component, shown on hover in each of four directions.
#include <rtxui/rtxui.hpp>
#include <string>

using namespace rtxui;

class TooltipDemo : public Component<TooltipDemo> {
 public:
  std::string custom_text = "Press any key to modify this text!";

  std::string_view view = R"html(
    <div class="container">
      <h2>RTXUI Tooltip Component Demo</h2>
      <p class="desc">
        Hover over the buttons below using your mouse cursor to see tooltips rendered in different directions and alignments.
      </p>

      <div class="showcase">
        <div class="section-title">Top Placements:</div>
        <div class="row">
          <tooltip content="Top Start (Left Aligned)" placement="top-start">
            <button class="btn">top-start</button>
          </tooltip>
          <tooltip content="Top Center (Centered)" placement="top">
            <button class="btn">top (center)</button>
          </tooltip>
          <tooltip content="Top End (Right Aligned)" placement="top-end">
            <button class="btn">top-end</button>
          </tooltip>
        </div>

        <div class="section-title">Bottom Placements:</div>
        <div class="row">
          <tooltip content="Bottom Start (Left Aligned)" placement="bottom-start">
            <button class="btn">bottom-start</button>
          </tooltip>
          <tooltip content="Bottom Center (Centered)" placement="bottom">
            <button class="btn">bottom (center)</button>
          </tooltip>
          <tooltip content="Bottom End (Right Aligned)" placement="bottom-end">
            <button class="btn">bottom-end</button>
          </tooltip>
        </div>

        <div class="section-title">Left Placements:</div>
        <div class="row">
          <tooltip content="Left Start (Top Aligned)" placement="left-start">
            <button class="btn tall-btn">left-start</button>
          </tooltip>
          <tooltip content="Left Center (Centered)" placement="left">
            <button class="btn tall-btn">left (center)</button>
          </tooltip>
          <tooltip content="Left End (Bottom Aligned)" placement="left-end">
            <button class="btn tall-btn">left-end</button>
          </tooltip>
        </div>

        <div class="section-title">Right Placements:</div>
        <div class="row">
          <tooltip content="Right Start (Top Aligned)" placement="right-start">
            <button class="btn tall-btn">right-start</button>
          </tooltip>
          <tooltip content="Right Center (Centered)" placement="right">
            <button class="btn tall-btn">right (center)</button>
          </tooltip>
          <tooltip content="Right End (Bottom Aligned)" placement="right-end">
            <button class="btn tall-btn">right-end</button>
          </tooltip>
        </div>

        <div class="section-title">Dynamic Tooltip:</div>
        <div class="row center-row">
          <tooltip content="{custom_text}" placement="top">
            <button class="btn highlight-btn">Dynamic Text Tooltip (top)</button>
          </tooltip>
        </div>

        <div class="input-section">
          <p>Customize Dynamic Tooltip Text:</p>
          <input value="{custom_text}" />
        </div>
      </div>
    </div>

    <style>
      self {
        --muted: rgb(139, 148, 158);
        --accent: rgb(88, 166, 255);

        display: block;
        padding: 2;
        background-color: rgb(13, 17, 23);
        color: rgb(230, 237, 243);
      }
      h2 {
        color: var(--accent);
        font-weight: bold;
        margin-bottom: 0;
      }
      .desc {
        color: var(--muted);
        margin-bottom: 2;
      }
      .showcase {
        max-width: 80;
        margin: auto;
        display: flex;
        flex-direction: column;
        gap: 1;
        width: 76;
      }
      .section-title {
        color: var(--accent);
        font-weight: bold;
        margin-top: 1;
        margin-bottom: 0;
      }
      .row {
        display: flex;
        flex-direction: row;
        justify-content: space-between;
        align-items: center;
        width: 100%;
        margin-bottom: 1;
      }
      .btn {
        background-color: rgb(22, 27, 34);
        color: white;
        cursor: pointer;
        padding: 0 1;
      }
      .btn:hover {
        background-color: var(--accent);
      }
      .tall-btn {
        height: 3;
      }
      .highlight-btn {
        color: rgb(254, 240, 138);
      }
      .highlight-btn:hover {
        background-color: rgb(234, 179, 8);
        color: black;
      }
      .input-section {
        display: block;
        margin-top: 1;
        padding-top: 1;
      }
      .input-section p {
        color: var(--muted);
        margin-bottom: 1;
      }
      input {
        width: 100%;
        padding-left: 1;
      }
    </style>
  )html";

  TooltipDemo() {
    Bind(custom_text);
    EnableHotReload();
  }
};

int main() {
  auto app = Ref<TooltipDemo>::New();
  Screen screen(app);
  screen.Loop();
  return 0;
}
