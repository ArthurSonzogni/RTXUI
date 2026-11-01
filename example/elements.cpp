// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
//
// Every built-in element, one page each.
//
// Each page is a small component. Beside it is that component's template,
// shown as it is: what you read on the right is what draws on the left. The
// look of the controls comes from one theme, kTheme, as an application would
// have.
//
// Try it: pick an element on the left, or press Page Down and Page Up. Use
// each one, with the mouse or with Tab and the arrows.
#include <algorithm>
#include <cstddef>
#include <rtxui/rtxui.hpp>
#include <string>
#include <string_view>
#include <vector>

using namespace rtxui;

namespace {

// `text` without the indentation all its lines share, nor its blank first
// and last lines: a template as it reads in the source.
std::string Dedent(std::string_view text) {
  std::vector<std::string_view> lines;
  size_t start = 0;
  while (start <= text.size()) {
    const size_t end = std::min(text.find('\n', start), text.size());
    lines.push_back(text.substr(start, end - start));
    start = end + 1;
  }
  const auto blank = [](std::string_view line) {
    return line.find_first_not_of(' ') == std::string_view::npos;
  };
  while (!lines.empty() && blank(lines.front())) {
    lines.erase(lines.begin());
  }
  while (!lines.empty() && blank(lines.back())) {
    lines.pop_back();
  }
  size_t indent = std::string_view::npos;
  for (std::string_view line : lines) {
    if (!blank(line)) {
      indent = std::min(indent, line.find_first_not_of(' '));
    }
  }
  std::string out;
  for (std::string_view line : lines) {
    out += blank(line) ? "" : line.substr(indent);
    out += '\n';
  }
  return out;
}

// The look every page's controls share. A component's styles apply to its
// own template only, so each page adds this stylesheet to its markup.
constexpr std::string_view kTheme = R"css(
  <style>
    p {
      color: var(--muted);
      margin: 1 0 0 0;
    }
    .row {
      display: flex;
      gap: 2;
      align-items: center;
    }
    .column {
      display: flex;
      flex-direction: column;
    }
    .value {
      color: var(--accent);
      font-weight: bold;
    }
    button {
      border: tall;
      border-color: var(--border);
      background-color: var(--bg);
      color: var(--text);
      padding: 0 1;
      flex-shrink: 0;
      opacity: 1;
      transition: border-color 150ms, color 150ms;
    }
    button:hover, button:focus {
      background-color: var(--bg);
      border-color: var(--accent);
      color: var(--accent);
    }
    button.primary {
      background-color: var(--accent);
      border-color: var(--accent);
      color: var(--bg);
    }
    button:disabled {
      opacity: 0.4;
    }
    label {
      display: block;
      color: var(--muted);
    }
    input, textarea {
      width: 32;
      background-color: var(--bg);
      border: tall;
      border-color: var(--border);
      opacity: 1;
      margin-bottom: 1;
    }
    input:focus, textarea:focus {
      border-color: var(--accent);
    }
    textarea {
      width: 100%;
      height: 8;
    }
    checkbox, radio {
      padding: 0 1;
    }
    checkbox:focus, radio:focus {
      color: var(--accent);
    }
    checkbox::part(checkmark), radio::part(radio-mark) {
      color: var(--accent);
    }
    select::part(select-btn) {
      border: tall;
      border-color: var(--border);
      background-color: var(--bg);
    }
    select:focus::part(select-btn) {
      border-color: var(--accent);
    }
    slider::part(track-left), progress::part(filled) {
      color: var(--accent);
    }
    slider::part(thumb) {
      color: var(--text);
    }
    slider::part(track-right), progress::part(empty) {
      color: var(--border);
    }
    fieldset {
      border-color: var(--border);
      margin-bottom: 1;
    }
    legend {
      color: var(--accent);
    }
    .clickable {
      display: flex;
      gap: 1;
    }
    details {
      margin-bottom: 1;
    }
    summary {
      color: var(--text);
    }
    tree-item::part(arrow) {
      color: var(--muted);
    }
    dialog::part(dialog-box) {
      border: tall;
      border-color: var(--danger);
      background-color: var(--raised);
    }
    toast::part(toast) {
      border-color: var(--success);
      background-color: var(--raised);
    }
    toast.error::part(toast) {
      border-color: var(--danger);
    }
    h1 {
      color: var(--accent);
      margin: 0;
    }
    h2 {
      color: var(--text);
    }
    code {
      color: var(--code);
      background-color: var(--raised);
    }
    kbd {
      color: var(--text);
      background-color: var(--raised);
    }
    blockquote {
      border-color: var(--accent);
      color: var(--muted);
      margin-bottom: 1;
    }
    hr {
      border-color: var(--border);
    }
    pre {
      color: var(--success);
    }
    th {
      color: var(--accent);
      text-align: left;
      padding-right: 3;
    }
    td {
      padding-right: 3;
    }
  </style>
)css";

// A page's template: its markup, shown beside it, then the theme.
template <typename Page>
std::string_view Themed() {
  static const std::string view =
      std::string(Page::kView) + std::string(kTheme);
  return view;
}

}  // namespace

// --- Forms -----------------------------------------------------------------

class ButtonPage : public Component<ButtonPage> {
 public:
  std::string_view Setup() { return Themed<ButtonPage>(); }
  static constexpr std::string_view kView = R"html(
    <div class="row">
      <button onclick="Add">Save</button>
      <button class="primary">Deploy</button>
      <button disabled="true">Disabled</button>
    </div>
    <p>Clicked {clicks} times.</p>
  )html";
  int clicks = 0;
  void Add() { clicks++; }
  ButtonPage() {
    Bind(clicks);
    Bind(Add);
  }
};

class InputPage : public Component<InputPage> {
 public:
  std::string_view Setup() { return Themed<InputPage>(); }
  static constexpr std::string_view kView = R"html(
    <label>Name</label>
    <input value="{name}" placeholder="Ada" />
    <label>Password</label>
    <input type="password" value="{password}" />
    <p>Hello, {greeting}.</p>
  )html";
  std::string name;
  std::string password;
  std::string greeting() const { return name.empty() ? "stranger" : name; }
  InputPage() {
    Bind(name);
    Bind(password);
    Bind(greeting);
  }
};

class TextareaPage : public Component<TextareaPage> {
 public:
  std::string_view Setup() { return Themed<TextareaPage>(); }
  static constexpr std::string_view kView = R"html(
    <textarea value="{text}" linenumbers="true"
              highlight_current_line="true" />
  )html";
  std::string text =
      "Terminal user interfaces,\nwritten in HTML and CSS,\nbound to C++.";
  TextareaPage() { Bind(text); }
};

class CheckboxPage : public Component<CheckboxPage> {
 public:
  std::string_view Setup() { return Themed<CheckboxPage>(); }
  static constexpr std::string_view kView = R"html(
    <div class="column">
      <checkbox checked="{wifi}">Wi-Fi</checkbox>
      <checkbox checked="{bt}">Bluetooth</checkbox>
      <checkbox disabled="true">Locked</checkbox>
    </div>
    <p>Wi-Fi {wifi}, Bluetooth {bt}.</p>
  )html";
  bool wifi = true;
  bool bt = false;
  CheckboxPage() {
    Bind(wifi);
    Bind(bt);
  }
};

class RadioPage : public Component<RadioPage> {
 public:
  std::string_view Setup() { return Themed<RadioPage>(); }
  static constexpr std::string_view kView = R"html(
    <div class="column">
      <radio name="plan" checked="{free}">Free</radio>
      <radio name="plan" checked="{pro}">Pro</radio>
      <radio name="plan" checked="{team}">Team</radio>
    </div>
    <p>One radio per name is checked at a time.</p>
  )html";
  bool free = false;
  bool pro = true;
  bool team = false;
  RadioPage() {
    Bind(free);
    Bind(pro);
    Bind(team);
  }
};

class SelectPage : public Component<SelectPage> {
 public:
  std::string_view Setup() { return Themed<SelectPage>(); }
  static constexpr std::string_view kView = R"html(
    <select value="{theme}">
      <option value="midnight">Midnight</option>
      <option value="daylight">Daylight</option>
      <option value="solarized">Solarized</option>
    </select>
    <p>Theme: {theme}</p>
  )html";
  std::string theme = "midnight";
  SelectPage() { Bind(theme); }
};

class SliderPage : public Component<SliderPage> {
 public:
  std::string_view Setup() { return Themed<SliderPage>(); }
  static constexpr std::string_view kView = R"html(
    <div class="row">
      <slider value="{volume}" step="5" width="24" />
      <span class="value">{volume}%</span>
    </div>
  )html";
  int volume = 60;
  SliderPage() { Bind(volume); }
};

class ProgressPage : public Component<ProgressPage> {
 public:
  std::string_view Setup() { return Themed<ProgressPage>(); }
  static constexpr std::string_view kView = R"html(
    <div class="row">
      <progress value="{done}" width="24" />
      <span class="value">{done}%</span>
    </div>
    <div class="row">
      <button onclick="Step">Step</button>
      <button onclick="Reset">Reset</button>
    </div>
  )html";
  int done = 40;
  void Step() { done = std::min(100, done + 15); }
  void Reset() { done = 0; }
  ProgressPage() {
    Bind(done);
    Bind(Step);
    Bind(Reset);
  }
};

class FieldsetPage : public Component<FieldsetPage> {
 public:
  std::string_view Setup() { return Themed<FieldsetPage>(); }
  static constexpr std::string_view kView = R"html(
    <fieldset>
      <legend>Notifications</legend>
      <checkbox checked="{email}">Email</checkbox>
      <checkbox checked="{push}">Push</checkbox>
    </fieldset>
    <label class="clickable">
      Clicking this label toggles
      <checkbox checked="{sound}">Sound</checkbox>
    </label>
  )html";
  bool email = true;
  bool push = false;
  bool sound = true;
  FieldsetPage() {
    Bind(email);
    Bind(push);
    Bind(sound);
  }
};

// --- Disclosure and overlays ------------------------------------------------

class DetailsPage : public Component<DetailsPage> {
 public:
  std::string_view Setup() { return Themed<DetailsPage>(); }
  static constexpr std::string_view kView = R"html(
    <details open="true">
      <summary>What is RTXUI?</summary>
      <p>A browser engine for the terminal.</p>
    </details>
    <details>
      <summary>Does it need a browser?</summary>
      <p>No. It draws characters.</p>
    </details>
  )html";
};

class TabsPage : public Component<TabsPage> {
 public:
  std::string_view Setup() { return Themed<TabsPage>(); }
  static constexpr std::string_view kView = R"html(
    <tabs value="{tab}">
      <tab-pane name="overview" label="Overview">
        <p>14 processes running.</p>
      </tab-pane>
      <tab-pane name="logs" label="Logs">
        <p>09:41 Server started.</p>
      </tab-pane>
      <tab-pane name="settings" label="Settings">
        <p>Nothing to set yet.</p>
      </tab-pane>
    </tabs>
  )html";
  std::string tab = "overview";
  TabsPage() { Bind(tab); }
};

class TreePage : public Component<TreePage> {
 public:
  std::string_view Setup() { return Themed<TreePage>(); }
  static constexpr std::string_view kView = R"html(
    <tree-item label="src" open="true">
      <tree-item label="main.cpp" />
      <tree-item label="ui" open="true">
        <tree-item label="app.cpp" />
        <tree-item label="app.hpp" />
      </tree-item>
    </tree-item>
    <tree-item label="tests">
      <tree-item label="app_test.cpp" />
    </tree-item>
    <tree-item label="README.md" />
  )html";
};

class TooltipPage : public Component<TooltipPage> {
 public:
  std::string_view Setup() { return Themed<TooltipPage>(); }
  static constexpr std::string_view kView = R"html(
    <div class="row">
      <tooltip content="Copy" placement="bottom">
        <button>Copy</button>
      </tooltip>
      <tooltip content="Trash it" placement="right">
        <button>Delete</button>
      </tooltip>
    </div>
    <p>Hover a button.</p>
  )html";
};

class DialogPage : public Component<DialogPage> {
 public:
  std::string_view Setup() { return Themed<DialogPage>(); }
  static constexpr std::string_view kView = R"html(
    <button onclick="Ask">Delete the branch</button>
    <p>Answer: {answer}</p>
    <dialog open="{asking}" title="Delete it?">
      <p>Its commits are on no other branch.</p>
      <div class="row">
        <button onclick="Cancel">Cancel</button>
        <button onclick="Confirm">Delete</button>
      </div>
    </dialog>
  )html";
  bool asking = false;
  std::string answer = "none yet";
  void Ask() { asking = true; }
  void Cancel() {
    asking = false;
    answer = "kept";
  }
  void Confirm() {
    asking = false;
    answer = "deleted";
  }
  DialogPage() {
    Bind(asking);
    Bind(answer);
    Bind(Ask);
    Bind(Cancel);
    Bind(Confirm);
  }
};

class ToastPage : public Component<ToastPage> {
 public:
  std::string_view Setup() { return Themed<ToastPage>(); }
  static constexpr std::string_view kView = R"html(
    <div class="row">
      <button onclick="Save">Save</button>
      <button onclick="Fail">Build</button>
    </div>
    <toast open="{saved}" placement="top-right">
      ✔ Saved
    </toast>
    <toast class="error" open="{failed}">
      ✘ Build failed
    </toast>
  )html";
  bool saved = false;
  bool failed = false;
  void Save() { saved = true; }
  void Fail() { failed = true; }
  ToastPage() {
    Bind(saved);
    Bind(failed);
    Bind(Save);
    Bind(Fail);
  }
};

// --- Content ----------------------------------------------------------------

class TextPage : public Component<TextPage> {
 public:
  std::string_view Setup() { return Themed<TextPage>(); }
  static constexpr std::string_view kView = R"html(
    <h1>Heading</h1>
    <h2>Subheading</h2>
    <p><b>Bold</b>, <i>italic</i>, <u>underline</u></p>
    <p><s>Strike</s>, <code>code</code>, <kbd>Ctrl C</kbd></p>
    <p>And <a href="https://github.com">links</a>.</p>
  )html";
};

class ListsPage : public Component<ListsPage> {
 public:
  std::string_view Setup() { return Themed<ListsPage>(); }
  static constexpr std::string_view kView = R"html(
    <ul>
      <li>Flexbox</li>
      <li>Grid
        <ul><li>Areas</li><li>Spans</li></ul>
      </li>
    </ul>
    <ol>
      <li>Write the template</li>
      <li>Bind the state</li>
    </ol>
  )html";
};

class BlocksPage : public Component<BlocksPage> {
 public:
  std::string_view Setup() { return Themed<BlocksPage>(); }
  static constexpr std::string_view kView = R"html(
    <blockquote>
      Simple things should be simple.
    </blockquote>
    <hr />
    <pre>
  $ cmake -B build
  $ ./build/app</pre>
  )html";
};

class TablePage : public Component<TablePage> {
 public:
  std::string_view Setup() { return Themed<TablePage>(); }
  static constexpr std::string_view kView = R"html(
    <table>
      <tr><th>Name</th><th>CPU</th><th>RAM</th></tr>
      <tr><td>clang</td><td>98%</td><td>1.2G</td></tr>
      <tr><td>vim</td><td>12%</td><td>3.1G</td></tr>
      <tr><td>nginx</td><td>6%</td><td>512M</td></tr>
    </table>
  )html";
};

class MarkdownPage : public Component<MarkdownPage> {
 public:
  std::string_view Setup() { return Themed<MarkdownPage>(); }
  static constexpr std::string_view kView = R"html(
    <markdown content="{text}" />
  )html";
  std::string text =
      "## Release notes\n"
      "\n"
      "- **Windows**: arm64 builds\n"
      "- *Faster* layout\n"
      "\n"
      "> Written in `Markdown`.";
  MarkdownPage() { Bind(text); }
};

// --- The gallery -------------------------------------------------------------

struct Entry {
  std::string group;  // Set on the first entry of each group.
  std::string tag;
  std::string description;
  std::string code;
};

class Elements : public Component<Elements> {
 public:
  std::vector<Entry> entries = {
      {"Forms", "<button>", "Runs a handler on click, Enter or Space.",
       Dedent(ButtonPage::kView)},
      {"", "<input>", "One line of text, bound to a std::string.",
       Dedent(InputPage::kView)},
      {"", "<textarea>", "Lines of text, with a gutter if you want one.",
       Dedent(TextareaPage::kView)},
      {"", "<checkbox>", "A bool, toggled by click or Space.",
       Dedent(CheckboxPage::kView)},
      {"", "<radio>", "One choice among those sharing a name.",
       Dedent(RadioPage::kView)},
      {"", "<select>", "One choice among options, in a dropdown.",
       Dedent(SelectPage::kView)},
      {"", "<slider>", "An int in a range: drag it, or use the arrows.",
       Dedent(SliderPage::kView)},
      {"", "<progress>", "How far along something is.",
       Dedent(ProgressPage::kView)},
      {"", "<fieldset>", "Groups controls under a legend. Labels act on them.",
       Dedent(FieldsetPage::kView)},
      {"Disclosure", "<details>", "Content that opens under its summary.",
       Dedent(DetailsPage::kView)},
      {"", "<tabs>", "Panes, one shown at a time.", Dedent(TabsPage::kView)},
      {"", "<tree-item>", "A tree: Right opens a branch, Left closes it.",
       Dedent(TreePage::kView)},
      {"", "<tooltip>", "A hint shown while the pointer is over it.",
       Dedent(TooltipPage::kView)},
      {"", "<dialog>", "A modal question. Escape closes it.",
       Dedent(DialogPage::kView)},
      {"", "<toast>", "A message that closes itself.",
       Dedent(ToastPage::kView)},
      {"Content", "<h1> <b> <code>", "Text, styled as on the web.",
       Dedent(TextPage::kView)},
      {"", "<ul> <ol>", "Lists, nested as deep as you need.",
       Dedent(ListsPage::kView)},
      {"", "<blockquote>", "Quotes, rules and preformatted text.",
       Dedent(BlocksPage::kView)},
      {"", "<table>", "Columns sized to their content.",
       Dedent(TablePage::kView)},
      {"", "<markdown>", "Markdown, rendered as elements.",
       Dedent(MarkdownPage::kView)},
  };
  int page = 0;

  std::string tag() const { return entries[page].tag; }
  std::string description() const { return entries[page].description; }
  std::string code() const { return entries[page].code; }
  std::string position() const {
    return std::to_string(page + 1) + "/" + std::to_string(entries.size());
  }
  bool on_button() const { return page == 0; }
  bool on_input() const { return page == 1; }
  bool on_textarea() const { return page == 2; }
  bool on_checkbox() const { return page == 3; }
  bool on_radio() const { return page == 4; }
  bool on_select() const { return page == 5; }
  bool on_slider() const { return page == 6; }
  bool on_progress() const { return page == 7; }
  bool on_fieldset() const { return page == 8; }
  bool on_details() const { return page == 9; }
  bool on_tabs() const { return page == 10; }
  bool on_tree() const { return page == 11; }
  bool on_tooltip() const { return page == 12; }
  bool on_dialog() const { return page == 13; }
  bool on_toast() const { return page == 14; }
  bool on_text() const { return page == 15; }
  bool on_lists() const { return page == 16; }
  bool on_blocks() const { return page == 17; }
  bool on_table() const { return page == 18; }

  void Go(const std::string& index) { page = std::stoi(index); }

  Elements() {
    Bind(entries, &Elements::EntryFields);
    Bind(page);
    Bind(tag);
    Bind(description);
    Bind(code);
    Bind(position);
    Bind(on_button);
    Bind(on_input);
    Bind(on_textarea);
    Bind(on_checkbox);
    Bind(on_radio);
    Bind(on_select);
    Bind(on_slider);
    Bind(on_progress);
    Bind(on_fieldset);
    Bind(on_details);
    Bind(on_tabs);
    Bind(on_tree);
    Bind(on_tooltip);
    Bind(on_dialog);
    Bind(on_toast);
    Bind(on_text);
    Bind(on_lists);
    Bind(on_blocks);
    Bind(on_table);
    Bind(Go);
    Import<ButtonPage>();
    Import<InputPage>();
    Import<TextareaPage>();
    Import<CheckboxPage>();
    Import<RadioPage>();
    Import<SelectPage>();
    Import<SliderPage>();
    Import<ProgressPage>();
    Import<FieldsetPage>();
    Import<DetailsPage>();
    Import<TabsPage>();
    Import<TreePage>();
    Import<TooltipPage>();
    Import<DialogPage>();
    Import<ToastPage>();
    Import<TextPage>();
    Import<ListsPage>();
    Import<BlocksPage>();
    Import<TablePage>();
    Import<MarkdownPage>();
    EnableHotReload();
  }

  // Page Down and Page Up turn the pages, whatever has the focus.
  bool OnEvent(Event event) override {
    const int last = static_cast<int>(entries.size()) - 1;
    if (event == Event::PageDown()) {
      page = std::min(last, page + 1);
      return true;
    }
    if (event == Event::PageUp()) {
      page = std::max(0, page - 1);
      return true;
    }
    return Component<Elements>::OnEvent(event);
  }

  void EntryFields(const Entry& entry, FieldSink& out) const {
    out("group", entry.group);
    out("has_group", !entry.group.empty());
    out("tag", entry.tag);
    out("class", &entry == &entries[page] ? "nav active" : "nav");
  }

  std::string_view view = R"html(
      <div class="app">
        <div class="sidebar">
          <div class="brand"><span class="logo">◆</span> Elements</div>
          <for each="{entries}" as="e">
            <div class="group" if="{e.has_group}">{e.group}</div>
            <div class="{e.class}" onclick="Go({$index})">{e.tag}</div>
          </for>
          <div class="grow"></div>
          <div class="hint"><kbd>PgDn</kbd> {position}</div>
        </div>

        <div class="main">
          <div class="header">
            <h1>{tag}</h1>
            <p class="description">{description}</p>
          </div>
          <div class="panels">
            <div class="panel preview">
              <div class="caption">Preview</div>
              <div class="stage">
                <if condition="{on_button}"><ButtonPage /></if>
                <elif condition="{on_input}"><InputPage /></elif>
                <elif condition="{on_textarea}"><TextareaPage /></elif>
                <elif condition="{on_checkbox}"><CheckboxPage /></elif>
                <elif condition="{on_radio}"><RadioPage /></elif>
                <elif condition="{on_select}"><SelectPage /></elif>
                <elif condition="{on_slider}"><SliderPage /></elif>
                <elif condition="{on_progress}"><ProgressPage /></elif>
                <elif condition="{on_fieldset}"><FieldsetPage /></elif>
                <elif condition="{on_details}"><DetailsPage /></elif>
                <elif condition="{on_tabs}"><TabsPage /></elif>
                <elif condition="{on_tree}"><TreePage /></elif>
                <elif condition="{on_tooltip}"><TooltipPage /></elif>
                <elif condition="{on_dialog}"><DialogPage /></elif>
                <elif condition="{on_toast}"><ToastPage /></elif>
                <elif condition="{on_text}"><TextPage /></elif>
                <elif condition="{on_lists}"><ListsPage /></elif>
                <elif condition="{on_blocks}"><BlocksPage /></elif>
                <elif condition="{on_table}"><TablePage /></elif>
                <else><MarkdownPage /></else>
              </div>
            </div>
            <div class="panel source">
              <div class="caption">HTML</div>
              <pre class="code">{code}</pre>
            </div>
          </div>
        </div>
      </div>

      <style>
        self {
          --bg: rgb(13, 17, 23);
          --surface: rgb(22, 27, 34);
          --raised: rgb(31, 38, 47);
          --border: rgb(48, 54, 61);
          --muted: rgb(139, 148, 158);
          --text: rgb(230, 237, 243);
          --accent: rgb(88, 166, 255);
          --success: rgb(63, 185, 80);
          --danger: rgb(248, 81, 73);
          --code: rgb(255, 166, 87);

          display: block;
          width: 100%;
          height: 100%;
          background-color: var(--bg);
          color: var(--text);
        }
        .app {
          display: flex;
          height: 100%;
        }

        /* The list of elements. */
        .sidebar {
          display: flex;
          flex-direction: column;
          width: 22;
          background-color: var(--surface);
          border-right: tall;
          border-color: var(--border);
          padding: 1 1;
          overflow-y: auto;
        }
        .sidebar > div {
          flex-shrink: 0;
        }
        .brand {
          font-weight: bold;
          margin: 0 1;
        }
        .logo {
          color: var(--accent);
        }
        .group {
          color: var(--muted);
          margin: 1 1 0 1;
        }
        .nav {
          padding: 0 1;
          color: var(--muted);
          border-left: tall;
          border-color: var(--surface);
          transition: background-color 150ms, color 150ms, border-color 150ms;
        }
        .nav:hover {
          background-color: var(--raised);
          color: var(--text);
        }
        .nav.active {
          color: var(--accent);
          border-color: var(--accent);
          background-color: var(--raised);
        }
        .grow {
          flex-grow: 1;
        }
        .hint {
          color: var(--muted);
          padding: 0 1;
        }

        /* The page: a title, then the element beside its code. */
        .main {
          display: flex;
          flex-direction: column;
          flex-grow: 1;
          padding: 1 2;
        }
        .header h1 {
          color: var(--code);
          font-weight: bold;
          text-decoration: none;
          margin: 0;
        }
        .description {
          color: var(--muted);
          margin: 0 0 1 0;
        }
        .panels {
          display: flex;
          gap: 2;
          flex-grow: 1;
        }
        .panel {
          display: flex;
          flex-direction: column;
          border: tall;
          border-color: var(--border);
          background-color: var(--surface);
          padding: 0 1;
        }
        .preview {
          flex-grow: 1;
          flex-basis: 0;
          overflow: hidden;
        }
        .source {
          flex-shrink: 0;
          width: 54;
        }
        .caption {
          color: var(--muted);
          margin-bottom: 1;
        }
        .stage {
          flex-grow: 1;
        }
        /* A narrow terminal shows the element alone. */
        @media (max-width: 99) {
          .source {
            display: none;
          }
        }
        .code {
          margin: 0;
          color: var(--muted);
          overflow-x: hidden;
          overflow-y: auto;
        }

      </style>
    )html";
};

int main() {
  auto app = Ref<Elements>::New();
  Screen screen(app);
  screen.SetInline(false);
  screen.Loop();
  return 0;
}
