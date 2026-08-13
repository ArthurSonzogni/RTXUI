// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
//
// The smallest possible RTXUI application.
//
// A component is a class deriving from Component<Derived> whose `view` member
// holds an HTML template plus a <style> block. `self` selects the component's
// own root element, and custom properties declared there (--bg, --accent, ...)
// inherit into every descendant, so a single palette styles the whole tree.
#include <rtxui/rtxui.hpp>

using namespace rtxui;

class HelloWorldApp : public Component<HelloWorldApp> {
 public:
  std::string_view view = R"html(
      <div class="card">
        <h1>Hello, RTXUI</h1>
        <p>A reactive terminal UI, styled with CSS.</p>
      </div>

      <style>
        self {

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
          border-color: rgb(48, 54, 61);
          background-color: rgb(22, 27, 34);
          padding: 1 3;
          text-align: center;
        }
        h1 {
          color: rgb(88, 166, 255);
          font-weight: bold;
        }
        p {
          color: rgb(139, 148, 158);
          margin-top: 1;
        }
      </style>
    )html";
};

int main() {
  auto app = Ref<HelloWorldApp>::New();
  Screen screen(app);
  screen.Loop();
  return 0;
}
