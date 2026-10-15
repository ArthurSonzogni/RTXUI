// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include "rtxui/component/default/markdown/markdown.hpp"

#include <string_view>
#include <utility>

#include "rtxui/base/diagnostic_internal.hpp"
#include "rtxui/base/string.hpp"
#include "rtxui/bind.hpp"
#include "rtxui/component.hpp"
#include "rtxui/component/component_internal.hpp"
#include "rtxui/diagnostic.hpp"
#include "rtxui/markdown/markdown.hpp"
#include "rtxui/xml/xml.hpp"

namespace rtxui {

void markdown::InitReflection() {
  Bind(content);
  Bind(stylesheet);
  Component<markdown>::InitReflection();
}

std::string_view markdown::Setup() {
  return "";
}

std::string_view markdown::GetView() const {
  generated_html_.clear();
  built_content_ = content;
  built_stylesheet_ = stylesheet;
  if (!stylesheet.empty()) {
    generated_html_ += "<style>\n";
    generated_html_ += stylesheet;
    generated_html_ += "\n</style>\n";
  }
  generated_html_ += MarkdownToHtml(content);
  return generated_html_;
}

bool markdown::Digest() {
  ComponentInternals::StyleResolutionScope scope(this);
  bool changed = false;
  for (auto& entry : ComponentInternals::data(*this).entries_) {
    if (entry.check_and_update && entry.check_and_update()) {
      changed = true;
    }
  }
  for (auto& range_entry : ComponentInternals::data(*this).range_entries_) {
    if (range_entry.range->CheckAndUpdate()) {
      changed = true;
    }
  }
  // Compared with what the template was built from, not with the last
  // Digest(): a parent passing new content re-renders this component before
  // its Digest() runs, which leaves nothing for change detection to report
  // but a template still built from the old content.
  if (content != built_content_ || stylesheet != built_stylesheet_) {
    changed = true;
    ComponentInternals::data(*this).template_.clear();
    ComponentInternals::data(*this).template_ =
        ComponentInternals::Template(*this);
    ComponentInternals::data(*this).xml_string_ =
        StripIndent(ComponentInternals::data(*this).template_);
    if (auto nodes = xml::Parse(ComponentInternals::data(*this).xml_string_)) {
      ComponentInternals::data(*this).SetXmlNodes(std::move(nodes.value()));
    } else {
      const xml::Error& error = nodes.error();
      ReportSyntaxError(
          DiagnosticInternals::Make(error.message, Diagnostic::Kind::XmlSyntax,
                                    error.line, error.column),
          ComponentInternals::data(*this).xml_string_);
    }
    ComponentInternals::Render(*this);
  }
  for (auto& child : ComponentInternals::data(*this).children_) {
    if (child->Digest()) {
      changed = true;
    }
  }
  return changed;
}

}  // namespace rtxui
