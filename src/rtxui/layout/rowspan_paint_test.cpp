#include <catch2/catch_all.hpp>
#include <iostream>
#include <map>
#include <string>
#include <vector>
#include "rtxui/rtxui.hpp"
#include "rtxui/component/default_components_internal.hpp"
#include "rtxui/layout/layout.hpp"
#include "rtxui/layout/layout_tree_builder.hpp"
#include "rtxui/paint/paint.hpp"
#include "rtxui/paint/texture.hpp"

namespace rtxui {
namespace {

std::string GetTextLayer(const Texture& texture) {
  std::string out;
  for (int y = 0; y < texture.height(); ++y) {
    for (int x = 0; x < texture.width(); ++x) {
      const auto& cell = const_cast<Texture&>(texture)[x, y];
      if (cell.is_continuation) {
        out += "^";
      } else if (cell.character.empty()) {
        out += " ";
      } else {
        out += cell.character;
      }
    }
    out += "\n";
  }
  return out;
}

std::string GetColorLayer(const Texture& texture,
                          bool background,
                          const std::map<Color, char>& color_map) {
  std::string out;
  for (int y = 0; y < texture.height(); ++y) {
    for (int x = 0; x < texture.width(); ++x) {
      const auto& cell = const_cast<Texture&>(texture)[x, y];
      Color c = background ? cell.background_color : cell.foreground_color;
      if (c.a == 0) {
        out += ".";
      } else {
        auto it = color_map.find(c);
        out += (it != color_map.end()) ? it->second : '?';
      }
    }
    out += "\n";
  }
  return out;
}

} // namespace

TEST_CASE("Layout: Rowspan painting order regression test", "[layout][table]") {
  struct RowspanPaintTest : Component<RowspanPaintTest> {
    std::string_view Setup() override {
      return R"html(
        <style>
          self { display: block; width: 10; height: 2; }
          table { display: block; width: 10; height: 2; border: none; padding: 0; margin: 0; }
          tr { display: block; padding: 0; margin: 0; border: none; }
          td { padding: 0; margin: 0; border: none; }
          .green { background-color: rgb(0, 255, 0); }
          .red { background-color: rgb(255, 0, 0); }
        </style>
        <table>
          <tr>
            <td rowspan="2" class="green">S</td>
            <td>A</td>
          </tr>
          <tr class="red">
            <td>B</td>
          </tr>
        </table>
      )html";
    }
  };

  auto app = Ref<RowspanPaintTest>::New();
  app->Mount();
  
  auto layout_box = LayoutTreeBuilder::Build(app->Root());
  
  LayoutConstraints constraints;
  constraints.width = {10, MeasureMode::Exactly};
  constraints.height = {2, MeasureMode::Exactly};
  auto fragment = RunLayout({layout_box.get()}, constraints);
  
  Texture texture(10, 2);
  Paint(fragment.get(), texture);

  std::map<Color, char> colors = {
      {Color::RGB(0, 255, 0), 'G'},
      {Color::RGB(255, 0, 0), 'R'},
  };
  std::string bg_grid = GetColorLayer(texture, true, colors);
  std::string text_grid = GetTextLayer(texture);
  
  INFO("BG Grid:\n" << bg_grid);
  INFO("Text Grid:\n" << text_grid);

  // Row 1, Col 0: 'S' with Green BG
  CHECK(texture[0, 0].character == "S");
  CHECK(texture[0, 0].background_color == Color::RGB(0, 255, 0));
  
  // Row 2, Col 0: Should be Green BG from the rowspan, NOT red from row 2
  CHECK(texture[0, 1].background_color == Color::RGB(0, 255, 0));
  
  // Row 2, Col 1: 'B' with Red BG (inherited from row 2)
  CHECK(texture[5, 1].character == "B");
  CHECK(texture[5, 1].background_color == Color::RGB(255, 0, 0));
}

} // namespace rtxui
