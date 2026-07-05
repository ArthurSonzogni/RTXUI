# Cookbook

Self-contained patterns for interface problems that come up in most terminal
applications. Each recipe is a complete component you can paste into a project
and adapt.

The recipes rely on three RTXUI idioms worth internalizing:

- **State lives in C++ members; the template reads it.** Templates interpolate
  bound names — they do not evaluate expressions, contain statements, or
  perform assignments.
- **Logic is a computed method.** A `const` method returns a class string or a
  boolean; the template binds it with `class="{method_name}"` or
  `condition="{method_name}"`. Ternaries and comparisons live in C++.
- **Event handlers name a bound method**, optionally with one argument:
  `onclick="Select(home)"` invokes `void Select(std::string)` with `"home"`.

---

## Tabbed navigation

Track the active tab in a state variable. Buttons call a single parameterized
method; each button's class and each pane's visibility come from computed
methods. The `<if>`/`<elif>`/`<else>` chain shows one pane at a time.

```cpp
#include <rtxui/rtxui.hpp>

class TabbedApp : public rtxui::Component<TabbedApp> {
 public:
  std::string active_tab = "home";

  void Select(std::string tab) { active_tab = tab; }

  bool is_home() const { return active_tab == "home"; }
  bool is_settings() const { return active_tab == "settings"; }

  std::string home_class() const { return TabClass("home"); }
  std::string settings_class() const { return TabClass("settings"); }
  std::string about_class() const { return TabClass("about"); }

  void InitReflection() override {
    Bind(active_tab);
    Bind(Select);
    Bind(is_home);
    Bind(is_settings);
    Bind(home_class);
    Bind(settings_class);
    Bind(about_class);
    Import<rtxui::div>();
    Import<rtxui::button>();
    Component<TabbedApp>::InitReflection();
  }

  std::string_view view = R"html(
    <div class="container">
      <div class="tabs-header">
        <button class="{home_class}" onclick="Select(home)">Home</button>
        <button class="{settings_class}" onclick="Select(settings)">Settings</button>
        <button class="{about_class}" onclick="Select(about)">About</button>
      </div>

      <div class="tab-content">
        <if condition="{is_home}">
          <div>Welcome to the home screen.</div>
        </if>
        <elif condition="{is_settings}">
          <div>Settings go here.</div>
        </elif>
        <else>
          <div>About this application.</div>
        </else>
      </div>
    </div>

    <style>
      .container { display: flex; flex-direction: column; width: 50; }
      .tabs-header { display: flex; flex-direction: row; border-bottom: solid; border-color: #334155; }
      .tab-btn { padding: 0 2; cursor: pointer; background-color: transparent; }
      .tab-btn.active { background-color: #1e3a8a; color: #fff; font-weight: bold; }
      .tab-content { padding: 1; min-height: 5; }
    </style>
  )html";

 private:
  std::string TabClass(const std::string& tab) const {
    return active_tab == tab ? "tab-btn active" : "tab-btn";
  }
};
```

For most tabbed interfaces the built-in
[`<tabs>`/`<tab-pane>` components](/html_reference) are enough; this recipe
is the underlying pattern for when you need full control over the header.

---

## Background work without freezing the UI

Callbacks run on the main thread, so blocking inside one (network requests,
large file reads) freezes rendering. Run the work on a `std::thread` and post
the result back to the main loop with `task::TaskRunner`, which may be called
from other threads. State mutations posted this way are picked up by the
normal digest cycle.

```cpp
#include <rtxui/rtxui.hpp>

#include <chrono>
#include <thread>

class AsyncApp : public rtxui::Component<AsyncApp> {
 public:
  std::string status = "Idle";
  bool is_loading = false;

  void StartBackgroundTask() {
    if (is_loading) {
      return;
    }
    status = "Fetching data...";
    is_loading = true;

    auto* main_runner = task::TaskRunner::Current();
    std::thread([this, main_runner]() {
      // Stand-in for real work: a network call, a database query, ...
      std::this_thread::sleep_for(std::chrono::seconds(2));

      // Only the main thread may touch component state. PostTask schedules
      // this lambda on the UI loop and wakes it.
      main_runner->PostTask([this]() {
        status = "Loaded 42 items.";
        is_loading = false;
      });
    }).detach();
  }

  void InitReflection() override {
    Bind(status);
    Bind(is_loading);
    Bind(StartBackgroundTask);
    Import<rtxui::div>();
    Import<rtxui::button>();
    Import<rtxui::strong>();
    Component<AsyncApp>::InitReflection();
  }

  std::string_view view = R"html(
    <div class="panel">
      <div>Status: <strong>{status}</strong></div>
      <if condition="{is_loading}">
        <div class="loader">Processing...</div>
      </if>
      <else>
        <button onclick="StartBackgroundTask">Trigger load</button>
      </else>
    </div>

    <style>
      .panel { padding: 1; border: solid; width: 40; }
      .loader { color: #f59e0b; }
    </style>
  )html";
};
```

A detached thread must not outlive the component it captures. In real
applications, join or signal worker threads before the component is
destroyed, or route results through state that survives the component.

---

## Confirmation dialog

The built-in [`<dialog>`](/html_reference) element renders centered above the
rest of the interface with a dimmed backdrop, and closes on
<kbd>Escape</kbd>. Bind its `open` attribute to a boolean and flip that
boolean from methods.

```cpp
#include <rtxui/rtxui.hpp>

class ConfirmApp : public rtxui::Component<ConfirmApp> {
 public:
  bool confirming = false;
  std::string result = "No action taken.";

  void Ask() { confirming = true; }
  void Confirm() {
    result = "Action confirmed.";
    confirming = false;
  }
  void Cancel() {
    result = "Action cancelled.";
    confirming = false;
  }

  void InitReflection() override {
    Bind(confirming);
    Bind(result);
    Bind(Ask);
    Bind(Confirm);
    Bind(Cancel);
    Import<rtxui::div>();
    Import<rtxui::button>();
    Import<rtxui::dialog>();
    Import<rtxui::p>();
    Component<ConfirmApp>::InitReflection();
  }

  std::string_view view = R"html(
    <div class="main">
      <div>{result}</div>
      <button onclick="Ask">Delete everything</button>

      <dialog open="{confirming}" title="Confirm">
        <p>Are you sure? This cannot be undone.</p>
        <div class="actions">
          <button class="danger" onclick="Confirm">Confirm</button>
          <button onclick="Cancel">Cancel</button>
        </div>
      </dialog>
    </div>

    <style>
      .main { padding: 1; width: 60; height: 10; }
      .actions { display: flex; flex-direction: row; justify-content: flex-end; gap: 1; margin-top: 1; }
      .danger { background-color: #ef4444; color: #fff; }
    </style>
  )html";
};
```

To build an overlay by hand instead — a dropdown, a toast, a context menu —
use `position: absolute` (or `fixed`) with `z-index` inside a
`position: relative` ancestor, and gate it behind an `<if>`. The
[positioning guide](/guide/css/positioning) covers the mechanics.
