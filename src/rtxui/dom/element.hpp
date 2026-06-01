#ifndef DOM_ElEMENT_HPP_
#define DOM_ElEMENT_HPP_

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "rtxui/internal/refcounted.hpp"
#include "rtxui/layout/style.hpp"

namespace rtxui {

struct ActiveTransition {
  double start_time_ms = 0.0;
  double duration_ms = 0.0;
  std::string timing_function = "ease";

  enum class Type { Color, Float, Length } type;

  Color start_color;
  Color target_color;

  float start_float = 0.0f;
  float target_float = 0.0f;

  Length start_length;
  Length target_length;
};

class ComponentBase;

class Element : public RefCounted {
 public:
  Element();
  Element(const ComponentBase* component);
  virtual ~Element() = default;

  // Non-copyable, non-movable.
  Element(const Element&) = delete;
  Element& operator=(const Element&) = delete;
  Element(Element&&) = default;
  Element& operator=(Element&&) = default;

  void AddChild(Ref<Element> child);
  void RemoveChildren();
  void Visit(std::function<void(Element&)> f);

  // Hierarchical accessors.
  Element* Parent() { return parent_; }
  const Element* Parent() const { return parent_; }
  size_t ChildCount() const { return children_.size(); }
  Element* ChildAt(size_t index) { return children_[index].get(); }
  const std::vector<Ref<Element>>& children() const { return children_; }

  // Virtual tag.
  virtual std::string_view tag() const;
  void SetTag(std::string tag) { tag_ = std::move(tag); }

  // Attributes.
  void SetAttribute(std::string name, std::string value);
  const std::map<std::string, std::string>& Attributes() const {
    return attributes_;
  }

  // Common properties.
  std::string id;
  std::vector<std::string> classes;
  ComputedStyle style;
  ComputedStyle base_style;
  ComputedStyle target_style;
  std::map<std::string, ActiveTransition> active_transitions;

  // Rendering.
  virtual std::string Print(int depth = 0) const;

  // Query selector for testing.
  Element* QuerySelector(std::string_view selector);

  const ComponentBase* component() const { return component_; }
  const ComponentBase* owner_component() const { return owner_component_; }
  void set_owner_component(const ComponentBase* owner) {
    owner_component_ = owner;
  }

  bool is_slot() const { return is_slot_; }
  bool is_text() const { return is_text_; }

  int scroll_y() const { return scroll_y_; }
  void set_scroll_y(int y, bool smooth = false);
  int scroll_height() const { return scroll_height_; }
  void set_scroll_height(int h) { scroll_height_ = h; }

  int scroll_x() const { return scroll_x_; }
  void set_scroll_x(int x, bool smooth = false);
  int scroll_width() const { return scroll_width_; }
  void set_scroll_width(int w) { scroll_width_ = w; }

  int target_scroll_y() const { return target_scroll_y_; }
  int target_scroll_x() const { return target_scroll_x_; }

  float visual_scroll_y() const { return visual_scroll_y_; }
  float visual_scroll_x() const { return visual_scroll_x_; }

  bool IsAnimatingScroll() const {
    return scroll_y_animating_ || scroll_x_animating_ ||
           visual_scroll_y_animating_ || visual_scroll_x_animating_;
  }

  void ClampScrollY(int max_scroll);
  void ClampScrollX(int max_scroll);

  int layout_width() const { return layout_width_; }
  void set_layout_width(int w) { layout_width_ = w; }
  int layout_height() const { return layout_height_; }
  void set_layout_height(int h) { layout_height_ = h; }

  bool focused() const { return focused_; }
  void set_focused(bool f) { focused_ = f; }

  bool hovered() const { return hovered_; }
  void set_hovered(bool h) { hovered_ = h; }

  bool active() const { return active_; }
  void set_active(bool a) { active_ = a; }

  void TriggerTransitions(double current_time_ms);
  bool TickTransitions(double current_time_ms);

  int absolute_x() const { return absolute_x_; }
  int absolute_y() const { return absolute_y_; }
  void set_absolute_position(int x, int y) {
    absolute_x_ = x;
    absolute_y_ = y;
  }

 protected:
  bool is_slot_ : 1 = false;
  bool is_text_ : 1 = false;
  int scroll_y_ = 0;
  int scroll_height_ = 0;
  int scroll_x_ = 0;
  int scroll_width_ = 0;
  int layout_width_ = 0;
  int layout_height_ = 0;
  bool focused_ = false;
  bool hovered_ = false;
  bool active_ = false;
  int absolute_x_ = 0;
  int absolute_y_ = 0;

  int target_scroll_y_ = 0;
  int target_scroll_x_ = 0;
  float anim_scroll_y_ = 0.0f;
  float anim_scroll_x_ = 0.0f;
  float start_scroll_y_ = 0.0f;
  float start_scroll_x_ = 0.0f;
  double scroll_y_anim_start_time_ = 0.0;
  double scroll_x_anim_start_time_ = 0.0;
  bool scroll_y_animating_ = false;
  bool scroll_x_animating_ = false;

  float visual_scroll_y_ = 0.0f;
  float visual_scroll_x_ = 0.0f;
  float visual_start_scroll_y_ = 0.0f;
  float visual_start_scroll_x_ = 0.0f;
  float visual_target_scroll_y_ = 0.0f;
  float visual_target_scroll_x_ = 0.0f;
  double visual_scroll_y_anim_start_time_ = 0.0;
  double visual_scroll_x_anim_start_time_ = 0.0;
  bool visual_scroll_y_animating_ = false;
  bool visual_scroll_x_animating_ = false;

  std::string tag_ = "div";
  std::map<std::string, std::string> attributes_;
  std::vector<Ref<Element>> children_;
  Element* parent_ = nullptr;
  const ComponentBase* component_ = nullptr;
  const ComponentBase* owner_component_ = nullptr;
};

namespace time {
using ClockFn = double (*)();
void SetCustomClock(ClockFn clock);
double GetTimeMs();
}  // namespace time

}  // namespace rtxui

#endif  // DOM_ElEMENT_HPP_
