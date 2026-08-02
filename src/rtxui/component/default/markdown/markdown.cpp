// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#include "rtxui/component/default/markdown/markdown.hpp"

#include "rtxui/base/string.hpp"
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
  if (!stylesheet.empty()) {
    generated_html_ += "<style>\n";
    generated_html_ += stylesheet;
    generated_html_ += "\n</style>\n";
  }
  generated_html_ += MarkdownToHtml(content);
  return generated_html_;
}

bool markdown::Digest() {
  bool changed = false;
  for (auto& entry : entries_) {
    if (entry.check_and_update && entry.check_and_update()) {
      changed = true;
    }
  }
  for (auto& range_entry : range_entries_) {
    if (range_entry.range->CheckAndUpdate()) {
      changed = true;
    }
  }
  if (changed) {
    template_.clear();
    template_ = Template();
    xml_string_ = StripIndent(template_);
    if (auto nodes = xml::Parse(xml_string_)) {
      xml_nodes_ = std::move(nodes.value());
    } else {
      const xml::Error& error = nodes.error();
      ReportXmlError({error.message, error.line, error.column}, xml_string_);
    }
    this->Render();
  }
  for (auto& child : children_) {
    if (child->Digest()) {
      changed = true;
    }
  }
  return changed;
}

namespace {
int RegisterThis = []() {
  RegisterGlobalComponent("markdown", []() { return Ref<markdown>::New(); });
  return 0;
}();
}  // namespace

}  // namespace rtxui
