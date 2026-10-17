// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
//
// Double-width text.
//
// CJK ideographs and emoji occupy two terminal cells. Layout measures text in
// cells, not code points, so alignment holds for mixed-width content.
#include <rtxui/rtxui.hpp>
#include <string_view>

using namespace rtxui;

class CJKDemo : public Component<CJKDemo> {
 public:
  std::string_view view = R"html(
      <div class="card">
        <h1>Double-width text</h1>
        <p>CJK ideographs take two cells each. Layout counts cells, so the columns stay aligned.</p>

        <div class="table">
          <span class="lang">English</span>
          <span>Terminal user interface</span>
          <span class="lang">Chinese</span>
          <span>终端用户界面</span>
          <span class="lang">Japanese</span>
          <span>ターミナルの画面</span>
          <span class="lang">Korean</span>
          <span>터미널 사용자 인터페이스</span>
          <span class="lang">Mixed</span>
          <span>RTXUI 支持中文 and 日本語</span>
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
          height: 100%;
          background-color: rgb(13, 17, 23);
          color: rgb(230, 237, 243);
        }
        .card {
          border: tall;
          border-color: var(--border);
          background-color: rgb(22, 27, 34);
          padding: 1 3;
          width: 56;
        }
        h1 {
          color: var(--accent);
        }
        p {
          color: var(--muted);
        }
        .table {
          display: grid;
          grid-template-columns: 10 auto;
          border: round;
          border-color: var(--border);
          padding: 0 1;
        }
        .lang {
          color: var(--muted);
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
