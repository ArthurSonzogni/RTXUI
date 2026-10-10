// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#ifndef RTXUI_DIAGNOSTIC_HPP_
#define RTXUI_DIAGNOSTIC_HPP_

#include <functional>
#include <rtxui/rtxui_export.hpp>
#include <string>

namespace rtxui {

/// A mistake in a template or a stylesheet.
struct Diagnostic {
  enum class Kind {
    /// A construct that parses but that RTXUI does not support, so it would
    /// otherwise be silently ignored: an unknown CSS property or value, a
    /// `{name}` that is not bound, an expression inside `{}`, a handler that
    /// is not bound, or attribute syntax from another framework (`v-if`,
    /// `*ngFor`, `className`, ...).
    Unsupported,
    /// A <style> block that does not parse.
    CssSyntax,
    /// A template, or markup built at run time such as a <markdown>
    /// component's, that does not parse.
    XmlSyntax,
  };

  std::string message;
  Kind kind = Kind::Unsupported;
  /// For a syntax error, where it is in the text that failed to parse:
  /// 0-based, and relative to that <style> block or that markup. -1 for the
  /// other kinds.
  int line = -1;
  int column = -1;
};

/// Installs a handler invoked for each diagnostic. An unsupported construct is
/// reported once; a syntax error each time the text fails to parse, so that an
/// app showing live-edited CSS or markup (a playground) hears about every
/// attempt. Without a handler, each is printed to stderr, a syntax error with
/// the lines around it, and a template that does not parse ends the process.
/// When the environment variable RTXUI_STRICT is set to 1, the default handler
/// aborts after printing, so tests and CI fail on the first one. An app that
/// owns the terminal should install one: printing lands in the middle of its
/// frame. Pass nullptr to restore the default.
RTXUI_EXPORT void SetDiagnosticHandler(
    std::function<void(const Diagnostic&)> handler);

/// Reports a diagnostic. Repeats of a message already reported are dropped.
RTXUI_EXPORT void ReportDiagnostic(std::string message);

}  // namespace rtxui

#endif  // RTXUI_DIAGNOSTIC_HPP_
