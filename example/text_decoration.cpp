// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
//
// Text decoration: bold, dim, italic, underline and strikethrough.
#include <rtxui/rtxui.hpp>
#include <string_view>

using namespace rtxui;

class TextDecorationDemo : public Component<TextDecorationDemo> {
 public:
  std::string_view view = R"html(
      <div class="card">
        <h1>Text decoration</h1>
        <p>Terminal text attributes, set from CSS.</p>

        <div class="samples">
          <span class="name">font-weight: bold</span>
          <span class="bold">The quick brown fox</span>
          <span class="name">font-weight: lighter</span>
          <span class="lighter">The quick brown fox</span>
          <span class="name">font-style: italic</span>
          <span class="italic">The quick brown fox</span>
          <span class="name">underline</span>
          <span class="underline">The quick brown fox</span>
          <span class="name">double-underline</span>
          <span class="double-underline">The quick brown fox</span>
          <span class="name">overline</span>
          <span class="overline">The quick brown fox</span>
          <span class="name">line-through</span>
          <span class="line-through">The quick brown fox</span>
          <span class="name">blink</span>
          <span class="blink">The quick brown fox</span>
          <span class="name">combined</span>
          <span class="combined">The quick brown fox</span>
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
          width: 54;
        }
        h1 {
          color: var(--accent);
        }
        p {
          color: var(--muted);
        }
        .samples {
          display: grid;
          grid-template-columns: 24 auto;
        }
        .name {
          color: var(--muted);
        }
        .bold {
          font-weight: bold;
        }
        .lighter {
          font-weight: lighter;
        }
        .italic {
          font-style: italic;
        }
        .underline {
          text-decoration: underline;
        }
        .double-underline {
          text-decoration: double-underline;
        }
        .overline {
          text-decoration: overline;
        }
        .line-through {
          text-decoration: line-through;
        }
        .blink {
          text-decoration: blink;
        }
        .combined {
          font-weight: bold;
          font-style: italic;
          text-decoration: underline line-through;
          color: var(--accent);
        }
      </style>
    )html";
};

int main() {
  auto app = Ref<TextDecorationDemo>::New();
  Screen screen(app);
  screen.Loop();
  return 0;
}
