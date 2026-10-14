// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include <cstddef>
#include <deque>
#include <mutex>

#include "rtxui/layout/style.hpp"

namespace rtxui {

namespace {
std::mutex g_minmax_mutex;
// Interned expressions. A deque never relocates elements and entries are
// immutable after insertion, but reads still take the lock: growth from a
// concurrent Register could otherwise race with the size/index access.
std::deque<MinMaxExpr>& Registry() {
  static auto* registry = new std::deque<MinMaxExpr>();
  return *registry;
}
}  // namespace

int RegisterMinMaxExpr(const MinMaxExpr& expr) {
  std::lock_guard<std::mutex> lock(g_minmax_mutex);
  auto& registry = Registry();
  for (size_t i = 0; i < registry.size(); ++i) {
    if (registry[i] == expr) {
      return static_cast<int>(i);
    }
  }
  registry.push_back(expr);
  return static_cast<int>(registry.size() - 1);
}

namespace {
std::deque<GridTrack>& TrackRegistry() {
  static auto* registry = new std::deque<GridTrack>();
  return *registry;
}
}  // namespace

int RegisterGridTrack(const GridTrack& track) {
  std::lock_guard<std::mutex> lock(g_minmax_mutex);
  auto& registry = TrackRegistry();
  for (size_t i = 0; i < registry.size(); ++i) {
    if (registry[i] == track) {
      return static_cast<int>(i);
    }
  }
  registry.push_back(track);
  return static_cast<int>(registry.size() - 1);
}

GridTrack GetGridTrack(int id) {
  std::lock_guard<std::mutex> lock(g_minmax_mutex);
  auto& registry = TrackRegistry();
  if (id < 0 || static_cast<size_t>(id) >= registry.size()) {
    return GridTrack{};
  }
  return registry[static_cast<size_t>(id)];
}

GridTrack AsGridTrack(Length track) {
  if (track.unit == Unit::Track) {
    return GetGridTrack(Length::ToCells(track.value));
  }
  return {track, track};
}

MinMaxExpr GetMinMaxExpr(int id) {
  std::lock_guard<std::mutex> lock(g_minmax_mutex);
  auto& registry = Registry();
  if (id < 0 || static_cast<size_t>(id) >= registry.size()) {
    return MinMaxExpr{};
  }
  return registry[static_cast<size_t>(id)];
}

}  // namespace rtxui
