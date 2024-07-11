#ifndef DOM_NODE_HPP_
#define DOM_NODE_HPP_

#include <functional>
#include <vector>

#include "core/refcounted.hpp"

namespace rtxui {

// A generic HTML node.
class Node : public RefCounted {
 public:
  Node() = default;
  Node(const Node&) = delete;
  Node& operator=(const Node&) = delete;
  Node(Node&&) = default;
  Node& operator=(Node&&) = default;

  void AddChild(Ref<Node> child);
  void Visit(std::function<void(Node&)> f);

 private:
  std::vector<Ref<Node>> children_;
};

}  // namespace rtxui

#endif  // DOM_NODE_HPP_
