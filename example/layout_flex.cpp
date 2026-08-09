// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
//
// An interactive flexbox playground.
//
// Every flex property is driven from the UI: change flex-direction,
// justify-content, align-items, and each item's grow/shrink/basis, and watch the
// boxes redistribute.
#include <rtxui/rtxui.hpp>
#include <string>
#include <vector>

using namespace rtxui;

struct DemoBox {
  std::string label;
  std::string color;
  int width;
  int height;
  bool operator==(const DemoBox&) const = default;
};

class LayoutFlexDemo : public Component<LayoutFlexDemo> {
 public:
  std::string direction = "row";
  std::string wrap = "wrap";
  std::string justify = "flex-start";
  std::string align = "stretch";
  std::string gap = "0";

  std::string child_grow = "0";
  std::string child_shrink = "1";
  std::string child_basis = "auto";

  int container_width = 54;
  int container_height = 16;

  int item_counter = 0;
  std::vector<DemoBox> items;

  void CycleDirection() {
    if (direction == "row") {
      direction = "row-reverse";
    } else if (direction == "row-reverse") {
      direction = "column";
    } else if (direction == "column") {
      direction = "column-reverse";
    } else {
      direction = "row";
    }
  }

  void CycleWrap() {
    if (wrap == "nowrap") {
      wrap = "wrap";
    } else if (wrap == "wrap") {
      wrap = "wrap-reverse";
    } else {
      wrap = "nowrap";
    }
  }

  void CycleJustify() {
    if (justify == "flex-start") {
      justify = "center";
    } else if (justify == "center") {
      justify = "flex-end";
    } else if (justify == "flex-end") {
      justify = "space-between";
    } else if (justify == "space-between") {
      justify = "space-around";
    } else if (justify == "space-around") {
      justify = "space-evenly";
    } else {
      justify = "flex-start";
    }
  }

  void CycleAlign() {
    if (align == "flex-start") {
      align = "center";
    } else if (align == "center") {
      align = "flex-end";
    } else if (align == "flex-end") {
      align = "stretch";
    } else {
      align = "flex-start";
    }
  }

  void CycleChildGrow() {
    if (child_grow == "0") {
      child_grow = "1";
    } else if (child_grow == "1") {
      child_grow = "2";
    } else {
      child_grow = "0";
    }
  }

  void CycleChildShrink() {
    if (child_shrink == "1") {
      child_shrink = "0";
    } else if (child_shrink == "0") {
      child_shrink = "2";
    } else {
      child_shrink = "1";
    }
  }

  void CycleChildBasis() {
    if (child_basis == "auto") {
      child_basis = "0";
    } else if (child_basis == "0") {
      child_basis = "10";
    } else {
      child_basis = "auto";
    }
  }

  void CycleGap() {
    if (gap == "0") {
      gap = "1";
    } else if (gap == "1") {
      gap = "2";
    } else if (gap == "2") {
      gap = "3";
    } else if (gap == "3") {
      gap = "0 2";
    } else if (gap == "0 2") {
      gap = "2 0";
    } else if (gap == "2 0") {
      gap = "1 2";
    } else {
      gap = "0";
    }
  }

  void AddItem() {
    item_counter++;
    std::string colors[] = {
        "var(--danger)",  "var(--accent)", "var(--success)",
        "rgb(245, 158, 11)", "rgb(139, 92, 246)",
    };
    std::string color = colors[(item_counter - 1) % 5];
    int width = 10 + (item_counter % 3) * 2;
    int height = 4 + (item_counter % 2);
    items.push_back({std::to_string(item_counter), color, width, height});
  }

  void RemoveItem() {
    if (!items.empty()) {
      items.pop_back();
    }
  }

  std::string container_css() const {
    return "display: flex;\n\n"
           "flex-direction: " +
           direction +
           ";\n\n"
           "flex-wrap: " +
           wrap +
           ";\n\n"
           "justify-content: " +
           justify +
           ";\n\n"
           "align-items: " +
           align +
           ";\n\n"
           "gap: " +
           gap + ";";
  }

  std::string children_css() const {
    return "flex-grow: " + child_grow +
           ";\n\n"
           "flex-shrink: " +
           child_shrink +
           ";\n\n"
           "flex-basis: " +
           child_basis + ";";
  }

  std::string_view view = R"html(
      <div class="app-container">
        <div class="header">
          <span class="title">RTXUI Flexbox Layout Demo</span>
          <span class="desc">Toggle properties and add/remove elements to test wrapping and display rules.</span>
        </div>

        <div class="content-split">
          <!-- Controls Panel -->
          <div class="sidebar">
            <span class="section-title">Container Controls</span>
            
            <div class="control-row">
              <button onclick="CycleDirection">direction: {direction}</button>
            </div>
            
            <div class="control-row">
              <button onclick="CycleWrap">wrap: {wrap}</button>
            </div>

            <div class="control-row">
              <button onclick="CycleJustify">justify: {justify}</button>
            </div>

            <div class="control-row">
              <button onclick="CycleAlign">align: {align}</button>
            </div>

            <div class="control-row">
              <button onclick="CycleGap">gap: {gap}</button>
            </div>

            <div class="control-row divider"></div>

            <span class="section-title">Children Controls</span>

            <div class="control-row">
              <button onclick="CycleChildGrow">grow: {child_grow}</button>
            </div>

            <div class="control-row">
              <button onclick="CycleChildShrink">shrink: {child_shrink}</button>
            </div>

            <div class="control-row">
              <button onclick="CycleChildBasis">basis: {child_basis}</button>
            </div>

            <div class="control-row divider"></div>

            <div class="control-row action-row">
              <button class="btn-add" onclick="AddItem">Add Box</button>
              <button class="btn-remove" onclick="RemoveItem">Remove Box</button>
            </div>
          </div>

          <!-- Preview Area (Center) -->
          <div class="preview-area">
            <span class="section-title">Live Preview (Container: {container_width} x {container_height})</span>
            
            <div class="slider-row">
              <span class="slider-label">Width:</span>
              <slider value="{container_width}" min="20" max="80" width="30" />
            </div>

            <div class="slider-row">
              <span class="slider-label">Height:</span>
              <slider value="{container_height}" min="5" max="30" width="30" />
            </div>

            <div class="flex-container">
              <for each="{items}" as="b">
                <div class="demo-box" style="background-color: {b.color}; width: {b.width}; height: {b.height}; border-color: {b.color};">
                  <span>Box {b.label}</span>
                  <span class="box-dims">{b.width}x{b.height}</span>
                </div>
              </for>
            </div>
          </div>

          <!-- CSS Display (Right Column) -->
          <div class="css-panel">
            <span class="section-title">Container CSS</span>
            <pre class="css-display">{container_css}</pre>

            <span class="section-title">Children CSS</span>
            <pre class="css-display">{children_css}</pre>
          </div>
        </div>
      </div>

      <style>
        self {
          --bg: rgb(13, 17, 23);
          --surface: rgb(22, 27, 34);
          --border: rgb(48, 54, 61);
          --text: rgb(230, 237, 243);
          --muted: rgb(139, 148, 158);
          --accent: rgb(88, 166, 255);
          --accent-bright: rgb(121, 192, 255);
          --danger: rgb(248, 81, 73);
          --success: rgb(63, 185, 80);

          display: block;
          padding: 1;
          background-color: var(--bg);
          color: white;
          width: 100%;
          height: 100%;
        }
        .app-container {
          display: block;
        }
        .header {
          display: block;
          margin-bottom: 1;
        }
        .title {
          display: block;
          font-weight: bold;
          color: rgb(56, 189, 248);
        }
        .desc {
          display: block;
          color: var(--muted);
        }
        .content-split {
          display: flex;
          flex-direction: row;
          gap: 2;
        }
        .sidebar {
          display: block;
          width: 32;
          flex-shrink: 0;
          border-right: solid;
          border-color: var(--border);
          padding-right: 2;
          overflow-y: auto;
        }
        .preview-area {
          display: block;
          flex-grow: 1;
          background-color: var(--surface);
          padding: 1;
          border: solid;
          border-color: var(--border);
          overflow: hidden;
        }
        .css-panel {
          display: block;
          width: 32;
          flex-shrink: 0;
          border-left: solid;
          border-color: var(--border);
          padding-left: 2;
          overflow-y: auto;
        }
        .section-title {
          display: block;
          font-weight: bold;
          color: rgb(244, 63, 94);
          margin-bottom: 1;
        }
        .control-row {
          display: block;
          margin-bottom: 1;
        }
        .divider {
          border-bottom: dashed;
          border-color: var(--border);
          margin-top: 1;
          margin-bottom: 1;
        }
        .action-row {
          display: flex;
          gap: 2;
        }
        button {
          width: 100%;
          border: tall;
          border-color: var(--border);
          color: var(--text);
          background-color: transparent;
          padding-left: 1;
          padding-right: 1;
          text-align: left;
        }
        button:hover {
          background-color: var(--surface);
          border-color: var(--accent-bright);
          color: white;
        }
        .btn-add {
          border-color: var(--success);
          color: var(--success);
        }
        .btn-add:hover {
          background-color: rgb(6, 95, 70);
          color: white;
        }
        .btn-remove {
          border-color: var(--danger);
          color: var(--danger);
        }
        .btn-remove:hover {
          background-color: rgb(153, 27, 27);
          color: white;
        }
        .slider-row {
          display: flex;
          align-items: center;
          margin-bottom: 1;
          gap: 2;
        }
        .slider-label {
          display: inline;
          width: 8;
          color: var(--muted);
        }
        .css-display {
          display: block;
          font-family: monospace;
          color: rgb(253, 224, 71);
          background-color: rgb(15, 23, 42);
          padding: 1;
          border: solid;
          border-color: var(--border);
          margin-bottom: 1;
        }
        .flex-container {
          display: flex;
          width: {container_width};
          height: {container_height};
          border: double;
          border-color: rgb(56, 189, 248);
          background-color: rgb(2, 6, 23);
          gap: {gap};
          flex-direction: {direction};
          flex-wrap: {wrap};
          justify-content: {justify};
          align-items: {align};
          overflow: hidden;
        }
        .demo-box {
          display: flex;
          flex-direction: column;
          align-items: center;
          justify-content: center;
          border: solid;
          color: white;
          font-weight: bold;
          flex-grow: {child_grow};
          flex-shrink: {child_shrink};
          flex-basis: {child_basis};
        }
        .box-dims {
          font-weight: normal;
          color: rgba(255, 255, 255, 0.7);
        }
      </style>
    )html";

  LayoutFlexDemo() {
    Bind(direction);
    Bind(wrap);
    Bind(justify);
    Bind(align);
    Bind(gap);
    Bind(child_grow);
    Bind(child_shrink);
    Bind(child_basis);
    Bind(container_width);
    Bind(container_height);
    Bind(items, [this](const DemoBox& b) {
      bool should_stretch_h =
          (align == "stretch" &&
           (direction == "row" || direction == "row-reverse"));
      bool should_stretch_w =
          (align == "stretch" &&
           (direction == "column" || direction == "column-reverse"));
      return std::make_shared<ManualStructVisitor>(
          std::map<std::string, std::string, std::less<>>{
              {"label", b.label},
              {"color", b.color},
              {"width", should_stretch_w ? "auto" : std::to_string(b.width)},
              {"height",
               should_stretch_h ? "auto" : std::to_string(b.height)}});
    });
    Bind(CycleDirection);
    Bind(CycleWrap);
    Bind(CycleJustify);
    Bind(CycleAlign);
    Bind(CycleGap);
    Bind(CycleChildGrow);
    Bind(CycleChildShrink);
    Bind(CycleChildBasis);
    Bind(AddItem);
    Bind(RemoveItem);
    Bind(container_css);
    Bind(children_css);

    // Initialize with 4 boxes to immediately demonstrate layout wrapping
    AddItem();
    AddItem();
    AddItem();
    AddItem();
  }
};

int main() {
  auto app = Ref<LayoutFlexDemo>::New();
  Screen screen(app);
  screen.Loop();
  return 0;
}
