#include "terminal/screen.hpp"
#include "terminal/terminal_device.hpp"
#include "component/component.hpp"
#include "component/default_components.hpp"
#include "catch2/catch_test_macros.hpp"
#include <memory>

namespace rtxui {
namespace {

class DummyComponent : public Component<DummyComponent> {
 public:
  void InitReflection() override {
    Import<rtxui::div>();
    Component<DummyComponent>::InitReflection();
  }
  std::string_view view = R"(
    <div>Hello Mock</div>
  )";
};

TEST_CASE("TerminalDevice.MockIOWrites", "[terminal]") {
  auto device = std::make_shared<MockTerminalDevice>();
  auto component = Ref<DummyComponent>::New();
  
  Screen screen(component, device);
  
  // Verify that drawing wrote the expected component output to the mock terminal device.
  std::string output = device->GetOutput();
  REQUIRE_FALSE(output.empty());
  REQUIRE(output.find("Hello Mock") != std::string::npos);
}

TEST_CASE("TerminalDevice.ResizeTrigger", "[terminal]") {
  auto device = std::make_shared<MockTerminalDevice>();
  auto component = Ref<DummyComponent>::New();
  
  Screen screen(component, device);
  device->ClearOutput();
  
  // Set size to a new dimension and check if resizing updates width/height.
  device->TriggerResize(100, 30);
  
  // Screen size can be updated manually via Screen::UpdateSize or inside the event loop.
  // We will test direct Step-by-Step loop size updates in Step 2.
  int width = 0, height = 0;
  device->GetSize(width, height);
  REQUIRE(width == 100);
  REQUIRE(height == 30);
}

} // namespace
} // namespace rtxui
