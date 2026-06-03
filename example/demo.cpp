#include <iostream>
#include <memory>
#include <rtxui/rtxui.hpp>
#include <rtxui/component/default_components_internal.hpp>
#include <string>
#include <vector>

using namespace rtxui;

class App : public Component<App> {
 public:
  // --- Reactive States ---
  int count = 0;
  int slider_val = 65;
  bool show_secret = false;
  std::string text_input = "Interactive CSS App";
  std::string textarea_input = "RTXUI styling engine\nsupports box models,\nflexbox, & transitions.";
  std::vector<std::string> todos = {"Style dashboard", "Expose CSS layout"};

  std::string markdown_content = R"md(
# Markdown Live Preview

You can edit this **Markdown** text and see the results rendered in real-time!

- **Bold text** and *italic text*
- [RTXUI Homepage](https://github.com/ArthurSonzogni/RTXUI)
- Inline `code` elements
- Unordered list items

> "Real-time rendering brings documents to life."
)md";

  // --- Handlers ---
  void Increment() { count++; }
  void Decrement() { count--; }
  void ToggleSecret() { show_secret = !show_secret; }
  
  void AddTodo() {
    todos.push_back("Task #" + std::to_string(todos.size() + 1));
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
    Bind(todos);
    Bind(markdown_content);

    Import<rtxui::markdown>();

    Bind(Increment);
    Bind(Decrement);
    Bind(ToggleSecret);
    Bind(AddTodo);
    Import("RemoveTodo", [this](std::string index_str) {
      RemoveTodo(index_str);
    });
  }

  std::string_view view = R"html(
      <div class="container">
        <div class="root">
        <!-- Sticky Header Bar -->
        <div class="header">
          <div class="header-row">
            <span class="header-title">RTXUI demo</span>
          </div>
        </div>

        <!-- Main Workspace -->
        <div class="workspace">
          <!-- Left Sidebar Menu -->
          <div class="sidebar">
            <div class="sidebar-title">NAV PANEL</div>
            <div class="nav-item active">Dashboard</div>
            <div class="nav-item">Components</div>
            <div class="nav-item">Layouts</div>
            
            <div class="sidebar-footer">
              <div>Clicks: {count}</div>
            </div>
          </div>

          <!-- Main Scroll Area -->
          <div class="main-scroll">
            
            <!-- Section 1: Transitions & Pseudo-Classes -->
            <div class="card">
              <div class="card-title">1. CSS Transitions & States</div>
              <div class="row">
                <button class="btn-hover" onclick="Increment" oncontextmenu="Decrement">Clicks: {count}</button>
                <span class="desc">Hover to transition background-color.</span>
              </div>
              
              <div class="divider"></div>
              
              <div class="row">
                <checkbox :checked="show_secret" onchange="ToggleSecret">Toggle Secret CSS Panel</checkbox>
              </div>
              <div :if="show_secret" class="alert-box">
                ✨ Conditional display flow shifts tags dynamically.
              </div>
            </div>

            <!-- Section 2: Box Model & Range Sliders -->
            <div class="card">
              <div class="card-title">2. Box Model & Values</div>
              <div class="row">
                <span>Slider:  </span>
                <slider :value="slider_val" min="0" max="100" step="5" width="18" />
                <span class="badge">{slider_val}%</span>
              </div>
              <div class="row">
                <span>Progress:</span>
                <progress :value="slider_val" max="100" width="18" />
              </div>
            </div>

            <!-- Section 3: Keyboard Text Inputs -->
            <div class="card">
              <div class="card-title">3. Text Form Fields</div>
              <div class="row">
                <span>Input:</span>
                <input :value="text_input" />
              </div>
              <div class="echo-text">
                <span>Echo: "{text_input}"</span>
              </div>
              <div class="row">
                <span>Notes:</span>
                <textarea :value="textarea_input" />
              </div>
            </div>

            <!-- Section 4: Color Gallery -->
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

            <!-- Section 5: Border Style Gallery -->
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

            <!-- Section 6: Text-Align Gallery -->
            <div class="card">
              <div class="card-title">6. Text Alignment Gallery</div>
              <div class="align-row">
                <div class="align-card align-l">Left</div>
                <div class="align-card align-c">Center</div>
                <div class="align-card align-r">Right</div>
              </div>
            </div>

            <!-- Section 7: Dynamic loop rendering -->
            <div class="card">
              <div class="card-title">7. Loop Array Layout</div>
              <div class="row">
                <button class="btn-add" onclick="AddTodo">Add Task</button>
              </div>
              <div class="todo-list">
                <for each="{todos}" as="todo">
                  <div class="todo-item">
                    <span>[{$index}] {todo}</span>
                    <button class="btn-delete" @click="RemoveTodo({$index})">Delete</button>
                  </div>
                </for>
              </div>
            </div>

            <!-- Section 8: Markdown Live Editor -->
            <div class="card">
              <div class="card-title">8. Markdown Live Editor</div>
              <div class="markdown-layout">
                <div class="markdown-editor-pane">
                  <span class="sub-label">Editor (Source)</span>
                  <textarea :value="markdown_content" class="md-editor-textarea" />
                </div>
                <div class="markdown-preview-pane">
                  <span class="sub-label">Preview (Rendered)</span>
                  <div class="md-preview-container">
                    <markdown content="{markdown_content}" />
                  </div>
                </div>
              </div>
            </div>

            <div class="footer-msg">
              End of Dashboard - Scroll Up
            </div>
          </div>
        </div>

      </div>
      
      <!-- Fixed Footer Bar -->
      <div class="footer-bar">
        <div class="footer-row">
          <span class="footer-text">Fixed layout: sticky headers & footers frame the viewport</span>
        </div>
      </div>
    </div>

      <style>
        .container {
          display: block;
          max-width: 80;
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
        .header {
          position: fixed;
          top: 0;
          left: 0;
          right: 0;
          padding: 1;
          background-color: rgba(30, 59, 138, 0.75);
          color: #f8fafc;            /* Light gray text */
          border-bottom: tall;
          border-color: #3b82f6;     /* Vibrant blue bottom border */
          z-index: 50;
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
        .footer-bar {
          position: fixed;
          bottom: 0;
          left: 0;
          right: 0;
          padding: 1;
          background-color: rgba(30, 59, 138, 0.75);
          color: #94a3b8;            /* Grayish blue text */
          border-top: tall;
          border-color: #3b82f6;     /* Vibrant blue top border */
          z-index: 50;
        }
        .footer-row {
          display: flex;
          flex-direction: row;
          justify-content: space-between;
        }
        .footer-text {
          color: #94a3b8;
        }
        .footer-keys {
          color: #f8fafc;
          font-weight: bold;
        }
        .workspace {
          display: flex;
          flex-direction: row;
          flex-grow: 1;
          width: 100%;
        }
        .sidebar {
          display: flex;
          flex-direction: column;
          width: 24;
          border-right: tall;
          border-color: #334155;
          background-color: #0f172a;
          padding: 1;
          flex-shrink: 0;
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
        .main-scroll {
          flex-grow: 1;
          width: 0;
          padding: 1;
          display: block;
        }
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
        .badge {
          color: #fbbf24;
          font-weight: bold;
        }
        input {
          border: tall;
          border-color: #475569;
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
          border: tall;
          border-color: #475569;
          padding-left: 1;
          padding-right: 1;
          background-color: #0f172a;
          color: white;
          height: 3;
        }
        .border-style-grid {
          display: flex;
          flex-direction: column;
          width: 100%;
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
        .btn-add {
          background-color: #047857;
          color: white;
          border: tall;
          border-color: #10b981;
          padding-left: 1;
          padding-right: 1;
          cursor: pointer;
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

        .markdown-layout {
          display: flex;
          flex-direction: row;
          gap: 2;
          width: 100%;
        }
        .markdown-editor-pane {
          display: flex;
          flex-direction: column;
          flex-grow: 1;
          width: 0;
        }
        .markdown-preview-pane {
          display: flex;
          flex-direction: column;
          flex-grow: 1;
          width: 0;
        }
        .sub-label {
          color: #64748b;
          font-weight: bold;
          margin-bottom: 1;
        }
        .md-editor-textarea {
          border: tall;
          border-color: #475569;
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
          height: 10;
          width: 100%;
          overflow-y: scroll;
        }

        .footer-msg {
          text-align: center;
          color: #64748b;
          margin-top: 1;
          margin-bottom: 1;
        }
      </style>
    )html";
};

int main() {
  auto app = Ref<App>::New();
  Screen screen(app);
  screen.Loop();
  return 0;
}
