# Screen Runtime & Lifecycle

The runtime engine of an RTXUI application is managed by the `rtxui::Screen` class.

## The Main Loop

To launch your application, mount your root component onto a `Screen` instance and invoke the `.Loop()` method. This initializes the terminal, configures mouse/keyboard event listeners, and blocks until an exit signal (e.g. `Ctrl-C` or closing input) is received:

```cpp
#include <rtxui/rtxui.hpp>

int main() {
  auto root = std::make_shared<MyRootComponent>();
  
  // Creates standard output screen device
  rtxui::Screen screen(root);
  
  // Enters blocking event loop
  screen.Loop();
  
  return 0;
}
```

## The Digest Lifecycle

The render loop behaves similarly to web frameworks:
1.  **Event Dispatching**: The screen listens to raw ANSI keyboard/mouse sequences, compiles them into a standard `rtxui::Event`, and propagates it down the DOM tree.
2.  **State Mutation**: Handlers/callbacks mutate registered bindings.
3.  **Draw Iteration**: The layout tree re-calculates styles, reflows flex boundaries, and repaints modified regions.

### Manual Refreshes
If you modify state variables outside of template-driven event listeners (such as from a background thread or timers), you must manually run a repaint loop iteration by calling `Draw()` on the screen:

```cpp
// E.g., on a background thread callback
void OnNetworkUpdate(rtxui::Screen& screen) {
  UpdateSharedState();
  screen.Draw(); // Force a layout digest cycle and repaint
}
```
