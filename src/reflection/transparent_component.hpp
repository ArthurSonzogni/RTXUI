// Copyright 2025 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#ifndef RTXUI_REFLECTION_TRANSPARENT_COMPONENT_HPP_
#define RTXUI_REFLECTION_TRANSPARENT_COMPONENT_HPP_

// Compatibility layer for the Bloomberg Clang fork.
// This bypasses the need for the full libc++ installation.
namespace std {
  namespace meta {
    using info = decltype(^^void);
  }
}

#include <tuple>
#include <vector>
#include <string>
#include <iostream>
#include <type_traits>

#include "component/component.hpp"
#include "cell/cell.hpp"

namespace rtxui {

template <typename Derived>
class TransparentComponent : public ComponentBase {
 public:
  TransparentComponent() {
    AutoBindProperties();
    TakeSnapshot();
  }

  void Digest() {
    bool is_dirty = false;
    // Bloomberg fork uses __builtin_get_public_data_members or similar
    // For the PoC, we use the proposed standard syntax which the fork supports.
    constexpr auto members = std::meta::nonstatic_data_members_of(^^Derived);

    template for (constexpr auto m : members) {
      auto& current_value = static_cast<Derived*>(this)->*[:m:];
      auto& snapshot_value = std::get<MemberIndex<m>()>(snapshot_);

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
  template <std::meta::info M>
  static constexpr size_t MemberIndex() {
    constexpr auto members = std::meta::nonstatic_data_members_of(^^Derived);
    for (size_t i = 0; i < members.size(); ++i) {
      if (members[i] == M) return i;
    }
    return 0;
  }

  static consteval auto MakeSnapshotType() {
    constexpr auto members = std::meta::nonstatic_data_members_of(^^Derived);
    std::vector<std::meta::info> types;
    for (auto m : members) {
      types.push_back(std::meta::type_of(m));
    }
    return std::meta::substitute(^^std::tuple, types);
  }

  typename [: MakeSnapshotType() :] snapshot_;

  void TakeSnapshot() {
    constexpr auto members = std::meta::nonstatic_data_members_of(^^Derived);
    template for (constexpr auto m : members) {
      std::get<MemberIndex<m>()>(snapshot_) = static_cast<Derived*>(this)->*[:m:];
    }
  }

  void AutoBindProperties() {
    constexpr auto members = std::meta::nonstatic_data_members_of(^^Derived);
    template for (constexpr auto m : members) {
      using T = typename [: std::meta::type_of(m) :];
      auto cell = Ref<ComputedTypedCell<T>>::New([this]() {
        return static_cast<Derived*>(this)->*[:m:];
      });
      this->Import(std::meta::identifier_of(m), cell);
    }
  }
};

}  // namespace rtxui

#endif  // RTXUI_REFLECTION_TRANSPARENT_COMPONENT_HPP_
