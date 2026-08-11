#include "rtxui/dom/element.hpp"

#include <algorithm>
#include <cassert>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <fstream>

#include "rtxui/internal/component.hpp"

#include <atomic>
std::atomic<int> g_elements_created{0};
std::atomic<int> g_elements_destroyed{0};

namespace rtxui {

namespace time {
ClockFn custom_clock = nullptr;
void SetCustomClock(ClockFn clock) {
  custom_clock = clock;
}
double GetTimeMs() {
  if (custom_clock) {
    return custom_clock();
  }
  auto now = std::chrono::steady_clock::now();
  return std::chrono::duration<double, std::milli>(now.time_since_epoch())
      .count();
}
}  // namespace time

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
    return t < 0.5f ? 2.0f * t * t : 1.0f - std::pow(-2.0f * t + 2.0f, 2.0f) / 2.0f;
  }

  // Cubic
  if (timing == "ease-in-cubic") {
    return t * t * t;
  }
  if (timing == "ease-out-cubic") {
    return 1.0f - std::pow(1.0f - t, 3.0f);
  }
  if (timing == "ease-in-out-cubic") {
    return t < 0.5f ? 4.0f * t * t * t : 1.0f - std::pow(-2.0f * t + 2.0f, 3.0f) / 2.0f;
  }

  // Quart
  if (timing == "ease-in-quart") {
    return t * t * t * t;
  }
  if (timing == "ease-out-quart") {
    return 1.0f - std::pow(1.0f - t, 4.0f);
  }
  if (timing == "ease-in-out-quart") {
    return t < 0.5f ? 8.0f * t * t * t * t : 1.0f - std::pow(-2.0f * t + 2.0f, 4.0f) / 2.0f;
  }

  // Quint
  if (timing == "ease-in-quint") {
    return t * t * t * t * t;
  }
  if (timing == "ease-out-quint") {
    return 1.0f - std::pow(1.0f - t, 5.0f);
  }
  if (timing == "ease-in-out-quint") {
    return t < 0.5f ? 16.0f * t * t * t * t * t : 1.0f - std::pow(-2.0f * t + 2.0f, 5.0f) / 2.0f;
  }

  // Expo
  if (timing == "ease-in-expo") {
    return std::pow(2.0f, 10.0f * t - 10.0f);
  }
  if (timing == "ease-out-expo") {
    return 1.0f - std::pow(2.0f, -10.0f * t);
  }
  if (timing == "ease-in-out-expo") {
    return t < 0.5f ? std::pow(2.0f, 20.0f * t - 10.0f) / 2.0f : (2.0f - std::pow(2.0f, -20.0f * t + 10.0f)) / 2.0f;
  }

  // Circ
  if (timing == "ease-in-circ") {
    return 1.0f - std::sqrt(1.0f - t * t);
  }
  if (timing == "ease-out-circ") {
    return std::sqrt(1.0f - (t - 1.0f) * (t - 1.0f));
  }
  if (timing == "ease-in-out-circ") {
    return t < 0.5f ? (1.0f - std::sqrt(1.0f - 4.0f * t * t)) / 2.0f : (std::sqrt(1.0f - std::pow(-2.0f * t + 2.0f, 2.0f)) + 1.0f) / 2.0f;
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
        ? (std::pow(2.0f * t, 2.0f) * ((c2 + 1.0f) * 2.0f * t - c2)) / 2.0f
        : (std::pow(2.0f * t - 2.0f, 2.0f) * ((c2 + 1.0f) * (t * 2.0f - 2.0f) + c2) + 2.0f) / 2.0f;
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

  if (timing.starts_with("cubic-bezier(") && timing.ends_with(")")) {
    std::string_view params = timing.substr(13, timing.size() - 14);
    float x1 = 0.0f, y1 = 0.0f, x2 = 0.0f, y2 = 0.0f;
    std::string params_str(params);
    if (std::sscanf(params_str.c_str(), "%f,%f,%f,%f", &x1, &y1, &x2, &y2) == 4) {
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

Length InterpolateLength(Length start, Length target, float progress) {
  if (start.unit == target.unit) {
    return {start.value + (target.value - start.value) * progress, start.unit};
  }
  return progress >= 1.0f ? target : start;
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
    if (config.property == "border-color" && property.starts_with("border-") &&
        property.ends_with("-color")) {
      return &config;
    }
    if (config.property == "scrollbar-color" &&
        (property == "scrollbar-color-thumb" || property == "scrollbar-color-track")) {
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
        float t = static_cast<float>((current_time_ms - anim_start_time) / duration_ms);
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
        float t = static_cast<float>((current_time_ms - visual_anim_start_time) / duration_ms);
        if (t < 0.0f) {
          t = 0.0f;
        }
        if (t >= 1.0f) {
          visual = visual_target;
          visual_animating = false;
          updated = true;
        } else {
          float eased_t = ApplyEasing(t, "ease");
          float next_anim = visual_start + (visual_target - visual_start) * eased_t;
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

Element::~Element() {
  g_elements_destroyed++;
}

void Element::AddChild(Ref<Element> child) {
  assert(child->parent_ == nullptr);
  child->parent_ = this;
  children_.push_back(std::move(child));
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
  if (from == to) return;
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
          classes.push_back(std::string(sub));
        }
        break;
      }
      auto sub = value.substr(start, pos - start);
      if (!sub.empty()) {
        classes.push_back(std::string(sub));
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

std::string Element::Print(int depth) const {
  std::string out;
  out += std::string(depth, ' ') + "<" + std::string(tag()) + "";
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
  out += std::string(depth, ' ') + "</" + std::string(tag()) + ">\n";
  return out;
}

std::string_view Element::tag() const {
  if (component_) {
    return component_->Tag();
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
    return;
  }
  auto old_style = style;
  style = target_style;

  style.background_color = old_style.background_color;
  style.foreground_color = old_style.foreground_color;
  style.border_color_top = old_style.border_color_top;
  style.border_color_right = old_style.border_color_right;
  style.border_color_bottom = old_style.border_color_bottom;
  style.border_color_left = old_style.border_color_left;
  style.width = old_style.width;
  style.height = old_style.height;
  style.flex_grow = old_style.flex_grow;
  style.flex_shrink = old_style.flex_shrink;
  style.opacity = old_style.opacity;
  style.has_scrollbar_color_thumb = old_style.has_scrollbar_color_thumb;
  style.scrollbar_color_thumb = old_style.scrollbar_color_thumb;
  style.has_scrollbar_color_track = old_style.has_scrollbar_color_track;
  style.scrollbar_color_track = old_style.scrollbar_color_track;

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
        trans.start_time_ms = current_time_ms + config->delay_seconds * 1000.0;
        trans.duration_ms = config->duration_seconds * 1000.0;
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
        trans.start_time_ms = current_time_ms + config->delay_seconds * 1000.0;
        trans.duration_ms = config->duration_seconds * 1000.0;
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
      target_changed = (active.target_length.value != target_val.value ||
                        active.target_length.unit != target_val.unit);
    } else {
      target_changed = (current_val.value != target_val.value ||
                        current_val.unit != target_val.unit);
    }

    if (target_changed) {
      const TransitionConfig* config = FindTransitionConfig(this, prop_name);
      if (config && config->duration_seconds > 0.0f) {
        ActiveTransition trans;
        trans.start_time_ms = current_time_ms + config->delay_seconds * 1000.0;
        trans.duration_ms = config->duration_seconds * 1000.0;
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
  HandleColorProperty("foreground-color", style.foreground_color,
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

  HandleFloatProperty("flex-grow", style.flex_grow, target_style.flex_grow);
  HandleFloatProperty("flex-shrink", style.flex_shrink,
                      target_style.flex_shrink);
  HandleFloatProperty("opacity", style.opacity, target_style.opacity);

  std::optional<Color> current_thumb = style.has_scrollbar_color_thumb ? std::optional<Color>(style.scrollbar_color_thumb) : std::nullopt;
  std::optional<Color> target_thumb = target_style.has_scrollbar_color_thumb ? std::optional<Color>(target_style.scrollbar_color_thumb) : std::nullopt;
  HandleColorProperty("scrollbar-color-thumb", current_thumb, target_thumb);
  if (current_thumb) {
    style.has_scrollbar_color_thumb = true;
    style.scrollbar_color_thumb = *current_thumb;
  } else {
    style.has_scrollbar_color_thumb = false;
  }

  std::optional<Color> current_track = style.has_scrollbar_color_track ? std::optional<Color>(style.scrollbar_color_track) : std::nullopt;
  std::optional<Color> target_track = target_style.has_scrollbar_color_track ? std::optional<Color>(target_style.scrollbar_color_track) : std::nullopt;
  HandleColorProperty("scrollbar-color-track", current_track, target_track);
  if (current_track) {
    style.has_scrollbar_color_track = true;
    style.scrollbar_color_track = *current_track;
  } else {
    style.has_scrollbar_color_track = false;
  }
}

bool Element::TickTransitions(double current_time_ms) {
  if (active_transitions.empty() && !scroll_y_animating_ &&
      !scroll_x_animating_ && !visual_scroll_y_animating_ &&
      !visual_scroll_x_animating_) {
    return false;
  }

  bool updated = false;

  if (scroll_y_animating_ || visual_scroll_y_animating_) {
    ScrollAxis axis_y{
      scroll_y_, scroll_height_, target_scroll_y_, anim_scroll_y_, start_scroll_y_, scroll_y_anim_start_time_, scroll_y_animating_,
      visual_scroll_y_, visual_start_scroll_y_, visual_target_scroll_y_, visual_scroll_y_anim_start_time_, visual_scroll_y_animating_
    };
    updated |= axis_y.Tick(current_time_ms, kScrollAnimationDurationMs);
  }

  if (scroll_x_animating_ || visual_scroll_x_animating_) {
    ScrollAxis axis_x{
      scroll_x_, scroll_width_, target_scroll_x_, anim_scroll_x_, start_scroll_x_, scroll_x_anim_start_time_, scroll_x_animating_,
      visual_scroll_x_, visual_start_scroll_x_, visual_target_scroll_x_, visual_scroll_x_anim_start_time_, visual_scroll_x_animating_
    };
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

  return updated;
}

void Element::set_scroll_y(int y, bool smooth) {
  ScrollAxis axis_y{
    scroll_y_, scroll_height_, target_scroll_y_, anim_scroll_y_, start_scroll_y_, scroll_y_anim_start_time_, scroll_y_animating_,
    visual_scroll_y_, visual_start_scroll_y_, visual_target_scroll_y_, visual_scroll_y_anim_start_time_, visual_scroll_y_animating_
  };
  axis_y.Set(y, smooth, style.scroll_behavior);
}

void Element::set_scroll_x(int x, bool smooth) {
  ScrollAxis axis_x{
    scroll_x_, scroll_width_, target_scroll_x_, anim_scroll_x_, start_scroll_x_, scroll_x_anim_start_time_, scroll_x_animating_,
    visual_scroll_x_, visual_start_scroll_x_, visual_target_scroll_x_, visual_scroll_x_anim_start_time_, visual_scroll_x_animating_
  };
  axis_x.Set(x, smooth, style.scroll_behavior);
}

void Element::ClampScrollY(int max_scroll) {
  ScrollAxis axis_y{
    scroll_y_, scroll_height_, target_scroll_y_, anim_scroll_y_, start_scroll_y_, scroll_y_anim_start_time_, scroll_y_animating_,
    visual_scroll_y_, visual_start_scroll_y_, visual_target_scroll_y_, visual_scroll_y_anim_start_time_, visual_scroll_y_animating_
  };
  axis_y.Clamp(max_scroll);
}

void Element::ClampScrollX(int max_scroll) {
  ScrollAxis axis_x{
    scroll_x_, scroll_width_, target_scroll_x_, anim_scroll_x_, start_scroll_x_, scroll_x_anim_start_time_, scroll_x_animating_,
    visual_scroll_x_, visual_start_scroll_x_, visual_target_scroll_x_, visual_scroll_x_anim_start_time_, visual_scroll_x_animating_
  };
  axis_x.Clamp(max_scroll);
}

}  // namespace rtxui
