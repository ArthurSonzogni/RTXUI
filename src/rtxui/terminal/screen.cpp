// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include "rtxui/internal/screen.hpp"

#include <sys/ioctl.h>
#include <unistd.h>
#ifndef __EMSCRIPTEN__
#include <sys/select.h>
#else
#include <emscripten.h>
#endif

#include <algorithm>
#include <cerrno>
#include <fstream>
#include <iostream>

#include "rtxui/dom/element.hpp"
#include "rtxui/layout/layout.hpp"
#include "rtxui/layout/layout_tree_builder.hpp"
#include "rtxui/layout/physical_fragment.hpp"
#include "rtxui/paint/paint.hpp"
#include "rtxui/paint/texture.hpp"
#include "rtxui/terminal/terminal_device.hpp"
#include "rtxui/terminal/terminal_input_parser.hpp"
#include "rtxui/style/style.hpp"

namespace rtxui {

namespace {

void handle_sigwinch(int sig) {}

Element* FindElementAtImpl(const std::shared_ptr<PhysicalFragment>& fragment,
                           int target_x,
                           int target_y,
                           int accum_scroll_x,
                           int accum_scroll_y) {
  if (!fragment) {
    return nullptr;
  }
  if (target_x < 0 || target_y < 0 || target_x >= fragment->width ||
      target_y >= fragment->height) {
    return nullptr;
  }

  int scroll_x_offset = 0;
  int scroll_y_offset = 0;
  if (fragment->clips_descendants) {
    scroll_x_offset = fragment->scroll_x;
    scroll_y_offset = fragment->scroll_y;
  }

  int next_accum_scroll_x = accum_scroll_x + scroll_x_offset;
  int next_accum_scroll_y = accum_scroll_y + scroll_y_offset;

  // Traverse children in reverse order (top-most elements first)
  for (auto it = fragment->children.rbegin(); it != fragment->children.rend();
       ++it) {
    bool is_fixed = (it->fragment && it->fragment->dom_node &&
                     it->fragment->dom_node->style.position == PositionType::Fixed);

    int rel_x = target_x - it->x;
    int rel_y = target_y - it->y;
    int child_accum_scroll_x = next_accum_scroll_x;
    int child_accum_scroll_y = next_accum_scroll_y;

    if (is_fixed) {
      rel_x -= accum_scroll_x;
      rel_y -= accum_scroll_y;
      child_accum_scroll_x = 0;
      child_accum_scroll_y = 0;
    } else {
      rel_x += scroll_x_offset;
      rel_y += scroll_y_offset;
    }

    if (auto* found = FindElementAtImpl(it->fragment, rel_x, rel_y,
                                        child_accum_scroll_x, child_accum_scroll_y)) {
      return found;
    }
  }

  if (fragment->dom_node) {
    return fragment->dom_node;
  }
  return nullptr;
}

Element* FindElementAt(const std::shared_ptr<PhysicalFragment>& fragment,
                       int target_x,
                       int target_y) {
  return FindElementAtImpl(fragment, target_x, target_y, 0, 0);
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
  if (!element) {
    return nullptr;
  }
  if (element->component()) {
    return GetOwningComponent(element->Parent());
  }
  return GetOwningComponent(element);
}

ComponentBase* GetParentComponent(ComponentBase* comp) {
  if (!comp || !comp->Root()) {
    return nullptr;
  }
  return GetOwningComponent(comp->Root()->Parent());
}

std::shared_ptr<PhysicalFragment> FindScrollableFragmentAt(
    const std::shared_ptr<PhysicalFragment>& fragment,
    int target_x,
    int target_y) {
  if (!fragment) {
    return nullptr;
  }
  if (target_x < 0 || target_y < 0 || target_x >= fragment->width ||
      target_y >= fragment->height) {
    return nullptr;
  }
  // Traverse children in reverse order (top-most elements first)
  for (auto it = fragment->children.rbegin(); it != fragment->children.rend();
       ++it) {
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
  if (fragment->dom_node &&
      (fragment->dom_node->style.overflow_y == Overflow::Scroll ||
       fragment->dom_node->style.overflow_x == Overflow::Scroll)) {
    return fragment;
  }
  return nullptr;
}

std::shared_ptr<PhysicalFragment> FindFragmentForElement(
    const std::shared_ptr<PhysicalFragment>& fragment,
    Element* element) {
  if (!fragment) {
    return nullptr;
  }
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
  if (!fragment) {
    return nullptr;
  }
  if (fragment->dom_node &&
      (fragment->dom_node->style.overflow_y == Overflow::Scroll ||
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

}  // namespace

class ScreenImpl {
 public:
  ScreenImpl(Ref<ComponentBase> component,
             std::shared_ptr<TerminalDevice> device);
  ~ScreenImpl();

  void Loop();
  void Step();
  void Dispatch(Event event);
  void Draw();

  void UpdateSize();
  void DigestAndDraw();
  void HandleEvent(const Event& event);
  bool HasActiveTransitions();
  bool TickTransitions(double current_time_ms);

  Ref<ComponentBase> component_;
  int width_ = 80;
  int height_ = 24;
  int last_height_ = 0;
  bool has_drawn_ = false;
  bool running_ = true;
  std::shared_ptr<PhysicalFragment> root_fragment_;
  std::shared_ptr<TerminalDevice> device_;
  std::unique_ptr<TerminalInputParser> parser_;
  Element* focused_element_ = nullptr;

  struct RawTerminal {
    TerminalDevice* device_ = nullptr;
    explicit RawTerminal(TerminalDevice* device);
    ~RawTerminal();
  };
};

ScreenImpl::ScreenImpl(Ref<ComponentBase> component,
                       std::shared_ptr<TerminalDevice> device)
    : component_(std::move(component)),
      device_(std::move(device)),
      parser_(std::make_unique<TerminalInputParser>()) {
  if (!device_) {
    device_ = std::make_shared<SystemTerminalDevice>();
  }
  UpdateSize();
  css::g_terminal_width = width_;
  css::g_terminal_height = height_;
  component_->Mount();
  component_->Digest();
  Draw();
}

ScreenImpl::~ScreenImpl() {}

void ScreenImpl::Loop() {
  RawTerminal raw_terminal(device_.get());
  Draw();

  running_ = true;
  while (running_) {
    Step();
  }
}

void ScreenImpl::Step() {
#ifdef __EMSCRIPTEN__
  EM_ASM({
    window.rtxui_has_active_transitions = $0;
  }, HasActiveTransitions());
#endif

  bool input_available = true;

#ifndef __EMSCRIPTEN__
  if (dynamic_cast<SystemTerminalDevice*>(device_.get())) {
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);

    struct timeval tv;
    struct timeval* timeout = nullptr;
    if (HasActiveTransitions()) {
      tv.tv_sec = 0;
      tv.tv_usec = 16667; // ~60 FPS
      timeout = &tv;
    }

    int retval = select(STDIN_FILENO + 1, &fds, nullptr, nullptr, timeout);
    if (retval == 0) {
      input_available = false;
    } else if (retval < 0) {
      if (errno == EINTR) {
        UpdateSize();
      }
      input_available = false;
    } else {
      input_available = FD_ISSET(STDIN_FILENO, &fds);
    }
  }
#endif

  if (input_available) {
    char c;
    int bytes_read = device_->Read(&c, 1);
    if (bytes_read == 1) {
      UpdateSize();
      parser_->Add(c);
      while (auto event = parser_->GetEvent()) {
        HandleEvent(*event);
      }
    } else if (bytes_read == 0) {
      running_ = false;
    } else {
      if (bytes_read == -1 && errno == EINTR) {
        UpdateSize();
      }
    }
  }

  if (TickTransitions(time::GetTimeMs())) {
    Draw();
  }
}

bool ScreenImpl::HasActiveTransitions() {
  if (!component_ || !component_->Root()) {
    return false;
  }
  std::function<bool(Element*)> CheckActive = [&](Element* element) {
    if (!element) return false;
    if (!element->active_transitions.empty()) return true;
    for (size_t i = 0; i < element->ChildCount(); ++i) {
      if (CheckActive(element->ChildAt(i))) return true;
    }
    return false;
  };
  return CheckActive(component_->Root());
}

bool ScreenImpl::TickTransitions(double current_time_ms) {
  if (!component_ || !component_->Root()) {
    return false;
  }
  std::function<bool(Element*)> TickAll = [&](Element* element) {
    if (!element) return false;
    bool updated = element->TickTransitions(current_time_ms);
    for (size_t i = 0; i < element->ChildCount(); ++i) {
      if (TickAll(element->ChildAt(i))) {
        updated = true;
      }
    }
    return updated;
  };
  return TickAll(component_->Root());
}

void ScreenImpl::Dispatch(Event event) {
  HandleEvent(event);
}

void ScreenImpl::HandleEvent(const Event& event) {
  css::g_terminal_width = width_;
  css::g_terminal_height = height_;
  if (event.is<Event::Mouse>()) {
    auto mouse = event.get<Event::Mouse>();

    bool state_changed = false;
    if (root_fragment_) {
      int tx = mouse.x - 1;
      int ty = mouse.y - 1;
      Element* target_el = FindElementAt(root_fragment_, tx, ty);

      auto IsAncestorOf = [](const Element* element, const Element* target) {
        for (const Element* curr = target; curr; curr = curr->Parent()) {
          if (curr == element) {
            return true;
          }
        }
        return false;
      };

      if (component_->Root()) {
        component_->Root()->Visit([&](Element& el) {
          if (mouse.motion == Event::Mouse::Motion::Pressed) {
            bool should_be_active = IsAncestorOf(&el, target_el);
            if (el.active() != should_be_active) {
              el.set_active(should_be_active);
              state_changed = true;
            }
          } else if (mouse.motion == Event::Mouse::Motion::Released) {
            if (el.active()) {
              el.set_active(false);
              state_changed = true;
            }
          }

          bool should_be_hovered = IsAncestorOf(&el, target_el);
          if (el.hovered() != should_be_hovered) {
            el.set_hovered(should_be_hovered);
            state_changed = true;
          }
        });
      }
    }

    if (state_changed) {
      component_->ResolveTargetStyles();
      Draw();
    }

    if (mouse.motion == Event::Mouse::Motion::Pressed &&
        (mouse.button == Event::Mouse::Button::Left ||
         mouse.button == Event::Mouse::Button::Right)) {
      if (root_fragment_) {
        int tx = mouse.x - 1;
        int ty = mouse.y - 1;
        if (auto* clicked_element = FindElementAt(root_fragment_, tx, ty)) {
          bool focus_changed = (focused_element_ != clicked_element);
          if (component_->Root()) {
            component_->Root()->Visit(
                [](Element& el) { el.set_focused(false); });
          }
          focused_element_ = clicked_element;
          focused_element_->set_focused(true);
          if (focus_changed) {
            component_->ResolveTargetStyles();
            Draw();
          }
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
              std::string_view action_view = action;
              std::string callback_name = action;
              std::string callback_arg = "";

              size_t paren_open = action.find('(');
              if (paren_open != std::string::npos && action.ends_with(')')) {
                callback_name = action.substr(0, paren_open);
                callback_arg =
                    action.substr(paren_open + 1, action.size() - paren_open - 2);
              }

              ComponentBase* comp = GetAttributeOwnerComponent(curr);
              bool executed = false;
              while (comp) {
                if (comp->RunCallback(callback_name, callback_arg)) {
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
        if (auto scroll_frag =
                FindScrollableFragmentAt(root_fragment_, tx, ty)) {
          Element* curr = scroll_frag->dom_node;
          while (curr) {
            bool is_horizontal_wheel =
                (mouse.button == Event::Mouse::Button::WheelLeft ||
                 mouse.button == Event::Mouse::Button::WheelRight);
            if (curr->style.overflow_y == Overflow::Scroll &&
                !is_horizontal_wheel) {
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
                if (mouse.button == Event::Mouse::Button::WheelLeft ||
                    mouse.button == Event::Mouse::Button::WheelUp) {
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
    std::vector<Element*> document_order;
    std::function<void(Element*)> CollectAll = [&](Element* el) {
      if (!el) return;
      document_order.push_back(el);
      for (const auto& child : el->children()) {
        CollectAll(child.get());
      }
    };
    if (component_->Root()) {
      CollectAll(component_->Root());
    }

    struct FocusEntry {
      Element* element;
      int tabindex;
      int document_index;
    };

    auto GetEffectiveTabIndex = [](Element* el) -> std::optional<int> {
      if (!el) return std::nullopt;
      const auto& attrs = el->Attributes();
      if (attrs.count("tabindex")) {
        try {
          return std::stoi(attrs.at("tabindex"));
        } catch (...) {
          // ignore invalid values
        }
      }
      if (attrs.count("focusable")) {
        std::string val = attrs.at("focusable");
        if (val == "true" || val == "1") {
          return 0;
        }
      }
      std::string_view tag = el->tag();
      if (tag == "input" || tag == "textarea" || tag == "checkbox" ||
          tag == "slider" || tag == "button" || tag == "select") {
        return 0;
      }
      return std::nullopt;
    };

    std::vector<FocusEntry> navigable;
    for (int i = 0; i < static_cast<int>(document_order.size()); ++i) {
      Element* el = document_order[i];
      auto tab_index_opt = GetEffectiveTabIndex(el);
      if (tab_index_opt.has_value() && tab_index_opt.value() >= 0) {
        navigable.push_back({el, tab_index_opt.value(), i});
      }
    }

    if (!navigable.empty()) {
      std::sort(navigable.begin(), navigable.end(), [](const FocusEntry& a, const FocusEntry& b) {
        if (a.tabindex > 0 && b.tabindex > 0) {
          if (a.tabindex != b.tabindex) {
            return a.tabindex < b.tabindex;
          }
          return a.document_index < b.document_index;
        }
        if (a.tabindex > 0) return true;
        if (b.tabindex > 0) return false;
        return a.document_index < b.document_index;
      });

      int curr_idx = -1;
      for (int i = 0; i < static_cast<int>(navigable.size()); ++i) {
        if (navigable[i].element->focused()) {
          curr_idx = i;
          break;
        }
      }

      int next_idx = 0;
      if (event == Event::Tab()) {
        next_idx = (curr_idx == -1) ? 0 : (curr_idx + 1) % navigable.size();
      } else {
        next_idx = (curr_idx == -1)
                       ? (static_cast<int>(navigable.size()) - 1)
                       : (curr_idx - 1 + navigable.size()) % navigable.size();
      }

      if (component_->Root()) {
        component_->Root()->Visit([](Element& el) { el.set_focused(false); });
      }
      navigable[next_idx].element->set_focused(true);
      focused_element_ = navigable[next_idx].element;
      component_->ResolveTargetStyles();
      DigestAndDraw();
      return;
    }
  }

  if (event == Event::ArrowUp() || event == Event::ArrowDown() ||
      event == Event::PageUp() || event == Event::PageDown() ||
      event == Event::ArrowLeft() || event == Event::ArrowRight()) {
    bool is_horiz =
        (event == Event::ArrowLeft() || event == Event::ArrowRight());
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

void ScreenImpl::Draw() {
  css::g_terminal_width = width_;
  css::g_terminal_height = height_;
  auto root = component_->Root();

  focused_element_ = nullptr;
  if (root) {
    std::function<void(Element*)> FindFocused = [&](Element* el) {
      if (!el) {
        return;
      }
      if (el->focused()) {
        focused_element_ = el;
        return;
      }
      for (size_t i = 0; i < el->ChildCount(); ++i) {
        FindFocused(el->ChildAt(i));
        if (focused_element_) {
          return;
        }
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

  if (has_drawn_) {
    if (last_height_ > 0) {
      device_->Write("\r\x1b[" + std::to_string(last_height_) + "A");
    }
  } else {
    device_->Write("\x1b[2J\x1b[H");
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

void ScreenImpl::UpdateSize() {
  int new_width = width_;
  int new_height = height_;
  if (device_->GetSize(new_width, new_height)) {
    if (new_width != width_ || new_height != height_) {
      width_ = new_width;
      height_ = new_height;
      css::g_terminal_width = width_;
      css::g_terminal_height = height_;
      if (has_drawn_) {
        device_->Write("\x1b[2J\x1b[H");
        last_height_ = 0;
        component_->Render();
        Draw();
      }
    }
  }
}

void ScreenImpl::DigestAndDraw() {
  if (component_->Digest()) {
    Draw();
  }
}

// --- RawTerminal RAII Implementation ---

ScreenImpl::RawTerminal::RawTerminal(TerminalDevice* device) : device_(device) {
  if (device_ && device_->IsAtty()) {
    device_->EnterRawMode(handle_sigwinch);
  }
}

ScreenImpl::RawTerminal::~RawTerminal() {
  if (device_ && device_->IsAtty()) {
    device_->ExitRawMode();
  }
}

Screen::Screen(Ref<ComponentBase> component,
               std::shared_ptr<TerminalDevice> device)
    : impl_(std::make_unique<ScreenImpl>(std::move(component),
                                         std::move(device))) {}

Screen::~Screen() = default;

void Screen::Loop() {
  impl_->Loop();
}

void Screen::Step() {
  impl_->Step();
}

void Screen::Dispatch(Event event) {
  impl_->Dispatch(event);
}

void Screen::Draw() {
  impl_->Draw();
}

}  // namespace rtxui
