#include "dom/node.hpp"

namespace rtxui {

void Node::AddChild(Ref<Node> child) {
  children_.push_back(std::move(child));
}

void Node::Visit(std::function<void(Node&)> f) {
  f(*this);
  for (auto& child : children_) {
    child->Visit(f);
  }
}

}  // namespace rtxui
