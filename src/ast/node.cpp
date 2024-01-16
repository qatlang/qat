#include "./node.hpp"

namespace qat::ast {

Node::Node(FileRange _fileRange) : fileRange(std::move(_fileRange)) { Node::allNodes.push_back(this); }

Vec<Node*> Node::allNodes = {};

void Node::clearAll() {
  for (auto* node : allNodes) {
    std::destroy_at(node);
  }
  allNodes.clear();
}

} // namespace qat::ast