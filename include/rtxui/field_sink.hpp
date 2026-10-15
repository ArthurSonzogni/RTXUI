// Copyright 2026 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#ifndef RTXUI_FIELD_SINK_HPP_
#define RTXUI_FIELD_SINK_HPP_

#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "rtxui/internal/convert.hpp"

namespace rtxui {

/// The fields of one item of a bound collection, as a template reads them:
/// `{book.title}` in `<for each="{books}" as="book">`. A type describes its
/// fields once with a function found next to it:
///
///   void Fields(const Book& book, FieldSink& out) {
///     out("title", book.title);
///     out("length", book.pages > 300 ? "long" : "short");
///   }
///
/// and `Bind(books)` alone then binds a collection of it. A binding can give
/// its own instead, with a function or a const member function of the same
/// shape: `Bind(books, &Library::BookFields)`.
class FieldSink {
 public:
  /// Adds the field `name`, converted to text as `{name}` would be.
  template <typename T>
  void operator()(std::string_view name, const T& value) {
    fields_.emplace_back(std::string(name), internal::to_string(value));
  }

  /// The value of the field `name`, or "" when there is none.
  std::string_view Get(std::string_view name) const {
    for (const auto& [field, value] : fields_) {
      if (field == name) {
        return value;
      }
    }
    return {};
  }

  bool operator==(const FieldSink&) const = default;

 private:
  std::vector<std::pair<std::string, std::string>> fields_;
};

}  // namespace rtxui

#endif  // RTXUI_FIELD_SINK_HPP_
