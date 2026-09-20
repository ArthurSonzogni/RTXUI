#ifndef DOM_ElEMENT_HPP_
#define DOM_ElEMENT_HPP_

#include <functional>
#include <map>
#include <memory>
#include <rtxui/rtxui_export.hpp>
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
      map_ = std::make_unique<std::map<std::string, ActiveTransition>>(
          *other.map_);
    }
  }

  ActiveTransitionsMap& operator=(const ActiveTransitionsMap& other) {
    if (this != &other) {
      if (other.map_) {
        map_ = std::make_unique<std::map<std::string, ActiveTransition>>(
            *other.map_);
      } else {
        map_.reset();
      }
    }
    return *this;
  }

  ActiveTransitionsMap(ActiveTransitionsMap&&) noexcept = default;
  ActiveTransitionsMap& operator=(ActiveTransitionsMap&&) noexcept = default;

  bool empty() const { return !map_ || map_->empty(); }

  size_t size() const { return map_ ? map_->size() : 0; }

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

  void clear() { map_.reset(); }

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

class RTXUI_EXPORT Element : public RefCounted {
 public:
  Element();
  Element(const ComponentBase* component);
  virtual ~Element();

  // Non-copyable, non-movable.
  Element(const Element&) = delete;
  Element& operator=(const Element&) = delete;
  Element(Element&&) = default;
  Element& operator=(Element&&) = default;

  /// Attaches `child`, detaching it from its current parent first if it has
  /// one. Reconciliation can hand back an element that is still where it was
  /// last frame -- a nested component's root that moved from one slot to
  /// another, say -- and the alternative to detaching is an element sitting in
  /// two parents' child lists at once.
  void AddChild(Ref<Element> child);
  /// Removes this element from its parent's child list, if it has a parent.
  /// The caller must hold a reference: this can drop the parent's.
  void DetachFromParent();
  void RemoveChildren();
  void ReplaceChild(size_t index, Ref<Element> new_child);
  void MoveChild(size_t from, size_t to);
  void TruncateChildren(size_t count);
  // By reference: taking it by value copied the std::function -- often a
  // heap allocation -- once per node of the subtree.
  void Visit(const std::function<void(Element&)>& f);

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
    if (!attributes_) {
      return nullptr;
    }
    auto it = attributes_->find(name);
    if (it == attributes_->end()) {
      return nullptr;
    }
    return &it->second;
  }

  // Common properties.
  std::string id;
  std::vector<std::string> classes;
  ComputedStyle style;
  ComputedStyle base_style;
  ComputedStyle target_style;
  // CSS custom properties resolved for this element: inherited from the DOM
  // parent, overlaid with this element's own --* declarations. Rebuilt on
  // each base style resolution pass (parents are resolved before children).
  std::map<std::string, std::string, std::less<>> custom_properties;
  // The --* declarations matching this element itself, accumulated across
  // the components styling it in the current frame.
  std::map<std::string, std::string, std::less<>> own_custom_properties;
  /// Identity stamped by `<for key="...">`: which item of the collection
  /// produced this element. Reconciliation matches on it instead of position,
  /// so reordering a collection carries focus, scroll and in-flight
  /// transitions along with the item rather than leaving them at the index.
  /// Empty for an unkeyed loop, which stays position-matched.
  std::string for_key;
  /// How many consecutive siblings that iteration produced, counting this
  /// one. Lets a keyed match move a whole multi-element loop body.
  uint16_t for_run = 0;
  ActiveTransitionsMap active_transitions;
  const ComponentBase* styled_by_1 = nullptr;
  const ComponentBase* styled_by_2 = nullptr;
  /// Set when a resolution pass recomputed `base_style`, cleared once
  /// `target_style` and `style` have been seeded from it. Seeding is what
  /// makes a new base style visible, but it also overwrites the
  /// transition-blended `style`, so it must happen only for the elements that
  /// actually changed -- not for every element on every pass.
  bool needs_style_seed = true;

  /// Whether a base style pass still has work to do here. False once a pass
  /// has visited the element, true again as soon as anything invalidates it.
  /// Lets a frame ask "is any of this stale?" with a walk and a hash, instead
  /// of paying for a full resolution pass to discover there was nothing to do.
  bool needs_style_resolve = true;

  bool IsStyleResolvedFor(const ComponentBase* comp) const {
    return styled_by_1 == comp || styled_by_2 == comp;
  }

  /// Drops the resolved-style memo if `classes` no longer matches the list it
  /// was built from, and reports whether it did.
  ///
  /// `classes` is a plain public vector, so assigning or push_back-ing to it
  /// cannot invalidate anything by itself the way SetAttribute() does. Every
  /// caller was expected to follow a mutation with ClearResolvedStyles(), and
  /// a caller that forgot got styles that silently never applied -- the bug
  /// behind both tabs regressions. Checking the content instead of trusting
  /// the callers makes that impossible to get wrong.
  bool RevalidateStyleMemo() {
    const uint64_t hash = ClassesHash();
    if (hash == resolved_classes_hash_) {
      return false;
    }
    resolved_classes_hash_ = hash;
    ClearResolvedStyles();
    return true;
  }

  /// Records that `comp` has resolved this element, so the next pass can skip
  /// it.
  ///
  /// This skips the ELEMENT, never its subtree, and it cannot be widened to do
  /// the latter. Both shapes of that idea were tried and both break the same
  /// way: a component's `Digest()` override can add elements after its walk has
  /// run, and an ancestor marked as already covered stops the next walk from
  /// ever descending to them. `<select>` and `<tabs>` build elements exactly
  /// that way, and they are what fails. Skipping a subtree would need a dirty
  /// flag that propagates up to the ancestors when a descendant changes; the
  /// memo below carries no such promise.
  ///
  /// Only two components are remembered. An element can in principle be styled
  /// by more -- its own component, its owner, and an outer one reaching it
  /// through `::part()` -- and the third evicts the second, which then
  /// re-resolves on every pass. That is a cost, not a wrong answer: a
  /// re-resolve re-applies the same declarations over a base style that
  /// already holds them. Measured at one eviction across the whole test suite
  /// and none in the layout benchmark's 4009 elements, so two slots stay
  /// cheaper than anything that would hold more.
  void MarkStyleResolvedFor(const ComponentBase* comp) {
    resolved_classes_hash_ = ClassesHash();
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
    needs_style_resolve = true;
  }

 private:
  /// FNV-1a over the class list. Only ever compared against itself, so the
  /// choice of hash matters only for collisions, and a collision costs a
  /// skipped restyle rather than anything unsafe.
  uint64_t ClassesHash() const {
    uint64_t hash = 1469598103934665603ull;
    for (const std::string& name : classes) {
      for (unsigned char c : name) {
        hash = (hash ^ c) * 1099511628211ull;
      }
      hash = (hash ^ 0xff) * 1099511628211ull;  // separator
    }
    return hash;
  }

  uint64_t resolved_classes_hash_ = ClassesHash();

 public:
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

  // Mirrors TextInputBase::disabled/readonly (or any future component with
  // the same notion) each digest, the same way focused_/hovered_ mirror
  // interaction state -- so :disabled/:read-only matching and Screen's
  // click/tab-order handling don't need to reach into a specific
  // component's own members.
  bool disabled() const { return disabled_; }
  void set_disabled(bool d) { disabled_ = d; }

  bool checked() const {
    if (checked_) {
      return true;
    }
    const std::string* attr = GetAttribute("checked");
    if (attr && (*attr == "true" || *attr == "checked" || attr->empty())) {
      return true;
    }
    return false;
  }
  void set_checked(bool c) { checked_ = c; }

  bool read_only() const { return read_only_; }
  void set_read_only(bool r) { read_only_ = r; }

  bool scrollbar_hovered() const { return scrollbar_hovered_; }
  void set_scrollbar_hovered(bool h) { scrollbar_hovered_ = h; }

  bool scrollbar_active() const { return scrollbar_active_; }
  void set_scrollbar_active(bool a) { scrollbar_active_ = a; }

  bool scrollbar_thumb_hovered() const { return scrollbar_thumb_hovered_; }
  void set_scrollbar_thumb_hovered(bool h) { scrollbar_thumb_hovered_ = h; }

  bool scrollbar_thumb_active() const { return scrollbar_thumb_active_; }
  void set_scrollbar_thumb_active(bool a) { scrollbar_thumb_active_ = a; }

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
  bool disabled_ = false;
  bool checked_ = false;
  bool read_only_ = false;
  bool scrollbar_hovered_ = false;
  bool scrollbar_active_ = false;
  bool scrollbar_thumb_hovered_ = false;
  bool scrollbar_thumb_active_ = false;
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
  // avoids the constructor/destructor overhead of std::map for the vast
  // majority of DOM elements which don't have custom attributes.
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
