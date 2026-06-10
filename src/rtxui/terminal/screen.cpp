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
#include "rtxui/component/component_internal.hpp"
#include "rtxui/layout/layout_tree_builder.hpp"
#include "rtxui/layout/physical_fragment.hpp"
#include "rtxui/paint/paint.hpp"
#include "rtxui/paint/texture.hpp"
#include "rtxui/style/style.hpp"
#include "rtxui/terminal/terminal_device.hpp"
#include "rtxui/terminal/terminal_input_parser.hpp"
#include "rtxui/core/task_runner.hpp"


namespace rtxui {

namespace {

void handle_sigwinch(int sig) {}

std::optional<int> GetEffectiveTabIndex(Element* el) {
  if (!el) {
    return std::nullopt;
  }
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
}

struct FocusableFragment {
  Element* element;
  int x, y, width, height;
};

void CollectFocusableFragments(
    const std::shared_ptr<PhysicalFragment>& fragment,
    int abs_x,
    int abs_y,
    std::vector<FocusableFragment>& focusable_fragments) {
  if (!fragment) {
    return;
  }

  if (fragment->dom_node) {
    auto tab_index_opt = GetEffectiveTabIndex(fragment->dom_node);
    if (tab_index_opt.has_value() && tab_index_opt.value() >= 0) {
      focusable_fragments.push_back({fragment->dom_node, abs_x, abs_y,
                                     fragment->width, fragment->height});
    }
  }

  int scroll_x_offset = 0;
  int scroll_y_offset = 0;
  if (fragment->clips_descendants) {
    scroll_x_offset = fragment->scroll_x;
    scroll_y_offset = fragment->scroll_y;
  }

  for (const auto& child : fragment->children) {
    bool is_fixed =
        (child.fragment && child.fragment->dom_node &&
         child.fragment->dom_node->style.position == PositionType::Fixed);

    int child_abs_x = is_fixed ? child.x : abs_x + child.x - scroll_x_offset;
    int child_abs_y = is_fixed ? child.y : abs_y + child.y - scroll_y_offset;

    CollectFocusableFragments(child.fragment, child_abs_x, child_abs_y,
                              focusable_fragments);
  }
}

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
    bool is_fixed =
        (it->fragment && it->fragment->dom_node &&
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

    if (auto* found =
            FindElementAtImpl(it->fragment, rel_x, rel_y, child_accum_scroll_x,
                              child_accum_scroll_y)) {
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
  void ScrollIntoView(Element* element);

  bool SpatialNavigate(Event event);
  void SimulateClick(Element* element);

  void SetSmoothScrollEnabled(bool enabled) {
    smooth_scroll_enabled_ = enabled;
  }
  bool smooth_scroll_enabled() const { return smooth_scroll_enabled_; }

  Ref<ComponentBase> component_;
  int width_ = 80;
  int height_ = 24;
  int last_height_ = 0;
  bool has_drawn_ = false;
  bool running_ = true;
  std::shared_ptr<PhysicalFragment> root_fragment_;
  std::shared_ptr<LayoutBox> root_box_;
  std::unique_ptr<Texture> last_texture_;
  std::shared_ptr<TerminalDevice> device_;
  std::unique_ptr<TerminalInputParser> parser_;
  Element* focused_element_ = nullptr;
  bool smooth_scroll_enabled_ = true;
  task::TaskRunner task_runner_;

  struct RawTerminal {
    ScreenImpl* screen_ = nullptr;
    explicit RawTerminal(ScreenImpl* screen);
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
  RawTerminal raw_terminal(this);
  Draw();

  running_ = true;
  while (running_) {
    Step();
  }
}

void ScreenImpl::Step() {
#ifdef __EMSCRIPTEN__
  EM_ASM({ window.rtxui_has_active_transitions = $0; }, HasActiveTransitions());
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
      tv.tv_usec = 16667;  // ~60 FPS
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

  task_runner_.RunUntilNextDelayedTask();

  if (TickTransitions(time::GetTimeMs())) {
    Draw();
  }
}

bool ScreenImpl::HasActiveTransitions() {
  if (!component_ || !component_->Root()) {
    return false;
  }
  std::function<bool(Element*)> CheckActive = [&](Element* element) {
    if (!element) {
      return false;
    }
    if (!element->active_transitions.empty() || element->IsAnimatingScroll()) {
      return true;
    }
    for (size_t i = 0; i < element->ChildCount(); ++i) {
      if (CheckActive(element->ChildAt(i))) {
        return true;
      }
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
    if (!element) {
      return false;
    }
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
      Element* target_el = nullptr;
      if (auto* capturer = ComponentBase::GetMouseCapturer()) {
        target_el = capturer->Root();
      }
      if (!target_el) {
        target_el = FindElementAt(root_fragment_, tx, ty);
      }

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

    if (!ComponentBase::GetMouseCapturer() &&
        mouse.motion == Event::Mouse::Motion::Pressed &&
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
          ScrollIntoView(focused_element_);
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
                callback_arg = action.substr(paren_open + 1,
                                             action.size() - paren_open - 2);
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
    } else if (!ComponentBase::GetMouseCapturer() &&
               (mouse.button == Event::Mouse::Button::WheelUp ||
                mouse.button == Event::Mouse::Button::WheelDown ||
                mouse.button == Event::Mouse::Button::WheelLeft ||
                mouse.button == Event::Mouse::Button::WheelRight)) {
      if (root_fragment_) {
        int tx = mouse.x - 1;
        int ty = mouse.y - 1;
        // Collect scroll info from fragments first, then release them before
        // calling Draw() to avoid use-after-arena-reset (Draw() resets the
        // arena while fragment shared_ptrs might still be in scope).
        struct ScrollAction {
          Element* element = nullptr;
          bool is_vertical = false;
          int new_value = 0;
        };
        ScrollAction action;

        if (auto scroll_frag =
                FindScrollableFragmentAt(root_fragment_, tx, ty)) {
          Element* curr = scroll_frag->dom_node;
          // Release scroll_frag immediately — we only needed dom_node.
          scroll_frag.reset();
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
                int curr_y = curr->target_scroll_y();
                int speed = curr->style.scroll_speed_y;
                frag.reset();  // Release before Draw().

                int new_y = curr_y;
                if (mouse.button == Event::Mouse::Button::WheelUp) {
                  new_y = std::max(0, curr_y - speed);
                } else {
                  new_y = std::min(max_scroll, curr_y + speed);
                }

                if (new_y != curr_y) {
                  action = {curr, true, new_y};
                  break;
                }
              }
            } else if (curr->style.overflow_x == Overflow::Scroll) {
              auto frag = FindFragmentForElement(root_fragment_, curr);
              if (frag) {
                int scroll_width = curr->scroll_width();
                int max_scroll = std::max(0, scroll_width - frag->width);
                int curr_x = curr->target_scroll_x();
                int speed = curr->style.scroll_speed_x;
                frag.reset();  // Release before Draw().

                int new_x = curr_x;
                if (mouse.button == Event::Mouse::Button::WheelLeft ||
                    mouse.button == Event::Mouse::Button::WheelUp) {
                  new_x = std::max(0, curr_x - speed);
                } else {
                  new_x = std::min(max_scroll, curr_x + speed);
                }

                if (new_x != curr_x) {
                  action = {curr, false, new_x};
                  break;
                }
              }
            }
            curr = curr->Parent();
          }
        }

        if (action.element) {
          if (action.is_vertical) {
            action.element->set_scroll_y(action.new_value, false);
          } else {
            action.element->set_scroll_x(action.new_value, false);
          }
          Draw();
          return;
        }
      }
    }
  }

  bool event_handled = false;
  if (event.is<Event::Mouse>() && ComponentBase::GetMouseCapturer()) {
    event_handled = ComponentBase::GetMouseCapturer()->OnEvent(event);
  } else {
    event_handled = component_->OnEvent(event);
  }

  if (event_handled) {
    DigestAndDraw();
    return;
  }

  if (event == Event::Tab() || event == Event::TabReverse()) {
    std::vector<Element*> document_order;
    std::function<void(Element*)> CollectAll = [&](Element* el) {
      if (!el) {
        return;
      }
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

    std::vector<FocusEntry> navigable;
    for (int i = 0; i < static_cast<int>(document_order.size()); ++i) {
      Element* el = document_order[i];
      auto tab_index_opt = GetEffectiveTabIndex(el);
      if (tab_index_opt.has_value() && tab_index_opt.value() >= 0) {
        navigable.push_back({el, tab_index_opt.value(), i});
      }
    }

    if (!navigable.empty()) {
      std::sort(navigable.begin(), navigable.end(),
                [](const FocusEntry& a, const FocusEntry& b) {
                  if (a.tabindex > 0 && b.tabindex > 0) {
                    if (a.tabindex != b.tabindex) {
                      return a.tabindex < b.tabindex;
                    }
                    return a.document_index < b.document_index;
                  }
                  if (a.tabindex > 0) {
                    return true;
                  }
                  if (b.tabindex > 0) {
                    return false;
                  }
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
      ScrollIntoView(focused_element_);
      Draw();
      return;
    }
  }

  if (event == Event::Return() ||
      (event.is<Event::Keyboard>() &&
       event.get<Event::Keyboard>().codepoint == 32)) {
    if (focused_element_) {
      SimulateClick(focused_element_);
      return;
    }
  }

  if (event == Event::ArrowUp() || event == Event::ArrowDown() ||
      event == Event::ArrowLeft() || event == Event::ArrowRight() ||
      event == Event::h() || event == Event::j() || event == Event::k() ||
      event == Event::l()) {
    if (SpatialNavigate(event)) {
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
              int curr_x = curr->target_scroll_x();
              int speed = curr->style.scroll_speed_x;
              scroll_frag.reset();  // Release before Draw().

              int delta = (event == Event::ArrowLeft()) ? -speed : speed;
              int new_x = std::clamp(curr_x + delta, 0, max_scroll);
              if (new_x != curr_x) {
                curr->set_scroll_x(new_x, false);
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
              int curr_y = curr->target_scroll_y();
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
              scroll_frag.reset();  // Release before Draw().

              int new_y = std::clamp(curr_y + delta, 0, max_scroll);
              if (new_y != curr_y) {
                curr->set_scroll_y(new_y, false);
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
          int curr_x = scroll_frag->dom_node->target_scroll_x();
          int speed = scroll_frag->dom_node->style.scroll_speed_x;
          Element* el = scroll_frag->dom_node;
          scroll_frag.reset();  // Release before Draw().

          int delta = (event == Event::ArrowLeft()) ? -speed : speed;
          int new_x = std::clamp(curr_x + delta, 0, max_scroll);
          if (new_x != curr_x) {
            el->set_scroll_x(new_x, false);
            Draw();
            return;
          }
        }
      } else {
        if (scroll_frag->dom_node->style.overflow_y == Overflow::Scroll) {
          int scroll_height = scroll_frag->dom_node->scroll_height();
          int max_scroll = std::max(0, scroll_height - scroll_frag->height);
          int curr_y = scroll_frag->dom_node->target_scroll_y();
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
          Element* el = scroll_frag->dom_node;
          scroll_frag.reset();  // Release before Draw().

          int new_y = std::clamp(curr_y + delta, 0, max_scroll);
          if (new_y != curr_y) {
            el->set_scroll_y(new_y, false);
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

  root_fragment_ = nullptr;
  root_box_ = nullptr;
  ResetLayoutArena();

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
  root_box_ = root_box;

  Texture texture(width_, height_);
  if (root_fragment) {
    Paint(root_fragment.get(), texture);
  }

  std::string new_output;
  bool use_diff = has_drawn_ && last_texture_ &&
                  last_texture_->width() == width_ &&
                  last_texture_->height() == height_;
  if (use_diff) {
    new_output = texture.RenderDiff(*last_texture_);
  } else {
    new_output = texture.Render();
  }

  if (has_drawn_) {
    if (last_height_ > 0) {
      device_->Write("\r\x1b[" + std::to_string(last_height_) + "A");
    }
  } else {
    device_->Write("\x1b[2J\x1b[H");
  }

  device_->Write(new_output);

  Element* cursor_element = nullptr;
  if (root) {
    std::function<void(Element*)> FindCursor = [&](Element* el) {
      if (!el) return;
      if (std::find(el->classes.begin(), el->classes.end(), "cursor-focused") != el->classes.end()) {
        cursor_element = el;
        return;
      }
      for (size_t i = 0; i < el->ChildCount(); ++i) {
        FindCursor(el->ChildAt(i));
        if (cursor_element) return;
      }
    };
    FindCursor(root);
  }

  if (cursor_element) {
    int cx = cursor_element->absolute_x() + 1;
    int cy = cursor_element->absolute_y() + 1;
    device_->Write("\x1b[?25h\x1b[5 q\x1b[" + std::to_string(cy) + ";" + std::to_string(cx) + "H");
  } else {
    device_->Write("\x1b[?25l");
  }

  if (use_diff) {
    last_height_ = height_ - 1;
  } else {
    last_height_ = 0;
    for (char ch : new_output) {
      if (ch == '\n') {
        last_height_++;
      }
    }
  }
  last_texture_ = std::make_unique<Texture>(texture);
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

void ScreenImpl::ScrollIntoView(Element* element) {
  if (!element || !root_fragment_) {
    return;
  }

  struct FragmentNode {
    std::shared_ptr<PhysicalFragment> fragment;
    int x_rel_parent = 0;
    int y_rel_parent = 0;
  };

  std::vector<FragmentNode> path;

  std::function<bool(const std::shared_ptr<PhysicalFragment>&)> find_path =
      [&](const std::shared_ptr<PhysicalFragment>& frag) -> bool {
    if (!frag) {
      return false;
    }
    if (frag->dom_node == element) {
      return true;
    }
    for (const auto& child : frag->children) {
      path.push_back({child.fragment, child.x, child.y});
      if (find_path(child.fragment)) {
        return true;
      }
      path.pop_back();
    }
    return false;
  };

  path.push_back({root_fragment_, 0, 0});
  if (!find_path(root_fragment_)) {
    return;
  }

  // The last node in path is the fragment for element
  int target_left = 0;
  int target_top = 0;
  int target_right = path.back().fragment->width;
  int target_bottom = path.back().fragment->height;

  for (size_t i = path.size() - 1; i > 0; --i) {
    const auto& curr = path[i];
    auto& parent = path[i - 1];

    target_left += curr.x_rel_parent;
    target_right += curr.x_rel_parent;
    target_top += curr.y_rel_parent;
    target_bottom += curr.y_rel_parent;

    if (parent.fragment->dom_node) {
      bool parent_scrollable_y =
          (parent.fragment->dom_node->style.overflow_y == Overflow::Scroll);
      bool parent_scrollable_x =
          (parent.fragment->dom_node->style.overflow_x == Overflow::Scroll);

      if (parent_scrollable_y) {
        int curr_scroll_y = parent.fragment->dom_node->target_scroll_y();
        int new_scroll_y = curr_scroll_y;
        int viewport_h = parent.fragment->height;

        if (target_top < curr_scroll_y) {
          new_scroll_y = target_top;
        } else if (target_bottom > curr_scroll_y + viewport_h) {
          new_scroll_y = target_bottom - viewport_h;
          if (target_top < new_scroll_y) {
            new_scroll_y = target_top;
          }
        }

        int scroll_h = parent.fragment->dom_node->scroll_height();
        int max_scroll_y = std::max(0, scroll_h - viewport_h);
        new_scroll_y = std::clamp(new_scroll_y, 0, max_scroll_y);

        if (new_scroll_y != curr_scroll_y) {
          parent.fragment->dom_node->set_scroll_y(new_scroll_y,
                                                  smooth_scroll_enabled_);
          parent.fragment->scroll_y = parent.fragment->dom_node->scroll_y();
        }
      }

      if (parent_scrollable_x) {
        int curr_scroll_x = parent.fragment->dom_node->target_scroll_x();
        int new_scroll_x = curr_scroll_x;
        int viewport_w = parent.fragment->width;

        if (target_left < curr_scroll_x) {
          new_scroll_x = target_left;
        } else if (target_right > curr_scroll_x + viewport_w) {
          new_scroll_x = target_right - viewport_w;
          if (target_left < new_scroll_x) {
            new_scroll_x = target_left;
          }
        }

        int scroll_w = parent.fragment->dom_node->scroll_width();
        int max_scroll_x = std::max(0, scroll_w - viewport_w);
        new_scroll_x = std::clamp(new_scroll_x, 0, max_scroll_x);

        if (new_scroll_x != curr_scroll_x) {
          parent.fragment->dom_node->set_scroll_x(new_scroll_x,
                                                  smooth_scroll_enabled_);
          parent.fragment->scroll_x = parent.fragment->dom_node->scroll_x();
        }
      }
    }

    target_left -= parent.fragment->scroll_x;
    target_right -= parent.fragment->scroll_x;
    target_top -= parent.fragment->scroll_y;
    target_bottom -= parent.fragment->scroll_y;
    }
    }

    bool ScreenImpl::SpatialNavigate(Event event) {
      enum class Direction { Up, Down, Left, Right } dir;
      if (event == Event::ArrowUp() || event == Event::k())
        dir = Direction::Up;
      else if (event == Event::ArrowDown() || event == Event::j())
        dir = Direction::Down;
      else if (event == Event::ArrowLeft() || event == Event::h())
        dir = Direction::Left;
      else if (event == Event::ArrowRight() || event == Event::l())
        dir = Direction::Right;
      else
        return false;

  std::vector<FocusableFragment> focusable_fragments;
  CollectFocusableFragments(root_fragment_, 0, 0, focusable_fragments);

  if (focusable_fragments.empty())
    return false;

  int cur_x = 0, cur_y = 0, cur_w = 0, cur_h = 0;
  bool start_from_element = false;
  if (focused_element_) {
    for (const auto& f : focusable_fragments) {
      if (f.element == focused_element_) {
        cur_x = f.x;
        cur_y = f.y;
        cur_w = f.width;
        cur_h = f.height;
        start_from_element = true;
        break;
      }
    }
  }

  if (!start_from_element) {
    // If no element focused, start from outside the screen depending on direction
    switch (dir) {
      case Direction::Down:
        cur_x = 0;
        cur_y = -1;
        cur_w = width_;
        cur_h = 0;
        break;
      case Direction::Up:
        cur_x = 0;
        cur_y = height_;
        cur_w = width_;
        cur_h = 0;
        break;
      case Direction::Right:
        cur_x = -1;
        cur_y = 0;
        cur_w = 0;
        cur_h = height_;
        break;
      case Direction::Left:
        cur_x = width_;
        cur_y = 0;
        cur_w = 0;
        cur_h = height_;
        break;
    }
  }

  int cur_cx = cur_x + cur_w / 2;
  int cur_cy = cur_y + cur_h / 2;

  FocusableFragment* best = nullptr;
  long long best_score = -1;

  for (auto& cand : focusable_fragments) {
    if (cand.element == focused_element_)
      continue;

    int cand_cx = cand.x + cand.width / 2;
    int cand_cy = cand.y + cand.height / 2;

    long long d_primary = 0;
    long long d_secondary = 0;

    switch (dir) {
      case Direction::Left:
        d_primary = cur_cx - cand_cx;
        d_secondary = std::abs(cur_cy - cand_cy);
        break;
      case Direction::Right:
        d_primary = cand_cx - cur_cx;
        d_secondary = std::abs(cur_cy - cand_cy);
        break;
      case Direction::Up:
        d_primary = cur_cy - cand_cy;
        d_secondary = std::abs(cur_cx - cand_cx);
        break;
      case Direction::Down:
        d_primary = cand_cy - cur_cy;
        d_secondary = std::abs(cur_cx - cand_cx);
        break;
    }

    if (d_primary <= 0)
      continue;

    // Spatial navigation distance metric: primary distance squared + secondary distance squared * 2
    long long score = d_primary * d_primary + d_secondary * d_secondary * 2;
    if (best == nullptr || score < best_score) {
      best = &cand;
      best_score = score;
    }
  }

  if (best) {
    if (component_->Root()) {
      component_->Root()->Visit([](Element& el) { el.set_focused(false); });
    }
    best->element->set_focused(true);
    focused_element_ = best->element;
    component_->ResolveTargetStyles();
    ScrollIntoView(focused_element_);
    Draw();
    return true;
  }

  return false;
}
    void ScreenImpl::SimulateClick(Element* element) {
    if (!element)
    return;
    std::vector<std::string> attr_keys = {"onclick", "@click.left", "@click"};
    Element* curr = element;
    while (curr) {
    const auto& attrs = curr->Attributes();
    for (const auto& key : attr_keys) {
      if (attrs.count(key)) {
        std::string action = attrs.at(key);
        if (auto* comp = GetAttributeOwnerComponent(curr)) {
          if (comp->RunCallback(action)) {
            DigestAndDraw();
            return;
          }
        }
      }
    }
    curr = curr->Parent();
    }
    }

    // --- RawTerminal RAII Implementation ---

ScreenImpl::RawTerminal::RawTerminal(ScreenImpl* screen) : screen_(screen) {
  if (screen_ && screen_->device_) {
    if (screen_->device_->IsAtty()) {
      screen_->device_->EnterRawMode(handle_sigwinch);
    }
    screen_->has_drawn_ = false;
    screen_->last_texture_.reset();
  }
}

ScreenImpl::RawTerminal::~RawTerminal() {
  if (screen_ && screen_->device_ && screen_->device_->IsAtty()) {
    screen_->device_->ExitRawMode();
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

void Screen::SetSmoothScrollEnabled(bool enabled) {
  impl_->SetSmoothScrollEnabled(enabled);
}

bool Screen::smooth_scroll_enabled() const {
  return impl_->smooth_scroll_enabled();
}

}  // namespace rtxui
