#include "rtxui/dom/element.hpp"

#include <algorithm>
#include <atomic>
#include <cassert>
#include <charconv>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <fstream>

#include "rtxui/component.hpp"
#include "rtxui/diagnostic.hpp"
#include "rtxui/dom/text_element.hpp"
#include "rtxui/style/apply_style.hpp"
#include "rtxui/style/style.hpp"
std::atomic<int> g_elements_created{0};
std::atomic<int> g_elements_destroyed{0};

namespace rtxui {

namespace {

float SolveCubicBezier(float x1, float y1, float x2, float y2, float t) {
  if (t <= 0.0f) {
    return 0.0f;
  }
  if (t >= 1.0f) {
    return 1.0f;
  }

  float low = 0.0f, high = 1.0f;
  for (int i = 0; i < 14; ++i) {
    float u = (low + high) / 2.0f;
    float x = 3.0f * (1.0f - u) * (1.0f - u) * u * x1 +
              3.0f * (1.0f - u) * u * u * x2 + u * u * u;
    if (x < t) {
      low = u;
    } else {
      high = u;
    }
  }
  float u = (low + high) / 2.0f;
  return 3.0f * (1.0f - u) * (1.0f - u) * u * y1 +
         3.0f * (1.0f - u) * u * u * y2 + u * u * u;
}

}  // namespace

const float kPi = 3.1415926535f;

float ApplyEasing(float t, std::string_view timing) {
  if (t <= 0.0f) {
    return 0.0f;
  }
  if (t >= 1.0f) {
    return 1.0f;
  }

  if (timing == "linear") {
    return t;
  }

  // Sine
  if (timing == "ease-in-sine") {
    return 1.0f - std::cos((t * kPi) / 2.0f);
  }
  if (timing == "ease-out-sine") {
    return std::sin((t * kPi) / 2.0f);
  }
  if (timing == "ease-in-out-sine") {
    return -(std::cos(kPi * t) - 1.0f) / 2.0f;
  }

  // Quad
  if (timing == "ease-in-quad") {
    return t * t;
  }
  if (timing == "ease-out-quad") {
    return 1.0f - (1.0f - t) * (1.0f - t);
  }
  if (timing == "ease-in-out-quad") {
    return t < 0.5f ? 2.0f * t * t
                    : 1.0f - std::pow(-2.0f * t + 2.0f, 2.0f) / 2.0f;
  }

  // Cubic
  if (timing == "ease-in-cubic") {
    return t * t * t;
  }
  if (timing == "ease-out-cubic") {
    return 1.0f - std::pow(1.0f - t, 3.0f);
  }
  if (timing == "ease-in-out-cubic") {
    return t < 0.5f ? 4.0f * t * t * t
                    : 1.0f - std::pow(-2.0f * t + 2.0f, 3.0f) / 2.0f;
  }

  // Quart
  if (timing == "ease-in-quart") {
    return t * t * t * t;
  }
  if (timing == "ease-out-quart") {
    return 1.0f - std::pow(1.0f - t, 4.0f);
  }
  if (timing == "ease-in-out-quart") {
    return t < 0.5f ? 8.0f * t * t * t * t
                    : 1.0f - std::pow(-2.0f * t + 2.0f, 4.0f) / 2.0f;
  }

  // Quint
  if (timing == "ease-in-quint") {
    return t * t * t * t * t;
  }
  if (timing == "ease-out-quint") {
    return 1.0f - std::pow(1.0f - t, 5.0f);
  }
  if (timing == "ease-in-out-quint") {
    return t < 0.5f ? 16.0f * t * t * t * t * t
                    : 1.0f - std::pow(-2.0f * t + 2.0f, 5.0f) / 2.0f;
  }

  // Expo
  if (timing == "ease-in-expo") {
    return std::pow(2.0f, 10.0f * t - 10.0f);
  }
  if (timing == "ease-out-expo") {
    return 1.0f - std::pow(2.0f, -10.0f * t);
  }
  if (timing == "ease-in-out-expo") {
    return t < 0.5f ? std::pow(2.0f, 20.0f * t - 10.0f) / 2.0f
                    : (2.0f - std::pow(2.0f, -20.0f * t + 10.0f)) / 2.0f;
  }

  // Circ
  if (timing == "ease-in-circ") {
    return 1.0f - std::sqrt(1.0f - t * t);
  }
  if (timing == "ease-out-circ") {
    return std::sqrt(1.0f - (t - 1.0f) * (t - 1.0f));
  }
  if (timing == "ease-in-out-circ") {
    return t < 0.5f
               ? (1.0f - std::sqrt(1.0f - 4.0f * t * t)) / 2.0f
               : (std::sqrt(1.0f - std::pow(-2.0f * t + 2.0f, 2.0f)) + 1.0f) /
                     2.0f;
  }

  // Back
  const float c1 = 1.70158f;
  const float c3 = c1 + 1.0f;
  if (timing == "ease-in-back") {
    return c3 * t * t * t - c1 * t * t;
  }
  if (timing == "ease-out-back") {
    return 1.0f + c3 * std::pow(t - 1.0f, 3.0f) + c1 * std::pow(t - 1.0f, 2.0f);
  }
  if (timing == "ease-in-out-back") {
    const float c2 = c1 * 1.525f;
    return t < 0.5f
               ? (std::pow(2.0f * t, 2.0f) * ((c2 + 1.0f) * 2.0f * t - c2)) /
                     2.0f
               : (std::pow(2.0f * t - 2.0f, 2.0f) *
                      ((c2 + 1.0f) * (t * 2.0f - 2.0f) + c2) +
                  2.0f) /
                     2.0f;
  }

  if (timing == "ease-in") {
    return SolveCubicBezier(0.42f, 0.0f, 1.0f, 1.0f, t);
  }
  if (timing == "ease-out") {
    return SolveCubicBezier(0.0f, 0.0f, 0.58f, 1.0f, t);
  }
  if (timing == "ease-in-out") {
    return SolveCubicBezier(0.42f, 0.0f, 0.58f, 1.0f, t);
  }

  // Steps hold a value, then jump to the next: `steps(4)` jumps at the end of
  // each quarter, `steps(4, start)` at its start.
  if (timing == "step-start") {
    return 1.0f;
  }
  if (timing == "step-end") {
    return 0.0f;
  }
  if (timing.starts_with("steps(") && timing.ends_with(")")) {
    std::string_view params = timing.substr(6, timing.size() - 7);
    const size_t comma = params.find(',');
    std::string_view count_text = params.substr(0, comma);
    std::string_view position;
    if (comma != std::string_view::npos) {
      position = params.substr(comma + 1);
    }
    auto trim = [](std::string_view text) {
      while (!text.empty() && text.front() == ' ') {
        text.remove_prefix(1);
      }
      while (!text.empty() && text.back() == ' ') {
        text.remove_suffix(1);
      }
      return text;
    };
    count_text = trim(count_text);
    position = trim(position);
    int count = 0;
    const auto [ptr, ec] = std::from_chars(
        count_text.data(), count_text.data() + count_text.size(), count);
    if (ec == std::errc() && ptr == count_text.data() + count_text.size() &&
        count > 0) {
      const auto steps = static_cast<float>(count);
      if (position == "start" || position == "jump-start") {
        return std::min(1.0f, std::floor(t * steps + 1.0f) / steps);
      }
      if (position == "jump-none" && count > 1) {
        return std::floor(t * steps) / (steps - 1.0f);
      }
      if (position == "jump-both") {
        return std::floor(t * steps + 1.0f) / (steps + 1.0f);
      }
      return std::floor(t * steps) / steps;
    }
  }

  if (timing.starts_with("cubic-bezier(") && timing.ends_with(")")) {
    std::string_view params = timing.substr(13, timing.size() - 14);
    float x1 = 0.0f, y1 = 0.0f, x2 = 0.0f, y2 = 0.0f;
    std::string params_str(params);
    if (std::sscanf(params_str.c_str(), "%f,%f,%f,%f", &x1, &y1, &x2, &y2) ==
        4) {
      return SolveCubicBezier(x1, y1, x2, y2, t);
    }
  }

  // Default is "ease"
  return SolveCubicBezier(0.25f, 0.1f, 0.25f, 1.0f, t);
}

namespace {

Color InterpolateColor(Color start, Color target, float progress) {
  return Color::RGBA(
      static_cast<std::uint8_t>(start.r + (target.r - start.r) * progress),
      static_cast<std::uint8_t>(start.g + (target.g - start.g) * progress),
      static_cast<std::uint8_t>(start.b + (target.b - start.b) * progress),
      static_cast<std::uint8_t>(start.a + (target.a - start.a) * progress));
}

std::optional<Color> InterpolateOptionalColor(std::optional<Color> start,
                                              std::optional<Color> target,
                                              float progress) {
  if (!start && !target) {
    return std::nullopt;
  }
  Color s = start.value_or(Color::RGBA(0, 0, 0, 0));
  Color t = target.value_or(Color::RGBA(0, 0, 0, 0));
  if (!start && target) {
    s = Color::RGBA(target->r, target->g, target->b, 0);
  }
  if (start && !target) {
    t = Color::RGBA(start->r, start->g, start->b, 0);
  }
  return InterpolateColor(s, t, progress);
}

// A CSS time, stored as float seconds, in milliseconds. Rounded to the
// microsecond: `80ms` is 0.08f, which is 79.99999 ms as is, and an animation
// sampled at its exact end or middle would land a frame short.
double ToMs(float seconds) {
  return std::round(static_cast<double>(seconds) * 1e6) / 1e3;
}

// A length as `cells + percent%`, the form calc() folds into. Nothing for
// the units that have none: auto, fr and min()/max().
std::optional<std::pair<float, float>> AsLinear(Length length) {
  switch (length.unit) {
    case Unit::Cells:
      return std::pair{length.value, 0.0f};
    case Unit::Percent:
      return std::pair{0.0f, length.value};
    case Unit::Calc:
      return std::pair{length.value, length.calc_percent};
    default:
      return std::nullopt;
  }
}

Length InterpolateLength(Length start, Length target, float progress) {
  if (start.unit == target.unit && start.unit != Unit::Calc) {
    return {start.value + (target.value - start.value) * progress, start.unit};
  }
  // Cells, percents and calc() blend through calc(): `50%` to `10` is
  // `25% + 5` halfway. Others jump at the end.
  const auto from = AsLinear(start);
  const auto to = AsLinear(target);
  if (!from || !to || progress <= 0.0f || progress >= 1.0f) {
    return progress >= 1.0f ? target : start;
  }
  return Length::MakeCalc(
      from->first + (to->first - from->first) * progress,
      from->second + (to->second - from->second) * progress);
}

const TransitionConfig* FindTransitionConfig(const Element* element,
                                             std::string_view property) {
  if (!element->target_style.transitions) {
    return nullptr;
  }
  for (const auto& config : *element->target_style.transitions) {
    if (config.property == "all") {
      return &config;
    }
    if (config.property == property) {
      return &config;
    }
    if ((config.property == "color" || config.property == "foreground-color") &&
        (property == "color" || property == "foreground-color")) {
      return &config;
    }
    if (config.property == "border-color" &&
        ((property.starts_with("border-") && property.ends_with("-color")) ||
         property.starts_with("border-color-"))) {
      return &config;
    }
    if ((config.property == "border-top-color" ||
         config.property == "border-color-top") &&
        (property == "border-top-color" || property == "border-color-top")) {
      return &config;
    }
    if ((config.property == "border-right-color" ||
         config.property == "border-color-right") &&
        (property == "border-right-color" ||
         property == "border-color-right")) {
      return &config;
    }
    if ((config.property == "border-bottom-color" ||
         config.property == "border-color-bottom") &&
        (property == "border-bottom-color" ||
         property == "border-color-bottom")) {
      return &config;
    }
    if ((config.property == "border-left-color" ||
         config.property == "border-color-left") &&
        (property == "border-left-color" || property == "border-color-left")) {
      return &config;
    }
    if (config.property == "translate" &&
        (property == "translate-x" || property == "translate-y")) {
      return &config;
    }
    if (config.property == "inset" &&
        (property == "top" || property == "right" || property == "bottom" ||
         property == "left")) {
      return &config;
    }
    if (config.property == "scrollbar-color" &&
        (property == "scrollbar-color-thumb" ||
         property == "scrollbar-color-track")) {
      return &config;
    }
  }
  return nullptr;
}

struct ScrollAxis {
  int& scroll;
  int& max_size;
  int& target;
  float& anim;
  float& start;
  double& anim_start_time;
  bool& animating;

  float& visual;
  float& visual_start;
  float& visual_target;
  double& visual_anim_start_time;
  bool& visual_animating;

  void Set(int value, bool smooth, ScrollBehavior behavior) {
    if (!smooth || behavior != ScrollBehavior::Smooth) {
      scroll = value;
      target = value;
      anim = static_cast<float>(value);
      animating = false;

      visual = static_cast<float>(value);
      visual_target = static_cast<float>(value);
      visual_animating = false;
    } else {
      if (value == target) {
        return;
      }
      start = anim;
      target = value;
      anim_start_time = time::GetTimeMs();
      animating = true;

      visual_target = static_cast<float>(value);
      visual_animating = false;
    }
  }

  void Clamp(int max_scroll) {
    if (target > max_scroll) {
      target = max_scroll;
    }
    if (scroll > max_scroll) {
      scroll = max_scroll;
    }
    if (anim > max_scroll) {
      anim = static_cast<float>(max_scroll);
    }
    if (start > max_scroll) {
      start = static_cast<float>(max_scroll);
    }

    if (visual_target > max_scroll) {
      visual_target = static_cast<float>(max_scroll);
    }
    if (visual > max_scroll) {
      visual = static_cast<float>(max_scroll);
    }
    if (visual_start > max_scroll) {
      visual_start = static_cast<float>(max_scroll);
    }

    if (anim == static_cast<float>(target)) {
      animating = false;
    }
    if (visual == visual_target) {
      visual_animating = false;
    }
  }

  bool Tick(double current_time_ms, double duration_ms) {
    bool updated = false;
    if (animating) {
      if (current_time_ms >= anim_start_time) {
        float t = static_cast<float>((current_time_ms - anim_start_time) /
                                     duration_ms);
        if (t < 0.0f) {
          t = 0.0f;
        }
        if (t >= 1.0f) {
          scroll = target;
          anim = static_cast<float>(target);
          animating = false;
          visual = static_cast<float>(target);
          updated = true;
        } else {
          float eased_t = ApplyEasing(t, "ease");
          float next_anim = start + (target - start) * eased_t;
          int next_scroll = static_cast<int>(std::round(next_anim));
          if (next_scroll != scroll || next_anim != anim) {
            scroll = next_scroll;
            anim = next_anim;
            visual = next_anim;
            updated = true;
          }
        }
      }
    } else if (visual_animating) {
      if (current_time_ms >= visual_anim_start_time) {
        float t = static_cast<float>(
            (current_time_ms - visual_anim_start_time) / duration_ms);
        if (t < 0.0f) {
          t = 0.0f;
        }
        if (t >= 1.0f) {
          visual = visual_target;
          visual_animating = false;
          updated = true;
        } else {
          float eased_t = ApplyEasing(t, "ease");
          float next_anim =
              visual_start + (visual_target - visual_start) * eased_t;
          if (next_anim != visual) {
            visual = next_anim;
            updated = true;
          }
        }
      }
    }
    return updated;
  }
};

constexpr double kScrollAnimationDurationMs = 500.0;

}  // namespace

Element::Element() {
  g_elements_created++;
}
Element::Element(const ComponentBase* component)
    : component_(component), owner_component_(component) {
  g_elements_created++;
}

Element* Element::GeneratedBox(bool after) {
  Ref<Element>& box = generated_boxes_[after ? 1 : 0];
  const GeneratedContent* content = nullptr;
  if (generated) {
    content = after ? &generated->after : &generated->before;
  }
  if (!content || !content->content) {
    box = nullptr;
    return nullptr;
  }
  if (!box) {
    box = Ref<Element>::New();
    box->AddChild(Ref<TextElement>::New(*content->content));
  } else {
    static_cast<TextElement*>(box->ChildAt(0))->set_text(*content->content);
  }
  box->style = content->style;
  // Not one of this element's children, so selectors and DOM walks never
  // meet it, but events on it reach this element.
  box->set_parent(this);
  return box.get();
}

Element::~Element() {
  // A child can outlive its parent: reconciliation holds references to the
  // elements it reuses, so a child's last reference is not always its parent's.
  // Leaving parent_ pointing at freed memory means the next thing to attach
  // that child dereferences it -- and `parent_ != nullptr` also makes it look
  // attached to a tree that no longer exists.
  for (auto& child : children_) {
    child->parent_ = nullptr;
  }
  if (handle_target_) {
    handle_target_->element = nullptr;
  }
  g_elements_destroyed++;
}

Ref<ElementHandleTarget> Element::HandleTarget() {
  if (!handle_target_) {
    handle_target_ = Ref<ElementHandleTarget>::New();
    handle_target_->element = this;
  }
  return handle_target_;
}

void Element::AddChild(Ref<Element> child) {
  // Reconciliation reuses elements, and one can still be attached where it was
  // last frame: a nested component's root that moved between slots reaches
  // here with a parent, because the caller only searched *this* slot for it
  // before deciding it was unattached. Asserting caught that in a debug build
  // and did nothing in a release one, where the element then sat in two
  // parents' child lists at once -- rendered twice, and destroyed by whichever
  // parent went first. `child` is held by value, so dropping the old parent's
  // reference cannot destroy it here.
  child->DetachFromParent();
  child->parent_ = this;
  children_.push_back(std::move(child));
}

void Element::DetachFromParent() {
  if (!parent_) {
    return;
  }
  auto& siblings = parent_->children_;
  for (auto it = siblings.begin(); it != siblings.end(); ++it) {
    if (it->get() == this) {
      siblings.erase(it);
      break;
    }
  }
  parent_ = nullptr;
}

void Element::RemoveChildren() {
  for (auto& child : children_) {
    child->parent_ = nullptr;
  }
  children_.clear();
}

void Element::ReplaceChild(size_t index, Ref<Element> new_child) {
  assert(index < children_.size());
  children_[index]->parent_ = nullptr;
  new_child->parent_ = this;
  children_[index] = std::move(new_child);
}

void Element::MoveChild(size_t from, size_t to) {
  assert(from < children_.size());
  assert(to < children_.size());
  if (from == to) {
    return;
  }
  Ref<Element> child = std::move(children_[from]);
  children_.erase(children_.begin() + from);
  children_.insert(children_.begin() + to, std::move(child));
}

void Element::TruncateChildren(size_t count) {
  if (count < children_.size()) {
    for (size_t i = count; i < children_.size(); ++i) {
      children_[i]->parent_ = nullptr;
    }
    children_.resize(count);
  }
}

void Element::Visit(const std::function<void(Element&)>& f) {
  f(*this);
  for (auto& child : children_) {
    child->Visit(f);
  }
}

void Element::SetAttribute(std::string name, std::string value) {
  ClearResolvedStyles();
  if (const std::string* previous = GetAttribute(name);
      !previous || *previous != value) {
    selector_inputs_changed = true;
  }
  if (name == "id") {
    id = value;
  } else if (name == "class") {
    classes.clear();
    size_t start = 0;
    while (true) {
      size_t pos = value.find(' ', start);
      if (pos == std::string::npos) {
        auto sub = value.substr(start);
        if (!sub.empty()) {
          classes.emplace_back(sub);
        }
        break;
      }
      auto sub = value.substr(start, pos - start);
      if (!sub.empty()) {
        classes.emplace_back(sub);
      }
      start = pos + 1;
    }
  }
  if (!attributes_) {
    attributes_ = std::make_unique<std::map<std::string, std::string>>();
  }
  (*attributes_)[std::move(name)] = std::move(value);
}

void Element::RemoveAttribute(const std::string& name) {
  ClearResolvedStyles();
  if (GetAttribute(name)) {
    selector_inputs_changed = true;
  }
  if (name == "id") {
    id.clear();
  } else if (name == "class") {
    classes.clear();
  }
  if (attributes_) {
    attributes_->erase(name);
    if (attributes_->empty()) {
      attributes_.reset();
    }
  }
}

// NOLINTNEXTLINE(google-default-arguments): see the declaration.
std::string Element::Print(int depth) const {
  std::string out;
  // The component's class rather than the tag it was written as: a dump is
  // read to see what each tag turned into.
  out += std::string(depth, ' ') + "<" +
         std::string(component_ ? component_->Tag() : tag()) + "";
  if (!id.empty()) {
    out += " id=\"" + id + "\"";
  }
  if (!classes.empty()) {
    out += " class=\"";
    for (size_t i = 0; i < classes.size(); ++i) {
      if (i > 0) {
        out += " ";
      }
      out += classes[i];
    }
    out += "\"";
  }
  if (attributes_) {
    for (const auto& [name, value] : *attributes_) {
      if (name == "id" || name == "class") {
        continue;
      }
      out += " " + name + "=\"" + value + "\"";
    }
  }
  out += ">\n";
  for (const auto& child : children_) {
    out += child->Print(depth + 2);
  }
  out += std::string(depth, ' ') + "</" +
         std::string(component_ ? component_->Tag() : tag()) + ">\n";
  return out;
}

std::string_view Element::tag() const {
  if (component_) {
    return component_->WrittenTag();
  }
  return tag_;
}

Element* Element::QuerySelector(std::string_view selector) {
  if (selector.empty()) {
    return nullptr;
  }
  if (selector[0] == '#') {
    std::string_view target_id = selector.substr(1);
    if (id == target_id) {
      return this;
    }
  } else if (selector[0] == '.') {
    std::string_view target_class = selector.substr(1);
    if (std::find(classes.begin(), classes.end(), target_class) !=
        classes.end()) {
      return this;
    }
  } else {
    if (tag() == selector) {
      return this;
    }
  }
  for (const auto& child : children_) {
    if (auto* found = child->QuerySelector(selector)) {
      return found;
    }
  }
  return nullptr;
}

void Element::TriggerTransitions(double current_time_ms) {
  if (!target_style.transitions && active_transitions.empty()) {
    style = target_style;
    UpdateAnimations(current_time_ms);
    return;
  }
  // The properties a transition animates keep their current values, for the
  // handlers below to move from; everything else takes the target's at once.
  // Only those values are kept aside, rather than a copy of the whole style.
  const struct {
    decltype(ComputedStyle::background_color) background_color;
    decltype(ComputedStyle::foreground_color) foreground_color;
    decltype(ComputedStyle::border_color_top) border_color_top;
    decltype(ComputedStyle::border_color_right) border_color_right;
    decltype(ComputedStyle::border_color_bottom) border_color_bottom;
    decltype(ComputedStyle::border_color_left) border_color_left;
    decltype(ComputedStyle::width) width;
    decltype(ComputedStyle::height) height;
    decltype(ComputedStyle::top) top;
    decltype(ComputedStyle::right) right;
    decltype(ComputedStyle::bottom) bottom;
    decltype(ComputedStyle::left) left;
    decltype(ComputedStyle::translate_x) translate_x;
    decltype(ComputedStyle::translate_y) translate_y;
    decltype(ComputedStyle::flex_grow) flex_grow;
    decltype(ComputedStyle::flex_shrink) flex_shrink;
    decltype(ComputedStyle::opacity) opacity;
    decltype(ComputedStyle::has_scrollbar_color_thumb)
        has_scrollbar_color_thumb;
    decltype(ComputedStyle::scrollbar_color_thumb) scrollbar_color_thumb;
    decltype(ComputedStyle::has_scrollbar_color_track)
        has_scrollbar_color_track;
    decltype(ComputedStyle::scrollbar_color_track) scrollbar_color_track;
  } old{
      style.background_color,
      style.foreground_color,
      style.border_color_top,
      style.border_color_right,
      style.border_color_bottom,
      style.border_color_left,
      style.width,
      style.height,
      style.top,
      style.right,
      style.bottom,
      style.left,
      style.translate_x,
      style.translate_y,
      style.flex_grow,
      style.flex_shrink,
      style.opacity,
      style.has_scrollbar_color_thumb,
      style.scrollbar_color_thumb,
      style.has_scrollbar_color_track,
      style.scrollbar_color_track,
  };
  style = target_style;

  style.background_color = old.background_color;
  style.foreground_color = old.foreground_color;
  style.border_color_top = old.border_color_top;
  style.border_color_right = old.border_color_right;
  style.border_color_bottom = old.border_color_bottom;
  style.border_color_left = old.border_color_left;
  style.width = old.width;
  style.height = old.height;
  style.top = old.top;
  style.right = old.right;
  style.bottom = old.bottom;
  style.left = old.left;
  style.translate_x = old.translate_x;
  style.translate_y = old.translate_y;
  style.flex_grow = old.flex_grow;
  style.flex_shrink = old.flex_shrink;
  style.opacity = old.opacity;
  style.has_scrollbar_color_thumb = old.has_scrollbar_color_thumb;
  style.scrollbar_color_thumb = old.scrollbar_color_thumb;
  style.has_scrollbar_color_track = old.has_scrollbar_color_track;
  style.scrollbar_color_track = old.scrollbar_color_track;

  auto HandleColorProperty = [&](std::string_view prop_name,
                                 std::optional<Color>& current_val,
                                 const std::optional<Color>& target_val) {
    bool has_active = active_transitions.count(std::string(prop_name)) > 0;
    bool target_changed = false;
    if (has_active) {
      target_changed =
          (active_transitions[std::string(prop_name)].target_color !=
           target_val.value_or(Color::RGBA(0, 0, 0, 0)));
    } else {
      target_changed = (current_val != target_val);
    }

    if (target_changed) {
      const TransitionConfig* config = FindTransitionConfig(this, prop_name);
      if (config && config->duration_seconds > 0.0f) {
        ActiveTransition trans;
        trans.start_time_ms = current_time_ms + ToMs(config->delay_seconds);
        trans.duration_ms = ToMs(config->duration_seconds);
        trans.timing_function = config->timing_function;
        trans.type = ActiveTransition::Type::Color;
        trans.start_color = current_val.value_or(Color::RGBA(0, 0, 0, 0));
        if (!current_val && target_val) {
          trans.start_color =
              Color::RGBA(target_val->r, target_val->g, target_val->b, 0);
        }
        trans.target_color = target_val.value_or(Color::RGBA(0, 0, 0, 0));
        if (current_val && !target_val) {
          trans.target_color =
              Color::RGBA(current_val->r, current_val->g, current_val->b, 0);
        }
        active_transitions[std::string(prop_name)] = trans;
      } else {
        current_val = target_val;
        active_transitions.erase(std::string(prop_name));
      }
    }
  };

  auto HandleFloatProperty = [&](std::string_view prop_name, float& current_val,
                                 float target_val) {
    bool has_active = active_transitions.count(std::string(prop_name)) > 0;
    bool target_changed = false;
    if (has_active) {
      target_changed =
          (active_transitions[std::string(prop_name)].target_float !=
           target_val);
    } else {
      target_changed = (current_val != target_val);
    }

    if (target_changed) {
      const TransitionConfig* config = FindTransitionConfig(this, prop_name);
      if (config && config->duration_seconds > 0.0f) {
        ActiveTransition trans;
        trans.start_time_ms = current_time_ms + ToMs(config->delay_seconds);
        trans.duration_ms = ToMs(config->duration_seconds);
        trans.timing_function = config->timing_function;
        trans.type = ActiveTransition::Type::Float;
        trans.start_float = current_val;
        trans.target_float = target_val;
        active_transitions[std::string(prop_name)] = trans;
      } else {
        current_val = target_val;
        active_transitions.erase(std::string(prop_name));
      }
    }
  };

  auto HandleLengthProperty = [&](std::string_view prop_name,
                                  Length& current_val, Length target_val) {
    bool has_active = active_transitions.count(std::string(prop_name)) > 0;
    bool target_changed = false;
    if (has_active) {
      const auto& active = active_transitions[std::string(prop_name)];
      target_changed = active.target_length != target_val;
    } else {
      target_changed = current_val != target_val;
    }

    if (target_changed) {
      const TransitionConfig* config = FindTransitionConfig(this, prop_name);
      if (config && config->duration_seconds > 0.0f) {
        ActiveTransition trans;
        trans.start_time_ms = current_time_ms + ToMs(config->delay_seconds);
        trans.duration_ms = ToMs(config->duration_seconds);
        trans.timing_function = config->timing_function;
        trans.type = ActiveTransition::Type::Length;
        trans.start_length = current_val;
        trans.target_length = target_val;
        active_transitions[std::string(prop_name)] = trans;
      } else {
        current_val = target_val;
        active_transitions.erase(std::string(prop_name));
      }
    }
  };

  HandleColorProperty("background-color", style.background_color,
                      target_style.background_color);
  HandleColorProperty("color", style.foreground_color,
                      target_style.foreground_color);

  HandleColorProperty("border-top-color", style.border_color_top,
                      target_style.border_color_top);
  HandleColorProperty("border-right-color", style.border_color_right,
                      target_style.border_color_right);
  HandleColorProperty("border-bottom-color", style.border_color_bottom,
                      target_style.border_color_bottom);
  HandleColorProperty("border-left-color", style.border_color_left,
                      target_style.border_color_left);

  HandleLengthProperty("width", style.width, target_style.width);
  HandleLengthProperty("height", style.height, target_style.height);
  HandleLengthProperty("top", style.top, target_style.top);
  HandleLengthProperty("right", style.right, target_style.right);
  HandleLengthProperty("bottom", style.bottom, target_style.bottom);
  HandleLengthProperty("left", style.left, target_style.left);
  HandleLengthProperty("translate-x", style.translate_x,
                       target_style.translate_x);
  HandleLengthProperty("translate-y", style.translate_y,
                       target_style.translate_y);

  HandleFloatProperty("flex-grow", style.flex_grow, target_style.flex_grow);
  HandleFloatProperty("flex-shrink", style.flex_shrink,
                      target_style.flex_shrink);
  HandleFloatProperty("opacity", style.opacity, target_style.opacity);

  std::optional<Color> current_thumb =
      style.has_scrollbar_color_thumb
          ? std::optional<Color>(style.scrollbar_color_thumb)
          : std::nullopt;
  std::optional<Color> target_thumb =
      target_style.has_scrollbar_color_thumb
          ? std::optional<Color>(target_style.scrollbar_color_thumb)
          : std::nullopt;
  HandleColorProperty("scrollbar-color-thumb", current_thumb, target_thumb);
  if (current_thumb) {
    style.has_scrollbar_color_thumb = true;
    style.scrollbar_color_thumb = *current_thumb;
  } else {
    style.has_scrollbar_color_thumb = false;
  }

  std::optional<Color> current_track =
      style.has_scrollbar_color_track
          ? std::optional<Color>(style.scrollbar_color_track)
          : std::nullopt;
  std::optional<Color> target_track =
      target_style.has_scrollbar_color_track
          ? std::optional<Color>(target_style.scrollbar_color_track)
          : std::nullopt;
  HandleColorProperty("scrollbar-color-track", current_track, target_track);
  if (current_track) {
    style.has_scrollbar_color_track = true;
    style.scrollbar_color_track = *current_track;
  } else {
    style.has_scrollbar_color_track = false;
  }

  UpdateAnimations(current_time_ms);
}

namespace {

// The properties @keyframes can animate: those transitions can interpolate.
// The order gives each its bit in AnimationFrame::properties.
enum Animatable : uint8_t {
  kBackgroundColor,
  kColor,
  kBorderTopColor,
  kBorderRightColor,
  kBorderBottomColor,
  kBorderLeftColor,
  kOpacity,
  kFlexGrow,
  kFlexShrink,
  kWidth,
  kHeight,
  kTop,
  kRight,
  kBottom,
  kLeft,
  kTranslateX,
  kTranslateY,
  kAnimatableCount,
};

constexpr uint32_t Bit(Animatable property) {
  return 1u << property;
}

// The animatable properties a declaration sets, or 0 when it sets none.
uint32_t AnimatedBy(std::string_view property) {
  constexpr uint32_t kAllBorders =
      Bit(kBorderTopColor) | Bit(kBorderRightColor) | Bit(kBorderBottomColor) |
      Bit(kBorderLeftColor);
  if (property == "background-color") {
    return Bit(kBackgroundColor);
  }
  if (property == "color" || property == "foreground-color") {
    return Bit(kColor);
  }
  if (property == "border-color") {
    return kAllBorders;
  }
  if (property == "border-color-top") {
    return Bit(kBorderTopColor);
  }
  if (property == "border-color-right") {
    return Bit(kBorderRightColor);
  }
  if (property == "border-color-bottom") {
    return Bit(kBorderBottomColor);
  }
  if (property == "border-color-left") {
    return Bit(kBorderLeftColor);
  }
  if (property == "opacity") {
    return Bit(kOpacity);
  }
  if (property == "flex-grow") {
    return Bit(kFlexGrow);
  }
  if (property == "flex-shrink") {
    return Bit(kFlexShrink);
  }
  if (property == "width") {
    return Bit(kWidth);
  }
  if (property == "height") {
    return Bit(kHeight);
  }
  if (property == "top") {
    return Bit(kTop);
  }
  if (property == "right") {
    return Bit(kRight);
  }
  if (property == "bottom") {
    return Bit(kBottom);
  }
  if (property == "left") {
    return Bit(kLeft);
  }
  if (property == "inset") {
    return Bit(kTop) | Bit(kRight) | Bit(kBottom) | Bit(kLeft);
  }
  if (property == "translate") {
    return Bit(kTranslateX) | Bit(kTranslateY);
  }
  return 0;
}

// Sets `property` on `out` to its value `progress` of the way from `from` to
// `to`.
void InterpolateAnimatable(Animatable property,
                           const ComputedStyleCore& from,
                           const ComputedStyleCore& to,
                           float progress,
                           ComputedStyleCore& out) {
  switch (property) {
    case kBackgroundColor:
      out.background_color = InterpolateOptionalColor(
          from.background_color, to.background_color, progress);
      return;
    case kColor:
      out.foreground_color = InterpolateOptionalColor(
          from.foreground_color, to.foreground_color, progress);
      return;
    case kBorderTopColor:
      out.border_color_top = InterpolateOptionalColor(
          from.border_color_top, to.border_color_top, progress);
      return;
    case kBorderRightColor:
      out.border_color_right = InterpolateOptionalColor(
          from.border_color_right, to.border_color_right, progress);
      return;
    case kBorderBottomColor:
      out.border_color_bottom = InterpolateOptionalColor(
          from.border_color_bottom, to.border_color_bottom, progress);
      return;
    case kBorderLeftColor:
      out.border_color_left = InterpolateOptionalColor(
          from.border_color_left, to.border_color_left, progress);
      return;
    case kOpacity:
      out.opacity = from.opacity + (to.opacity - from.opacity) * progress;
      return;
    case kFlexGrow:
      out.flex_grow =
          from.flex_grow + (to.flex_grow - from.flex_grow) * progress;
      return;
    case kFlexShrink:
      out.flex_shrink =
          from.flex_shrink + (to.flex_shrink - from.flex_shrink) * progress;
      return;
    case kWidth:
      out.width = InterpolateLength(from.width, to.width, progress);
      return;
    case kHeight:
      out.height = InterpolateLength(from.height, to.height, progress);
      return;
    case kTop:
      out.top = InterpolateLength(from.top, to.top, progress);
      return;
    case kRight:
      out.right = InterpolateLength(from.right, to.right, progress);
      return;
    case kBottom:
      out.bottom = InterpolateLength(from.bottom, to.bottom, progress);
      return;
    case kLeft:
      out.left = InterpolateLength(from.left, to.left, progress);
      return;
    case kTranslateX:
      out.translate_x =
          InterpolateLength(from.translate_x, to.translate_x, progress);
      return;
    case kTranslateY:
      out.translate_y =
          InterpolateLength(from.translate_y, to.translate_y, progress);
      return;
    case kAnimatableCount:
      return;
  }
}

// Applies the declarations of each keyframe block onto `underlying`, noting
// which animatable properties each one sets. A declaration that cannot be
// animated is reported rather than dropped silently.
std::vector<AnimationFrame> BuildFrames(const css::KeyframesRule& keyframes,
                                        const ComputedStyle& underlying,
                                        const css::CustomProperties& vars) {
  std::vector<AnimationFrame> frames;
  frames.reserve(keyframes.frames.size());
  for (const css::Ruleset* block : keyframes.frames) {
    ComputedStyle values = underlying;
    AnimationFrame frame;
    frame.offset = block->keyframe_offset;
    for (const css::Declaration& declaration : block->declarations) {
      if (declaration.property == "animation-timing-function") {
        frame.timing_function = std::string(declaration.value);
        continue;
      }
      const uint32_t animated = AnimatedBy(declaration.property);
      if (!animated) {
        ReportDiagnostic("'" + std::string(declaration.property) +
                         "' in @keyframes " + keyframes.name +
                         " cannot be animated; @keyframes animates colors, "
                         "border colors, opacity, flex-grow, flex-shrink, "
                         "width, height, top/right/bottom/left and "
                         "translate");
        continue;
      }
      if (declaration.value.find("var(") != std::string_view::npos) {
        auto expanded = css::SubstituteVars(declaration.value, vars);
        if (!expanded) {
          continue;
        }
        ApplyStyle(values, {declaration.property, *expanded});
      } else {
        ApplyStyle(values, declaration);
      }
      frame.properties |= animated;
    }
    frame.values = values;
    frames.push_back(std::move(frame));
  }
  return frames;
}

// Where in its keyframes `animation` is at `current_time_ms`, from 0 to 1,
// direction applied. Nothing while it has no effect: before its delay
// without a backwards fill, or once done without a forwards one. Marks the
// animation finished once it is done.
std::optional<float> AnimationProgress(RunningAnimation& animation,
                                       double current_time_ms) {
  const AnimationConfig& config = animation.config;
  const double now =
      animation.paused_at_ms >= 0.0 ? animation.paused_at_ms : current_time_ms;
  const double elapsed =
      now - animation.start_time_ms - ToMs(config.delay_seconds);
  const double duration = ToMs(config.duration_seconds);
  const double iterations = config.iteration_count;

  // The progress `fraction` of the way through iteration `iteration`.
  auto directed = [&](double fraction, double iteration) {
    bool reversed = false;
    const bool odd = std::fmod(iteration, 2.0) >= 1.0;
    switch (config.direction) {
      case AnimationDirection::Normal:
        break;
      case AnimationDirection::Reverse:
        reversed = true;
        break;
      case AnimationDirection::Alternate:
        reversed = odd;
        break;
      case AnimationDirection::AlternateReverse:
        reversed = !odd;
        break;
    }
    const float progress = static_cast<float>(std::clamp(fraction, 0.0, 1.0));
    return reversed ? 1.0f - progress : progress;
  };

  const bool fills_backwards =
      config.fill_mode == AnimationFillMode::Backwards ||
      config.fill_mode == AnimationFillMode::Both;
  const bool fills_forwards = config.fill_mode == AnimationFillMode::Forwards ||
                              config.fill_mode == AnimationFillMode::Both;

  if (elapsed < 0.0) {
    if (fills_backwards) {
      return directed(0.0, 0.0);
    }
    return std::nullopt;
  }

  // A zero duration plays every iteration in no time at all.
  const double active = duration > 0.0 ? iterations * duration : 0.0;
  if (elapsed >= active) {
    animation.finished = true;
    if (!fills_forwards) {
      return std::nullopt;
    }
    if (iterations <= 0.0) {
      return directed(0.0, 0.0);
    }
    // Where the last iteration ended: all the way through it, or part way
    // for a fractional count such as 1.5.
    const double whole = std::floor(iterations);
    if (iterations == whole) {
      return directed(1.0, whole - 1.0);
    }
    return directed(iterations - whole, whole);
  }

  const double iteration = std::floor(elapsed / duration);
  return directed((elapsed - iteration * duration) / duration, iteration);
}

// Writes `animation`'s values at `progress` onto `style`, for the properties
// its keyframes set. A property missing from the first or last keyframe runs
// from or to its value without the animation, as in CSS.
void ApplyAnimationAt(const RunningAnimation& animation,
                      float progress,
                      const ComputedStyleCore& underlying,
                      ComputedStyleCore& style) {
  uint32_t animated = 0;
  for (const AnimationFrame& frame : animation.frames) {
    animated |= frame.properties;
  }
  for (uint8_t i = 0; i < kAnimatableCount; ++i) {
    const auto property = static_cast<Animatable>(i);
    if (!(animated & Bit(property))) {
      continue;
    }
    // The keyframes setting this property on either side of `progress`,
    // standing in the underlying value at 0 and 1 when none sits there.
    float from_offset = 0.0f;
    const ComputedStyleCore* from_values = &underlying;
    std::string_view from_timing;
    float to_offset = 1.0f;
    const ComputedStyleCore* to_values = &underlying;
    bool found_to = false;
    for (const AnimationFrame& frame : animation.frames) {
      if (!(frame.properties & Bit(property))) {
        continue;
      }
      if (frame.offset <= progress) {
        from_offset = frame.offset;
        from_values = &frame.values;
        from_timing = frame.timing_function;
      } else if (!found_to) {
        to_offset = frame.offset;
        to_values = &frame.values;
        found_to = true;
      }
    }
    // Past the last keyframe, which sits at 1: hold its value rather than
    // easing towards the underlying one.
    if (!found_to && from_offset >= 1.0f) {
      to_offset = from_offset;
      to_values = from_values;
    }
    float local = 0.0f;
    if (to_offset > from_offset) {
      local = (progress - from_offset) / (to_offset - from_offset);
    }
    const std::string_view timing =
        from_timing.empty() ? animation.config.timing_function : from_timing;
    InterpolateAnimatable(property, *from_values, *to_values,
                          ApplyEasing(local, timing), style);
  }
}

// Applies every animation `element` runs to its `style`, as of
// `current_time_ms`. Returns whether one of them is still playing, and so
// whether the next frame will differ.
bool ApplyAnimations(Element& element, double current_time_ms) {
  if (element.running_animations.empty()) {
    return false;
  }
  // Every property an animation touches starts from its value without
  // animations, so that one which stops having an effect -- it ended without
  // filling forwards -- puts the property back.
  for (const RunningAnimation& animation : element.running_animations) {
    for (const AnimationFrame& frame : animation.frames) {
      for (uint8_t i = 0; i < kAnimatableCount; ++i) {
        const auto property = static_cast<Animatable>(i);
        if (frame.properties & Bit(property)) {
          InterpolateAnimatable(property, element.target_style,
                                element.target_style, 0.0f, element.style);
        }
      }
    }
  }
  bool playing = false;
  for (RunningAnimation& animation : element.running_animations) {
    const bool was_finished = animation.finished;
    const std::optional<float> progress =
        AnimationProgress(animation, current_time_ms);
    if (progress) {
      ApplyAnimationAt(animation, *progress, element.target_style,
                       element.style);
    }
    if (!was_finished && animation.finished) {
      animation.end_pending = true;
    }
    // The frame an animation finishes on still changes the style.
    playing = playing || (!was_finished && animation.paused_at_ms < 0.0);
  }
  return playing;
}

}  // namespace

void Element::UpdateAnimations(double current_time_ms) {
  // As in CSS, an element with `display: none` runs no animation: they start
  // over once it is displayed, rather than having played out unseen.
  const std::vector<AnimationConfig>* declared =
      target_style.display_none ? nullptr : target_style.animations.get();
  if (!declared && running_animations.empty()) {
    return;
  }
  std::vector<RunningAnimation> next;
  if (declared) {
    next.reserve(declared->size());
    for (const AnimationConfig& config : *declared) {
      if (config.name.empty() || config.name == "none") {
        continue;
      }
      // An animation already running carries on, whatever else about it
      // changed: only removing it and declaring it again restarts it.
      auto running = std::ranges::find_if(
          running_animations, [&](const RunningAnimation& candidate) {
            return candidate.config.name == config.name;
          });
      RunningAnimation animation;
      if (running != running_animations.end()) {
        animation = std::move(*running);
        running_animations.erase(running);
        if (config.paused && animation.paused_at_ms < 0.0) {
          animation.paused_at_ms = current_time_ms;
        } else if (!config.paused && animation.paused_at_ms >= 0.0) {
          animation.start_time_ms += current_time_ms - animation.paused_at_ms;
          animation.paused_at_ms = -1.0;
        }
      } else {
        if (!config.keyframes) {
          ReportDiagnostic("animation '" + config.name +
                           "' has no matching @keyframes rule");
        }
        animation.start_time_ms = current_time_ms;
        animation.paused_at_ms = config.paused ? current_time_ms : -1.0;
      }
      animation.config = config;
      // Rebuilt each time: the keyframes are applied over the element's own
      // style, which may have changed since.
      animation.frames.clear();
      if (config.keyframes) {
        animation.frames =
            BuildFrames(*config.keyframes, target_style, custom_properties);
      }
      next.push_back(std::move(animation));
    }
  }
  // Whatever an animation that just stopped running animated returns to its
  // value without it.
  for (const RunningAnimation& stopped : running_animations) {
    for (const AnimationFrame& frame : stopped.frames) {
      for (uint8_t i = 0; i < kAnimatableCount; ++i) {
        const auto property = static_cast<Animatable>(i);
        if (frame.properties & Bit(property)) {
          InterpolateAnimatable(property, target_style, target_style, 0.0f,
                                style);
        }
      }
    }
  }
  running_animations = std::move(next);
  ApplyAnimations(*this, current_time_ms);
}

bool Element::HasPlayingAnimations(bool include_infinite) const {
  return std::ranges::any_of(
      running_animations, [&](const RunningAnimation& animation) {
        return !animation.finished && animation.paused_at_ms < 0.0 &&
               (include_infinite ||
                !std::isinf(animation.config.iteration_count));
      });
}

bool Element::AnimatesLayout() const {
  if (IsAnimatingScroll()) {
    return true;
  }
  static constexpr std::string_view kPaintOnly[] = {
      "background-color",      "color",
      "foreground-color",      "border-top-color",
      "border-right-color",    "border-bottom-color",
      "border-left-color",     "opacity",
      "scrollbar-color-thumb", "scrollbar-color-track",
      "translate-x",           "translate-y",
  };
  for (const auto& [property, transition] : active_transitions) {
    if (std::ranges::find(kPaintOnly, property) == std::end(kPaintOnly)) {
      return true;
    }
  }
  constexpr uint32_t kPaintOnlyBits =
      Bit(kBackgroundColor) | Bit(kColor) | Bit(kBorderTopColor) |
      Bit(kBorderRightColor) | Bit(kBorderBottomColor) | Bit(kBorderLeftColor) |
      Bit(kOpacity) | Bit(kTranslateX) | Bit(kTranslateY);
  for (const RunningAnimation& animation : running_animations) {
    if (animation.finished) {
      continue;
    }
    for (const AnimationFrame& frame : animation.frames) {
      if (frame.properties & ~kPaintOnlyBits) {
        return true;
      }
    }
  }
  return false;
}

bool Element::HasEndedAnimations() const {
  return std::ranges::any_of(running_animations,
                             &RunningAnimation::end_pending);
}

bool Element::TakeEndedAnimations() {
  bool ended = false;
  for (RunningAnimation& animation : running_animations) {
    ended = ended || animation.end_pending;
    animation.end_pending = false;
  }
  return ended;
}

bool Element::TickTransitions(double current_time_ms) {
  if (active_transitions.empty() && !scroll_y_animating_ &&
      !scroll_x_animating_ && !visual_scroll_y_animating_ &&
      !visual_scroll_x_animating_ && !HasPlayingAnimations()) {
    return false;
  }

  bool updated = false;

  if (scroll_y_animating_ || visual_scroll_y_animating_) {
    ScrollAxis axis_y{scroll_y_,
                      scroll_height_,
                      target_scroll_y_,
                      anim_scroll_y_,
                      start_scroll_y_,
                      scroll_y_anim_start_time_,
                      scroll_y_animating_,
                      visual_scroll_y_,
                      visual_start_scroll_y_,
                      visual_target_scroll_y_,
                      visual_scroll_y_anim_start_time_,
                      visual_scroll_y_animating_};
    updated |= axis_y.Tick(current_time_ms, kScrollAnimationDurationMs);
  }

  if (scroll_x_animating_ || visual_scroll_x_animating_) {
    ScrollAxis axis_x{scroll_x_,
                      scroll_width_,
                      target_scroll_x_,
                      anim_scroll_x_,
                      start_scroll_x_,
                      scroll_x_anim_start_time_,
                      scroll_x_animating_,
                      visual_scroll_x_,
                      visual_start_scroll_x_,
                      visual_target_scroll_x_,
                      visual_scroll_x_anim_start_time_,
                      visual_scroll_x_animating_};
    updated |= axis_x.Tick(current_time_ms, kScrollAnimationDurationMs);
  }

  if (!active_transitions.empty()) {
    std::vector<std::string> to_remove;

    for (auto& [prop_name, trans] : active_transitions) {
      if (current_time_ms < trans.start_time_ms) {
        continue;
      }

      float t = 1.0f;
      if (trans.duration_ms > 0.0f) {
        t = static_cast<float>((current_time_ms - trans.start_time_ms) /
                               trans.duration_ms);
      }
      if (t < 0.0f) {
        t = 0.0f;
      }
      if (t > 1.0f) {
        t = 1.0f;
      }

      float eased_t = ApplyEasing(t, trans.timing_function);

      if (trans.type == ActiveTransition::Type::Color) {
        std::optional<Color> start = trans.start_color;
        std::optional<Color> target = trans.target_color;
        auto val = InterpolateOptionalColor(start, target, eased_t);

        if (prop_name == "background-color") {
          style.background_color = val;
        } else if (prop_name == "color" || prop_name == "foreground-color") {
          style.foreground_color = val;
        } else if (prop_name == "border-top-color") {
          style.border_color_top = val;
        } else if (prop_name == "border-right-color") {
          style.border_color_right = val;
        } else if (prop_name == "border-bottom-color") {
          style.border_color_bottom = val;
        } else if (prop_name == "border-left-color") {
          style.border_color_left = val;
        } else if (prop_name == "scrollbar-color-thumb") {
          if (val) {
            style.has_scrollbar_color_thumb = true;
            style.scrollbar_color_thumb = *val;
          } else {
            style.has_scrollbar_color_thumb = false;
          }
        } else if (prop_name == "scrollbar-color-track") {
          if (val) {
            style.has_scrollbar_color_track = true;
            style.scrollbar_color_track = *val;
          } else {
            style.has_scrollbar_color_track = false;
          }
        }
        updated = true;
      } else if (trans.type == ActiveTransition::Type::Float) {
        float val = trans.start_float +
                    (trans.target_float - trans.start_float) * eased_t;
        if (prop_name == "flex-grow") {
          style.flex_grow = val;
        } else if (prop_name == "flex-shrink") {
          style.flex_shrink = val;
        } else if (prop_name == "opacity") {
          style.opacity = val;
        }
        updated = true;
      } else if (trans.type == ActiveTransition::Type::Length) {
        Length val =
            InterpolateLength(trans.start_length, trans.target_length, eased_t);
        if (prop_name == "width") {
          style.width = val;
        } else if (prop_name == "height") {
          style.height = val;
        } else if (prop_name == "top") {
          style.top = val;
        } else if (prop_name == "right") {
          style.right = val;
        } else if (prop_name == "bottom") {
          style.bottom = val;
        } else if (prop_name == "left") {
          style.left = val;
        } else if (prop_name == "translate-x") {
          style.translate_x = val;
        } else if (prop_name == "translate-y") {
          style.translate_y = val;
        }
        updated = true;
      }

      if (t >= 1.0f) {
        to_remove.push_back(prop_name);
        if (prop_name == "background-color") {
          style.background_color = target_style.background_color;
        } else if (prop_name == "color" || prop_name == "foreground-color") {
          style.foreground_color = target_style.foreground_color;
        } else if (prop_name == "border-top-color") {
          style.border_color_top = target_style.border_color_top;
        } else if (prop_name == "border-right-color") {
          style.border_color_right = target_style.border_color_right;
        } else if (prop_name == "border-bottom-color") {
          style.border_color_bottom = target_style.border_color_bottom;
        } else if (prop_name == "border-left-color") {
          style.border_color_left = target_style.border_color_left;
        } else if (prop_name == "width") {
          style.width = target_style.width;
        } else if (prop_name == "height") {
          style.height = target_style.height;
        } else if (prop_name == "top") {
          style.top = target_style.top;
        } else if (prop_name == "right") {
          style.right = target_style.right;
        } else if (prop_name == "bottom") {
          style.bottom = target_style.bottom;
        } else if (prop_name == "left") {
          style.left = target_style.left;
        } else if (prop_name == "translate-x") {
          style.translate_x = target_style.translate_x;
        } else if (prop_name == "translate-y") {
          style.translate_y = target_style.translate_y;
        } else if (prop_name == "flex-grow") {
          style.flex_grow = target_style.flex_grow;
        } else if (prop_name == "flex-shrink") {
          style.flex_shrink = target_style.flex_shrink;
        } else if (prop_name == "opacity") {
          style.opacity = target_style.opacity;
        } else if (prop_name == "scrollbar-color-thumb") {
          if (target_style.has_scrollbar_color_thumb) {
            style.has_scrollbar_color_thumb = true;
            style.scrollbar_color_thumb = target_style.scrollbar_color_thumb;
          } else {
            style.has_scrollbar_color_thumb = false;
          }
        } else if (prop_name == "scrollbar-color-track") {
          if (target_style.has_scrollbar_color_track) {
            style.has_scrollbar_color_track = true;
            style.scrollbar_color_track = target_style.scrollbar_color_track;
          } else {
            style.has_scrollbar_color_track = false;
          }
        }
      }
    }

    for (const auto& prop : to_remove) {
      active_transitions.erase(prop);
    }
  }

  // After the transitions: an animation overrides a transition of the same
  // property, as in CSS.
  if (!running_animations.empty()) {
    updated |= ApplyAnimations(*this, current_time_ms);
  }

  return updated;
}

void Element::set_scroll_y(int y, bool smooth) {
  ScrollAxis axis_y{scroll_y_,
                    scroll_height_,
                    target_scroll_y_,
                    anim_scroll_y_,
                    start_scroll_y_,
                    scroll_y_anim_start_time_,
                    scroll_y_animating_,
                    visual_scroll_y_,
                    visual_start_scroll_y_,
                    visual_target_scroll_y_,
                    visual_scroll_y_anim_start_time_,
                    visual_scroll_y_animating_};
  axis_y.Set(y, smooth, style.scroll_behavior);
}

void Element::set_scroll_x(int x, bool smooth) {
  ScrollAxis axis_x{scroll_x_,
                    scroll_width_,
                    target_scroll_x_,
                    anim_scroll_x_,
                    start_scroll_x_,
                    scroll_x_anim_start_time_,
                    scroll_x_animating_,
                    visual_scroll_x_,
                    visual_start_scroll_x_,
                    visual_target_scroll_x_,
                    visual_scroll_x_anim_start_time_,
                    visual_scroll_x_animating_};
  axis_x.Set(x, smooth, style.scroll_behavior);
}

void Element::ClampScrollY(int max_scroll) {
  ScrollAxis axis_y{scroll_y_,
                    scroll_height_,
                    target_scroll_y_,
                    anim_scroll_y_,
                    start_scroll_y_,
                    scroll_y_anim_start_time_,
                    scroll_y_animating_,
                    visual_scroll_y_,
                    visual_start_scroll_y_,
                    visual_target_scroll_y_,
                    visual_scroll_y_anim_start_time_,
                    visual_scroll_y_animating_};
  axis_y.Clamp(max_scroll);
}

void Element::ClampScrollX(int max_scroll) {
  ScrollAxis axis_x{scroll_x_,
                    scroll_width_,
                    target_scroll_x_,
                    anim_scroll_x_,
                    start_scroll_x_,
                    scroll_x_anim_start_time_,
                    scroll_x_animating_,
                    visual_scroll_x_,
                    visual_start_scroll_x_,
                    visual_target_scroll_x_,
                    visual_scroll_x_anim_start_time_,
                    visual_scroll_x_animating_};
  axis_x.Clamp(max_scroll);
}

}  // namespace rtxui
