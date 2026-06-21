#include <rtxui/rtxui.hpp>
#include <string>

using namespace rtxui;

class ConditionalApp : public Component<ConditionalApp> {
 public:
  int mode = 0;

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

  std::string_view view = R"html(
      <div class="container">
        <div class="tabs">
          <button onclick="SetHome" class="{home_class}">Home</button>
          <button onclick="SetSettings" class="{settings_class}">Settings</button>
          <button onclick="SetAbout" class="{about_class}">About</button>
        </div>

        <div class="content">
          <if condition="{is_home}">
            <h1>Welcome to the Void</h1>
            <p>You have reached the home screen. It's safe here. Probably.</p>
            <div class="card">
              <span>Daily Fortune:</span>
              <i>"Your terminal will never betray you, unless you forget a semicolon."</i>
            </div>
          </if>

          <if condition="{is_settings}">
            <h1>Bureaucracy Settings</h1>
            <p>Fine-tune your existential dread and terminal aesthetics.</p>
            
            <div class="card">
              <span>Gravity Level (Local)</span>
              <button>Normalize</button>
            </div>
            
            <div class="card">
              <span>Infinite Loop Protection</span>
              <button>Disable (Risky!)</button>
            </div>

            <div class="card">
              <span>Coffee Intensity</span>
              <button>Maximum</button>
            </div>
          </if>

          <if condition="{is_about}">
            <h1>About RTXUI</h1>
            <p>A reactive terminal UI library so fast it might actually finish your project for you.</p>
            <div class="card">
              <span>Fun Fact #42:</span>
              <span>RTXUI was originally developed to communicate with deep-space probes that only support ASCII.</span>
            </div>
            <div class="card">
              <span>Disclaimer:</span>
              <span>Side effects may include excessive use of <span style="color: #6366f1;">Indigo</span> and a sudden urge to refactor everything.</span>
            </div>
          </if>

          <div if="{is_home}" class="footer">
            Watching you from the bottom of the stack...
          </div>
        </div>
      </div>

      <style>
        self {
          display: block;
          padding: 2;
          background-color: rgb(18, 18, 18);
          color: #eee;
          overflow-y: scroll;

          .container {
            max-width: 80;
            margin: 0 auto;
          }

          button {
            border: vkey;
            border-color-left: rgb(71, 85, 105);
            border-color-right: rgb(10, 10, 10);
            background-color: rgb(30, 41, 59);
            color: white;
            padding: 0 1;
            opacity: 0.6;
            transition: all 0.2s linear;

            &:hover, &:focus, &.active {
              opacity: 1.0;
            }
            &.active {
              font-weight: bold;
            }
          }

          .content {
            border: vkey;
            border-color-left: rgb(71, 85, 105);
            border-color-right: rgb(10, 10, 10);
            background-color: rgb(30, 41, 59);
            padding: 1;
            min-height: 15;

            .card {
              border: tall;
              border-color: rgb(51, 65, 85);
              background-color: rgb(30, 41, 59);
              padding: 1;
              margin-top: 1;
              display: flex;
              justify-content: space-between;
            }
          }

          .footer {
            margin-top: 2;
            color: rgb(148, 163, 184);
            font-style: italic;
          }
        }
      </style>
    )html";

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
};

int main() {
  auto app = Ref<ConditionalApp>::New();
  Screen screen(app);
  screen.Loop();
  return 0;
}
