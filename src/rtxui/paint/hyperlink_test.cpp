// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include <catch2/catch_test_macros.hpp>
#include <memory>
#include <string>

#include "rtxui/component.hpp"
#include "rtxui/paint/texture.hpp"
#include "rtxui/screen.hpp"
#include "rtxui/terminal/terminal_device.hpp"

namespace rtxui {
namespace {

constexpr std::string_view kClose = "\x1B]8;;\x1B\\";

std::string Open(std::string_view url) {
  return "\x1B]8;;" + std::string(url) + "\x1B\\";
}

// What `output` writes between opening the link to `url` and closing it,
// style changes included. Empty when it never opens that link.
std::string Linked(const std::string& output, std::string_view url) {
  const size_t open = output.find(Open(url));
  if (open == std::string::npos) {
    return "";
  }
  const size_t start = open + Open(url).size();
  return output.substr(start, output.find(kClose, start) - start);
}

TEST_CASE("A texture writes its links as OSC 8 hyperlinks", "[paint][link]") {
  Texture texture(4, 2);
  const uint16_t link = texture.AddLink("https://example.com");
  REQUIRE(link != 0);
  CHECK(texture.AddLink("https://example.com") == link);
  CHECK(texture.LinkUrl(link) == "https://example.com");
  for (int x = 1; x < 3; ++x) {
    texture[x, 0].character = "a";
    texture[x, 0].link = link;
  }
  // Opened before the first linked cell, closed after the last, and never
  // left open across a line break.
  CHECK(texture.Render() == " " + Open("https://example.com") + "aa" +
                                std::string(kClose) + " \n    ");

  SECTION("redrawn when only the URL changed") {
    Texture next(4, 2);
    const uint16_t other = next.AddLink("https://other.org");
    REQUIRE(other == link);  // Same id, different URL.
    for (int x = 1; x < 3; ++x) {
      next[x, 0].character = "a";
      next[x, 0].link = other;
    }
    CHECK(next.RenderDiff(texture).find(Open("https://other.org")) !=
          std::string::npos);
    CHECK(texture.RenderDiff(texture).find("\x1B]8") == std::string::npos);
  }
}

TEST_CASE("A URL an escape sequence cannot carry is no link", "[paint][link]") {
  Texture texture(1, 1);
  CHECK(texture.AddLink("") == 0);
  CHECK(texture.AddLink("https://a.b/\x1B]0;title\x07") == 0);
  CHECK(texture.AddLink("https://a.b/\xC3\xA9") == 0);
  CHECK(texture.LinkUrl(0).empty());
}

class LinkApp : public Component<LinkApp> {
 public:
  std::string_view view = R"(
    <div>
      <a href="https://example.com/docs">docs</a>
      <a href="#section">jump</a>
      <a href="notes.txt">file</a>
      <a href="mailto:me@example.com">mail <b>me</b></a>
      <section id="section">x</section>
    </div>
  )";
};

TEST_CASE("An <a> with a URL is a terminal hyperlink", "[paint][link]") {
  auto device = std::make_shared<MockTerminalDevice>();
  device->TriggerResize(40, 8);
  Screen screen(Ref<LinkApp>::New(), device);
  screen.Draw();
  const std::string output = device->GetOutput();

  CHECK(Linked(output, "https://example.com/docs").ends_with("docs"));
  // Text nested inside the link belongs to it too.
  const std::string mail = Linked(output, "mailto:me@example.com");
  CHECK(mail.starts_with("mail "));
  CHECK(mail.ends_with("me"));
  // An in-app anchor and a relative path are not URLs the terminal can open.
  CHECK(output.find(Open("#section")) == std::string::npos);
  CHECK(output.find(Open("notes.txt")) == std::string::npos);
}

}  // namespace
}  // namespace rtxui
