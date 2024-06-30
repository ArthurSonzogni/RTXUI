#include "element.hpp"

#include <cstdlib>
#include <iostream>

#include "core/expected.hpp"
#include "core/string.hpp"
#include "xml/xml.hpp"

void Element::Attribute(const std::string_view label,
                        reactive::Reactive value) {
  //
}

void Element::Event(const std::string_view label) {
  //
}

void Element::Model(const std::string_view label) {
  //
}

void Element::Ref(const std::string_view label, reactive::Reactive value) {
  //
}

void Element::Computed(const std::string_view label,
                       std::function<reactive::Reactive(reactive::Reactive&)>) {
  //
}

void Element::Dom(const std::string_view dom) {
  std::string unindented_dom = StripIndent(dom);
  Expected<xml::Nodes, xml::Error> nodes = xml::Parse(unindented_dom);
  if (!nodes) {
    std::cerr << "======== Error parsing DOM ========" << std::endl;
    int error_line = nodes.error().line;
    int error_column = nodes.error().column;

    std::vector<std::string_view> dom_lines = Split(unindented_dom, '\n');

    std::cerr << " ┌" << Repeat("─", 76) << std::endl;
    for (int line = 0; line < dom_lines.size(); line++) {
      std::cerr << line << "│ " << dom_lines[line] << std::endl;
      if (line != error_line) {
        continue;
      }
      std::string arrow_line = Repeat("-", std::max(76, error_column));
      arrow_line[error_column + 1] = '^';
      std::cerr << " └" << arrow_line << std::endl;
      std::cerr << "   " << Repeat(" ", error_column) << "|" << std::endl;
      std::cerr << error_line << ":" << error_column << ": "
                << nodes.error().message << std::endl;
      std::cerr << std::endl;
      std::cerr << " ┌" << Repeat("─", 76) << std::endl;
    }

    // Pretty print the error message using the dom, line and column.
    std::cerr << " └" << Repeat("─", 76) << std::endl;
    std::cerr << std::flush;
    std::exit(1);
    return;
  }
}

void Element::Style(const std::string_view) {
  //
}
