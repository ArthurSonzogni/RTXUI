#include <iostream>
#include <memory>
#include <rtxui/rtxui.hpp>
#include <rtxui/component/default_components_internal.hpp>
#include <string>
#include <vector>
#include "rtxui/core/task_runner.hpp"

using namespace rtxui;

struct Todo {
  std::string text;
  bool appearing = true;
  bool operator==(const Todo& other) const = default;
};

class Header : public Component<Header> {
 public:
  Header() = default;

  std::string_view view = R"html(
    <div class="header-row">
      <span class="header-title">RTXUI demo</span>
    </div>
    <style>
      self {
        position: fixed;
        top: 0;
        left: 0;
        right: 0;
        padding: 1;
        background-color: rgba(30, 59, 138, 0.75);
        color: #f8fafc;            /* Light gray text */
        border-bottom: hkey;
        border-color: #00c8ff;     /* Very bright blue bottom border */
        z-index: 50;
        display: block;
      }
      .header-row {
        display: flex;
        flex-direction: row;
        justify-content: space-between;
      }
      .header-title {
        font-weight: bold;
        color: #f8fafc;
      }
    </style>
  )html";
};

class Sidebar : public Component<Sidebar> {
 public:
  struct Props {
    int count = 0;
  } props;

  Sidebar() = default;

  void InitReflection() override {
    ComponentBase::InitReflection();
    Bind(props.count);
  }

  std::string_view view = R"html(
    <div class="sidebar">
      <div class="sidebar-title">NAV PANEL</div>
      <div class="nav-item active">Dashboard</div>
      <div class="nav-item">Components</div>
      <div class="nav-item">Layouts</div>
      
      <div class="sidebar-footer">
        <div>Clicks: {props.count}</div>
      </div>
    </div>
    <style>
      self {
        position: sticky;
        top: 4;
        width: 16;
        flex-shrink: 0;
        z-index: 10;
        display: block;
      }
      .sidebar {
        display: flex;
        flex-direction: column;
        width: 100%;
        height: 100%;
        border-right: tall;
        border-color: #334155;
        background-color: #0f172a;
        padding: 1;
      }
      .sidebar-title {
        display: block;
        color: #64748b;
        font-weight: bold;
        margin-bottom: 1;
      }
      .nav-item {
        display: block;
        padding-left: 1;
        margin-bottom: 1;
        color: #94a3b8;
        transition: background-color 0.15s ease-in-out, color 0.15s ease-in-out;
      }
      .nav-item:hover {
        background-color: #1e293b;
        color: #38bdf8;
        cursor: pointer;
      }
      .active {
        color: #0284c7;
        font-weight: bold;
        border-left: solid;
        border-color: #0284c7;
      }
      .sidebar-footer {
        margin-top: auto;
        border-top: dashed;
        border-color: #334155;
        padding-top: 1;
        color: #64748b;
      }
    </style>
  )html";
};

class SectionTransitions : public Component<SectionTransitions> {
 public:
  struct Props {
    int count = 0;
    bool show_secret = false;
  } props;

  SectionTransitions() = default;

  void InitReflection() override {
    ComponentBase::InitReflection();
    Bind(props.count);
    Bind(props.show_secret);
  }

  std::string_view view = R"html(
    <div class="card">
      <div class="card-title">1. CSS Transitions & States</div>
      <div class="row">
        <button class="btn-hover" onclick="Increment" oncontextmenu="Decrement">Clicks: {props.count}</button>
        <span class="desc">Hover to transition background-color.</span>
      </div>
      
      <div class="divider"></div>
      
      <div class="row">
        <checkbox :checked="props.show_secret" onchange="ToggleSecret">Toggle Secret CSS Panel</checkbox>
      </div>
      <div :if="props.show_secret" class="alert-box">
        ✨ Conditional display flow shifts tags dynamically.
      </div>
    </div>
    <style>
      .card {
        display: block;
        border: tall;
        border-color: #334155;
        padding: 1;
        margin-bottom: 1;
        background-color: #1e293b;
      }
      .card-title {
        color: #38bdf8;
        font-weight: bold;
        margin-bottom: 1;
      }
      .row {
        display: flex;
        flex-direction: row;
        gap: 1;
        margin-bottom: 1;
      }
      .desc {
        color: #94a3b8;
      }
      .divider {
        border-top: tall;
        border-color: #334155;
        margin: 1 0;
        height: 1;
      }
      .btn-hover {
        background-color: #1d4ed8;
        color: white;
        border: tall;
        border-color: #3b82f6;
        padding-left: 1;
        padding-right: 1;
        cursor: pointer;
        transition: background-color 0.2s ease-in-out, border-color 0.2s ease-in-out;
      }
      .btn-hover:hover {
        background-color: #2563eb;
        border-color: #60a5fa;
      }
      .btn-hover:active {
        background-color: #1e40af;
      }
      .alert-box {
        background-color: #065f46;
        color: #a7f3d0;
        border: tall;
        border-color: #059669;
        padding: 0 1;
      }
    </style>
  )html";
};

class SectionBoxModel : public Component<SectionBoxModel> {
 public:
  struct Props {
    int slider_val = 0;
  } props;

  SectionBoxModel() = default;

  void InitReflection() override {
    ComponentBase::InitReflection();
    Bind(props.slider_val);
  }

  std::string_view view = R"html(
    <div class="card">
      <div class="card-title">2. Box Model & Values</div>
      <div class="row">
        <span>Slider:  </span>
        <slider :value="props.slider_val" min="0" max="100" step="5" width="18" />
        <span class="badge">{props.slider_val}%</span>
      </div>
      <div class="row">
        <span>Progress:</span>
        <progress :value="props.slider_val" max="100" width="18" />
      </div>
    </div>
    <style>
      .card {
        display: block;
        border: tall;
        border-color: #334155;
        padding: 1;
        margin-bottom: 1;
        background-color: #1e293b;
      }
      .card-title {
        color: #38bdf8;
        font-weight: bold;
        margin-bottom: 1;
      }
      .row {
        display: flex;
        flex-direction: row;
        gap: 1;
        margin-bottom: 1;
      }
      .badge {
        color: #fbbf24;
        font-weight: bold;
      }
    </style>
  )html";
};

class SectionInputs : public Component<SectionInputs> {
 public:
  struct Props {
    std::string text_input = "";
    std::string textarea_input = "";
  } props;

  SectionInputs() = default;

  void InitReflection() override {
    ComponentBase::InitReflection();
    Bind(props.text_input);
    Bind(props.textarea_input);
  }

  std::string_view view = R"html(
    <div class="card">
      <div class="card-title">3. Text Form Fields</div>
      <div class="row">
        <span>Input:</span>
        <input :value="props.text_input" />
      </div>
      <div class="echo-text">
        <span>Echo: "{props.text_input}"</span>
      </div>
      <div class="row">
        <span>Notes:</span>
        <textarea :value="props.textarea_input" />
      </div>
    </div>
    <style>
      .card {
        display: block;
        border: tall;
        border-color: #334155;
        padding: 1;
        margin-bottom: 1;
        background-color: #1e293b;
      }
      .card-title {
        color: #38bdf8;
        font-weight: bold;
        margin-bottom: 1;
      }
      .row {
        display: flex;
        flex-direction: row;
        gap: 1;
        margin-bottom: 1;
      }
      input {
        padding-left: 1;
        padding-right: 1;
        background-color: #0f172a;
        color: white;
      }
      .echo-text {
        color: #64748b;
        margin-bottom: 1;
      }
      textarea {
        padding-left: 1;
        padding-right: 1;
        background-color: #0f172a;
        color: white;
        height: 3;
      }
    </style>
  )html";
};

class SectionColors : public Component<SectionColors> {
 public:
  SectionColors() = default;

  std::string_view view = R"html(
    <div class="card">
      <div class="card-title">4. CSS Color Gallery (4x6 Grid)</div>
      <!-- Row 1 -->
      <div class="color-row">
        <div class="color-block r1">Red1</div>
        <div class="color-block r2">Red2</div>
        <div class="color-block r3">Red3</div>
        <div class="color-block r4">Red4</div>
      </div>
      <!-- Row 2 -->
      <div class="color-row">
        <div class="color-block o1">Orng1</div>
        <div class="color-block o2">Orng2</div>
        <div class="color-block o3">Orng3</div>
        <div class="color-block o4">Orng4</div>
      </div>
      <!-- Row 3 -->
      <div class="color-row">
        <div class="color-block g1">Grn1</div>
        <div class="color-block g2">Grn2</div>
        <div class="color-block g3">Grn3</div>
        <div class="color-block g4">Grn4</div>
      </div>
      <!-- Row 4 -->
      <div class="color-row">
        <div class="color-block c1">Cyan1</div>
        <div class="color-block c2">Cyan2</div>
        <div class="color-block c3">Cyan3</div>
        <div class="color-block c4">Cyan4</div>
      </div>
      <!-- Row 5 -->
      <div class="color-row">
        <div class="color-block b1">Blue1</div>
        <div class="color-block b2">Blue2</div>
        <div class="color-block b3">Blue3</div>
        <div class="color-block b4">Blue4</div>
      </div>
      <!-- Row 6 -->
      <div class="color-row">
        <div class="color-block p1">Prpl1</div>
        <div class="color-block p2">Prpl2</div>
        <div class="color-block p3">Prpl3</div>
        <div class="color-block p4">Prpl4</div>
      </div>
    </div>
    <style>
      .card {
        display: block;
        border: tall;
        border-color: #334155;
        padding: 1;
        margin-bottom: 1;
        background-color: #1e293b;
      }
      .card-title {
        color: #38bdf8;
        font-weight: bold;
        margin-bottom: 1;
      }
      .color-row {
        display: flex;
        flex-direction: row;
        gap: 1;
        margin-bottom: 1;
      }
      .color-block {
        flex-grow: 1;
        margin: 0 1;
        padding: 1;
        text-align: center;
        border: tall;
        border-color: black;
      }
      /* Curated Palette: 6 Columns, 4 Rows */
      .r1 { background-color: #fecdd3; color: black; }
      .r2 { background-color: #fb7185; color: black; }
      .r3 { background-color: #e11d48; color: white; }
      .r4 { background-color: #9f1239; color: white; }
      
      .o1 { background-color: #fef08a; color: black; }
      .o2 { background-color: #fbbf24; color: black; }
      .o3 { background-color: #d97706; color: white; }
      .o4 { background-color: #7c2d12; color: white; }
      
      .g1 { background-color: #a7f3d0; color: black; }
      .g2 { background-color: #34d399; color: black; }
      .g3 { background-color: #059669; color: white; }
      .g4 { background-color: #064e3b; color: white; }
      
      .c1 { background-color: #cffafe; color: black; }
      .c2 { background-color: #22d3ee; color: black; }
      .c3 { background-color: #0891b2; color: white; }
      .c4 { background-color: #164e63; color: white; }
      
      .b1 { background-color: #e0e7ff; color: black; }
      .b2 { background-color: #818cf8; color: black; }
      .b3 { background-color: #4f46e5; color: white; }
      .b4 { background-color: #312e81; color: white; }
      
      .p1 { background-color: #fae8ff; color: black; }
      .p2 { background-color: #e879f9; color: black; }
      .p3 { background-color: #c084fc; color: white; }
      .p4 { background-color: #701a75; color: white; }
    </style>
  )html";
};

class SectionBorders : public Component<SectionBorders> {
 public:
  SectionBorders() = default;

  std::string_view view = R"html(
    <div class="card">
      <div class="card-title">5. Border Style Gallery (4x6 Grid)</div>
      <!-- Row 1 -->
      <div class="border-row">
        <div class="border-cell b-solid">Solid</div>
        <div class="border-cell b-double">Double</div>
        <div class="border-cell b-heavy">Heavy</div>
        <div class="border-cell b-rounded">Round</div>
      </div>
      <!-- Row 2 -->
      <div class="border-row">
        <div class="border-cell b-ascii">ASCII</div>
        <div class="border-cell b-shadow">Shadow</div>
        <div class="border-cell b-dashed">Dash</div>
        <div class="border-cell b-dotted">Dot</div>
      </div>
      <!-- Row 3 -->
      <div class="border-row">
        <div class="border-cell b-wide">Wide</div>
        <div class="border-cell b-tall">Tall</div>
        <div class="border-cell b-squiggle">Squig</div>
        <div class="border-cell b-panel">Panel</div>
      </div>
      <!-- Row 4 -->
      <div class="border-row">
        <div class="border-cell b-inner">Inner</div>
        <div class="border-cell b-outer">Outer</div>
        <div class="border-cell b-hkey">HKey</div>
        <div class="border-cell b-vkey">VKey</div>
      </div>
      <!-- Row 5 -->
      <div class="border-row">
        <div class="border-cell b-shade-l">ShdeL</div>
        <div class="border-cell b-shade-m">ShdeM</div>
        <div class="border-cell b-shade-d">ShdeD</div>
        <div class="border-cell b-dbl-h">DblH</div>
      </div>
      <!-- Row 6 -->
      <div class="border-row">
        <div class="border-cell b-dbl-v">DblV</div>
        <div class="border-cell b-blank">Blank</div>
        <div class="border-cell b-none">None</div>
        <div class="border-cell b-solid-red">SolidR</div>
      </div>
    </div>
    <style>
      .card {
        display: block;
        border: tall;
        border-color: #334155;
        padding: 1;
        margin-bottom: 1;
        background-color: #1e293b;
      }
      .card-title {
        color: #38bdf8;
        font-weight: bold;
        margin-bottom: 1;
      }
      .border-row {
        display: flex;
        flex-direction: row;
        gap: 1;
        margin-bottom: 1;
      }
      .border-cell {
        flex-grow: 1;
        text-align: center;
        margin: 0 1;
        padding: 1;
        border-color: #38bdf8;
      }
      /* 24 styles */
      .b-solid { border: solid; }
      .b-double { border: double; border-color: #fbbf24; }
      .b-heavy { border: heavy; border-color: #34d399; }
      .b-rounded { border: rounded; }
      .b-ascii { border: ascii; border-color: #a7f3d0; }
      .b-shadow { border: shadow; border-color: #f87171; }
      .b-dashed { border: dashed; }
      .b-dotted { border: dotted; }
      .b-wide { border: wide; }
      .b-tall { border: tall; }
      .b-squiggle { border: squiggle; }
      .b-panel { border: panel; }
      .b-inner { border: inner; }
      .b-outer { border: outer; }
      .b-hkey { border: hkey; }
      .b-vkey { border: vkey; }
      .b-shade-l { border: shade-light; }
      .b-shade-m { border: shade-medium; }
      .b-shade-d { border: shade-dark; }
      .b-dbl-h { border: double-horizontal; }
      .b-dbl-v { border: double-vertical; }
      .b-blank { border: blank; }
      .b-none { border: none; }
      .b-solid-red { border: solid; border-color: red; }
    </style>
  )html";
};

class SectionAlign : public Component<SectionAlign> {
 public:
  SectionAlign() = default;

  std::string_view view = R"html(
    <div class="card">
      <div class="card-title">6. Text Alignment Gallery</div>
      <div class="align-row">
        <div class="align-card align-l">Left</div>
        <div class="align-card align-c">Center</div>
        <div class="align-card align-r">Right</div>
      </div>
    </div>
    <style>
      .card {
        display: block;
        border: tall;
        border-color: #334155;
        padding: 1;
        margin-bottom: 1;
        background-color: #1e293b;
      }
      .card-title {
        color: #38bdf8;
        font-weight: bold;
        margin-bottom: 1;
      }
      .align-row {
        display: flex;
        flex-direction: row;
        gap: 1;
      }
      .align-card {
        flex-grow: 1;
        border: tall;
        border-color: #475569;
        height: 3;
      }
      .align-l { text-align: left; }
      .align-c { text-align: center; }
      .align-r { text-align: right; }
    </style>
  )html";
};

class SectionLoop : public Component<SectionLoop> {
 public:
  SectionLoop() = default;

  std::string_view view = R"html(
    <div class="card">
      <div class="card-title">7. Loop Array Layout</div>
      <div class="row">
        <button class="btn-add" onclick="AddTodo">Add Task</button>
      </div>
      <slot></slot>
    </div>
    <style>
      .card {
        display: block;
        border: tall;
        border-color: #334155;
        padding: 1;
        margin-bottom: 1;
        background-color: #1e293b;
      }
      .card-title {
        color: #38bdf8;
        font-weight: bold;
        margin-bottom: 1;
      }
      .row {
        display: flex;
        flex-direction: row;
        gap: 1;
        margin-bottom: 1;
      }
      .btn-add {
        background-color: #047857;
        color: white;
        border: tall;
        border-color: #10b981;
        padding-left: 1;
        padding-right: 1;
        cursor: pointer;
      }
    </style>
  )html";
};

class SectionMarkdown : public Component<SectionMarkdown> {
 public:
  struct Props {
    std::string markdown_content = "";
    std::string custom_css = R"css(
      h1 {
        color: #3b82f6;
        border-bottom: solid;
        border-color: #3b82f6;
        margin-bottom: 1;
      }

      h2 {
        color: #60a5fa;
        margin-top: 1;
        border-bottom: solid;
        border-color: #334155;
      }

      h3 {
        color: #93c5fd;
      }

      strong {
        color: #facc15;
      }

      em {
        color: #a78bfa;
      }

      code {
        color: #94a3b8;
      }

      pre {
        background-color: #1e293b;
        border: tall;
        border-color: #334155;
        display: block;
      }

      blockquote {
        border-left: heavy;
        border-color: #4b5563;
        padding-left: 2;
        font-style: italic;
        color: #9ca3af;
      }

      ul, ol {
        margin-left: 2;
        color: #d1d5db;
      }

      li {
        margin-bottom: 0;
      }

      a {
        color: #3b82f6;
        text-decoration: underline;
      }

      table {
        border: solid;
        border-color: #334155;
        margin-top: 1;
        margin-bottom: 1;
      }

      th {
        font-weight: bold;
        color: #60a5fa;
        border-bottom: solid;
        border-color: #334155;
        padding-left: 1;
        padding-right: 1;
      }

      td {
        padding-left: 1;
        padding-right: 1;
      }
    )css";
  } props;

  SectionMarkdown() = default;

  void InitReflection() override {
    ComponentBase::InitReflection();
    Bind(props.markdown_content);
    Bind(props.custom_css);
    Import<rtxui::markdown>();
  }

  std::string_view view = R"html(
    <div class="card">
      <div class="card-title">8. Markdown Live Editor</div>
      <div class="markdown-layout">
        <div class="markdown-editor-pane">
          <span class="sub-label">Editor (Source)</span>
          <textarea :value="props.markdown_content" class="md-editor-textarea" />
        </div>
        <div class="markdown-preview-pane">
          <span class="sub-label">Preview (Rendered)</span>
          <div class="md-preview-container">
            <markdown content="{props.markdown_content}" stylesheet="{props.custom_css}" />
          </div>
        </div>
      </div>
    </div>
    <style>
      .card {
        display: block;
        border: tall;
        border-color: #334155;
        padding: 1;
        margin-bottom: 1;
        background-color: #1e293b;
      }
      .card-title {
        color: #38bdf8;
        font-weight: bold;
        margin-bottom: 1;
      }
      .markdown-layout {
        display: flex;
        flex-direction: column;
        gap: 1;
        width: 100%;
      }
      .markdown-editor-pane {
        display: flex;
        flex-direction: column;
        flex-grow: 1;
        width: 100%;
      }
      .markdown-preview-pane {
        display: flex;
        flex-direction: column;
        flex-grow: 1;
        width: 100%;
        margin-top: 1;
      }
      .sub-label {
        color: #64748b;
        font-weight: bold;
        margin-bottom: 1;
      }
      .md-editor-textarea {
        padding: 1;
        background-color: #0f172a;
        color: white;
        height: 10;
        width: 100%;
      }
      .md-preview-container {
        border: tall;
        border-color: #475569;
        padding: 1;
        background-color: #0f172a;
        width: 100%;
        display: block;
      }
    </style>
  )html";
};

class Footer : public Component<Footer> {
 public:
  Footer() = default;

  std::string_view view = R"html(
    <div class="footer-row">
      <span class="footer-text">Fixed layout: sticky headers & footers frame the viewport</span>
    </div>
    <style>
      self {
        position: fixed;
        bottom: 0;
        left: 0;
        right: 0;
        padding: 1;
        background-color: rgba(30, 59, 138, 0.75);
        color: #94a3b8;            /* Grayish blue text */
        border-top: hkey;
        border-color: #00c8ff;     /* Very bright blue top border */
        z-index: 50;
        display: block;
      }
      .footer-row {
        display: flex;
        flex-direction: row;
        justify-content: space-between;
      }
      .footer-text {
        color: #94a3b8;
      }
    </style>
  )html";
};

class App : public Component<App> {
 public:
  // --- Reactive States ---
  int count = 0;
  int slider_val = 65;
  bool show_secret = false;
  std::string text_input = "Interactive CSS App";
  std::string textarea_input = "RTXUI styling engine\nsupports box models,\nflexbox, & transitions.";
  std::vector<Todo> todos = {
    {.text = "Style dashboard", .appearing = false},
    {.text = "Expose CSS layout", .appearing = false}
  };

  std::string markdown_content = R"md(# Heading 1
## Heading 2
### Heading 3

This is a paragraph demonstrating native **Markdown** support in RTXUI. You can write *italic* or **bold** text, or even ***both***.

Here is a [link to the RTXUI repository](https://github.com/ArthurSonzogni/RTXUI).

### Lists & Blockquotes

- Unordered list item 1
- Unordered list item 2
  - Nested list item

1. Ordered list item 1
2. Ordered list item 2

> "Blockquotes are styled with a heavy left border and italicized gray text, giving them a premium quote look."

### Code Blocks

You can write inline `code` or write complete fenced code blocks:

```cpp
#include <rtxui/rtxui.hpp>

int main() {
  auto app = Ref<App>::New();
  Screen screen(app);
  screen.Loop();
  return 0;
}
```

### Tables

| Metric | Baseline | Optimized | Difference |
| --- | --- | --- | --- |
| DOM Digest | 11.70 ms | 0.53 ms | -95.4% |
| Layout & Paint | 3.85 ms | 0.52 ms | -86.5% |
)md";

  // --- Handlers ---
  void Increment() { count++; }
  void Decrement() { count--; }
  void ToggleSecret() { show_secret = !show_secret; }
  
  void AddTodo() {
    size_t index = todos.size();
    todos.push_back({
        .text = "Task #" + std::to_string(index + 1),
        .appearing = true,
    });
    task::TaskRunner::Current()->PostTask([this, index]() {
      if (index < todos.size()) {
        todos[index].appearing = false;
        Digest();
      }
    });
  }

  void RemoveTodo(std::string index_str) {
    size_t idx = std::stoull(index_str);
    if (idx < todos.size()) {
      todos.erase(todos.begin() + idx);
    }
  }

  void InitReflection() override {
    ComponentBase::InitReflection();
    Bind(count);
    Bind(slider_val);
    Bind(show_secret);
    Bind(text_input);
    Bind(textarea_input);
    BindCollection("todos", &todos, [](const Todo& item) {
      return std::make_shared<ManualStructVisitor>(
          std::unordered_map<std::string, std::string>{
              {"text", item.text},
              {"appearing_class", item.appearing ? "appearing" : ""}});
    });
    Bind(markdown_content);

    Bind(Increment);
    Bind(Decrement);
    Bind(ToggleSecret);
    Bind(AddTodo);
    Import("RemoveTodo", [this](std::string index_str) {
      RemoveTodo(index_str);
    });

    Import<Header>();
    Import<Sidebar>();
    Import<SectionTransitions>();
    Import<SectionBoxModel>();
    Import<SectionInputs>();
    Import<SectionColors>();
    Import<SectionBorders>();
    Import<SectionAlign>();
    Import<SectionLoop>();
    Import<SectionMarkdown>();
    Import<Footer>();
  }

  std::string_view view = R"html(
      <div class="container">
        <div class="root">
          <!-- Sticky Header Bar -->
          <Header />

          <!-- Main Workspace -->
          <div class="workspace">
            <!-- Left Sidebar Menu -->
            <Sidebar props.count="{count}" />

            <!-- Main Scroll Area -->
            <div class="main-scroll">
              <SectionTransitions props.count="{count}" props.show_secret="{show_secret}" />
              <SectionBoxModel props.slider_val="{slider_val}" />
              <SectionInputs props.text_input="{text_input}" props.textarea_input="{textarea_input}" />
              <SectionColors />
              <SectionBorders />
              <SectionAlign />
              
              <SectionLoop>
                <div class="todo-list">
                  <for each="{todos}" as="todo">
                    <div class="todo-item {todo.appearing_class}">
                      <span>[{$index}] {todo.text}</span>
                      <button class="btn-delete" @click="RemoveTodo({$index})">Delete</button>
                    </div>
                  </for>
                </div>
              </SectionLoop>

              <SectionMarkdown props.markdown_content="{markdown_content}" />

              <div class="footer-msg">
                End of Dashboard - Scroll Up
              </div>
            </div>
          </div>
        </div>
      </div>
      
      <!-- Fixed Footer Bar -->
      <Footer />
      
      <style>
        .container {
          display: block;
          max-width: 120;
          width: 100%;
          margin-left: auto;
          margin-right: auto;
          padding-top: 4;    /* Space for fixed header (height 4) */
          padding-bottom: 4; /* Space for fixed footer (height 4) */
        }
        self {
          display: block;
          width: 100%;
          height: 100%;
          color: #e2e8f0;
          background-color: #0f172a;
          overflow-y: scroll;
        }
        .root {
          display: flex;
          flex-direction: column;
          width: 100%;
        }
        .workspace {
          display: flex;
          flex-direction: row;
          flex-grow: 1;
          width: 100%;
        }
        .main-scroll {
          flex-grow: 1;
          width: 0;
          padding: 1;
          display: block;
        }
        .footer-msg {
          text-align: center;
          color: #64748b;
          margin-top: 1;
          margin-bottom: 1;
        }
        .todo-list {
          display: block;
        }
        .todo-item {
          display: flex;
          flex-direction: row;
          justify-content: space-between;
          border-bottom: dashed;
          border-color: #334155;
          padding: 0 1;
          margin-bottom: 1;
          opacity: 1.0;
          transition: opacity 0.4s ease-in-out;
        }
        .todo-item.appearing {
          opacity: 0.0;
        }
        .btn-delete {
          background-color: #9f1239;
          color: white;
          border: tall;
          border-color: #f43f5e;
          padding-left: 1;
          padding-right: 1;
          cursor: pointer;
        }
      </style>
  )html";
};

#ifndef RTXUI_BENCHMARK
int main() {
  auto app = Ref<App>::New();
  Screen screen(app);
  screen.Loop();
  return 0;
}
#endif

