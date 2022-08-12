#include "./node.hpp"

namespace qat::ast {

Node::Node(utils::FileRange _fileRange) : fileRange(std::move(_fileRange)) {
  Node::allNodes.push_back(this);
}

Vec<Node *> Node::allNodes = {};

void Node::clearAll() {
  for (auto *node : allNodes) {
    delete node;
  }
  allNodes.clear();
}

} // namespace qat::ast