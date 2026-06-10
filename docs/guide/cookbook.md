# Cookbook & Recipes

This cookbook contains practical design patterns and solutions for common user interface scenarios in RTXUI.

---

## 1. Multi-Tab Navigation Interfaces

To build a multi-tab view, use a state variable to track the active tab index/name, render button selectors, and conditionally display content blocks using the `<if>` tag.

```cpp
#include <rtxui/rtxui.hpp>

class TabbedApp : public rtxui::Component<TabbedApp> {
 public:
  std::string active_tab = "home";

  void InitReflection() override {
    Bind(active_tab);
    Import<rtxui::div>();
    Import<rtxui::button>();
    Component<TabbedApp>::InitReflection();
  }

  std::string_view view = R"html(
    <div class="container">
      <!-- Tab Header -->
      <div class="tabs-header">
        <button class="tab-btn {active_tab == 'home' ? 'active' : ''}" onclick="{active_tab = 'home'}">Home</button>
        <button class="tab-btn {active_tab == 'settings' ? 'active' : ''}" onclick="{active_tab = 'settings'}">Settings</button>
        <button class="tab-btn {active_tab == 'about' ? 'active' : ''}" onclick="{active_tab = 'about'}">About</button>
      </div>

      <!-- Tab Content Area -->
      <div class="tab-content">
        <if condition="{active_tab == 'home'}">
          <div>Welcome to the Home Screen!</div>
        </if>
        <elif condition="{active_tab == 'settings'}">
          <div>Configure your RTXUI application settings here.</div>
        </elif>
        <else>
          <div>RTXUI - Reactive Terminal UI Engine v1.0</div>
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
};
```

---

## 2. Asynchronous Background Tasks (Thread Safety)

Since RTXUI's UI logic is run on the main thread, performing heavy blocking operations (like downloading a file or doing a heavy database search) directly inside a callback will freeze the terminal render loop. 

To run tasks in the background, spin up a `std::thread`, and then post the results back to the main thread using `task::TaskRunner::Current()`.

```cpp
#include <rtxui/rtxui.hpp>
#include <thread>
#include <chrono>

class AsyncApp : public rtxui::Component<AsyncApp> {
 public:
  std::string status = "Idle";
  bool is_loading = false;

  void InitReflection() override {
    Bind(status);
    Bind(is_loading);
    Import<rtxui::div>();
    Import<rtxui::button>();
    Component<AsyncApp>::InitReflection();
  }

  void StartBackgroundTask() {
    if (is_loading) return;

    status = "Fetching data...";
    is_loading = true;

    // Get the main loop task runner pointer
    auto* main_runner = task::TaskRunner::Current();

    // Spawn a worker thread
    std::thread([this, main_runner]() {
      // Simulate heavy asynchronous network or disk work
      std::this_thread::sleep_for(std::chrono::seconds(2));

      // Use the main thread runner to safely update state and redraw the UI
      main_runner->PostTask([this]() {
        this->status = "Successfully loaded 42 items!";
        this->is_loading = false;
        // The Screen loop detects state changes during Digest and automatically repaints!
      });
    }).detach();
  }

  std::string_view view = R"html(
    <div class="panel">
      <div>Status: <strong>{status}</strong></div>
      <if condition="{is_loading}">
        <div class="loader">Processing...</div>
      </if>
      <else>
        <button onclick="StartBackgroundTask">Trigger Load</button>
      </else>
    </div>

    <style>
      .panel { padding: 1; border: solid; width: 40; }
      .loader { color: #f59e0b; }
      button { background-color: #10b981; color: #000; padding: 0 1; cursor: pointer; }
    </style>
  )html";
};
```

---

## 3. Creating Modal Dialogs & Overlays

To overlay alerts, popup windows, or dropdown context menus on top of existing components, use `position: absolute` or `fixed` combined with `z-index`.

```cpp
#include <rtxui/rtxui.hpp>

class ModalApp : public rtxui::Component<ModalApp> {
 public:
  bool show_modal = false;

  void InitReflection() override {
    Bind(show_modal);
    Import<rtxui::div>();
    Import<rtxui::button>();
    Component<ModalApp>::InitReflection();
  }

  std::string_view view = R"html(
    <div class="main-screen">
      <div>Main application contents.</div>
      <button onclick="{show_modal = true}">Open Dialog</button>

      <!-- Absolute Modal Overlay -->
      <if condition="{show_modal}">
        <div class="modal-backdrop">
          <div class="modal-card">
            <h3>Confirm Action</h3>
            <p>Are you sure you want to perform this action?</p>
            <div class="modal-actions">
              <button class="confirm-btn" onclick="{show_modal = false}">Confirm</button>
              <button onclick="{show_modal = false}">Cancel</button>
            </div>
          </div>
        </div>
      </if>
    </div>

    <style>
      .main-screen { position: relative; width: 60; height: 10; padding: 1; border: double; }
      
      .modal-backdrop {
        position: absolute;
        top: 0; left: 0; right: 0; bottom: 0;
        background-color: rgba(15, 23, 42, 0.7);
        display: flex;
        justify-content: center;
        align-items: center;
        z-index: 100;
      }

      .modal-card {
        background-color: #1e293b;
        border: solid;
        border-color: #ef4444;
        padding: 1;
        width: 30;
      }

      .modal-actions { display: flex; flex-direction: row; justify-content: flex-end; margin-top: 1; }
      .confirm-btn { margin-right: 1; background-color: #ef4444; color: #fff; }
    </style>
  )html";
};
```
