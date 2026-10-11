# Modules

> [!WARNING]
> This feature is still in development, and the API may change in future releases.
> Your contribution is needed to help us improve the compatibility and usability
> of C++ modules in RTXUI. If you encounter any issues or have suggestions,
> please open an issue.

RTXUI experimentally supports
[C++20 modules](https://en.cppreference.com/w/cpp/language/modules).

Use the `RTXUI_BUILD_MODULE` option to build the RTXUI project itself to provide the module,
for example with CMake and Ninja:

```sh
cmake \
    -DCMAKE_GENERATOR=Ninja \
    -DRTXUI_BUILD_MODULE=ON \
    ..

ninja
```

> [!NOTE]
> To use modules, you need a CMake version 3.28 or higher,
> and a compatible generator like Ninja. Note that Makefile
> generators **do not support modules**.

Then, in your own code you can consume the modules and code as normal:

```cpp
import rtxui;

using std::string_view;
using rtxui::Component;
using rtxui::Ref;
using rtxui::Screen;

class HelloWorldApp : public Component<HelloWorldApp> {
public:
  string_view view = R"html(
      <div class="card">
        Hello World from RTXUI!
      </div>
      <style>
        self {
          display: block;
          padding: 1;
          background-color: rgb(18, 18, 18);
        }
        .card {
          border: tall;
          border-color: rgb(59, 130, 246);
          padding: 1;
          color: rgb(241, 245, 249);
        }
      </style>
    )html";
};

int main() {
  Ref<HelloWorldApp> app = Ref<HelloWorldApp>::New();
  Screen screen(app);
  screen.Loop();
  return 0;
}
```

To properly find and link the modules with CMake, use `target_link_libraries` to get the right
compiler, linker, etc. flags.

```cmake
target_link_libraries(my_executable
    #...whatever...
    PRIVATE rtxui::module
)
```
