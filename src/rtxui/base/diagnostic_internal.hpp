// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#ifndef RTXUI_BASE_DIAGNOSTIC_INTERNAL_HPP_
#define RTXUI_BASE_DIAGNOSTIC_INTERNAL_HPP_

#include "rtxui/diagnostic.hpp"

namespace rtxui {

/// Hands `diagnostic` to the handler installed with SetDiagnosticHandler,
/// every time, without the de-duplication ReportDiagnostic does. False when
/// none is installed: what to do then is the caller's.
bool DeliverDiagnostic(const Diagnostic& diagnostic);

/// Whether RTXUI_STRICT=1: the default handling aborts on any diagnostic.
bool StrictDiagnostics();

}  // namespace rtxui

#endif  // RTXUI_BASE_DIAGNOSTIC_INTERNAL_HPP_
