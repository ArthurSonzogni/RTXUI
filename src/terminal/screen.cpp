// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include "terminal/screen.hpp"

#include <cerrno>
#include <sys/ioctl.h>
#include <unistd.h>
#include <iostream>

#include "dom/element.hpp"
#include "layout/layout.hpp"
#include "layout/layout_tree_builder.hpp"
#include "layout/physical_fragment.hpp"
#include "paint/paint.hpp"
#include "paint/texture.hpp"
#include "terminal/terminal_input_parser.hpp"

namespace rtxui {

namespace {

void handle_sigwinch(int sig) {}

Element* FindElementAt(const std::shared_ptr<PhysicalFragment>& fragment, int target_x, int target_y) {
  if (!fragment) {
    return nullptr;
  }
  if (target_x < 0 || target_y < 0 || target_x >= fragment->width || target_y >= fragment->height) {
    return nullptr;
  }
  // Traverse children in reverse order (top-most elements first)
  for (auto it = fragment->children.rbegin(); it != fragment->children.rend(); ++it) {
    int rel_x = target_x - it->x;
    int rel_y = target_y - it->y;
    if (auto* found = FindElementAt(it->fragment, rel_x, rel_y)) {
      return found;
    }
  }
  if (fragment->dom_node) {
    return fragment->dom_node;
  }
  return nullptr;
}

ComponentBase* GetOwningComponent(Element* element) {
  while (element) {
    if (element->component()) {
      return const_cast<ComponentBase*>(element->component());
    }
    element = element->Parent();
  }
  return nullptr;
}

ComponentBase* GetAttributeOwnerComponent(Element* element) {
  if (!element) return nullptr;
  if (element->component()) {
    return GetOwningComponent(element->Parent());
  }
  return GetOwningComponent(element);
}

ComponentBase* GetParentComponent(ComponentBase* comp) {
  if (!comp || !comp->Root()) return nullptr;
  return GetOwningComponent(comp->Root()->Parent());
}

} // namespace

Screen::Screen(Ref<ComponentBase> component, std::shared_ptr<TerminalDevice> device)
    : component_(std::move(component)), device_(std::move(device)) {
  if (!device_) {
    device_ = std::make_shared<SystemTerminalDevice>();
  }
  UpdateSize();
  component_->Mount();
  Draw();
}

Screen::~Screen() {}

void Screen::Loop() {
  RawTerminal raw_terminal(device_.get());
  Draw();

  running_ = true;
  while (running_) {
    Step();
  }
}

void Screen::Step() {
  char c;
  int bytes_read = device_->Read(&c, 1);
  if (bytes_read != 1) {
    if (bytes_read == -1 && errno == EINTR) {
      UpdateSize();
    }
    return;
  }

  UpdateSize();
  parser_.Add(c);

  while (auto event = parser_.GetEvent()) {
    HandleEvent(*event);
  }
}

void Screen::Dispatch(Event event) {
  HandleEvent(event);
}

void Screen::HandleEvent(const Event& event) {
  if (event.is<Event::Mouse>()) {
    auto mouse = event.get<Event::Mouse>();
    if (mouse.motion == Event::Mouse::Motion::Pressed &&
        (mouse.button == Event::Mouse::Button::Left ||
         mouse.button == Event::Mouse::Button::Right)) {
      if (root_fragment_) {
        int tx = mouse.x - 1;
        int ty = mouse.y - 1;
        if (auto* clicked_element = FindElementAt(root_fragment_, tx, ty)) {
          std::vector<std::string> attr_keys;
          if (mouse.button == Event::Mouse::Button::Left) {
            attr_keys = {"onclick", "@click.left", "@click"};
          } else {
            attr_keys = {"oncontextmenu", "@click.right"};
          }

          Element* curr = clicked_element;
          bool handled = false;
          while (curr) {
            std::string action;
            const auto& attrs = curr->Attributes();
            for (const auto& key : attr_keys) {
              if (attrs.count(key)) {
                action = attrs.at(key);
                break;
              }
            }

            if (!action.empty()) {
              ComponentBase* comp = GetAttributeOwnerComponent(curr);
              bool executed = false;
              while (comp) {
                if (comp->RunCallback(action)) {
                  DigestAndDraw();
                  handled = true;
                  executed = true;
                  break;
                }
                comp = GetParentComponent(comp);
              }
              if (executed) {
                break;
              }
            }
            curr = curr->Parent();
          }
          if (handled) {
            return;
          }
        }
      }
    }
  }

  if (component_->OnEvent(event)) {
    DigestAndDraw();
    return;
  }
  if (event == Event::Escape() || event == Event::CtrlC()) {
    running_ = false;
    return;
  }
}

void Screen::Draw() {
  auto root = component_->Root();
  auto root_box = LayoutTreeBuilder::Build(root);

  LayoutConstraints viewport = {
      {width_, MeasureMode::Exactly},
      {height_, MeasureMode::Exactly},
  };
  auto root_fragment = RunLayout({root_box.get()}, viewport);
  root_fragment_ = root_fragment;

  Texture texture(width_, height_);
  Paint(root_fragment.get(), texture);

  std::string new_output = texture.Render();

  if (has_drawn_ && last_height_ > 0) {
    // Reset cursor up by the number of printed lines
    device_->Write("\x1b[" + std::to_string(last_height_) + "A");
  }

  device_->Write(new_output);

  last_height_ = 0;
  for (char ch : new_output) {
    if (ch == '\n') {
      last_height_++;
    }
  }
  has_drawn_ = true;
}

void Screen::UpdateSize() {
  int new_width = width_;
  int new_height = height_;
  if (device_->GetSize(new_width, new_height)) {
    if (new_width != width_ || new_height != height_) {
      width_ = new_width;
      height_ = new_height;
      if (has_drawn_) {
        device_->Write("\x1b[2J\x1b[H");
        last_height_ = 0;
        component_->Render();
        Draw();
      }
    }
  }
}

void Screen::DigestAndDraw() {
  if (component_->Digest()) {
    Draw();
  }
}

// --- RawTerminal RAII Implementation ---

Screen::RawTerminal::RawTerminal(TerminalDevice* device) : device_(device) {
  if (device_ && device_->IsAtty()) {
    device_->EnterRawMode(handle_sigwinch);
  }
}

Screen::RawTerminal::~RawTerminal() {
  if (device_ && device_->IsAtty()) {
    device_->ExitRawMode();
  }
}

} // namespace rtxui
