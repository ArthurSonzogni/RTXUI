#include <rtxui/rtxui.hpp>
#include <string>

using namespace rtxui;

class ConditionalApp : public Component<ConditionalApp> {
 public:
  int mode = 0; // 0: Home, 1: Settings, 2: About

  ConditionalApp() {
    Bind(mode);
    Import("SetHome", [this]() { mode = 0; });
    Import("SetSettings", [this]() { mode = 1; });
    Import("SetAbout", [this]() { mode = 2; });
  }

  std::string_view Setup() override {
    return R"html(
      <div class="container">
        <div class="tabs">
          <button onclick="SetHome" class="{mode == 0 ? 'active' : ''}">Home</button>
          <button onclick="SetSettings" class="{mode == 1 ? 'active' : ''}">Settings</button>
          <button onclick="SetAbout" class="{mode == 2 ? 'active' : ''}">About</button>
        </div>

        <div class="content">
          <if condition="{mode == 0}">
            <h1>Welcome Home!</h1>
            <p>This is the home screen of the conditional rendering demo.</p>
          </if>
          <elif condition="{mode == 1}">
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

          <div if="{mode == 0}" class="footer">
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

  std::string GetInterpolatedValue(std::string_view expr) override {
    if (expr == "mode == 0") return mode == 0 ? "true" : "false";
    if (expr == "mode == 1") return mode == 1 ? "true" : "false";
    if (expr == "mode == 2") return mode == 2 ? "true" : "false";
    
    // Ternary-like logic for classes
    if (expr == "mode == 0 ? 'active' : ''") return mode == 0 ? "active" : "";
    if (expr == "mode == 1 ? 'active' : ''") return mode == 1 ? "active" : "";
    if (expr == "mode == 2 ? 'active' : ''") return mode == 2 ? "active" : "";

    return Component<ConditionalApp>::GetInterpolatedValue(expr);
  }
};

int main() {
  auto app = Ref<ConditionalApp>::New();
  Screen screen(app);
  screen.Loop();
  return 0;
}
