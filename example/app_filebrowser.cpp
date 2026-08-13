// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
//
// A complete application driven by the keyboard.
//
// Where app_dashboard.cpp is about composing layout and state, this one is
// about the two things a terminal UI lives or dies by: scrolling a list that
// is longer than the viewport, and moving a selection through it without a
// mouse. Rows carry `tabindex`, so RTXUI's built-in focus navigation walks
// them and scrolls the focused one into view; the preview pane re-renders
// from computed values as the selection moves.
//
// Try it: move with the arrow keys (or j/k via the buttons), Enter to open a
// directory, Backspace to go up. The list scrolls to follow the selection.
#include <rtxui/rtxui.hpp>

#include <string>
#include <vector>

using namespace rtxui;

namespace {

struct FileEntry {
  std::string name;
  std::string kind;  // "dir" | "file"
  std::string size;
  std::string modified;
  bool selected = false;

  bool operator==(const FileEntry& other) const = default;
};

// A small fixed tree, so the example stays self-contained and deterministic
// rather than depending on whatever happens to be on disk.
struct Directory {
  std::string path;
  std::vector<FileEntry> entries;
};

const std::vector<Directory>& Tree() {
  static const std::vector<Directory> tree = {
      {"/",
       {
           {"src", "dir", "-", "2026-08-01"},
           {"docs", "dir", "-", "2026-07-28"},
           {"example", "dir", "-", "2026-08-09"},
           {"CMakeLists.txt", "file", "14 kB", "2026-08-09"},
           {"README.md", "file", "6.2 kB", "2026-08-09"},
           {"LICENSE", "file", "1.1 kB", "2026-01-12"},
       }},
      {"/src",
       {
           {"rtxui", "dir", "-", "2026-08-09"},
           {"dummy.cpp", "file", "42 B", "2026-03-04"},
       }},
      {"/docs",
       {
           {"guide", "dir", "-", "2026-08-09"},
           {"index.md", "file", "2.4 kB", "2026-08-09"},
           {"css_reference.md", "file", "31 kB", "2026-08-09"},
           {"html_reference.md", "file", "18 kB", "2026-08-09"},
           {"reactivity.md", "file", "7.8 kB", "2026-08-09"},
       }},
      {"/example",
       {
           {"app_dashboard.cpp", "file", "9.1 kB", "2026-08-09"},
           {"app_filebrowser.cpp", "file", "8.4 kB", "2026-08-09"},
           {"counter.cpp", "file", "2.8 kB", "2026-08-09"},
           {"helloworld.cpp", "file", "1.4 kB", "2026-08-09"},
           {"playground.cpp", "file", "7.0 kB", "2026-08-09"},
           {"transitions.cpp", "file", "2.6 kB", "2026-08-09"},
           {"grid.cpp", "file", "2.2 kB", "2026-08-09"},
           {"lists.cpp", "file", "3.9 kB", "2026-08-09"},
           {"table.cpp", "file", "6.1 kB", "2026-08-09"},
           {"tooltip.cpp", "file", "4.3 kB", "2026-08-09"},
           {"sticky.cpp", "file", "4.6 kB", "2026-08-09"},
           {"markdown.cpp", "file", "6.8 kB", "2026-08-09"},
       }},
  };
  return tree;
}

}  // namespace

class FileBrowser : public Component<FileBrowser> {
 public:
  std::string path = "/";
  int cursor = 0;

  std::vector<FileEntry> entries = Tree().front().entries;

  // --- Computed values -----------------------------------------------------

  std::string current_path() const { return path; }
  std::string entry_count() const {
    return std::to_string(entries.size()) + " items";
  }

  std::string selected_name() const {
    const FileEntry* entry = Selected();
    return entry ? entry->name : "-";
  }
  std::string selected_kind() const {
    const FileEntry* entry = Selected();
    return entry ? (entry->kind == "dir" ? "Directory" : "File") : "-";
  }
  std::string selected_size() const {
    const FileEntry* entry = Selected();
    return entry ? entry->size : "-";
  }
  std::string selected_modified() const {
    const FileEntry* entry = Selected();
    return entry ? entry->modified : "-";
  }
  bool selected_is_dir() const {
    const FileEntry* entry = Selected();
    return entry && entry->kind == "dir";
  }
  bool can_go_up() const { return path != "/"; }

  // --- Handlers ------------------------------------------------------------

  void MoveDown() { SetCursor(cursor + 1); }
  void MoveUp() { SetCursor(cursor - 1); }

  void Open() {
    const FileEntry* entry = Selected();
    if (!entry || entry->kind != "dir") {
      return;
    }
    std::string next = path == "/" ? "/" + entry->name : path + "/" + entry->name;
    Navigate(next);
  }

  void GoUp() {
    if (path == "/") {
      return;
    }
    size_t slash = path.find_last_of('/');
    Navigate(slash == 0 ? "/" : path.substr(0, slash));
  }

  std::string_view view = R"html(
      <div class="app">
        <div class="header">
          <span class="brand">FILES</span>
          <span class="path">{current_path}</span>
          <span class="spacer"></span>
          <span class="count">{entry_count}</span>
        </div>

        <div class="body">
          <div class="listing" tabindex="0">
            <for each="{entries}" as="entry">
              <div class="row {entry.row_class}" tabindex="0" onclick="Select({$index})">
                <span class="glyph {entry.kind}">{entry.glyph}</span>
                <span class="name">{entry.name}</span>
                <span class="size">{entry.size}</span>
                <span class="modified">{entry.modified}</span>
              </div>
            </for>
          </div>

          <div class="preview">
            <span class="preview-name">{selected_name}</span>
            <span class="preview-kind">{selected_kind}</span>

            <div class="kv"><span class="k">Size</span><span class="v">{selected_size}</span></div>
            <div class="kv"><span class="k">Modified</span><span class="v">{selected_modified}</span></div>

            <if condition="{selected_is_dir}">
              <button class="primary" onclick="Open">Open directory</button>
            </if>
            <if condition="{can_go_up}">
              <button onclick="GoUp">Go up</button>
            </if>
          </div>
        </div>

        <div class="footer">
          <button class="key" onclick="MoveUp">k / up</button>
          <button class="key" onclick="MoveDown">j / down</button>
          <span class="spacer"></span>
          <span class="hint">the list scrolls to keep the selection visible</span>
        </div>
      </div>

      <style>
        self {
          --bg: rgb(13, 17, 23);
          --surface: rgb(22, 27, 34);
          --raised: rgb(31, 38, 47);
          --border: rgb(48, 54, 61);
          --text: rgb(230, 237, 243);
          --muted: rgb(139, 148, 158);
          --accent: rgb(88, 166, 255);

          display: block;
          width: 100%;
          height: 100%;
          background-color: var(--bg);
          color: var(--text);
        }
        .app {
          display: flex;
          flex-direction: column;
          height: 100%;
        }

        .header {
          display: flex;
          align-items: center;
          gap: 2;
          background-color: var(--surface);
          border-bottom: solid;
          border-color: var(--border);
          padding: 0 2;
        }
        .brand {
          color: var(--accent);
          font-weight: bold;
        }
        .path {
          color: var(--text);
        }
        .count {
          color: var(--muted);
        }
        .spacer {
          flex-grow: 1;
        }

        .body {
          display: flex;
          flex-grow: 1;
          gap: 1;
          padding: 1 2;
        }

        /* The list is deliberately shorter than its content: scroll-behavior
           smooth plus the browser-style scroll-into-view on focus is what
           keeps the selected row visible as the cursor moves. */
        .listing {
          display: flex;
          flex-direction: column;
          flex-grow: 1;
          border: tall;
          border-color: var(--border);
          background-color: var(--surface);
          padding: 0 1;
          overflow-y: scroll;
          scroll-behavior: smooth;
        }
        .row {
          display: flex;
          gap: 1;
          align-items: center;
          padding: 0 1;
          transition: background-color 0.15s ease;
        }
        .row:hover {
          background-color: var(--raised);
        }
        .row.selected, .row:focus {
          background-color: var(--raised);
          border-left: tall;
          border-color: var(--accent);
        }
        .glyph.dir {
          color: rgb(210, 153, 34);
        }
        .glyph.file {
          color: var(--muted);
        }
        .name {
          flex-grow: 1;
        }
        .size {
          width: 10;
          color: var(--muted);
          text-align: right;
        }
        .modified {
          width: 12;
          color: var(--muted);
          text-align: right;
        }

        .preview {
          display: flex;
          flex-direction: column;
          border: tall;
          border-color: var(--border);
          background-color: var(--surface);
          padding: 1 2;
          width: 30;
        }
        .preview-name {
          color: var(--accent);
          font-weight: bold;
        }
        .preview-kind {
          color: var(--muted);
          margin-bottom: 1;
        }
        .kv {
          display: flex;
          justify-content: space-between;
          width: 100%;
        }
        .k { color: var(--muted); }
        .v { font-weight: bold; }

        button {
          border: tall;
          border-color: var(--border);
          background-color: var(--bg);
          color: var(--text);
          padding: 0 1;
          margin-top: 1;
          text-align: center;
          transition: background-color 0.15s ease, border-color 0.15s ease,
                      color 0.15s ease;
        }
        button:hover, button:focus {
          border-color: var(--accent);
          color: var(--accent);
        }
        button:active {
          background-color: var(--accent);
          color: var(--bg);
        }
        button.primary {
          border-color: var(--accent);
          color: var(--accent);
        }

        .footer {
          display: flex;
          align-items: center;
          gap: 2;
          border-top: solid;
          border-color: var(--border);
          background-color: var(--surface);
          color: var(--muted);
          padding: 0 2;
        }
        .footer button.key {
          margin-top: 0;
        }
        .hint {
          color: var(--border);
        }
      </style>
    )html";

  FileBrowser() {
    BindCollection("entries", &entries, [](const FileEntry& entry) {
      return std::make_shared<ManualStructVisitor>(
          std::map<std::string, std::string, std::less<>>{
              {"name", entry.name},
              {"kind", entry.kind},
              {"size", entry.size},
              {"modified", entry.modified},
              {"glyph", entry.kind == "dir" ? "/" : "."},
              {"row_class", entry.selected ? "selected" : ""},
          });
    });

    Bind(path);
    Bind(cursor);
    Bind(current_path);
    Bind(entry_count);
    Bind(selected_name);
    Bind(selected_kind);
    Bind(selected_size);
    Bind(selected_modified);
    Bind(selected_is_dir);
    Bind(can_go_up);
    Bind(MoveDown);
    Bind(MoveUp);
    Bind(Open);
    Bind(GoUp);

    Import("Select", [this](std::string index_str) {
      SetCursor(std::stoi(index_str));
    });

    SetCursor(0);
  }

  // Arrow keys drive the cursor; Enter opens; Backspace goes up. Returning
  // true marks the event handled so it doesn't also reach the focus system.
  bool OnEvent(Event event) override {
    if (!event.is<Event::Keyboard>()) {
      return Component<FileBrowser>::OnEvent(event);
    }
    auto key = event.get<Event::Keyboard>();
    if (key.motion != Event::Keyboard::Motion::Pressed &&
        key.motion != Event::Keyboard::Motion::Repeat) {
      return Component<FileBrowser>::OnEvent(event);
    }

    switch (key.special) {
      case Event::Keyboard::Special::ArrowDown:
        MoveDown();
        return false;  // let focus move too, so the list scrolls to follow
      case Event::Keyboard::Special::ArrowUp:
        MoveUp();
        return false;
      case Event::Keyboard::Special::Return:
        Open();
        return true;
      case Event::Keyboard::Special::Backspace:
        GoUp();
        return true;
      default:
        break;
    }
    if (key.codepoint == 'j') {
      MoveDown();
      return true;
    }
    if (key.codepoint == 'k') {
      MoveUp();
      return true;
    }
    return Component<FileBrowser>::OnEvent(event);
  }

 private:
  const FileEntry* Selected() const {
    if (cursor < 0 || cursor >= static_cast<int>(entries.size())) {
      return nullptr;
    }
    return &entries[cursor];
  }

  void SetCursor(int index) {
    if (index < 0 || index >= static_cast<int>(entries.size())) {
      return;
    }
    for (FileEntry& entry : entries) {
      entry.selected = false;
    }
    entries[index].selected = true;
    cursor = index;
  }

  void Navigate(const std::string& next) {
    for (const Directory& directory : Tree()) {
      if (directory.path == next) {
        path = next;
        entries = directory.entries;
        SetCursor(0);
        return;
      }
    }
  }
};

int main() {
  auto app = Ref<FileBrowser>::New();
  Screen screen(app);
  screen.Loop();
  return 0;
}
