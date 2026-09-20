// Copyright 2024 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.
#ifndef XML_HPP_
#define XML_HPP_

#include <map>
#include <rtxui/rtxui_export.hpp>
#include <string>
#include <string_view>
#include <vector>

#include "rtxui/base/expected.hpp"

namespace xml {

using Attributes = std::map<std::string, std::string>;

/// The node object, which represents an XML node.
struct Node {
  /// The type of the xml::Node.
  enum Type {
    kText,     ///< Text node.
    kElement,  ///< Element node.
    kComment,  ///< Comment node.
  };
  Type type = kElement;

  /// The tag name of the node. Only valid if the type is kElement.
  std::string tag = {};

  /// The text of the node. Only valid if the type is kText or kComment.
  std::string text = {};

  /// The attributes of the node. Only valid if the type is kElement.
  Attributes attributes = {};

  /// The children of the node. Only valid if the type is kElement.
  std::vector<Node> children = {};
};

using Nodes = std::vector<Node>;

/// The error object, which contains the error message, line, and column.
struct Error {
  /// The error message.
  std::string message;

  /// The line where the error occurred. 0-based.
  int line;

  /// The column where the error occurred. 0-based.
  int column;
};

/// Parse the given XML string and return the nodes.
/// If the XML is invalid, return an error.
RTXUI_EXPORT auto Parse(std::string_view xml) -> Expected<Nodes, Error>;

/// Print the XML node with the given indentation level.
auto Print(const Node& node, int level = 0) -> std::string;

}  // namespace xml

#endif  // XML_HPP_
