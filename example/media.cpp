// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include <iostream>
#include <rtxui/rtxui.hpp>

using namespace rtxui;

class MediaQueriesDemo : public Component<MediaQueriesDemo> {
 public:
  std::string_view view = R"html(
    <div class="container">
      <h2 class="title">Responsive Media Queries</h2>
      <p class="desc">
        Resize your terminal to see the layout, border colors, and content adapt automatically!
      </p>

      <div class="card card-large">
        <h3>Desktop Mode</h3>
        <p>This panel is displayed when width >= 60 columns.</p>
        <span class="badge emerald">Wide Screen Layout</span>
      </div>

      <div class="card card-small">
        <h3>Mobile/Compact Mode</h3>
        <p>This panel is displayed when width &lt; 60 columns.</p>
        <span class="badge rose">Compact Layout</span>
      </div>

      <div class="compact-alert">
        ⚠️ Vertical space is very tight! (height &lt;= 15 rows)
      </div>
    </div>

    <style>
      self {
        display: block;
        padding: 1 2;
        background-color: rgb(15, 23, 42); /* Slate 900 */
        color: rgb(241, 245, 249);
      }
      .title {
        color: rgb(59, 130, 246); /* Blue 500 */
        margin-bottom: 0;
      }
      .desc {
        color: rgb(148, 163, 184); /* Slate 400 */
        margin-bottom: 1;
      }
      .badge {
        display: inline-block;
        padding: 0 1;
        text-align: center;
        width: 22;
        color: white;
      }
      .emerald {
        background-color: rgb(16, 185, 129);
      }
      .rose {
        background-color: rgb(239, 68, 68);
      }

      /* Desktop / Wide Screen rules */
      @media (min-width: 60) {
        .card-large {
          display: block;
          border: tall;
          border-color: rgb(16, 185, 129);
          padding: 1 2;
          width: 50;
        }
        .card-small {
          display: none;
        }
      }

      /* Mobile / Compact Width rules */
      @media (max-width: 59) {
        .card-large {
          display: none;
        }
        .card-small {
          display: block;
          border: tall;
          border-color: rgb(239, 68, 68);
          padding: 1 2;
          width: 35;
        }
      }

      /* Compact Height Rules */
      @media (min-height: 16) {
        .compact-alert {
          display: none;
        }
      }
      @media (max-height: 15) {
        .compact-alert {
          display: block;
          margin-top: 1;
          color: rgb(245, 158, 11);
          font-weight: bold;
        }
        self {
          padding: 0 1;
        }
        .desc {
          display: none;
        }
      }
    </style>
  )html";
};

int main() {
  auto app = Ref<MediaQueriesDemo>::New();
  Screen screen(app);
  screen.Loop();
  return 0;
}
