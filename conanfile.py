from conan import ConanFile
from conan.tools.cmake import CMake, CMakeToolchain, cmake_layout

class RtxuiConan(ConanFile):
    name = "rtxui"
    version = "0.1.0"
    license = "MIT"
    author = "Arthur Sonzogni <sonzogniarthur@gmail.com>"
    url = "https://github.com/ArthurSonzogni/RTXUI"
    description = "C++ reactive terminal rendering"
    topics = ("tui", "terminal", "html", "css", "reactive", "cpp23")
    settings = "os", "compiler", "build_type", "arch"
    options = {"shared": [True, False], "fPIC": [True, False]}
    default_options = {"shared": False, "fPIC": True}

    def config_options(self):
        if self.settings.os == "Windows":
            del self.options.fPIC

    def layout(self):
        cmake_layout(self)

    def generate(self):
        tc = CMakeToolchain(self)
        tc.variables["RTXUI_BUILD_TESTS"] = False
        tc.variables["RTXUI_BUILD_EXAMPLES"] = False
        tc.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        cmake = CMake(self)
        cmake.install()

    def package_info(self):
        self.cpp_info.libs = ["rtxui", "rtxui_lib"]
        self.cpp_info.set_property("cmake_file_name", "rtxui")
        self.cpp_info.set_property("cmake_target_name", "rtxui::rtxui")
