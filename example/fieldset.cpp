// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
//
// <fieldset> and <legend> grouping.
//
// The legend is nested into the top border of the group it captions.
#include <rtxui/rtxui.hpp>

using namespace rtxui;

class FieldsetDemo : public Component<FieldsetDemo> {
 public:
  std::string_view view = R"html(
      <div class="content">
        <fieldset>
          <legend>Personal Identification</legend>
          <p>Name: Arthur Sonzogni</p>
          <p>Role: Engineer & Creator</p>
        </fieldset>

        <fieldset>
          <legend>Project Metadata</legend>
          <p>Repository: RTXUI (Reactive Terminal User Interface)</p>
          <p>Language: C++23</p>
        </fieldset>
      </div>

      <style>
        self {

          display: block;
          padding: 1;
          background-color: rgb(13, 17, 23);
        }
        .content {
          display: flex;
          flex-direction: column;
          gap: 1;
        }
        p {
          margin-top: 1;
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
