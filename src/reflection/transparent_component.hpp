// Copyright 2025 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#ifndef RTXUI_REFLECTION_TRANSPARENT_COMPONENT_HPP_
#define RTXUI_REFLECTION_TRANSPARENT_COMPONENT_HPP_

// Compatibility layer for the Bloomberg Clang fork and GCC experimental.
// This bypasses the need for the full libc++ installation.
#if defined(__clang__)
namespace std {
  namespace meta {
    using info = decltype(^^void);
  }
}
#elif defined(__GNUC__)
#include <meta>
#endif

#include <tuple>
#include <vector>
#include <string>
#include <iostream>
#include <type_traits>

#include "component/component.hpp"
#include "cell/cell.hpp"

#if defined(__clang__)
#define RTXUI_NONSTATIC_DATA_MEMBERS_OF(x) std::meta::nonstatic_data_members_of(x)
#define RTXUI_IDENTIFIER_OF(x) std::meta::identifier_of(x)
#define RTXUI_TYPE_OF(x) std::meta::type_of(x)
#define RTXUI_SUBSTITUTE(x, y) std::meta::substitute(x, y)
#elif defined(__GNUC__)
#define RTXUI_NONSTATIC_DATA_MEMBERS_OF(x) std::meta::nonstatic_data_members_of(x, std::meta::access_context::current())
#define RTXUI_IDENTIFIER_OF(x) std::meta::identifier_of(x)
#define RTXUI_TYPE_OF(x) std::meta::type_of(x)
#define RTXUI_SUBSTITUTE(x, y) std::meta::substitute(x, y)
#endif

namespace rtxui {

template <typename Derived>
struct TransparentComponentHelper {
  static consteval auto MakeSnapshotType() {
    constexpr auto members = RTXUI_NONSTATIC_DATA_MEMBERS_OF(^^Derived);
    std::vector<std::meta::info> types;
    for (auto m : members) {
      types.push_back(RTXUI_TYPE_OF(m));
    }
    return RTXUI_SUBSTITUTE(^^std::tuple, types);
  }

  template <std::meta::info M>
  static constexpr size_t MemberIndex() {
    constexpr auto members = RTXUI_NONSTATIC_DATA_MEMBERS_OF(^^Derived);
    for (size_t i = 0; i < members.size(); ++i) {
      if (members[i] == M) return i;
    }
    return 0;
  }
};

template <typename Derived>
class TransparentComponent : public ComponentBase {
 public:
  TransparentComponent() {
    // These need to be called when the object is fully constructed
    // to ensure 'Derived' is complete.
  }

  // Initialization must be deferred or called from derived class.
  void InitTransparent() {
    AutoBindProperties();
    TakeSnapshot();
  }

  void Digest() {
    bool is_dirty = false;
    constexpr auto members = RTXUI_NONSTATIC_DATA_MEMBERS_OF(^^Derived);

    template for (constexpr auto m : members) {
      auto& current_value = static_cast<Derived*>(this)->*[:m:];
      auto& snapshot_value = std::get<TransparentComponentHelper<Derived>::template MemberIndex<m>()>(snapshot_);

      if (current_value != snapshot_value) {
        is_dirty = true;
        snapshot_value = current_value; 
        std::cout << "[Digest] Detected change in state." << std::endl;
      }
    }

    if (is_dirty) {
        this->Render();
    }
  }

 private:
  typename [: TransparentComponentHelper<Derived>::MakeSnapshotType() :] snapshot_;

  void TakeSnapshot() {
    constexpr auto members = RTXUI_NONSTATIC_DATA_MEMBERS_OF(^^Derived);
    template for (constexpr auto m : members) {
      std::get<TransparentComponentHelper<Derived>::template MemberIndex<m>()>(snapshot_) = static_cast<Derived*>(this)->*[:m:];
    }
  }

  void AutoBindProperties() {
    constexpr auto members = RTXUI_NONSTATIC_DATA_MEMBERS_OF(^^Derived);
    template for (constexpr auto m : members) {
      using T = typename [: RTXUI_TYPE_OF(m) :];
      auto cell = Ref<ComputedTypedCell<T>>::New([this]() {
        return static_cast<Derived*>(this)->*[:m:];
      });
      this->Import(RTXUI_IDENTIFIER_OF(m), cell);
    }
  }
};

}  // namespace rtxui

#endif  // RTXUI_REFLECTION_TRANSPARENT_COMPONENT_HPP_
