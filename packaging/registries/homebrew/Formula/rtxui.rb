class Rtxui < Formula
  desc "C++ terminal user interfaces built from HTML templates, CSS and reactive state"
  homepage "https://github.com/ArthurSonzogni/RTXUI"
  url "https://github.com/ArthurSonzogni/RTXUI/archive/refs/tags/v@VERSION@.tar.gz"
  sha256 "@SHA256@"
  license "MIT"
  head "https://github.com/ArthurSonzogni/RTXUI.git", branch: "main"

  depends_on "cmake" => [:build, :test]
  depends_on "ninja" => :build

  # RTXUI needs C++23: GCC 13 or newer on Linux.
  fails_with :gcc do
    version "12"
    cause "Requires C++23"
  end

  def install
    system "cmake", "-S", ".", "-B", "build", "-G", "Ninja",
                    "-DBUILD_SHARED_LIBS=ON",
                    "-DRTXUI_BUILD_TESTS=OFF",
                    "-DRTXUI_BUILD_EXAMPLES=OFF",
                    *std_cmake_args
    system "cmake", "--build", "build"
    system "cmake", "--install", "build"
  end

  test do
    (testpath/"CMakeLists.txt").write <<~CMAKE
      cmake_minimum_required(VERSION 3.24)
      project(app LANGUAGES CXX)
      find_package(rtxui CONFIG REQUIRED)
      add_executable(app main.cpp)
      target_link_libraries(app PRIVATE rtxui::rtxui)
    CMAKE
    (testpath/"main.cpp").write <<~CPP
      #include <rtxui/rtxui.hpp>
      #include <string_view>

      class App : public rtxui::Component<App> {
       public:
        int count = 0;
        void Bump() { count++; }
        std::string_view view = R"(<button onclick="Bump">clicked {count}</button>)";
        App() {
          Bind(count);
          Bind(Bump);
        }
      };

      int main() {
        rtxui::HeadlessScreen screen(rtxui::Ref<App>::New(), 30, 3);
        screen.Input("\\t\\r");
        return screen.Text().contains("clicked 1") ? 0 : 1;
      }
    CPP
    system "cmake", "-S", ".", "-B", "build", *std_cmake_args
    system "cmake", "--build", "build"
    system "./build/app"
  end
end
