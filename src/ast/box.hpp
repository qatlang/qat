#ifndef QAT_AST_BOX_HPP
#define QAT_AST_BOX_HPP

#include "../utils/visibility.hpp"
#include "./node.hpp"
#include <optional>
#include <string>
#include <vector>

namespace qat::ast {

// Box is a container for objects in the
// QAT language. It is not a structural container,
// but exists merely to avoid conflict between
// libraries
class Box : public Node {
  String name;

  Vec<Node *> members;

  utils::VisibilityKind visibility;

public:
  Box(String _name, Vec<Node *> _members, utils::VisibilityKind _visibility,
      utils::FileRange _fileRange)
      : Node(std::move(_fileRange)), name(std::move(_name)),
        members(std::move(_members)), visibility(_visibility) {}

  useit IR::Value *emit(IR::Context *ctx) override;

  useit nuo::Json toJson() const override;

  useit NodeType nodeType() const final { return NodeType::box; }
};

} // namespace qat::ast

#endif