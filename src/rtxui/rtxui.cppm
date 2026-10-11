// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
//
// The `rtxui` module. It exports what applications use, and nothing the
// engine uses internally: cmake/modules.cmake builds every example against
// it, so a name missing here fails that build. The Bind() macros cannot be
// exported; they come from <rtxui/bind.hpp>.
module;

#include "rtxui/rtxui.hpp"

export module rtxui;

export namespace rtxui {
using rtxui::Component;
using rtxui::Diagnostic;
using rtxui::Event;
using rtxui::ManualStructVisitor;
using rtxui::PostDelayedTask;
using rtxui::PostTask;
using rtxui::Ref;
using rtxui::Screen;
using rtxui::SetDiagnosticHandler;
}  // namespace rtxui
