#include "./node.hpp"

namespace qat::ast {

Node::Node(FileRangePtr _fileRange) : fileRange(_fileRange) { Node::allNodes.push_back(this); }

Vec<Node*> Node::allNodes = {};

void Node::clear_all() {
	for (auto* node : allNodes) {
		std::destroy_at(node);
	}
	allNodes.clear();
}

} // namespace qat::ast
