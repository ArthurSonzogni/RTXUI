// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include "rtxui/diagnostic.hpp"

#include <cstdlib>
#include <iostream>
#include <set>
#include <string_view>
#include <utility>

#include "rtxui/base/diagnostic_internal.hpp"

namespace rtxui {

namespace {

std::function<void(const Diagnostic&)>& Handler() {
  static std::function<void(const Diagnostic&)> handler;
  return handler;
}

// Styles are re-applied every frame, so the same mistake would otherwise be
// reported many times per second.
std::set<std::string, std::less<>>& Reported() {
  static std::set<std::string, std::less<>> reported;
  return reported;
}

}  // namespace

bool StrictDiagnostics() {
  const char* strict = std::getenv("RTXUI_STRICT");
  return strict && std::string_view(strict) == "1";
}

bool DeliverDiagnostic(const Diagnostic& diagnostic) {
  if (const auto& handler = Handler()) {
    handler(diagnostic);
    return true;
  }
  return false;
}

void SetDiagnosticHandler(std::function<void(const Diagnostic&)> handler) {
  Handler() = std::move(handler);
  // A new handler must hear about every mistake, including ones the previous
  // handler already saw.
  Reported().clear();
}

void ReportDiagnostic(std::string message) {
  if (!Reported().insert(message).second) {
    return;
  }
  if (DeliverDiagnostic(Diagnostic{message})) {
    return;
  }
  std::cerr << "rtxui: " << message << '\n';
  if (StrictDiagnostics()) {
    std::abort();
  }
}

}  // namespace rtxui
