// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
module;

#include "rtxui/rtxui.hpp"

export module rtxui;

export namespace rtxui {
    using rtxui::CategorizedRules;
    using rtxui::Color;
    using rtxui::Component;
    using rtxui::ComponentBase;
    using rtxui::Diagnostic;
    using rtxui::Element;
    using rtxui::ElementHandle;
    using rtxui::ElementHandleTarget;
    using rtxui::Event;
    using rtxui::EventModifier;
    using rtxui::GetGlobalComponentFactory;
    using rtxui::HeadlessScreen;
    using rtxui::HeadlessTerminalDevice;
    using rtxui::LocalScope;
    using rtxui::ManualStructVisitor;
    using rtxui::PostDelayedTask;
    using rtxui::PostTask;
    using rtxui::Ref;
    using rtxui::RefCounted;
    using rtxui::RegisterGlobalComponent;
    using rtxui::RenderToString;
    using rtxui::ReportDiagnostic;
    using rtxui::Screen;
    using rtxui::SetDiagnosticHandler;
    using rtxui::StructVisitor;
    using rtxui::TerminalDevice;
    using rtxui::TimelineScreen;
    using rtxui::TypeErasedRange;
    using rtxui::VirtualListViewport;

    #ifdef RTXUI_HAS_REFLECTION
    using rtxui::ReflectedStructVisitor;
    using rtxui::get_members;
    using rtxui::get_members_size;
    #endif

    using rtxui::operator<<;

    namespace css {
        using ::css::Ruleset;
        using ::css::StyleSheet;
    }  // namespace css

    namespace reflection {
        using rtxui::reflection::ParseInt;
        using rtxui::reflection::from_string;
        using rtxui::reflection::to_string;
    }  // namespace reflection

    namespace xml {
        using ::xml::Node;
        using ::xml::Nodes;
    }  // namespace xml
}  // namespace rtxui
