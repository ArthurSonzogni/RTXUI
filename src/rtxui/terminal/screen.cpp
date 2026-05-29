// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include "rtxui/terminal/screen.hpp"

#include <cerrno>
#include <sys/ioctl.h>
#include <unistd.h>
#include <iostream>
#include <algorithm>

#include "rtxui/dom/element.hpp"
#include "rtxui/layout/layout.hpp"
#include "rtxui/layout/layout_tree_builder.hpp"
#include "rtxui/layout/physical_fragment.hpp"
#include "rtxui/paint/paint.hpp"
#include "rtxui/paint/texture.hpp"
#include "rtxui/terminal/terminal_input_parser.hpp"

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
    if (fragment->clips_descendants) {
      rel_y += fragment->scroll_y;
    }
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

std::shared_ptr<PhysicalFragment> FindScrollableFragmentAt(
    const std::shared_ptr<PhysicalFragment>& fragment, int target_x, int target_y) {
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
    if (fragment->clips_descendants) {
      rel_x += fragment->scroll_x;
      rel_y += fragment->scroll_y;
    }
    if (auto found = FindScrollableFragmentAt(it->fragment, rel_x, rel_y)) {
      return found;
    }
  }
  if (fragment->dom_node && (fragment->dom_node->style.overflow_y == Overflow::Scroll ||
                             fragment->dom_node->style.overflow_x == Overflow::Scroll)) {
    return fragment;
  }
  return nullptr;
}

std::shared_ptr<PhysicalFragment> FindFragmentForElement(
    const std::shared_ptr<PhysicalFragment>& fragment, Element* element) {
  if (!fragment) return nullptr;
  if (fragment->dom_node == element) {
    return fragment;
  }
  for (const auto& child : fragment->children) {
    if (auto found = FindFragmentForElement(child.fragment, element)) {
      return found;
    }
  }
  return nullptr;
}

std::shared_ptr<PhysicalFragment> FindFirstScrollableFragment(
    const std::shared_ptr<PhysicalFragment>& fragment) {
  if (!fragment) return nullptr;
  if (fragment->dom_node && (fragment->dom_node->style.overflow_y == Overflow::Scroll ||
                             fragment->dom_node->style.overflow_x == Overflow::Scroll)) {
    return fragment;
  }
  for (const auto& child : fragment->children) {
    if (auto found = FindFirstScrollableFragment(child.fragment)) {
      return found;
    }
  }
  return nullptr;
}

} // namespace

Screen::Screen(Ref<ComponentBase> component, std::shared_ptr<TerminalDevice> device)
    : component_(std::move(component)), device_(std::move(device)) {
  if (!device_) {
    device_ = std::make_shared<SystemTerminalDevice>();
  }
  UpdateSize();
  component_->Mount();
  component_->Digest();
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
          if (component_->Root()) {
            component_->Root()->Visit([](Element& el) {
              el.set_focused(false);
            });
          }
          focused_element_ = clicked_element;
          focused_element_->set_focused(true);
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
    } else if (mouse.button == Event::Mouse::Button::WheelUp ||
               mouse.button == Event::Mouse::Button::WheelDown ||
               mouse.button == Event::Mouse::Button::WheelLeft ||
               mouse.button == Event::Mouse::Button::WheelRight) {
      if (root_fragment_) {
        int tx = mouse.x - 1;
        int ty = mouse.y - 1;
        if (auto scroll_frag = FindScrollableFragmentAt(root_fragment_, tx, ty)) {
          Element* curr = scroll_frag->dom_node;
          while (curr) {
            bool is_horizontal_wheel = (mouse.button == Event::Mouse::Button::WheelLeft ||
                                        mouse.button == Event::Mouse::Button::WheelRight);
            if (curr->style.overflow_y == Overflow::Scroll && !is_horizontal_wheel) {
              auto frag = FindFragmentForElement(root_fragment_, curr);
              if (frag) {
                int scroll_height = curr->scroll_height();
                int max_scroll = std::max(0, scroll_height - frag->height);
                int curr_y = curr->scroll_y();
                int speed = curr->style.scroll_speed_y;

                int new_y = curr_y;
                if (mouse.button == Event::Mouse::Button::WheelUp) {
                  new_y = std::max(0, curr_y - speed);
                } else {
                  new_y = std::min(max_scroll, curr_y + speed);
                }

                if (new_y != curr_y) {
                  curr->set_scroll_y(new_y);
                  Draw();
                  return;
                }
              }
            } else if (curr->style.overflow_x == Overflow::Scroll) {
              auto frag = FindFragmentForElement(root_fragment_, curr);
              if (frag) {
                int scroll_width = curr->scroll_width();
                int max_scroll = std::max(0, scroll_width - frag->width);
                int curr_x = curr->scroll_x();
                int speed = curr->style.scroll_speed_x;

                int new_x = curr_x;
                if (mouse.button == Event::Mouse::Button::WheelLeft || mouse.button == Event::Mouse::Button::WheelUp) {
                  new_x = std::max(0, curr_x - speed);
                } else {
                  new_x = std::min(max_scroll, curr_x + speed);
                }

                if (new_x != curr_x) {
                  curr->set_scroll_x(new_x);
                  Draw();
                  return;
                }
              }
            }
            curr = curr->Parent();
          }
        }
      }
    }
  }

  if (component_->OnEvent(event)) {
    DigestAndDraw();
    return;
  }

  if (event == Event::Tab() || event == Event::TabReverse()) {
    std::vector<Element*> focusable;
    auto IsFocusable = [](Element* el) {
      if (!el) return false;
      if (el->Attributes().count("focusable")) {
        std::string val = el->Attributes().at("focusable");
        return (val == "true" || val == "1");
      }
      std::string_view tag = el->tag();
      if (tag == "input" || tag == "textarea" || tag == "checkbox" || tag == "slider" || tag == "button") {
        return true;
      }
      return false;
    };
    std::function<void(Element*)> CollectFocusable = [&](Element* el) {
      if (!el) return;
      if (IsFocusable(el)) {
        focusable.push_back(el);
      }
      for (const auto& child : el->children()) {
        CollectFocusable(child.get());
      }
    };
    if (component_->Root()) {
      CollectFocusable(component_->Root());
    }

    if (!focusable.empty()) {
      int curr_idx = -1;
      for (int i = 0; i < static_cast<int>(focusable.size()); ++i) {
        if (focusable[i]->focused()) {
          curr_idx = i;
          break;
        }
      }

      int next_idx = 0;
      if (event == Event::Tab()) {
        next_idx = (curr_idx == -1) ? 0 : (curr_idx + 1) % focusable.size();
      } else {
        next_idx = (curr_idx == -1) ? (focusable.size() - 1)
                                    : (curr_idx - 1 + focusable.size()) % focusable.size();
      }

      if (component_->Root()) {
        component_->Root()->Visit([](Element& el) {
          el.set_focused(false);
        });
      }
      focusable[next_idx]->set_focused(true);
      focused_element_ = focusable[next_idx];
      DigestAndDraw();
      return;
    }
  }

  if (event == Event::ArrowUp() || event == Event::ArrowDown() ||
      event == Event::PageUp() || event == Event::PageDown() ||
      event == Event::ArrowLeft() || event == Event::ArrowRight()) {
    bool is_horiz = (event == Event::ArrowLeft() || event == Event::ArrowRight());
    if (focused_element_) {
      Element* curr = focused_element_;
      while (curr) {
        if (is_horiz) {
          if (curr->style.overflow_x == Overflow::Scroll) {
            auto scroll_frag = FindFragmentForElement(root_fragment_, curr);
            if (scroll_frag) {
              int scroll_width = curr->scroll_width();
              int max_scroll = std::max(0, scroll_width - scroll_frag->width);
              int curr_x = curr->scroll_x();
              int speed = curr->style.scroll_speed_x;

              int delta = (event == Event::ArrowLeft()) ? -speed : speed;
              int new_x = std::clamp(curr_x + delta, 0, max_scroll);
              if (new_x != curr_x) {
                curr->set_scroll_x(new_x);
                Draw();
                return;
              }
            }
          }
        } else {
          if (curr->style.overflow_y == Overflow::Scroll) {
            auto scroll_frag = FindFragmentForElement(root_fragment_, curr);
            if (scroll_frag) {
              int scroll_height = curr->scroll_height();
              int max_scroll = std::max(0, scroll_height - scroll_frag->height);
              int curr_y = curr->scroll_y();
              int speed = curr->style.scroll_speed_y;

              int delta = 0;
              if (event == Event::ArrowUp()) {
                delta = -speed;
              } else if (event == Event::ArrowDown()) {
                delta = speed;
              } else if (event == Event::PageUp()) {
                delta = -scroll_frag->height;
              } else if (event == Event::PageDown()) {
                delta = scroll_frag->height;
              }

              int new_y = std::clamp(curr_y + delta, 0, max_scroll);
              if (new_y != curr_y) {
                curr->set_scroll_y(new_y);
                Draw();
                return;
              }
            }
          }
        }
        curr = curr->Parent();
      }
    }

    if (auto scroll_frag = FindFirstScrollableFragment(root_fragment_)) {
      if (is_horiz) {
        if (scroll_frag->dom_node->style.overflow_x == Overflow::Scroll) {
          int scroll_width = scroll_frag->dom_node->scroll_width();
          int max_scroll = std::max(0, scroll_width - scroll_frag->width);
          int curr_x = scroll_frag->dom_node->scroll_x();
          int speed = scroll_frag->dom_node->style.scroll_speed_x;

          int delta = (event == Event::ArrowLeft()) ? -speed : speed;
          int new_x = std::clamp(curr_x + delta, 0, max_scroll);
          if (new_x != curr_x) {
            scroll_frag->dom_node->set_scroll_x(new_x);
            Draw();
            return;
          }
        }
      } else {
        if (scroll_frag->dom_node->style.overflow_y == Overflow::Scroll) {
          int scroll_height = scroll_frag->dom_node->scroll_height();
          int max_scroll = std::max(0, scroll_height - scroll_frag->height);
          int curr_y = scroll_frag->dom_node->scroll_y();
          int speed = scroll_frag->dom_node->style.scroll_speed_y;

          int delta = 0;
          if (event == Event::ArrowUp()) {
            delta = -speed;
          } else if (event == Event::ArrowDown()) {
            delta = speed;
          } else if (event == Event::PageUp()) {
            delta = -scroll_frag->height;
          } else if (event == Event::PageDown()) {
            delta = scroll_frag->height;
          }

          int new_y = std::clamp(curr_y + delta, 0, max_scroll);
          if (new_y != curr_y) {
            scroll_frag->dom_node->set_scroll_y(new_y);
            Draw();
            return;
          }
        }
      }
    }
  }

  if (event == Event::Escape() || event == Event::CtrlC()) {
    running_ = false;
    return;
  }
}

void Screen::Draw() {
  auto root = component_->Root();

  focused_element_ = nullptr;
  if (root) {
    std::function<void(Element*)> FindFocused = [&](Element* el) {
      if (!el) return;
      if (el->focused()) {
        focused_element_ = el;
        return;
      }
      for (size_t i = 0; i < el->ChildCount(); ++i) {
        FindFocused(el->ChildAt(i));
        if (focused_element_) return;
      }
    };
    FindFocused(root);
  }

  auto root_box = LayoutTreeBuilder::Build(root);

  LayoutConstraints viewport = {
      {width_, MeasureMode::Exactly},
      {height_, MeasureMode::Exactly},
  };
  std::shared_ptr<PhysicalFragment> root_fragment = nullptr;
  if (root_box) {
    root_fragment = RunLayout({root_box.get()}, viewport);
  }
  root_fragment_ = root_fragment;

  Texture texture(width_, height_);
  if (root_fragment) {
    Paint(root_fragment.get(), texture);
  }

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
