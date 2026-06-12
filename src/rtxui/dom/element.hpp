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

// OPTIMIZATION: Empty std::map containers (like active_transitions) incur
// constructor/destructor overhead (e.g., node initialization, tree traversal)
// for every DOM element created, even if they remain empty. Wrapping the map
// in a lazy unique_ptr/ActiveTransitionsMap results in a ~40% reduction in
// Element exit destruction overhead under layout stress tests.
class ActiveTransitionsMap {
 public:
  ActiveTransitionsMap() = default;
  ~ActiveTransitionsMap() = default;

  ActiveTransitionsMap(const ActiveTransitionsMap& other) {
    if (other.map_) {
      map_ = std::make_unique<std::map<std::string, ActiveTransition>>(*other.map_);
    }
  }

  ActiveTransitionsMap& operator=(const ActiveTransitionsMap& other) {
    if (this != &other) {
      if (other.map_) {
        map_ = std::make_unique<std::map<std::string, ActiveTransition>>(*other.map_);
      } else {
        map_.reset();
      }
    }
    return *this;
  }

  ActiveTransitionsMap(ActiveTransitionsMap&&) noexcept = default;
  ActiveTransitionsMap& operator=(ActiveTransitionsMap&&) noexcept = default;

  bool empty() const {
    return !map_ || map_->empty();
  }

  size_t size() const {
    return map_ ? map_->size() : 0;
  }

  size_t count(const std::string& key) const {
    return map_ ? map_->count(key) : 0;
  }

  void erase(const std::string& key) {
    if (map_) {
      map_->erase(key);
      if (map_->empty()) {
        map_.reset();
      }
    }
  }

  ActiveTransition& operator[](const std::string& key) {
    if (!map_) {
      map_ = std::make_unique<std::map<std::string, ActiveTransition>>();
    }
    return (*map_)[key];
  }

  void clear() {
    map_.reset();
  }

  using MapType = std::map<std::string, ActiveTransition>;
  typename MapType::iterator begin() {
    if (!map_) {
      map_ = std::make_unique<MapType>();
    }
    return map_->begin();
  }

  typename MapType::iterator end() {
    if (!map_) {
      map_ = std::make_unique<MapType>();
    }
    return map_->end();
  }

  typename MapType::const_iterator begin() const {
    if (!map_) {
      static const MapType empty_map;
      return empty_map.begin();
    }
    return map_->begin();
  }

  typename MapType::const_iterator end() const {
    if (!map_) {
      static const MapType empty_map;
      return empty_map.end();
    }
    return map_->end();
  }

 private:
  std::unique_ptr<std::map<std::string, ActiveTransition>> map_;
};

class Element : public RefCounted {
 public:
  Element();
  Element(const ComponentBase* component);
  virtual ~Element();

  // Non-copyable, non-movable.
  Element(const Element&) = delete;
  Element& operator=(const Element&) = delete;
  Element(Element&&) = default;
  Element& operator=(Element&&) = default;

  void AddChild(Ref<Element> child);
  void RemoveChildren();
  void ReplaceChild(size_t index, Ref<Element> new_child);
  void MoveChild(size_t from, size_t to);
  void TruncateChildren(size_t count);
  void Visit(std::function<void(Element&)> f);

  // Hierarchical accessors.
  Element* Parent() { return parent_; }
  const Element* Parent() const { return parent_; }
  void set_parent(Element* parent) { parent_ = parent; }
  size_t ChildCount() const { return children_.size(); }
  Element* ChildAt(size_t index) { return children_[index].get(); }
  const std::vector<Ref<Element>>& children() const { return children_; }

  // Virtual tag.
  virtual std::string_view tag() const;
  void SetTag(std::string tag) { tag_ = std::move(tag); }

  // Attributes.
  void SetAttribute(std::string name, std::string value);
  void RemoveAttribute(const std::string& name);
  void ClearAttributes() {
    id.clear();
    classes.clear();
    attributes_.reset();
  }
  const std::map<std::string, std::string>& Attributes() const {
    if (!attributes_) {
      static const std::map<std::string, std::string> empty_map;
      return empty_map;
    }
    return *attributes_;
  }
  const std::string* GetAttribute(const std::string& name) const {
    if (!attributes_) return nullptr;
    auto it = attributes_->find(name);
    if (it == attributes_->end()) return nullptr;
    return &it->second;
  }

  // Common properties.
  std::string id;
  std::vector<std::string> classes;
  ComputedStyle style;
  ComputedStyle base_style;
  ComputedStyle target_style;
  ActiveTransitionsMap active_transitions;
  const ComponentBase* styled_by_1 = nullptr;
  const ComponentBase* styled_by_2 = nullptr;

  bool IsStyleResolvedFor(const ComponentBase* comp) const {
    return styled_by_1 == comp || styled_by_2 == comp;
  }
  void MarkStyleResolvedFor(const ComponentBase* comp) {
    if (styled_by_1 == comp || styled_by_2 == comp) {
      return;
    }
    if (!styled_by_1) {
      styled_by_1 = comp;
    } else {
      styled_by_2 = comp;
    }
  }
  void ClearResolvedStyles() {
    styled_by_1 = nullptr;
    styled_by_2 = nullptr;
  }

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
  // OPTIMIZATION: Wrapping attributes in a unique_ptr and allocating it lazily
  // avoids the constructor/destructor overhead of std::map for the vast majority
  // of DOM elements which don't have custom attributes.
  std::unique_ptr<std::map<std::string, std::string>> attributes_;
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
