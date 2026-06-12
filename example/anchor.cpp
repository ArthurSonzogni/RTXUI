// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include <rtxui/rtxui.hpp>
#include <string>

using namespace rtxui;

class AnchorDemo : public Component<AnchorDemo> {
 public:
  std::string_view view = R"html(
    <div class="container">
      <div class="header">
        <h2>Anchor Navigation Demo</h2>
        <p class="description">
          Click the links in the sidebar to scroll the corresponding section into view.
        </p>
      </div>

      <div class="workspace">
        <!-- Sticky Sidebar Navigation -->
        <div class="sidebar">
          <div class="nav-title">SECTIONS</div>
          <a class="nav-link" href="#sec-intro">Introduction</a>
          <a class="nav-link" href="#sec-features">Features</a>
          <a class="nav-link" href="#sec-install">Installation</a>
          <a class="nav-link" href="#sec-usage">Usage</a>
          <a class="nav-link" href="#sec-components">Components</a>
          <a class="nav-link" href="#sec-docs">Documentation</a>
          <a class="nav-link" href="#sec-faq">FAQ</a>
          <a class="nav-link" href="#sec-contact">Contact</a>
        </div>

        <!-- Scrollable Content Pane -->
        <div class="scroll-window">
          <div id="sec-intro" class="section sec-odd">
            <div class="section-title">Introduction</div>
            <p>Welcome to RTXUI. This framework lets you build terminal user interfaces using familiar XML templates and CSS styles.</p>
            <p>Layout features include block, inline, flexbox, grid, fixed, absolute, and sticky positioning.</p>
          </div>

          <div id="sec-features" class="section sec-even">
            <div class="section-title">Features</div>
            <p>• Declarative XML markup parsing</p>
            <p>• Complete CSS layout and flexbox model</p>
            <p>• Rich borders, margins, padding, and z-index</p>
            <p>• Mouse support: hover, active, focus, and clicks</p>
          </div>

          <div id="sec-install" class="section sec-odd">
            <div class="section-title">Installation</div>
            <p>To use RTXUI in your CMake project, add the library using FetchContent:</p>
            <p>FetchContent_Declare(rtxui GIT_REPOSITORY ...)</p>
            <p>Then link it with target_link_libraries(your_target rtxui::rtxui).</p>
          </div>

          <div id="sec-usage" class="section sec-even">
            <div class="section-title">Usage</div>
            <p>Initialize a component class, define its view property with HTML/XML markup,</p>
            <p>implement InitReflection(), and start the main Screen loop.</p>
            <p>Bind C++ states to reactive template properties for dynamic UI updates.</p>
          </div>

          <div id="sec-components" class="section sec-odd">
            <div class="section-title">Components</div>
            <p>RTXUI supports custom reusable components. Standard built-in components</p>
            <p>include divs, spans, inputs, buttons, sliders, textareas, and checkboxes.</p>
            <p>Create nested hierarchies using standard XML slot definitions.</p>
          </div>

          <div id="sec-docs" class="section sec-even">
            <div class="section-title">Documentation</div>
            <p>Styles are resolved dynamically based on CSS selectors and active classes.</p>
            <p>Use the C++ API to bind state variables and handle interactive events.</p>
          </div>

          <div id="sec-faq" class="section sec-odd">
            <div class="section-title">FAQ</div>
            <p>Q: Does it support mouse inputs? Yes, hovering and clicking are fully supported.</p>
            <p>Q: Can I use CSS grid? Yes, grid-template-columns and grid-gap are available.</p>
            <p>Q: Does it have animations? Yes, CSS transitions are supported.</p>
          </div>

          <div id="sec-contact" class="section sec-even">
            <div class="section-title">Contact</div>
            <p>Created by Arthur Sonzogni.</p>
            <p>Licensed under the MIT License.</p>
          </div>
        </div>
      </div>
    </div>

    <style>
      self {
        display: block;
        padding: 1 2;
        background-color: rgb(15, 23, 42); /* Deep dark slate background */
        color: rgb(241, 245, 249);
      }
      .container {
        display: block;
        max-width: 80;
        width: 100%;
        margin-left: auto;
        margin-right: auto;
      }
      .header {
        display: block;
        margin-bottom: 1;
      }
      h2 {
        color: rgb(59, 130, 246); /* Bright blue */
        margin-bottom: 0;
      }
      .description {
        color: rgb(148, 163, 184); /* Muted gray text */
      }
      .workspace {
        display: flex;
        flex-direction: row;
        align-items: flex-start;
        gap: 2;
        width: 100%;
      }
      .sidebar {
        position: sticky;
        top: 0;
        display: flex;
        flex-direction: column;
        width: 18;
        border: tall;
        border-color: rgb(71, 85, 105);
        background-color: rgb(30, 41, 59, 0.4);
        padding: 1;
        flex-shrink: 0;
        z-index: 10;
      }
      .nav-title {
        color: rgb(148, 163, 184);
        font-weight: bold;
        margin-bottom: 1;
      }
      .nav-link {
        display: block;
        color: rgb(56, 189, 248); /* Cyan link */
        margin-bottom: 1;
        padding-left: 1;
        cursor: pointer;
      }
      .nav-link:hover {
        background-color: rgb(30, 41, 59, 0.8);
        color: white;
      }
      .scroll-window {
        display: block;
        height: 18;
        border: tall;
        border-color: rgb(71, 85, 105);
        overflow-y: scroll;
        scroll-speed: 1;
        scroll-behavior: smooth;
        flex-grow: 1;
      }
      .section {
        display: block;
        padding: 1 2;
        border-bottom: dashed;
        border-color: rgb(71, 85, 105);
        height: 10; /* Make each section tall so scrolling is required */
      }
      .section-title {
        font-weight: bold;
        margin-bottom: 1;
      }
      .sec-odd {
        background-color: rgba(30, 58, 138, 0.15); /* Deep slate royal blue */
      }
      .sec-even {
        background-color: rgba(14, 165, 233, 0.15); /* Modern cyan sky blue */
      }
      .sec-odd .section-title {
        color: rgb(96, 165, 250); /* Bright royal blue text */
      }
      .sec-even .section-title {
        color: rgb(56, 189, 248); /* Bright sky blue text */
      }
    </style>
  )html";
};

int main() {
  auto app = Ref<AnchorDemo>::New();
  Screen screen(app);
  screen.Loop();
  return 0;
}
