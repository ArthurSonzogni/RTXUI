#include <iostream>
#include <rtxui/rtxui.hpp>
#include <string>
#include <string_view>

class App : public rtxui::Component<App> {
 public:
  std::string_view view = R"html(<checkbox>check</checkbox>)html";
};

int main() {
  rtxui::HeadlessScreen screen(rtxui::Ref<App>::New(), 20, 1);
  std::cout << screen.Text() << "\n";
  // The built-in <checkbox> draws its box; without it, only the text shows.
  return screen.Text().find("☐") == std::string::npos;
}
