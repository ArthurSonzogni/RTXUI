// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
//
// Double-width text.
//
// CJK ideographs and emoji occupy two terminal cells. Layout measures text in
// cells, not code points, so alignment holds for mixed-width content.
#include <rtxui/rtxui.hpp>

using namespace rtxui;

class CJKDemo : public Component<CJKDemo> {
 public:
  std::string_view view = R"html(
      <div class="container">
        <h1>RTXUI CJK Character Demo</h1>
        <p>This demo showcases full-width CJK (Chinese, Japanese, Korean) characters taking exactly 2 terminal cell slots, aligning perfectly inside layout boxes.</p>

        <div class="card">
          <div class="title">Chinese (中文)</div>
          <div class="row">
            <div class="label">Text:</div>
            <div class="val">RTXUI 支持中文渲染</div>
          </div>
          <div class="row">
            <div class="label">Width check:</div>
            <div class="val">123456789012345678</div>
          </div>
        </div>

        <div class="card">
          <div class="title">Japanese (日本語)</div>
          <div class="row">
            <div class="label">Text:</div>
            <div class="val">ターミナルでの日本語表示</div>
          </div>
          <div class="row">
            <div class="label">Width check:</div>
            <div class="val">123456789012345678901234</div>
          </div>
        </div>

        <div class="card">
          <div class="title">Korean (한국어)</div>
          <div class="row">
            <div class="label">Text:</div>
            <div class="val">한국어 글꼴이 올바르게 렌더링됩니다.</div>
          </div>
          <div class="row">
            <div class="label">Width check:</div>
            <div class="val">123456789012345678901234567890123456</div>
          </div>
        </div>
      </div>

      <style>
        self {
          --bg: rgb(13, 17, 23);

          display: block;
          padding: 2;
          background-color: var(--bg);
          color: #eee;
          overflow-y: scroll;
        }
        .container {
          display: block;
          max-width: 80;
          margin: 0 auto;
        }
        h1 {
          color: #38bdf8;
        }
        p {
          margin-bottom: 1;
        }
        .card {
          display: block;
          margin-top: 1;
          padding: 1;
          border: tall;
          border-color: #3b82f6;
        }
        .title {
          display: block;
          color: #60a5fa;
          font-weight: bold;
          margin-bottom: 1;
        }
        .row {
          display: flex;
          width: 100%;
        }
        .label {
          width: 15;
          color: #94a3b8;
        }
        .val {
          flex-grow: 1;
          color: #e2e8f0;
        }
      </style>
    )html";
};

int main() {
  auto app = Ref<CJKDemo>::New();
  Screen screen(app);
  screen.Loop();
  return 0;
}
