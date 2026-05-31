#include <rtxui/rtxui.hpp>
#include <string>

using namespace rtxui;

class ConditionalApp : public Component<ConditionalApp> {
 public:
  int mode = 0; // 0: Home, 1: Settings, 2: About

  ConditionalApp() {
    Bind(mode);
    Bind(is_home);
    Bind(is_settings);
    Bind(is_about);
    Bind(home_class);
    Bind(settings_class);
    Bind(about_class);
    Bind(SetHome);
    Bind(SetSettings);
    Bind(SetAbout);
  }

  // Computed properties
  bool is_home() const { return mode == 0; }
  bool is_settings() const { return mode == 1; }
  bool is_about() const { return mode == 2; }

  std::string home_class() const { return mode == 0 ? "active" : ""; }
  std::string settings_class() const { return mode == 1 ? "active" : ""; }
  std::string about_class() const { return mode == 2 ? "active" : ""; }

  // Event handlers
  void SetHome() { mode = 0; }
  void SetSettings() { mode = 1; }
  void SetAbout() { mode = 2; }

  std::string_view Setup() override {
    return R"html(
      <div class="container">
        <div class="tabs">
          <button onclick="SetHome" class="{home_class}">Home</button>
          <button onclick="SetSettings" class="{settings_class}">Settings</button>
          <button onclick="SetAbout" class="{about_class}">About</button>
        </div>

        <div class="content">
          <if condition="{is_home}">
            <h1>Welcome Home!</h1>
            <p>This is the home screen of the conditional rendering demo.</p>
          </if>
          <elif condition="{is_settings}">
            <h1>Settings</h1>
            <p>Here you can configure your application.</p>
            <div class="card">
              <span>Notification Settings</span>
              <button>Toggle</button>
            </div>
          </elif>
          <else>
            <h1>About</h1>
            <p>RTXUI is a reactive terminal UI library for C++.</p>
          </else>

          <div if="{is_home}" class="footer">
            Home-specific footer content
          </div>
        </div>
      </div>

      <style>
        .container { padding: 1; display: flex; flex-direction: column; gap: 1; }
        .tabs { display: flex; gap: 1; }
        .tabs button { border: solid; padding: 0 1; }
        .tabs button.active { background-color: blue; color: white; }
        .content { border: wide; border-color: gray; padding: 1; min-height: 10; }
        .card { border: solid; padding: 1; margin-top: 1; display: flex; justify-content: space-between; }
        .footer { margin-top: 2; color: gray; font-style: italic; }
      </style>
    )html";
  }
};

int main() {
  auto app = Ref<ConditionalApp>::New();
  Screen screen(app);
  screen.Loop();
  return 0;
}
