// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#ifndef RTXUI_DIAGNOSTIC_HPP_
#define RTXUI_DIAGNOSTIC_HPP_

#include <functional>
#include <rtxui/rtxui_export.hpp>
#include <string>

namespace rtxui {

/// A template or stylesheet construct that parses but that RTXUI does not
/// support, so it would otherwise be silently ignored: an unknown CSS property
/// or value, a `{name}` that is not bound, an expression inside `{}`, a
/// handler that is not bound, or attribute syntax from another framework
/// (`v-if`, `*ngFor`, `className`, ...).
struct Diagnostic {
  std::string message;
};

/// Installs a handler invoked for each distinct diagnostic. Without one, each
/// distinct message is printed once to stderr. When the environment variable
/// RTXUI_STRICT is set to 1, the default handler aborts after printing, so
/// tests and CI fail on the first one. Pass nullptr to restore the default.
RTXUI_EXPORT void SetDiagnosticHandler(
    std::function<void(const Diagnostic&)> handler);

/// Reports a diagnostic. Repeats of a message already reported are dropped.
RTXUI_EXPORT void ReportDiagnostic(std::string message);

}  // namespace rtxui

#endif  // RTXUI_DIAGNOSTIC_HPP_
