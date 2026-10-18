// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
//
// A gallery of interface patterns a terminal application needs.
//
// A command palette, notifications, a table with a details drawer, a form
// with validation, a task runner and a confirmation dialog. Every motion has
// a job: it shows where something came from, what changed, or how long is
// left. Nothing moves only to move.
//
// Try it: press Ctrl+K anywhere and type. Sort and filter the table, then
// press Enter on a row. Submit the form empty. Run the tasks.
#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstddef>
#include <cstdio>
#include <rtxui/rtxui.hpp>
#include <string>
#include <string_view>
#include <vector>

using namespace rtxui;

namespace {

// Whether `pattern`'s letters appear in `text` in order, ignoring case.
bool FuzzyMatch(std::string_view pattern, std::string_view text) {
  const auto lower = [](char c) {
    return std::tolower(static_cast<unsigned char>(c));
  };
  size_t at = 0;
  for (const char c : pattern) {
    while (at < text.size() && lower(text[at]) != lower(c)) {
      ++at;
    }
    if (at == text.size()) {
      return false;
    }
    ++at;
  }
  return true;
}

bool Contains(std::string_view text, std::string_view part) {
  return std::search(text.begin(), text.end(), part.begin(), part.end(),
                     [](char a, char b) {
                       return std::tolower(static_cast<unsigned char>(a)) ==
                              std::tolower(static_cast<unsigned char>(b));
                     }) != text.end();
}

std::string Fixed(double value, int decimals) {
  char text[32];
  std::snprintf(text, sizeof(text), "%.*f", decimals, value);
  return text;
}

}  // namespace

// ---------------------------------------------------------------------------
// Table and drawer: sort, filter, move with the arrows, open a row.

struct Process {
  int pid = 0;
  std::string name;
  std::string user;
  double cpu = 0;
  double memory = 0;  // MB
  std::string started;
  std::string command;
};

class TablePage : public Component<TablePage> {
 public:
  std::vector<Process> processes = {
      {4121, "clang++", "ada", 98.2, 1240, "12:04",
       "clang++ -O2 -c layout.cpp"},
      {2210, "chrome", "ada", 12.5, 3120, "09:12", "/opt/google/chrome/chrome"},
      {3301, "postgres", "postgres", 6.1, 512, "08:00",
       "postgres -D /var/lib/pg"},
      {1874, "node", "ada", 4.8, 680, "10:41", "node server.js --port 8080"},
      {1022, "dockerd", "root", 2.2, 210, "08:00", "/usr/bin/dockerd"},
      {2950, "nvim", "ada", 0.9, 96, "11:30", "nvim src/screen.cpp"},
      {812, "sshd", "root", 0.1, 8, "08:00", "sshd: /usr/sbin/sshd -D"},
      {1, "systemd", "root", 0.0, 12, "08:00", "/sbin/init splash"},
  };

  std::string filter;
  std::string sort = "cpu";
  bool descending = true;
  int selected = 0;
  bool drawer_open = false;
  std::vector<Process> rows;

  void SortBy(const std::string& column) {
    descending = column == sort ? !descending : column != "name";
    sort = column;
  }
  void Open(const std::string& index) {
    selected = std::stoi(index);
    drawer_open = true;
  }
  void Close() { drawer_open = false; }

  std::string arrow(std::string_view column) const {
    return sort != column ? "" : descending ? " ▾" : " ▴";
  }
  std::string name_arrow() const { return arrow("name"); }
  std::string cpu_arrow() const { return arrow("cpu"); }
  std::string mem_arrow() const { return arrow("mem"); }
  std::string cursor_offset() const { return std::to_string(selected); }
  std::string drawer_class() const {
    return drawer_open && !rows.empty() ? "drawer open" : "drawer";
  }
  std::string count() const {
    return std::to_string(rows.size()) + " of " +
           std::to_string(processes.size()) + " processes";
  }

  // The selected process, for the drawer.
  const Process& current() const {
    static const Process kNone;
    return rows.empty() ? kNone : rows[selected];
  }
  std::string d_name() const { return current().name; }
  std::string d_pid() const { return std::to_string(current().pid); }
  std::string d_user() const { return current().user; }
  std::string d_cpu() const { return Fixed(current().cpu, 1) + "%"; }
  std::string d_cpu_value() const {
    return std::to_string(static_cast<int>(current().cpu));
  }
  std::string d_memory() const { return Fixed(current().memory, 0) + " MB"; }
  std::string d_started() const { return current().started; }
  std::string d_command() const { return current().command; }

  TablePage() {
    Bind(rows, &TablePage::RowFields);
    Bind(filter);
    Bind(drawer_open);
    Bind(name_arrow);
    Bind(cpu_arrow);
    Bind(mem_arrow);
    Bind(cursor_offset);
    Bind(drawer_class);
    Bind(count);
    Bind(d_name);
    Bind(d_pid);
    Bind(d_user);
    Bind(d_cpu);
    Bind(d_cpu_value);
    Bind(d_memory);
    Bind(d_started);
    Bind(d_command);
    Bind(Close);
    Import("SortBy", [this](std::string column) { SortBy(column); });
    Import("Open", [this](std::string index) { Open(index); });
  }

  // The visible rows follow the filter and the sort before every render.
  bool Digest() override {
    rows.clear();
    for (const Process& process : processes) {
      if (Contains(process.name, filter) || Contains(process.user, filter)) {
        rows.push_back(process);
      }
    }
    std::stable_sort(rows.begin(), rows.end(),
                     [&](const Process& a, const Process& b) {
                       if (sort == "name") {
                         return descending ? a.name > b.name : a.name < b.name;
                       }
                       const double x = sort == "cpu" ? a.cpu : a.memory;
                       const double y = sort == "cpu" ? b.cpu : b.memory;
                       return descending ? x > y : x < y;
                     });
    const int count = static_cast<int>(rows.size());
    selected = count == 0 ? 0 : std::clamp(selected, 0, count - 1);
    return Component<TablePage>::Digest();
  }

  bool OnEvent(Event event) override {
    if (event == Event::ArrowDown()) {
      selected++;
      return true;
    }
    if (event == Event::ArrowUp()) {
      selected = std::max(0, selected - 1);
      return true;
    }
    if (event == Event::Return()) {
      drawer_open = true;
      return true;
    }
    if (event == Event::Escape() && drawer_open) {
      drawer_open = false;
      return true;
    }
    return Component<TablePage>::OnEvent(event);
  }

  void RowFields(const Process& process, FieldSink& out) const {
    out("pid", std::to_string(process.pid));
    out("name", process.name);
    out("user", process.user);
    out("cpu", Fixed(process.cpu, 1));
    out("mem", Fixed(process.memory, 0));
    out("started", process.started);
  }

  std::string_view view = R"html(
      <div class="page">
        <div class="toolbar">
          <input value="{filter}" placeholder="Filter by name or user" />
          <span class="count">{count}</span>
        </div>
        <div class="table" tabindex="0">
          <div class="row head">
            <span class="pid">PID</span>
            <span class="name sortable" onclick="SortBy(name)">Name{name_arrow}</span>
            <span class="user">User</span>
            <span class="num sortable" onclick="SortBy(cpu)">CPU %{cpu_arrow}</span>
            <span class="num sortable" onclick="SortBy(mem)">Mem MB{mem_arrow}</span>
            <span class="started">Started</span>
          </div>
          <div class="body">
            <div class="cursor" style="translate: 0 {cursor_offset};"></div>
            <for each="{rows}" as="p" key="{p.pid}">
              <div class="row" onclick="Open({$index})">
                <span class="pid">{p.pid}</span>
                <span class="name">{p.name}</span>
                <span class="user">{p.user}</span>
                <span class="num">{p.cpu}</span>
                <span class="num">{p.mem}</span>
                <span class="started">{p.started}</span>
              </div>
            </for>
          </div>
        </div>
        <div class="hint">↑ ↓ to move · Enter to open · Esc to close · click a header to sort</div>

        <div class="{drawer_class}">
          <div class="drawer-head">
            <span class="drawer-title">{d_name}</span>
            <span class="close" onclick="Close">esc ✕</span>
          </div>
          <div class="kv"><span>PID</span><span>{d_pid}</span></div>
          <div class="kv"><span>User</span><span>{d_user}</span></div>
          <div class="kv"><span>Started</span><span>{d_started}</span></div>
          <div class="kv"><span>Memory</span><span>{d_memory}</span></div>
          <div class="kv"><span>CPU</span><span>{d_cpu}</span></div>
          <progress value="{d_cpu_value}" max="100" width="30" />
          <div class="label">Command</div>
          <div class="command">{d_command}</div>
        </div>
      </div>
      <style>
        self {
          display: block;
          height: 100%;
        }
        .page {
          position: relative;
          display: flex;
          flex-direction: column;
          gap: 1;
          height: 100%;
        }
        .toolbar {
          display: flex;
          align-items: center;
          gap: 2;
        }
        input {
          width: 34;
          border: tall;
          border-color: var(--border);
          background-color: var(--surface);
          transition: border-color 150ms;
        }
        input:focus {
          border-color: var(--accent);
        }
        .count {
          color: var(--muted);
        }
        .table {
          border: tall;
          border-color: var(--border);
          background-color: var(--surface);
          transition: border-color 150ms;
        }
        .table:focus-within {
          border-color: var(--accent);
        }
        /* Above the highlight, which is drawn first. */
        .row {
          position: relative;
          z-index: 1;
          display: flex;
          gap: 2;
          padding: 0 1;
        }
        .row.head {
          color: var(--muted);
          border-bottom: solid;
          border-color: var(--border);
        }
        .sortable:hover {
          color: var(--text);
        }
        .body {
          position: relative;
        }
        /* One highlight for the whole list, moved rather than redrawn: the
           eye follows it from the old row to the new one. */
        .cursor {
          position: absolute;
          top: 0;
          left: 0;
          width: 100%;
          height: 1;
          background-color: var(--raised);
          border-left: tall;
          border-color: var(--accent);
          transition: translate 120ms ease-out;
        }
        .pid { width: 6; color: var(--muted); }
        .name { flex-grow: 1; }
        .user { width: 9; color: var(--muted); }
        .num { width: 9; text-align: right; }
        .started { width: 8; color: var(--muted); }
        .hint {
          color: var(--muted);
        }

        /* The drawer slides over the table's edge instead of replacing the
           page, so the list it came from stays in view. */
        .drawer {
          position: absolute;
          z-index: 2;
          top: 0;
          right: 0;
          width: 38;
          height: 100%;
          display: flex;
          flex-direction: column;
          border: tall;
          border-color: var(--accent);
          background-color: var(--raised);
          padding: 1 2;
          translate: calc(100% + 4);
          transition: translate 220ms ease-out;
        }
        .drawer.open {
          translate: 0;
        }
        .drawer-head {
          display: flex;
          justify-content: space-between;
          margin-bottom: 1;
        }
        .drawer-title {
          color: var(--accent);
          font-weight: bold;
        }
        .close {
          color: var(--muted);
        }
        .close:hover {
          color: var(--text);
        }
        .kv {
          display: flex;
          justify-content: space-between;
        }
        .kv span:first-child {
          color: var(--muted);
        }
        progress {
          color: var(--accent);
          background-color: var(--border);
          margin: 1 0;
        }
        .label {
          color: var(--muted);
        }
        .command {
          color: var(--code);
        }

        @media (max-width: 99) {
          .user, .started {
            display: none;
          }
        }
      </style>
    )html";
};

// ---------------------------------------------------------------------------
// Form: errors appear under the field they are about, once it matters.

class FormPage : public Component<FormPage> {
 public:
  std::string name;
  std::string email;
  std::string role = "developer";
  bool updates = true;
  bool submitted = false;  // Errors show from the first attempt on.
  bool saved = false;

  bool name_error() const { return submitted && name.empty(); }
  bool email_error() const {
    return submitted && email.find('@') == std::string::npos;
  }
  std::string name_class() const { return name_error() ? "invalid" : ""; }
  std::string email_class() const { return email_error() ? "invalid" : ""; }
  std::string summary() const {
    return name + " <" + email + ">, " + role +
           (updates ? ", gets updates" : "");
  }

  void Submit() {
    submitted = true;
    saved = !name_error() && !email_error();
    saved_name_ = name;
    saved_email_ = email;
  }

  FormPage() {
    Bind(name);
    Bind(email);
    Bind(role);
    Bind(updates);
    Bind(submitted);
    Bind(saved);
    Bind(name_error);
    Bind(email_error);
    Bind(name_class);
    Bind(email_class);
    Bind(summary);
    Bind(Submit);
  }

  // Editing a saved form makes it unsaved again.
  bool Digest() override {
    if (saved && (name != saved_name_ || email != saved_email_)) {
      saved = false;
    }
    return Component<FormPage>::Digest();
  }

  std::string_view view = R"html(
      <div class="form">
        <div class="field">
          <span class="label">Name</span>
          <div class="control">
            <input class="{name_class}" value="{name}" placeholder="Ada Lovelace" />
            <if condition="{name_error}"><span class="error">Enter a name.</span></if>
          </div>
        </div>
        <div class="field">
          <span class="label">Email</span>
          <div class="control">
            <input class="{email_class}" value="{email}" placeholder="ada@example.com" />
            <if condition="{email_error}"><span class="error">An email address has an @.</span></if>
          </div>
        </div>
        <div class="field">
          <span class="label">Role</span>
          <select value="{role}">
            <option value="developer">Developer</option>
            <option value="designer">Designer</option>
            <option value="manager">Manager</option>
          </select>
        </div>
        <div class="field">
          <span class="label"></span>
          <checkbox checked="{updates}">Send me release notes</checkbox>
        </div>
        <div class="field">
          <span class="label"></span>
          <button onclick="Submit">Create account</button>
        </div>
        <if condition="{saved}">
          <div class="saved">✓ Created {summary}.</div>
        </if>
      </div>
      <style>
        @keyframes appear {
          from { opacity: 0; }
          to { opacity: 1; }
        }
        .form {
          display: flex;
          flex-direction: column;
          gap: 1;
          width: 70;
        }
        .field {
          display: flex;
          gap: 2;
        }
        .label {
          width: 8;
          color: var(--muted);
          padding-top: 1;
        }
        .control {
          display: flex;
          flex-direction: column;
        }
        input {
          width: 40;
          border: tall;
          border-color: var(--border);
          background-color: var(--surface);
          transition: border-color 150ms;
        }
        input:focus {
          border-color: var(--accent);
        }
        input.invalid {
          border-color: var(--danger);
        }
        .error {
          color: var(--danger);
          animation: appear 150ms ease-out;
        }
        select {
          width: 40;
          margin-top: 1;
        }
        checkbox {
          margin-top: 1;
        }
        button {
          border: tall;
          border-color: var(--accent);
          background-color: var(--bg);
          color: var(--accent);
          padding: 0 2;
          transition: background-color 150ms, color 150ms;
        }
        button:hover, button:focus {
          background-color: var(--accent);
          color: var(--bg);
        }
        .saved {
          color: var(--success);
          animation: appear 200ms ease-out;
        }
      </style>
    )html";

 private:
  std::string saved_name_;
  std::string saved_email_;
};

// ---------------------------------------------------------------------------
// Tasks: what runs now, what is done, and how far along the whole is.

struct Step {
  std::string name;
  double seconds = 0;             // How long it pretends to take.
  std::string state = "pending";  // "pending" | "running" | "done"
};

class TasksPage : public Component<TasksPage> {
 public:
  std::vector<Step> steps = {
      {"Resolve dependencies", 0.6}, {"Download 14 packages", 1.4},
      {"Compile 212 files", 2.0},    {"Run 818 tests", 1.2},
      {"Package release", 0.6},
  };
  bool running = false;
  bool finished = false;

  std::string progress() const {
    double done = 0;
    double total = 0;
    for (const Step& step : steps) {
      total += step.seconds;
      done += step.state == "done" ? step.seconds : 0;
    }
    return std::to_string(static_cast<int>(100 * done / total));
  }
  std::string run_label() const { return finished ? "Run again" : "Run"; }
  std::string total() const {
    double seconds = 0;
    for (const Step& step : steps) {
      seconds += step.seconds;
    }
    return Fixed(seconds, 1) + " s";
  }

  void Run() {
    if (running) {
      return;
    }
    for (Step& step : steps) {
      step.state = "pending";
    }
    running = true;
    finished = false;
    Next(Ref<TasksPage>(this), 0);
  }

  TasksPage() {
    Bind(steps, &TasksPage::StepFields);
    Bind(running);
    Bind(finished);
    Bind(progress);
    Bind(run_label);
    Bind(total);
    Bind(Run);
  }

  void StepFields(const Step& step, FieldSink& out) const {
    out("name", step.name);
    out("state", step.state);
    out("icon", step.state == "done"      ? "✓"
                : step.state == "running" ? "●"
                                          : "○");
    out("time", step.state == "done" ? Fixed(step.seconds, 1) + " s" : "");
  }

  std::string_view view = R"html(
      <div class="tasks">
        <div class="steps">
          <for each="{steps}" as="s">
            <div class="step {s.state}">
              <span class="icon">{s.icon}</span>
              <span class="step-name">{s.name}</span>
              <span class="time">{s.time}</span>
            </div>
          </for>
        </div>
        <div class="track"><div class="bar" style="width: {progress}%;"></div></div>
        <div class="footer">
          <button onclick="Run">{run_label}</button>
          <if condition="{finished}"><span class="done">✓ Done in {total}.</span></if>
        </div>
      </div>
      <style>
        @keyframes appear {
          from { opacity: 0; }
          to { opacity: 1; }
        }
        @keyframes pulse {
          from { opacity: 1; }
          to { opacity: 0.3; }
        }
        .tasks {
          display: flex;
          flex-direction: column;
          gap: 1;
          width: 56;
        }
        .steps {
          border: tall;
          border-color: var(--border);
          background-color: var(--surface);
          padding: 1 2;
        }
        .step {
          display: flex;
          gap: 1;
          color: var(--muted);
          transition: color 200ms;
        }
        .step-name {
          flex-grow: 1;
        }
        .step.running {
          color: var(--text);
        }
        /* A pulse says "busy" without a spinner's noise. */
        .step.running .icon {
          color: var(--accent);
          animation: pulse 500ms ease-in-out infinite alternate;
        }
        .step.done {
          color: var(--text);
        }
        .step.done .icon {
          color: var(--success);
        }
        .time {
          color: var(--muted);
        }
        .track {
          height: 1;
          background-color: var(--border);
        }
        /* The bar eases to each new value instead of jumping. */
        .bar {
          height: 1;
          background-color: var(--accent);
          transition: width 400ms ease-out;
        }
        .footer {
          display: flex;
          align-items: center;
          gap: 2;
        }
        button {
          border: tall;
          border-color: var(--accent);
          background-color: var(--bg);
          color: var(--accent);
          padding: 0 2;
          transition: background-color 150ms, color 150ms;
        }
        button:hover, button:focus {
          background-color: var(--accent);
          color: var(--bg);
        }
        .done {
          color: var(--success);
          animation: appear 200ms ease-out;
        }
      </style>
    )html";

 private:
  // Starts step `index`, and schedules the one after. `self` keeps the page
  // alive while a step runs, even if the user moved to another page.
  static void Next(Ref<TasksPage> self, size_t index) {
    if (index == self->steps.size()) {
      self->running = false;
      self->finished = true;
      return;
    }
    self->steps[index].state = "running";
    PostDelayedTask(
        [self, index] {
          self->steps[index].state = "done";
          Next(self, index + 1);
        },
        std::chrono::milliseconds(
            static_cast<int>(self->steps[index].seconds * 1000)));
  }
};

// ---------------------------------------------------------------------------
// Confirmation: a destructive action asks first, and the safe answer is the
// default.

struct Branch {
  std::string name;
  std::string age;
  bool leaving = false;
};

class DialogPage : public Component<DialogPage> {
 public:
  std::vector<Branch> branches = {
      {"feature/inline-mode", "merged 2 days ago"},
      {"fix/resize-race", "merged 5 days ago"},
      {"experiment/old-ui", "merged 3 weeks ago"},
  };
  bool confirm_open = false;
  std::string pending;
  std::string status = "Delete a merged branch.";

  void Ask(const std::string& index) {
    pending = branches[std::stoul(index)].name;
    confirm_open = true;
  }
  void Cancel() {
    confirm_open = false;
    status = "Kept " + pending + ".";
  }
  void Delete() {
    confirm_open = false;
    status = "Deleted " + pending + ".";
    for (Branch& branch : branches) {
      branch.leaving = branch.leaving || branch.name == pending;
    }
    // The row fades out, then leaves the list.
    Ref<DialogPage> self(this);
    PostDelayedTask(
        [self] {
          std::erase_if(self->branches,
                        [](const Branch& branch) { return branch.leaving; });
        },
        std::chrono::milliseconds(250));
  }
  std::string title() const { return "Delete " + pending + "?"; }

  DialogPage() {
    Bind(branches, &DialogPage::BranchFields);
    Bind(confirm_open);
    Bind(pending);
    Bind(status);
    Bind(title);
    Bind(Cancel);
    Bind(Delete);
    Import("Ask", [this](std::string index) { Ask(index); });
  }

  void BranchFields(const Branch& branch, FieldSink& out) const {
    out("name", branch.name);
    out("age", branch.age);
    out("class", branch.leaving ? "branch leaving" : "branch");
  }

  std::string_view view = R"html(
      <div class="branches">
        <div class="list">
          <for each="{branches}" as="b" key="{b.name}">
            <div class="{b.class}">
              <span class="branch-name">{b.name}</span>
              <span class="age">{b.age}</span>
              <span class="delete" onclick="Ask({$index})">Delete</span>
            </div>
          </for>
        </div>
        <div class="status">{status}</div>
      </div>
      <dialog open="{confirm_open}" title="{title}">
        <p class="warning">Commits that are on no other branch are gone for good.</p>
        <if condition="{confirm_open}">
          <div class="actions">
            <button onclick="Cancel" autofocus="">Cancel</button>
            <button class="danger" onclick="Delete">Delete branch</button>
          </div>
        </if>
      </dialog>
      <style>
        @keyframes fade-in {
          from { opacity: 0; }
          to { opacity: 1; }
        }
        .branches {
          display: flex;
          flex-direction: column;
          gap: 1;
          width: 70;
        }
        .list {
          border: tall;
          border-color: var(--border);
          background-color: var(--surface);
          padding: 1 1;
        }
        .branch {
          display: flex;
          gap: 2;
          padding: 0 1;
          transition: opacity 250ms, background-color 150ms;
        }
        .branch:hover {
          background-color: var(--raised);
        }
        .branch.leaving {
          opacity: 0;
        }
        .branch-name {
          flex-grow: 1;
        }
        .age {
          color: var(--muted);
        }
        .status {
          color: var(--muted);
        }
        .delete {
          color: var(--danger);
          padding: 0 1;
          transition: background-color 150ms, color 150ms;
        }
        .delete:hover {
          background-color: var(--danger);
          color: var(--bg);
        }
        button {
          border: tall;
          border-color: var(--border);
          background-color: var(--bg);
          color: var(--text);
          padding: 0 2;
          transition: background-color 150ms, color 150ms, border-color 150ms;
        }
        button:hover, button:focus {
          border-color: var(--accent);
          color: var(--accent);
        }
        button.danger {
          color: var(--danger);
        }
        button.danger:hover, button.danger:focus {
          border-color: var(--danger);
          background-color: var(--danger);
          color: var(--bg);
        }
        dialog::part(dialog-box) {
          width: 52;
          max-width: 52;
          border: tall;
          border-color: var(--danger);
          background-color: var(--raised);
          padding: 1 2;
          animation: fade-in 100ms ease-out;
        }
        dialog::part(dialog-header) {
          color: var(--text);
          border-color: var(--border);
        }
        .warning {
          color: var(--muted);
          margin-bottom: 1;
        }
        .actions {
          display: flex;
          gap: 2;
          justify-content: flex-end;
          width: 100%;
        }
        .actions button {
          flex-shrink: 0;
        }
      </style>
    )html";
};

// ---------------------------------------------------------------------------
// The gallery: navigation, plus the two patterns every page shares -- the
// command palette and the notifications.

struct Page {
  std::string icon;
  std::string name;
  std::string title;
  std::string description;
};

struct Command {
  std::string title;
  std::string hint;
};

struct Toast {
  int id = 0;
  std::string kind;  // "success" | "warning" | "error"
  std::string title;
  std::string body;
  bool leaving = false;
};

class Showcase : public Component<Showcase> {
 public:
  std::vector<Page> pages = {
      {"❯", "Palette", "Command palette",
       "Every action, one keystroke away. Press Ctrl+K on any page."},
      {"●", "Notifications", "Notifications",
       "Feedback that comes and goes on its own. The bar shows the time left."},
      {"▤", "Table", "Table and drawer",
       "Sort, filter, move with the arrows. A row opens beside the list."},
      {"✎", "Form", "Form",
       "Errors appear under the field they are about, once it matters."},
      {"◷", "Tasks", "Tasks",
       "What runs now, what is done, and how far along the whole is."},
      {"◇", "Dialog", "Confirmation",
       "A destructive action asks first. The safe answer is the default."},
  };
  int page = 0;

  // Command palette.
  std::vector<Command> commands;
  bool palette_open = false;
  std::string query;
  std::vector<Command> matches;
  int selected = 0;
  std::string last_command = "None yet";

  // Notifications.
  std::vector<Toast> toasts;

  bool on_palette() const { return page == 0; }
  bool on_notifications() const { return page == 1; }
  bool on_table() const { return page == 2; }
  bool on_form() const { return page == 3; }
  bool on_tasks() const { return page == 4; }
  bool on_dialog() const { return page == 5; }
  std::string title() const { return pages[page].title; }
  std::string description() const { return pages[page].description; }
  std::string cursor_offset() const { return std::to_string(selected); }

  void Go(const std::string& index) { page = std::stoi(index); }
  void OpenPalette() {
    palette_open = true;
    query.clear();
    selected = 0;
  }
  void RunCommand(const std::string& index) {
    const size_t i = std::stoul(index);
    if (i < matches.size()) {
      Execute(matches[i].title);
    }
    palette_open = false;
  }
  void Notify(const std::string& kind) {
    if (kind == "success") {
      Push("success", "Saved", "Your changes are on disk.");
    } else if (kind == "warning") {
      Push("warning", "Disk almost full", "12 GB left on /home.");
    } else {
      Push("error", "Build failed", "layout.cpp:42: expected ';'.");
    }
  }
  void Dismiss(const std::string& id) { Leave(std::stoi(id)); }

  Showcase() {
    for (const Page& p : pages) {
      commands.push_back({"Go to " + p.title, "page"});
    }
    commands.push_back({"Notify: saved", "toast"});
    commands.push_back({"Notify: disk almost full", "toast"});
    commands.push_back({"Notify: build failed", "toast"});
    commands.push_back({"Clear notifications", "toast"});

    Bind(pages, &Showcase::PageFields);
    Bind(matches, &Showcase::CommandFields);
    Bind(toasts, &Showcase::ToastFields);
    Bind(page);
    Bind(palette_open);
    Bind(query);
    Bind(last_command);
    Bind(on_palette);
    Bind(on_notifications);
    Bind(on_table);
    Bind(on_form);
    Bind(on_tasks);
    Bind(on_dialog);
    Bind(title);
    Bind(description);
    Bind(cursor_offset);
    Bind(OpenPalette);
    Import("Go", [this](std::string index) { Go(index); });
    Import("RunCommand", [this](std::string index) { RunCommand(index); });
    Import("Notify", [this](std::string kind) { Notify(kind); });
    Import("Dismiss", [this](std::string id) { Dismiss(id); });
    Import<TablePage>();
    Import<FormPage>();
    Import<TasksPage>();
    Import<DialogPage>();
    // Save this file with a different template or style, and the running
    // application picks it up without a rebuild.
    EnableHotReload();
  }

  // The palette's matches follow the query before every render.
  bool Digest() override {
    matches.clear();
    for (const Command& command : commands) {
      if (FuzzyMatch(query, command.title)) {
        matches.push_back(command);
      }
    }
    const int count = static_cast<int>(matches.size());
    selected = count == 0 ? 0 : std::clamp(selected, 0, count - 1);
    return Component<Showcase>::Digest();
  }

  // Keys nothing focused used: the palette's shortcut and its navigation.
  bool OnEvent(Event event) override {
    if (event == Event::CtrlK()) {
      OpenPalette();
      return true;
    }
    if (palette_open) {
      if (event == Event::ArrowDown()) {
        selected++;
        return true;
      }
      if (event == Event::ArrowUp()) {
        selected = std::max(0, selected - 1);
        return true;
      }
      if (event == Event::Return()) {
        RunCommand(std::to_string(selected));
        return true;
      }
      if (event == Event::Escape()) {
        palette_open = false;
        return true;
      }
    }
    return Component<Showcase>::OnEvent(event);
  }

  void PageFields(const Page& item, FieldSink& out) const {
    out("icon", item.icon);
    out("name", item.name);
    out("class", &item == &pages[page] ? "nav active" : "nav");
  }
  void CommandFields(const Command& command, FieldSink& out) const {
    out("title", command.title);
    out("hint", command.hint);
  }
  void ToastFields(const Toast& toast, FieldSink& out) const {
    out("id", std::to_string(toast.id));
    out("class", "toast " + toast.kind + (toast.leaving ? " leaving" : ""));
    out("icon", toast.kind == "success"   ? "✓"
                : toast.kind == "warning" ? "!"
                                          : "✕");
    out("title", toast.title);
    out("body", toast.body);
  }

  std::string_view view = R"html(
      <div class="app">
        <div class="sidebar">
          <div class="brand"><span class="logo">◆</span><span class="label"> RTXUI</span></div>
          <for each="{pages}" as="item">
            <div class="{item.class}" onclick="Go({$index})">
              <span>{item.icon}</span><span class="label">{item.name}</span>
            </div>
          </for>
          <div class="grow"></div>
          <div class="shortcut label">Commands <kbd>Ctrl K</kbd></div>
        </div>

        <div class="main">
          <h1>{title}</h1>
          <p class="description">{description}</p>

          <if condition="{on_palette}">
            <div class="page">
              <button onclick="OpenPalette">Open the command palette</button>
              <div class="last"><span class="muted">Last command</span> {last_command}</div>
            </div>
          </if>
          <elif condition="{on_notifications}">
            <div class="page buttons">
              <button onclick="Notify(success)">Save</button>
              <button onclick="Notify(warning)">Check disk</button>
              <button onclick="Notify(error)">Build</button>
            </div>
          </elif>
          <elif condition="{on_table}">
            <div class="page grow"><TablePage /></div>
          </elif>
          <elif condition="{on_form}">
            <div class="page"><FormPage /></div>
          </elif>
          <elif condition="{on_tasks}">
            <div class="page"><TasksPage /></div>
          </elif>
          <else>
            <div class="page"><DialogPage /></div>
          </else>
        </div>
      </div>

      <dialog class="palette" open="{palette_open}" title="">
        <if condition="{palette_open}">
          <input value="{query}" placeholder="Type a command" autofocus="" />
        </if>
        <div class="results">
          <div class="cursor" style="translate: 0 {cursor_offset};"></div>
          <for each="{matches}" as="m" key="{m.title}">
            <div class="result" onclick="RunCommand({$index})">
              <span>{m.title}</span><span class="muted">{m.hint}</span>
            </div>
          </for>
        </div>
      </dialog>

      <div class="toasts">
        <for each="{toasts}" as="t" key="{t.id}">
          <div class="{t.class}" onclick="Dismiss({t.id})">
            <div class="toast-head"><span class="icon">{t.icon}</span><span class="toast-title">{t.title}</span></div>
            <div class="toast-body">{t.body}</div>
            <div class="timer"></div>
          </div>
        </for>
      </div>

      <style>
        @keyframes fade-in {
          from { opacity: 0; }
          to { opacity: 1; }
        }
        @keyframes slide-in {
          from { opacity: 0; translate: calc(100% + 2); }
          to { opacity: 1; translate: 0; }
        }
        @keyframes drain {
          from { width: 100%; }
          to { width: 0; }
        }

        self {
          --bg: rgb(13, 17, 23);
          --surface: rgb(22, 27, 34);
          --raised: rgb(31, 38, 47);
          --border: rgb(48, 54, 61);
          --muted: rgb(139, 148, 158);
          --text: rgb(230, 237, 243);
          --accent: rgb(88, 166, 255);
          --success: rgb(63, 185, 80);
          --warning: rgb(210, 153, 34);
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
        .muted {
          color: var(--muted);
        }

        .sidebar {
          display: flex;
          flex-direction: column;
          width: 22;
          background-color: var(--surface);
          border-right: tall;
          border-color: var(--border);
          padding: 1 1;
        }
        .brand {
          font-weight: bold;
          margin: 0 1 1 1;
        }
        .logo {
          color: var(--accent);
        }
        .nav {
          display: flex;
          gap: 1;
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
          color: var(--text);
          border-color: var(--accent);
          background-color: var(--raised);
        }
        .grow {
          flex-grow: 1;
        }
        .shortcut {
          color: var(--muted);
          padding: 0 1;
        }
        kbd {
          color: var(--text);
          background-color: var(--raised);
        }

        .main {
          display: flex;
          flex-direction: column;
          flex-grow: 1;
          padding: 1 3;
        }
        h1 {
          color: var(--text);
          font-weight: bold;
          text-decoration: none;
        }
        .description {
          color: var(--muted);
          margin-bottom: 1;
        }
        .buttons {
          display: flex;
          gap: 2;
        }
        .last {
          margin-top: 1;
        }
        button {
          border: tall;
          border-color: var(--border);
          background-color: var(--bg);
          color: var(--text);
          padding: 0 2;
          transition: border-color 150ms, color 150ms;
        }
        button:hover, button:focus {
          border-color: var(--accent);
          color: var(--accent);
        }

        dialog.palette::part(dialog-overlay) {
          align-items: flex-start;
          padding-top: 4;
        }
        dialog.palette::part(dialog-header) {
          display: none;
        }
        dialog.palette::part(dialog-box) {
          width: 60;
          max-width: 60;
          border: tall;
          border-color: var(--accent);
          background-color: var(--raised);
          padding: 0 1 1 1;
          animation: fade-in 100ms ease-out;
        }
        .palette input {
          width: 100%;
          background-color: var(--raised);
          border-bottom: solid;
          border-color: var(--border);
          margin-bottom: 1;
        }
        .results {
          position: relative;
          height: 10;
        }
        /* One highlight, moved rather than redrawn, so the eye can follow
           it from one result to the next. */
        .cursor {
          position: absolute;
          top: 0;
          left: 0;
          width: 100%;
          height: 1;
          background-color: rgb(40, 52, 68);
          border-left: tall;
          border-color: var(--accent);
          transition: translate 120ms ease-out;
        }
        /* Above the highlight, which is drawn first. */
        .result {
          position: relative;
          z-index: 1;
          display: flex;
          justify-content: space-between;
          padding: 0 1;
        }

        .toasts {
          position: fixed;
          right: 2;
          bottom: 1;
          width: 38;
          display: flex;
          flex-direction: column;
          gap: 1;
        }
        /* Toasts slide in from the edge they live on, and fade when they
           go, so their coming and going never jolts the page. */
        .toast {
          border: tall;
          border-color: var(--border);
          background-color: var(--raised);
          padding: 0 2;
          animation: slide-in 220ms ease-out;
          transition: opacity 250ms;
        }
        .toast.leaving {
          opacity: 0;
        }
        .toast-head {
          display: flex;
          gap: 1;
        }
        .toast-title {
          font-weight: bold;
        }
        .toast-body {
          color: var(--muted);
        }
        /* The time left before it closes on its own. */
        .timer {
          margin-top: 1;
          border-top: tall;
          width: 100%;
          animation: drain 4s linear forwards;
        }
        .toast.success .icon, .toast.success .timer { color: var(--success); border-color: var(--success); }
        .toast.warning .icon, .toast.warning .timer { color: var(--warning); border-color: var(--warning); }
        .toast.error .icon, .toast.error .timer { color: var(--danger); border-color: var(--danger); }

        @media (max-width: 99) {
          .sidebar {
            width: 8;
          }
          .label {
            display: none;
          }
        }
      </style>
    )html";

 private:
  void Execute(const std::string& command) {
    last_command = command;
    for (size_t i = 0; i < pages.size(); ++i) {
      if (command == "Go to " + pages[i].title) {
        page = static_cast<int>(i);
      }
    }
    if (command == "Notify: saved") {
      Notify("success");
    } else if (command == "Notify: disk almost full") {
      Notify("warning");
    } else if (command == "Notify: build failed") {
      Notify("error");
    } else if (command == "Clear notifications") {
      for (const Toast& toast : toasts) {
        Leave(toast.id);
      }
    }
  }

  void Push(std::string kind, std::string title, std::string body) {
    const int id = ++last_toast_id_;
    toasts.push_back({id, std::move(kind), std::move(title), std::move(body)});
    PostDelayedTask([this, id] { Leave(id); }, std::chrono::seconds(4));
  }

  // Fades the toast out, then removes it.
  void Leave(int id) {
    for (Toast& toast : toasts) {
      if (toast.id == id) {
        toast.leaving = true;
      }
    }
    PostDelayedTask(
        [this, id] {
          std::erase_if(toasts, [id](const Toast& t) { return t.id == id; });
        },
        std::chrono::milliseconds(250));
  }

  int last_toast_id_ = 0;
};

int main() {
  auto app = Ref<Showcase>::New();
  Screen screen(app);
  screen.Loop();
  return 0;
}
