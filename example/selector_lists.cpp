// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
//
// Selector lists: :is() and :where().
//
// `:is(.info, .ok, .warn, .error)` writes one rule for four kinds of badge.
// `:where()` matches the same way but weighs nothing, so the defaults it sets
// lose to any other rule: `.custom` overrides `:where(.badge)` although it
// comes first in the stylesheet, where it would otherwise lose the tie.
//
// Try it: hover the badges and Tab between them.
#include <rtxui/rtxui.hpp>
#include <string_view>

using namespace rtxui;

class SelectorListsDemo : public Component<SelectorListsDemo> {
 public:
  std::string_view view = R"html(
      <div class="card">
        <h1>Selector lists</h1>
        <p>One :is() rule styles all four badges.</p>

        <div class="row">
          <span class="badge info" tabindex="0">info</span>
          <span class="badge ok" tabindex="0">ok</span>
          <span class="badge warn" tabindex="0">warn</span>
          <span class="badge error" tabindex="0">error</span>
        </div>

        <p>:where() sets defaults any rule overrides.</p>

        <div class="row">
          <span class="badge">theme default</span>
          <span class="badge custom">overridden by .custom</span>
        </div>
      </div>

      <style>
        self {
          --bg: rgb(13, 17, 23);
          --muted: rgb(139, 148, 158);
          --text: rgb(230, 237, 243);

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
          border-color: rgb(48, 54, 61);
          background-color: rgb(22, 27, 34);
          padding: 1 3;
        }
        h1 {
          color: rgb(88, 166, 255);
          font-weight: bold;
        }
        p {
          color: var(--muted);
        }
        .row {
          display: flex;
          gap: 2;
          margin-bottom: 1;
        }

        /* Specificity (0,1,0): beats the :where() rule below, which comes
           later but weighs nothing. */
        .custom {
          color: rgb(210, 168, 255);
        }

        /* Defaults: weight (0,0,0), whatever is inside :where(). */
        :where(.badge) {
          border: rounded;
          border-color: currentColor;
          color: var(--muted);
          padding: 0 1;
        }

        /* One rule for every kind of badge. */
        .badge:is(.info, .ok, .warn, .error) {
          font-weight: bold;
          transition: color 0.2s ease;
        }
        .info {
          color: rgb(88, 166, 255);
        }
        .ok {
          color: rgb(63, 185, 80);
        }
        .warn {
          color: rgb(210, 153, 34);
        }
        .error {
          color: rgb(248, 81, 73);
        }

        .badge:is(.info, .ok, .warn, .error):is(:hover, :focus) {
          color: var(--text);
        }
      </style>
    )html";
};

int main() {
  auto app = Ref<SelectorListsDemo>::New();
  Screen screen(app);
  screen.Loop();
  return 0;
}
