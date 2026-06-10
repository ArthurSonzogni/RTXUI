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

float ApplyEasing(float t, std::string_view timing) {
  if (t <= 0.0f) {
    return 0.0f;
  }
  if (t >= 1.0f) {
    return 1.0f;
  }

  if (timing == "linear") {
    return t;
  } else if (timing == "ease-in") {
    return SolveCubicBezier(0.42f, 0.0f, 1.0f, 1.0f, t);
  } else if (timing == "ease-out") {
    return SolveCubicBezier(0.0f, 0.0f, 0.58f, 1.0f, t);
  } else if (timing == "ease-in-out") {
    return SolveCubicBezier(0.42f, 0.0f, 0.58f, 1.0f, t);
  } else if (timing.starts_with("cubic-bezier(") && timing.ends_with(")")) {
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
  }
  return nullptr;
}

constexpr double kScrollAnimationDurationMs = 50.0;

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

void Element::TruncateChildren(size_t count) {
  if (count < children_.size()) {
    for (size_t i = count; i < children_.size(); ++i) {
      children_[i]->parent_ = nullptr;
    }
    children_.resize(count);
  }
}

void Element::Visit(std::function<void(Element&)> f) {
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
}

bool Element::TickTransitions(double current_time_ms) {
  if (active_transitions.empty() && !scroll_y_animating_ &&
      !scroll_x_animating_ && !visual_scroll_y_animating_ &&
      !visual_scroll_x_animating_) {
    return false;
  }

  bool updated = false;

  if (scroll_y_animating_) {
    if (current_time_ms >= scroll_y_anim_start_time_) {
      float t =
          static_cast<float>((current_time_ms - scroll_y_anim_start_time_) /
                             kScrollAnimationDurationMs);
      if (t < 0.0f) {
        t = 0.0f;
      }
      if (t >= 1.0f) {
        scroll_y_ = target_scroll_y_;
        anim_scroll_y_ = static_cast<float>(target_scroll_y_);
        scroll_y_animating_ = false;
        visual_scroll_y_ = static_cast<float>(target_scroll_y_);
        updated = true;
      } else {
        float eased_t = ApplyEasing(t, "ease");
        float next_anim =
            start_scroll_y_ + (target_scroll_y_ - start_scroll_y_) * eased_t;
        int next_scroll = static_cast<int>(std::round(next_anim));
        if (next_scroll != scroll_y_ || next_anim != anim_scroll_y_) {
          scroll_y_ = next_scroll;
          anim_scroll_y_ = next_anim;
          visual_scroll_y_ = next_anim;
          updated = true;
        }
      }
    }
  } else if (visual_scroll_y_animating_) {
    if (current_time_ms >= visual_scroll_y_anim_start_time_) {
      float t = static_cast<float>(
          (current_time_ms - visual_scroll_y_anim_start_time_) /
          kScrollAnimationDurationMs);
      if (t < 0.0f) {
        t = 0.0f;
      }
      if (t >= 1.0f) {
        visual_scroll_y_ = visual_target_scroll_y_;
        visual_scroll_y_animating_ = false;
        updated = true;
      } else {
        float eased_t = ApplyEasing(t, "ease");
        float next_anim =
            visual_start_scroll_y_ +
            (visual_target_scroll_y_ - visual_start_scroll_y_) * eased_t;
        if (next_anim != visual_scroll_y_) {
          visual_scroll_y_ = next_anim;
          updated = true;
        }
      }
    }
  }

  if (scroll_x_animating_) {
    if (current_time_ms >= scroll_x_anim_start_time_) {
      float t =
          static_cast<float>((current_time_ms - scroll_x_anim_start_time_) /
                             kScrollAnimationDurationMs);
      if (t < 0.0f) {
        t = 0.0f;
      }
      if (t >= 1.0f) {
        scroll_x_ = target_scroll_x_;
        anim_scroll_x_ = static_cast<float>(target_scroll_x_);
        scroll_x_animating_ = false;
        visual_scroll_x_ = static_cast<float>(target_scroll_x_);
        updated = true;
      } else {
        float eased_t = ApplyEasing(t, "ease");
        float next_anim =
            start_scroll_x_ + (target_scroll_x_ - start_scroll_x_) * eased_t;
        int next_scroll = static_cast<int>(std::round(next_anim));
        if (next_scroll != scroll_x_ || next_anim != anim_scroll_x_) {
          scroll_x_ = next_scroll;
          anim_scroll_x_ = next_anim;
          visual_scroll_x_ = next_anim;
          updated = true;
        }
      }
    }
  } else if (visual_scroll_x_animating_) {
    if (current_time_ms >= visual_scroll_x_anim_start_time_) {
      float t = static_cast<float>(
          (current_time_ms - visual_scroll_x_anim_start_time_) /
          kScrollAnimationDurationMs);
      if (t < 0.0f) {
        t = 0.0f;
      }
      if (t >= 1.0f) {
        visual_scroll_x_ = visual_target_scroll_x_;
        visual_scroll_x_animating_ = false;
        updated = true;
      } else {
        float eased_t = ApplyEasing(t, "ease");
        float next_anim =
            visual_start_scroll_x_ +
            (visual_target_scroll_x_ - visual_start_scroll_x_) * eased_t;
        if (next_anim != visual_scroll_x_) {
          visual_scroll_x_ = next_anim;
          updated = true;
        }
      }
    }
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
      }
    }

    for (const auto& prop : to_remove) {
      active_transitions.erase(prop);
    }
  }

  return updated;
}

void Element::set_scroll_y(int y, bool smooth) {
  if (!smooth || style.scroll_behavior != ScrollBehavior::Smooth) {
    scroll_y_ = y;
    target_scroll_y_ = y;
    anim_scroll_y_ = static_cast<float>(y);
    scroll_y_animating_ = false;

    visual_scroll_y_ = static_cast<float>(y);
    visual_target_scroll_y_ = static_cast<float>(y);
    visual_scroll_y_animating_ = false;
  } else {
    if (y == target_scroll_y_) {
      return;
    }
    start_scroll_y_ = anim_scroll_y_;
    target_scroll_y_ = y;
    scroll_y_anim_start_time_ = time::GetTimeMs();
    scroll_y_animating_ = true;

    visual_target_scroll_y_ = static_cast<float>(y);
    visual_scroll_y_animating_ = false;
  }
}

void Element::set_scroll_x(int x, bool smooth) {
  if (!smooth || style.scroll_behavior != ScrollBehavior::Smooth) {
    scroll_x_ = x;
    target_scroll_x_ = x;
    anim_scroll_x_ = static_cast<float>(x);
    scroll_x_animating_ = false;

    visual_scroll_x_ = static_cast<float>(x);
    visual_target_scroll_x_ = static_cast<float>(x);
    visual_scroll_x_animating_ = false;
  } else {
    if (x == target_scroll_x_) {
      return;
    }
    start_scroll_x_ = anim_scroll_x_;
    target_scroll_x_ = x;
    scroll_x_anim_start_time_ = time::GetTimeMs();
    scroll_x_animating_ = true;

    visual_target_scroll_x_ = static_cast<float>(x);
    visual_scroll_x_animating_ = false;
  }
}

void Element::ClampScrollY(int max_scroll) {
  if (target_scroll_y_ > max_scroll) {
    target_scroll_y_ = max_scroll;
  }
  if (scroll_y_ > max_scroll) {
    scroll_y_ = max_scroll;
  }
  if (anim_scroll_y_ > max_scroll) {
    anim_scroll_y_ = static_cast<float>(max_scroll);
  }
  if (start_scroll_y_ > max_scroll) {
    start_scroll_y_ = static_cast<float>(max_scroll);
  }

  if (visual_target_scroll_y_ > max_scroll) {
    visual_target_scroll_y_ = static_cast<float>(max_scroll);
  }
  if (visual_scroll_y_ > max_scroll) {
    visual_scroll_y_ = static_cast<float>(max_scroll);
  }
  if (visual_start_scroll_y_ > max_scroll) {
    visual_start_scroll_y_ = static_cast<float>(max_scroll);
  }

  if (anim_scroll_y_ == static_cast<float>(target_scroll_y_)) {
    scroll_y_animating_ = false;
  }
  if (visual_scroll_y_ == visual_target_scroll_y_) {
    visual_scroll_y_animating_ = false;
  }
}

void Element::ClampScrollX(int max_scroll) {
  if (target_scroll_x_ > max_scroll) {
    target_scroll_x_ = max_scroll;
  }
  if (scroll_x_ > max_scroll) {
    scroll_x_ = max_scroll;
  }
  if (anim_scroll_x_ > max_scroll) {
    anim_scroll_x_ = static_cast<float>(max_scroll);
  }
  if (start_scroll_x_ > max_scroll) {
    start_scroll_x_ = static_cast<float>(max_scroll);
  }

  if (visual_target_scroll_x_ > max_scroll) {
    visual_target_scroll_x_ = static_cast<float>(max_scroll);
  }
  if (visual_scroll_x_ > max_scroll) {
    visual_scroll_x_ = static_cast<float>(max_scroll);
  }
  if (visual_start_scroll_x_ > max_scroll) {
    visual_start_scroll_x_ = static_cast<float>(max_scroll);
  }

  if (anim_scroll_x_ == static_cast<float>(target_scroll_x_)) {
    scroll_x_animating_ = false;
  }
  if (visual_scroll_x_ == visual_target_scroll_x_) {
    visual_scroll_x_animating_ = false;
  }
}

}  // namespace rtxui
